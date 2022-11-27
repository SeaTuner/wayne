/* * * * *
 *  AzTrTree.cpp 
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

#include "AzTrTree.hpp"
#include "AzTools.hpp"
#include "AzPrint.hpp"

/*--------------------------------------------------------*/
void AzTrTree::_release()
{
  ia_root_dx.reset(); 
  a_node.free(&nodes); nodes_used = 0; 
  a_split.free(&split); 
  a_sorted_arr.free(&sorted_arr); 

  root_nx = AzNone; 
  curr_min_pop = curr_max_depth = -1; 
}

/*--------------------------------------------------------*/
void AzTrTree::_releaseWork()
{
  a_split.free(&split); 
  a_sorted_arr.free(&sorted_arr);
}

/*--------------------------------------------------------*/
double AzTrTree::getRule(int inp_nx, 
                        AzTreeRule *rule) const
{
  _checkNode(inp_nx, "AzTrTree::getRule"); 
  int child_nx = inp_nx; 
  int nx = nodes[inp_nx].parent_nx; 
  for ( ; ; ) {
    if (nx < 0) break; 

    /*---  feat#, isLE, border_val  ---*/
    bool isLE = false; 
    if (child_nx == nodes[nx].le_nx) {
      isLE = true; 
    }
    rule->append(nodes[nx].fx, isLE, 
                 nodes[nx].border_val); 

    child_nx = nx; 
    nx = nodes[nx].parent_nx; 
  }

  rule->finalize(); 

  return nodes[inp_nx].weight; 
}

/*--------------------------------------------------------*/
void AzTrTree::concat_stat(AzBytArr *o) const 
{
  if (o == NULL) return; 
  int leaf_num = leafNum(); 
  o->c("#node=", nodes_used, 3);
  o->c(",#leaf=", leaf_num, 3);
  o->c(",depth=", curr_max_depth, 2);
  o->c(",min(pop)=", curr_min_pop, 2);
}

/*--------------------------------------------------------*/
void AzTrTree::_genRoot(int max_size, 
                        const AzDataForTrTree *data, 
                        const AzIntArr *ia_dx)
{
  _release(); 

  /*---  generate the root  --*/
  root_nx = _newNode(max_size); 
  AzTrTreeNode *root_np = &nodes[root_nx]; 

  root_np->depth = 0; 
  if (ia_dx == NULL) {
    int data_num = data->dataNum(); 
    ia_root_dx.range(0, data_num); 
  }
  else {
    ia_root_dx.reset(ia_dx); 
  }
  root_np->dxs = ia_root_dx.point(&root_np->dxs_num); 
  root_np->dxs_offset = 0; 
}

/*--------------------------------------------------------*/
int AzTrTree::_newNode(int max_size)
{
  const char *eyec = "AzTrTree::_newNode"; 

  int node_no = nodes_used; 
  int node_max = a_node.size(); 

  if (nodes_used >= node_max) {
    int inc = MAX(node_max, 128); 
    if (max_size > 0) {
      inc = MIN(inc, max_size*2); 
    }
    node_max += inc; 
    a_node.realloc(&nodes, node_max, eyec, "node"); 
    a_split.realloc(&split, node_max, eyec, "split"); 
    a_sorted_arr.realloc(&sorted_arr, node_max, eyec, "sorted_arr"); 
  } 
  else {
    /*---  initialize the new node  ---*/
    nodes[node_no].reset();
  }
  ++nodes_used; 
  return node_no; 
}

/*--------------------------------------------------------*/
void AzTrTree::show(const AzSvFeatInfo *feat, const AzOut &out) const
{
  if (out.isNull()) return; 
  if (nodes == NULL) {
    AzPrint::writeln(out, "AzTrTree::show, No tree"); 
    return; 
  }
  _show(feat, root_nx, 0, out); 
}

