/* * * * *
 *  AzOptOnTree.cpp 
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

#include "AzOptOnTree.hpp"
#include "AzTaskTools.hpp"
#include "AzHelp.hpp"

/*--------------------------------------------------------*/
void AzOptOnTree::reset(AzLossType l_type, 
                        const AzDvect *inp_v_y, 
                        const AzDvect *inp_v_fixed_dw, /* user-assigned data point weights */
                        const AzRegDepth *inp_reg_depth, 
                        AzParam &param, 
                        bool beVerbose, 
                        const AzOut out_req, 
                        /*---  for warm start  ---*/
                        const AzTrTreeEnsemble_ReadOnly *inp_ens, 
                        const AzTrTreeFeat *inp_tree_feat, 
                        const AzDvect *inp_v_p)

{
  _reset(); 

  reg_depth = inp_reg_depth; 
  out = out_req; 
  my_dmp_out = dmp_out; 

  v_y.set(inp_v_y); 
  if (!AzDvect::isNull(inp_v_fixed_dw)) {
    v_fixed_dw.set(inp_v_fixed_dw); 
  }
  v_p.reform(v_y.rowNum()); 
  loss_type = l_type; 

  resetParam(param); 

  if (max_delta < 0 && 
      AzLoss::isExpoFamily(loss_type)) {
    max_delta = 1; 
  }

  printParam(out, beVerbose); 
  checkParam(); 

  if (!beVerbose) {
    out.deactivate();  
    my_dmp_out.deactivate(); 
  }

  var_const = 0; 
  fixed_const = 0; 
  if (doUseAvg) {
    fixed_const = v_y.sum() / (double)v_y.rowNum(); 
    v_p.set(fixed_const); 
  }

  if (inp_ens != NULL) {
    _warmup(inp_ens, inp_tree_feat, inp_v_p); 
  }
}

/*--------------------------------------------------------*/
void 
AzOptOnTree::_warmup(const AzTrTreeEnsemble_ReadOnly *inp_ens, 
                    const AzTrTreeFeat *inp_tree_feat, 
                    const AzDvect *inp_v_p)
{
  v_w.reform(inp_tree_feat->featNum()); 
  var_const = inp_ens->constant() - fixed_const; 

  int fx; 
  int f_num = inp_tree_feat->featNum(); 
  for (fx = 0; fx < f_num; ++fx) {
    const AzTrTreeFeatInfo *fp = inp_tree_feat->featInfo(fx); 
    if (fp->isRemoved) continue; 
    double w = inp_ens->tree(fp->tx)->node(fp->nx)->weight; 
    v_w.set(fx, w); 
  }

  v_p.set(inp_v_p); 
}

/*--------------------------------------------------------*/
void AzOptOnTree::copy_from(const AzOptOnTree *inp)
{
  v_w.set(&inp->v_w); 
  v_y.set(&inp->v_y); 
  v_fixed_dw.set(&inp->v_fixed_dw); 
  v_p.set(&inp->v_p); 

  var_const = inp->var_const; 
  fixed_const = inp->fixed_const; 

  /*-----  parameters  -----*/
  eta = inp->eta; 
  lambda = inp->lambda; 
  sigma = inp->sigma; 
  exit_delta = inp->exit_delta; 
  max_delta = inp->max_delta; 
  reg_depth = inp->reg_depth; 
  loss_type = inp->loss_type; 
  max_ite_num = inp->max_ite_num; 

  doRefreshP = inp->doRefreshP; 
  doIntercept = inp->doIntercept; 
  doUnregIntercept = inp->doUnregIntercept; 
  doUseAvg = inp->doUseAvg; 

  ens = NULL; 
  tree_feat = NULL; 

  out = inp->out; 
  my_dmp_out = inp->my_dmp_out; 
}

/*--------------------------------------------------------*/
void AzOptOnTree::synchronize()
{
  checkParam();

  int f_num = tree_feat->featNum(); 
  int old_f_num = v_w.rowNum(); 
  v_w.resize(f_num); 

  bool isThereChange = false; 
  int fx; 
  for (fx = 0; fx < old_f_num; ++fx) {
    if (tree_feat->featInfo(fx)->isRemoved && v_w.get(fx) != 0) {
      isThereChange = true; 
      v_w.set(fx, 0); 
    }
  }
  if (isThereChange || doRefreshP) {
    refreshPred(); 
  }
}

/*--------------------------------------------------------*/
void AzOptOnTree::iterate(int inp_ite_num, 
                             double lam, 
                             double sig)
{
  int ite_num = max_ite_num; 
  if (inp_ite_num >= 0) {
    ite_num = inp_ite_num; 
  }
  if (ite_num > 0 && !out.isNull()) {
    AzPrint o(out); 
    o.reset_options(); 
    o.printBegin("AzOptOnTree::update", ","); 
    o.printV("lam=", (lam>=0)?lam:lambda); 
    o.printV_posiOnly("sig=", (sig>=0)?sig:sigma); 
//    o.printLoss("", loss_type); 
    o.printV("", AzLoss::lossName(loss_type)); 
    o.printEnd(); 
  }
  if (ite_num <= 0) {
    return; 
  }

  double nn; 
  if (AzDvect::isNull(&v_fixed_d