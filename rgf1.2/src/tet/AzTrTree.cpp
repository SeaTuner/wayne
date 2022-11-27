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
  _checkNode(nx, "AzTrTree::sp