/* * * * *
 *  AzSortedFeat.hpp 
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

#ifndef _AZ_SORTED_FEAT_HPP_
#define _AZ_SORTED_FEAT_HPP_

#include "AzUtil.hpp"
#include "AzSmat.hpp"
#include "AzDmat.hpp"


class AzSortedFeat
{
public:
  virtual int dataNum() const = 0; 
  virtual void rewind(AzCursor &cur) const = 0; 
  virtual const int *next(AzCursor &cur, double *out_val, int *out_num) const = 0; 
  virtual bool isForward() const = 0; 
  virtual void getIndexes(const int *inp_dxs, int inp_dxs_num, 
                              double border_val, 
                              /*---  output  ---*/
                              AzIntArr *ia_le_dx, 
                              AzIntArr *ia_gt_dx) const = 0; 
}; 

class AzSortedFeat_Dense : public virtual AzSortedFeat
{
protected:
  AzIntArr ia_index; 
  const int *index; 
  int index_num; 
  int offset; 
  const AzDvect *v_dx2v; 
  bool isOriginal; 

public:
  AzSortedFeat_Dense() : v_dx2v(NULL), index(NULL), index_num(0), 
                         offset(-1), isOriginal(false) {}
  AzSortedFeat_Dense(const AzDvect *v_data_transpose, 
                     const AzIntArr *ia_dx) 
                       : v_dx2v(NULL), index(NULL), index_num(0), 
                         offset(-1), isOriginal(false) {
    reset(v_data_transpose, ia_dx); 
  }
  AzSortedFeat_Dense(const AzSortedFeat_Dense *inp,  /* must not be NULL */
               const AzIntArr *ia_isYes,    
               int yes_num)
                       : v_dx2v(NULL), index(NULL), index_num(0), 
                         offset(-1), isOriginal(false) {
    filter(inp, ia_isYes, yes_num); 
  }
  AzSortedFeat_Dense(const AzSortedFeat_Dense *inp)
                       : v_dx2v(NULL), index(NULL), index_num(0), 
                         offset(-1), isOriginal(false) {
    copy_base(inp); 
  }

  void reset(const AzDvect *v_data_transpose, const AzIntArr *ia_dx); 
  void filter(const AzSortedFeat_Dense *inp,
              const AzIntArr *ia_isYes,
              int yes_num); 

  inline int dataNum() const {
    return index_num; 
  }

  inline int *base_index_for_update(int *len) {
    if (isOriginal) {
      throw new AzException("AzSortedFeat_Dense::base_index_for_update", 
                            "Not allowed"); 
    }
    return ia_index.point_u(len); 
  }

  inline void rewind(AzCursor &cur) const {
    cur.set(0); 
  }
  const int *next(AzCursor &cur, double *out_val, int *out_num) const; 

  inline bool isForward() const {
    return true; 
  }

  AzSortedFeat_Dense & operator =(const AzSortedFeat_Dense &inp) { /* never tested */
    if (this == &inp) return *this; 
    ia_index.reset(&inp.ia_index); 
    v_dx2v = inp.v_dx2v; 
    return *this; 
  }

  void getIndexes(const int *inp_dxs, int inp_dxs_num,  
                              double border_val, 
                              /*---  output  ---*/
                              AzIntArr *ia_le_dx, 
                         