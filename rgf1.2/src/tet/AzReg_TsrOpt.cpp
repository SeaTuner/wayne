/* * * * *
 *  AzReg_TsrOpt.cpp 
 *  Copyright (C) 2011, 2012 Rie Johnson
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * * * * */

#include "AzReg_TsrOpt.hpp"
#include "AzHelp.hpp"

/*
 * To evaluate node split: 
 *   Assume two new leaf nodes under the node on focus (the node to be split)
 *   with the weights same as the node on focus, so that predictions stay the same. 
 */

#define coeff_sum_index 3

/*--------------------------------------------------------*/
void AzReg_TsrOpt::resetTreeStructure()
{
  iia_le_gt.reset(); 
}

/*--------------------------------------------------------*/
/* static */
void AzReg_TsrOpt::storeTreeStructure()
{
  iia_le_gt.reset(); 
  iia_le_gt.prepare(tree->nodeNum()); 
  int nx; 
  for (nx = 0; nx < tree->nodeNum(); ++nx) {
    iia_le_gt.put(tree->node(nx)->le_nx, 
                  tree->node(nx)->gt_nx); 
  }
}

/*--------------------------------------------------------*/
/* static */
bool AzReg_TsrOpt::isSameTreeStructure() const
{
  if (tree->nodeNum() != iia_le_gt.size()) {
    return false; 
  }
  int nx; 
  for (nx = 0; nx < tree->nodeNum(); ++nx) {
    int le_nx, gt_nx; 
    iia_le_gt.get(nx, &le_nx, &gt_nx); 
    if (tree->node(nx)->le_nx != le_nx || 
        tree->node(nx)->gt_nx != gt_nx) {
      return false; 
    }
  }  

  return true; 
}

/*--------------------------------------------------------*/
void AzReg_TsrOpt::_reset(const AzTrTree_ReadOnly *inp_tree, 
                          const AzRegDepth *inp_reg_depth)
{
  tree = inp_tree; 
  reg_depth = inp_reg_depth; 

  if (tree == NULL) {
    throw new AzException("AzReg_TsrOpt::_reset", "null tree"); 
  }

  bool isSame = isSameTreeStructure(); 

  forNewLeaf = false; 
  int node_num = tree->nodeNum(); 
  v_bar.reform(node_num);  
  if (!isSame) {
    /*---  new tree structre  ---*/
    av_dbar.reset(node_num);  
  }
  else {
    /*---  same tree structre as before; reuse dbar  ---*/
    if (av_dbar.size() != node_num || 
        av_dv.size() != node_num || 
        v_dv2_sum.rowNum() != node_num) {
      throw new AzException("AzReg_TsrOpt::reset", 
                "tree structure is same but other info doesn't match"); 
    }
  }

  AzIntArr ia_nonleaf, ia_leaf; 
  int nx; 
  for (nx = 0; nx < node_num; ++nx) {
    if (!tree->node(nx)->isLeaf()) ia_nonleaf.put(nx); 
    else                           ia_leaf.put(nx); 
  }

  int ix; 
  for (ix = 0; ix < ia_leaf.size(); ++ix) {
    nx = ia_leaf.get(ix); 
    AzDvect *v_dbar = av_dbar.point_u(nx); 

    if (!isSame) {
      deriv(nx, &ia_nonleaf, v_dbar); 
    }

    double w = tree->node(nx)->weight; 
    v_bar.add(v_dbar, w); 
    v_bar.set(nx, w); 
  }  

  focus_nx = -1; 
  if (!isSame) {
    update_dv(); 
    storeTreeStructure();   
  }
  update_v(); 
}

/*--------------------------------------------------------*/
/* update v_dv and v_dv2_sum */
void AzReg_TsrOpt::update_dv()
{
  av_dv.reset(tree->nodeNum());  
  v_dv2_sum.reform(tree->nodeNum()); 
  int f_nx; 
  for (f_nx = 0; f_nx < tree->nodeNum(); ++f_nx) {
    if (!tree->node(f_nx)->isLeaf()) continue; 

    const AzDvect *v_dbar = av_dbar.point(f_nx); 
    AzDvect *v_dv = av_dv.point_u(f_nx); 
    v_dv->reform(tree->nodeNum()); 
    double dv2_sum = 0; 
    int nx; 
    for (nx = 0; nx < tree->nodeNum(); ++nx) {
      double dv = v_dbar->get(nx); 
      int px = tree->node(nx)->parent_nx; 
      if (px >= 0) {
        dv -= v_dbar->get(px); 
      }
      v_dv->set(nx, dv); 

 
      int depth = tree->node(nx)->depth; 
      dv2_sum += reg_depth->apply(dv*dv, depth); 
    }
    v_dv2_sum.set(f_nx, dv2_sum); 
  }
}

/*--------------------------------------------------------*/
/* update v_v */
void AzReg_TsrOpt::update_v()
{
  v_v.reform(tree->nodeNum()); 
  int nx; 
  for (nx = 0; nx < tree->nodeNum(); ++nx) {
    double v = v_bar.get(nx); 
    int px = tree->node(nx)->parent_nx; 
    if (px >= 0) {
      v -= v_bar.get(px); 
    }
    v_v.set(nx, v); 
  }
  v_bar.reset(); 
}

/*--------------------------------------------------------*/
void AzReg_TsrOpt::_reset_forNewLeaf(const AzTrTree_ReadOnly *inp_tree, 
                                     const AzRegDepth *inp_reg_depth) 
{
  tree = inp_tree; 
  forNewLeaf = false; 
  focus_nx = -1; 
  reg_depth = inp_reg_depth; 
  resetTreeStructure(); 
  reset_v_dv(); 
  
  int node_num = tree->nodeNum(); 

  AzIntArr ia_nonleaf, ia_leaf; 
  int nx; 
  for (nx = 0; nx < node_num; ++nx) {
    if (!tree->node(nx)->isLeaf()) ia_nonleaf.put(nx); 
    else                           ia_leaf.put(nx); 
  }
  int split_nx = -1; 
  reset_bar(split_nx, &ia_leaf, &ia_nonleaf);

  curr_penalty = AzReg_Tsrbase::penalty();
}

/*--------------------------------------------------------*/
double AzReg_TsrOpt::_reset_forNewLeaf(int inp_focus_nx, 
                      const AzTrTree_ReadOnly *inp_tree, 
                      const AzRegDepth *inp_reg_depth)
{
  tree = inp_tree; 
  forNewLeaf = true; 
  focus_nx = inp_focus_nx; 
  reg_depth = inp_reg_depth; 
  resetTreeStructure(); 
  reset_v_dv(); 
  
  int node_num = tree->nodeNum(); 

  AzIntArr ia_nonleaf, ia_leaf; 
  int nx; 
  for (nx = 0; nx < node_num; ++nx) {
    if (!tree->node(nx)->isLeaf()) ia_nonleaf.put(nx); 
    else                           ia_leaf.put(nx); 
  }

  av_dbar.reset(node_num); 
  AzDvect *v_dbar = av_dbar.point_u(focus_nx); 

  deriv(focus_nx, &ia_nonleaf, v_dbar); 
  int split_nx = focus_nx; 
  reset_bar(split_nx, &ia_leaf, &ia_nonleaf); 

  double penalty_offset = AzReg_Tsrbase::penalty() - curr_penalty; 
  return penalty_offset; 
}

/*-------------------------