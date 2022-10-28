/* * * * *
 *  AzSvFeatInfoClone.hpp 
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

#ifndef _AZ_SV_FEAT_INFO_CLONE_HPP_
#define _AZ_SV_FEAT_INFO_CLONE_HPP_

#include "AzSvFeatInfo.hpp"
#include "AzStrArray.hpp"

class AzSvFeatInfoClone : /* implements */ public virtual AzSvFeatInfo, 
                          /* implements */ public virtual AzStrArray 
{
protected:
  AzDataPool<AzBytArr> arr_desc; 

public: 
  AzSvFeatInfoClone() {}
  AzSvFeatInfoClone(const AzSvFeatInfo *inp) {
    reset(inp);   
  }
  AzSvFeatInfoClone(const AzStrArray *inp) {
    reset(inp);   
  }
  inline int featNum() const { 
    return arr_desc.size(); 
  }
  inline void concatDesc(int fx, AzBytArr *desc) const {
    if (fx < 0 || fx >= featNum()) {
      desc->c("?"); desc->cn(fx); desc->c("?");  
      return; 
    }
    desc->concat(arr_desc.point(fx)); 
  }
  void reset(const AzSvFeatInfo *inp) {
    int f_num = inp->featNum(); 
    arr_desc.reset(); 
    int fx; 
    for (fx = 0; fx < f