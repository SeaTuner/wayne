/* * * * *
 *  AzReg_TreeReg.hpp 
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

#ifndef _AZ_REG_TREE_REG_HPP_
#define _AZ_REG_TREE_REG_HPP_

#include "AzUtil.hpp"
#include "AzDmat.hpp"
#include "AzTrTree_ReadOnly.hpp"
#include "AzRegDepth.hpp"
#include "AzParam.hpp"

class AzReg_TreeRegShared {
public:
  virtual AzDmat *share() = 0; 
  virtual bool create(const AzTrTree_ReadOnly *tree, const AzDmat *info) = 0; 
  virtual AzDmat *share(const AzTrTree_ReadOnly *tree) = 0; 
}; 

//! Default implementation of AzReg_TreeRegShared 
class AzReg_TreeRegShared_Dflt : /* implements */ public virtual AzReg_TreeRegShared
{
protected:
  AzDmat m_by_alltree; /* info not specific to individual tree */

public:
  /*---  override these to store tree-specific info  ---*/
  virtual AzDmat *share(const AzTrTree_ReadOnly *tree) { return NULL; }
  virtual bool create(const AzTrTree_ReadOnly *tree, const AzDmat *) { return false; }
  /*----------------------------------------------------*/
  virtual AzDmat *share() {
    return &m_by_alltree;
  }
}; 

//! Abstract class: interface to tree-structured regularizer 
class AzReg_TreeReg {
public:
  virtual void set_shared(AzReg_TreeRegShared *shared) {} 
  virtual void check_reg_depth(const AzRe