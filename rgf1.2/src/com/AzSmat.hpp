
/* * * * *
 *  AzSmat.hpp 
 *  Copyright (C) 2011-2014 Rie Johnson
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

#ifndef _AZ_SMAT_HPP_
#define _AZ_SMAT_HPP_

#include "AzUtil.hpp"
#include "AzStrArray.hpp"
#include "AzReadOnlyMatrix.hpp"

/* Changed AZ_MTX_FLOAT from single-precision to double-precision  */
typedef double AZ_MTX_FLOAT; 
#define _checkVal(x) 
/* static double _too_large_ = 16777216; */
/* static double _too_small_ = -16777216; */

typedef struct {
  int no; 
  AZ_MTX_FLOAT val; 
} AZI_VECT_ELM; 

class AzSmat; 

//! sparse vector 
class AzSvect : /* implements */ public virtual AzReadOnlyVector {
protected:
  int row_num; 
  AZI_VECT_ELM *elm; 
  AzBaseArray<AZI_VECT_ELM> a; 
  int elm_num; 

  void _release() {
    a.free(&elm); elm_num = 0; 
  }

public:
  friend class AzDmat; 
  friend class AzDvect; 
  friend class AzPmatSpa; 
  
  AzSvect() : row_num(0), elm(NULL), elm_num(0) {}
  AzSvect(int inp_row_num, bool asDense=false) : row_num(0), elm(NULL), elm_num(0) {
    initialize(inp_row_num, asDense); 
  }
  AzSvect(const AzSvect *inp) : row_num(0), elm(NULL), elm_num(0) {
    row_num = inp->row_num;  
    set(inp); 
  }
  AzSvect(const AzSvect &inp) : row_num(0), elm(NULL), elm_num(0) {
    row_num = inp.row_num; 
    set(&inp); 
  }
  AzSvect & operator =(const AzSvect &inp) {
    if (this == &inp) return *this; 
    _release(); 
    row_num = inp.row_num; 
    set(&inp); 
    return *this; 
  }
  AzSvect(const AzReadOnlyVector *inp) : row_num(0), elm(NULL), elm_num(0) {
    row_num = inp->rowNum();  
    set(inp); 
  }
  AzSvect(AzFile *file) : row_num(0), elm(NULL), elm_num(0) {
    _read(file); 
  }
  ~AzSvect() {}

  void read(AzFile *file) {
    _release(); 
    _read(file); 
  }

  void resize(int new_row_num); /* new #row must be greater than #row */
  void reform(int new_row_num, bool asDense=false); 
  void change_rowno(int new_row_num, const AzIntArr *ia_old2new, bool do_zero_negaindex=false); 
  
  void write(AzFile *file); 

  inline int rowNum() const { return row_num; }  
  void load(const AzIntArr *ia_row, double val, bool do_ignore_negative_rowno=false); 
  void load(const AzIFarr *ifa_row_val); 
  void load(AzIFarr *ifa_row_val) {
    ifa_row_val->sort_Int(true); 
    load((const AzIFarr *)ifa_row_val); 
  }   
  bool isZero() const;
  bool isOneOrZero() const; 

  void cut(double min_val); 
  void only_keep(int num); /* keep this number of components with largest fabs */
  void zerooutNegative(); /* not tested */
  void nonZero(AzIFarr *ifq, const AzIntArr *ia_sorted_filter) const; 

  void filter(const AzIntArr *ia_sorted, /* must be sorted; can have duplications */
              /*---  output  ---*/
              AzIFarr *ifa_nonzero, 
              AzIntArr *ia_zero) const; 

  int nonZeroRowNo() const; /* returns the first one */
  void nonZero(AzIFarr *ifa) const; 
  void all(AzIFarr *ifa) const;  /* not tested */
  void zeroRowNo(AzIntArr *ia) const;  /* not tested */
  void nonZeroRowNo(AzIntArr *intq) const; 
  int nonZeroRowNum() const; 

  void set_inOrder(int row_no, double val); 
  void set(int row_no, double val); 
  void set(double val); 
  void set(const AzSvect *vect1, double coefficient=1); 
  void set(const AzReadOnlyVector *vect1, double coefficient=1);  

  double get(int row_no) const; 

  double sum() const; 
  double absSum() const; 

  void add(int row_no, double val); 

  void multiply(int row_no, double val); 
  void multiply(double val); 
  inline void divide(double val) {
    if (val == 0) throw new AzException("AzSvect::divide", "division by zero"); 
    multiply((double)1/val); 
  }
 
  void plus_one_log();  /* x <- log(x+1) */

  void scale(const double *vect1); 

  double selfInnerProduct() const; 
  inline double squareSum() const { return selfInnerProduct(); }
  
  void log_of_plusone(); /* log(x+1) */
  double normalize(); 
  double normalize1(); 
  void binarize();  /* posi -> 1, nega -> -1 */
  void binarize1(); /* nonzero -> 1 */
  
  void clear(); 
  void zeroOut(); 

  int next(AzCursor &cursor, double &out_val) const; 

  double minPositive(int *out_row_no = NULL) const; 
  double min(int *out_row_no = NULL, 
                     bool ignoreZero=false) const; 
  double max(int *out_row_no = NULL, 
                    bool ignoreZero=false) const; 
  double maxAbs(int *out_row_no = NULL, double *out_real_val = NULL) const; 

  void polarize(); 
  
  void dump(const AzOut &out, const char *header, 
            const AzStrArray *sp_row = NULL, 
            int cut_num = -1) const; 

  void clear_prepare(int num); 
  bool isSame(const AzSvect *inp) const; 
  void cap(double cap_val); 

protected:
  void _read(AzFile *file); 
  void _swap(); 

  void initialize(int inp_row_num, bool asDense); 
  int to_insert(int row_no); 
  int find(int row_no, int from_this = -1) const; 
  int find_forRoom(int row_no, 
                   int from_this = -1, 
                    bool *out_isFound = NULL) const; 

  void _dump(const AzOut &out, const AzStrArray *sp_row, 
             int cut_num = -1) const; 

  inline int inc() const {
    return MIN(4096, MAX(32, elm_num)); 
  }
}; 