/*--------------------------------------------------------*/
void AzTrTree::_show(const AzSvFeatInfo *feat, 
                    int nx, 
                    int depth, 
                    const AzOut &out) const
{
  if (out.isNull()) return; 

  const AzTrTreeNode *np = &nodes[nx]; 
  int pop = np->dxs_num; 

  AzPrint o(out); 
  o.printBegin("", ", ", "=", depth*2); 
  /* [nx], (pop,weight), depth=d, desc,border */
  if (np->isLeaf()) {
    o.print("*"); 
    o.disableDlm(); 
  }
  o.inBrackets(nx,3); 
  o.enableDlm(); 
  if (pop >= 0 || np->weight != 0) {
    AzBytArr s_pop_weight; 
    if (pop >= 0) s_pop_weight.cn(pop,3); 
    s_pop_weight.c(", "); 
    if (np->weight != 0) s_pop_weight.cn(np->weight,3); 
    o.inParen(s_pop_weight); 
  }

  o.printV("depth=", depth); 
  if (np->fx >= 0 && feat != NULL) {
    AzBytArr s_desc; 
    feat->concatDesc(np->fx, &s_desc); 
    o.print(&s_desc); 
    o.print(np->border_val,4,false); 
  }
  o.printEnd(); 

  if (!np->isLeaf()) {
    _show(feat, np->le_nx, depth+1, out); 
    _show(feat, np->gt_nx, depth+1, out); 
  }
}

/*--------------------------------------------------------*/
void AzTrTree::_splitNode(const AzDataForTrTree *data, 
                        int max_size, 
                        bool doUseInternalNodes, 
                        int nx,
                        const AzTrTsplit *inp, 
                        const AzOut &out)
{
  _checkNode(nx, "AzTrTree::splitNode"); 

  nodes[nx].fx = inp->fx; 
  nodes[nx].border_val = inp->border_val; 

  AzIntArr ia_le, ia_gt; 
  const AzSortedFeatArr *s_arr = sorted_arr[nx]; 
  if (s_arr == NULL) {
    if (nx == root_nx) {
      s_arr = data->sorted_array(); 
    }
    else {
      throw new AzException("AzTrTree::_splitNode", "sorted_arr[nx]=null"); 
    }
  }
  const AzSortedFeat *sorted = s_arr->sorted(inp->fx); 
  if (sorted == NULL) {
    AzSortedFeatWork tmp; 
    const AzSortedFeat *my_sorted = sorted_arr[nx]->sorted(data->sorted_array(), 
                                    inp->fx, &tmp); 
    my_sorted->getIndexes(nodes[nx].dxs, nodes[nx].dxs_num, inp->border_val, 
                          &ia_le, &ia_gt); 
  }
  else {
    sorted->getIndexes(nodes[nx].dxs, nodes[nx].dxs_num, inp->border_val, 
                       &ia_le, &ia_gt); 
  }

  int le_offset = nodes[nx].dxs_offset; 
  int gt_offset = le_offset + ia_le.size(); 

  int le_nx = _newNode(max_size); 
  nodes[nx].le_nx = le_nx; 
  AzTrTreeNode *np = &nodes[le_nx]; 
  np->depth = nodes[nx].depth + 1;
  np->dxs_offset = le_offset; 
  np->dxs = set_data_indexes(le_offset, ia_le.point(), ia_le.size()); 
  np->dxs_num = ia_le.size(); 
  np->parent_nx = nx; 
  np->weight = inp->bestP[0]; 
  if (curr_min_pop < 0 || np->dxs_num < curr_min_pop) curr_min_pop = np->dxs_num; 
  curr_max_depth = MAX(curr_max_depth, np->depth); 

  int gt_nx = _newNode(max_size);   
  nodes[nx].gt_nx = gt_nx; 
  np = &nodes[gt_nx]; 
  np->depth = nodes[nx].depth + 1; 
  np->dxs_offset = gt_offset; 
  np->dxs = set_data_indexes(gt_offset, ia_gt.point(), ia_gt.size()); 
  np->dxs_num = ia_gt.size(); 
  np->parent_nx = nx; 
  np->weight = inp->bestP[1]; 
  curr_min_pop = MIN(curr_min_pop, np->dxs_num); 

  /*------------------------------*/
  double org_weight = nodes[nx].weight; 
  if (!doUseInternalNodes) {
    nodes[nx].weight = 0; 
  }

  /*------------------------------*/
  dump_split(inp, nx, org_weight, out); 

  /*---  release split info for the node we just split  ---*/
  delete split[nx]; split[nx] = NULL; 
}

