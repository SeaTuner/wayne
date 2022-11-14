/* * * * *
 *  AzRgfTree.cpp 
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

#include "AzRgfTree.hpp"
#include "AzHelp.hpp"
#include "AzRgf_kw.hpp"

/*--------------------------------------------------------*/
void AzRgfTree::findSplit(AzRgf_FindSplit *fs, 
                          const AzRgf_FindSplit_input &inp, 
                          bool doRefreshAll, 
                          /*---  output  ---*/
                          AzTrTsplit *best_split) const 
{
  const char *eyec = "AzRgfTree::findSplit"; 
  if (nodes_used <= 0) {
    return; 
  }

  AzTrTree::_checkNodes(eyec); 
  int leaf_num = leafNum(); 
  if (max_leaf_num > 0) {
    if (leaf_num >= max_leaf_num) {
      return;   
    }
  }

  _findSplit_begin(fs, inp); 

  int nx; 
  for (nx = 0; nx < nodes_used; ++nx) {
    if (!nodes[nx].isLeaf()) continue; 

    /*---  ---*/
    if (max_depth > 0 && nodes[nx].depth >= max_depth) {
      continue; 
    }
    if (min_size > 0 && nodes[nx].dxs_num < min_size*2) {
      continue; 
    }

    _findSplit(fs, nx, doRefreshAll); 

    if (split[nx]->fx >= 0 && 
        split[nx]->gain > best_split->gain) {
      best_split->reset(split[nx], inp.tx, nx); 
    }
  }                              
  _findSplit_end(fs); 
}

/*--------------------------------------------------------*/
void AzRgfTree::removeSplitAssessme