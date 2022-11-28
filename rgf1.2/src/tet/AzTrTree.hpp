/* * * * *
 *  AzTrTree.hpp 
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

#ifndef _AZ_TR_TREE_HPP_
#define _AZ_TR_TREE_HPP_

#include "AzUtil.hpp"
#include "AzDataForTrTree.hpp"
#include "AzTreeRule.hpp"
#include "AzLoss.hpp"
#include "AzSvFeatInfo.hpp"
#include "AzTreeNodes.hpp"
#include "AzTrTree_ReadOnly.hpp"
#include "AzTrTsplit.hpp"
#include "AzTrTtarget.hpp"
#include "AzTrTreeNode.hpp"
#include "AzTree.hpp"

/*---------------------------------------------*/
/* Abstract class: Trainable Tree              */
/* Derived classes: AzStdTree, AzRgfTree       */
/*---------------------------------------------*/
//! Abstract class: Trainable trees.  Building blocks for AzStdTree and AzRgfTree.  
class AzTrTree
 : /* implements */ public virtual AzTrTree_ReadOnly
{
protected:
  int root_nx; 
  int nodes_used; 
  AzTrTreeNode *nodes; 
  AzObjArray<AzTrTreeNode> a_node; 

  AzIntArr ia_root_dx; /*!!! Do NOT add components after generating the root.  */
                       /*!!! All the nodes refer to the components by pointer. */

  AzTrTsplit **split;  
  AzObjPtrArray<AzTrTsplit> a_split; 

  AzSortedFeatArr **sorted_arr; 
  AzObjPtrArray<AzSortedFeatArr> a_sorted_arr; 

  int curr_min_pop, curr_max_depth; 
  bool isBagging; 

public:
  AzTrTree() : 
    nodes_used(0), nodes(NULL), split(NULL), sorted_arr(NULL), root_nx(AzNone), 
    curr_min_pop(-1), curr_max_depth(-1), isBagging(false) {}

  /*---  derived classes must implement these             ---*/
  /*---------------------------------------------------------*/
  virtual bool usingInternalNodes() const = 0; 
  /*!  Return which nodes should make features.  */
  virtual void isActiveNode(bool doAllowZeroWeightLeaf, 
                    AzIntArr *ia_isActiveNode) /*!< output: array of size #node */
                    const = 0;
  /*---------------------------------------------------------*/

  /*---  for faster node search  ---*/
  virtual const AzSortedFeatArr *sorted_array(int nx, 
                             const AzDataForTrTre