/*--------------------------------------------------------*/
void AzTrTree::dump_split(const AzTrTsplit *inp, 
                     int nx, 
                     double org_weight, 
                     const AzOut &out)
{
  if (out.isNull()) return; 

  _checkNode(nx, "AzTrTree::dump_split"); 
  int gt_nx = nodes[nx].gt_nx; 
  int le_nx = nodes[nx].le_nx; 

  AzPrint o(out); 
  o.printBegin("", ", ", "="); 
  if (inp->tx >= 0) {
    o.pair_inBrackets(inp->tx, nx, ":"); 
  }
  else {
    o.inBrackets(nx); 
  }
  o.print("d", nodes[nx].depth); 
  o.print("fx", nodes[nx].fx); o.print(nodes[nx].border_val, 5); 

  o.print(nodes[nx].dxs_num); 
  o.disableDlm(); 
  o.print("->"); o.pair_inParen(nodes[le_nx].dxs_num, nodes[gt_nx].dxs_num, ","); 
  o.enableDlm(); 

  o.print(org_weight,4); 
  o.disableDlm(); 
  o.print("->"); 
  o.set_precision(4); 
  o.pair_inParen(nodes[le_nx].weight, nodes[gt_nx].weight, ","); 
  o.enableDlm(); 

  o.print("gain", inp->gain, 5); 
  o.print(inp->str_desc.c_str()); 
  o.printEnd(); 
  o.flush(); 
}

/*--------------------------------------------------------*/
bool AzTrTree::isEmptyTree() const
{
  if (nodes_used == 1 && 
      nodes[0].weight == 0) {
    return true; 
  }
  return false; 
}

/*--------------------------------------------------------*/
int AzTrTree::leafNum() const
{
  _checkNodes("AzTrTree::leafNum"); 
  int leaf_num = 0; 
  int nx; 
  for (nx = 0; nx < nodes_used; ++nx) {
    if (nodes[nx].isLeaf()) {
      ++leaf_num; 
    }
  }
  return leaf_num; 
}

/*--------------------------------------------------------*/
void AzTrTree::concatDesc(const AzSvFeatInfo *feat, 
                      int nx, 
                      AzBytArr *str_desc, /* output */
                      int max_len) const
{
  if (feat == NULL) {
    str_desc->concat("not_available"); 
  }

  _checkNode(nx, "AzTrTree::concatDesc"); 
  if (nx == root_nx) {
    str_desc->concat("ROOT"); 
  }
  else {
    _genDesc(feat, nx, str_desc); 
  }

  if (max_len > 0 && str_desc->getLen() > max_len) {
    AzBytArr str(str_desc->point(), max_len); 
    str.concat("..."); 
    str_desc->clear(); 
    str_desc->concat(&str); 
  }
}

/*--------------------------------------------------------*/
void AzTrTree::_genDesc(const AzSvFeatInfo *feat, 
                       int nx, 
                       AzBytArr *str_desc) /* output */
					   const
{
  int px = nodes[nx].parent_nx; 
  if (px < 0) return; 

  _genDesc(feat, px, str_desc); 
  if (str_desc->getLen() > 0) {
    str_desc->concat(";"); 
  }
  feat->concatDesc(nodes[px].fx, str_desc); 
  if (nodes[px].le_nx == nx) {
    str_desc->concat("<="); 
  }
  else {
    str_desc->concat(">"); 
  }
  /* sprintf(work, "%5.3f", nodes[px]->border_val); */
  str_desc->concatFloat(nodes[px].border_val, 5); 
}

/*--------------------------------------------------------*/
void AzTrTree::_isActiveNode(bool doWantInternalNodes, 
                                 bool doAllowZeroWeightLeaf, 
                                 AzIntArr *ia_isActiveNode) const
{
  _checkNodes("AzTree_Reggfo::isActiveNode"); 
  ia_isActiveNode->reset(nodes_used, false); 
  int *isActive = ia_isActiveNode->point_u(); 
  int nx; 
  for (nx = 0; nx < nodes_used; ++nx) {
    if (nodes[nx].isLeaf()) {
      if (nodes[nx].weight != 0) {
        isActive[nx] = true; 
      