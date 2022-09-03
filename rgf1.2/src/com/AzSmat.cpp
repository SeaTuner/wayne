/* * * * *
 *  AzSmat.cpp 
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


#include "AzUtil.hpp"
#include "AzSmat.hpp"
#include "AzPrint.hpp"

#define AzVectSmall 32

/*-------------------------------------------------------------*/
void AzSmat::initialize(int inp_row_num, int inp_col_num, bool asDense) 
{
  const char *eyec = "AzSmat::initialize (asDense)"; 
  if (inp_col_num < 0 || inp_row_num < 0) {
    throw new AzException(eyec, "#column and #row must be non-negative"); 
  }
  if (column != NULL || col_num > 0) {
    throw new AzException(eyec, "_release() must be called before this"); 
  }
  col_num = inp_col_num; 
  row_num = inp_row_num; 
  a.alloc(&column, col_num, eyec, "column"); 
  if (asDense) {
    int cx; 
    for (cx = 0; cx < col_num; ++cx) {
      column[cx] = new AzSvect(this->row_num, asDense); 
    }
  }
  dummy_zero.reform(row_num); 
}

/*-------------------------------------------------------------*/
void AzSmat::initialize(const AzSmat *inp) 
{
  if (inp == NULL) {
    throw new AzException("AzSmat::initialize(AzSmat*)", "null input"); 
  }
  bool asDense = false; 
  initialize(inp->row_num, inp->col_num, asDense); 
  if (inp->column != NULL) {
    int cx;
    for (cx = 0; cx < this->col_num; ++cx) {
      if (inp->column[cx] != NULL) {
        column[cx] = new AzSvect(inp->column[cx]); 
      }
    }
  }
}

/*-------------------------------------------------------------*/
void AzSmat::reform(int row_num, int col_num, bool asDense)
{
  _release(); 
  initialize(row_num, col_num, asDense); 
}

/*-------------------------------------------------------------*/
void AzSmat::resize(int new_col_num) 
{
  const char *eyec = "AzSmat::resize"; 
  if (new_col_num == col_num) {
    return; 
  } 
  if (new_col_num < 0) {
    throw new AzException(eyec, "new #columns must be positive"); 
  }
  a.realloc(&column, new_col_num, eyec, "column"); 
  col_num = new_col_num; 
}

/*-------------------------------------------------------------*/
void AzSmat::resize(int new_row_num, int new_col_num) 
{
  resize(new_col_num); 

  if (column != NULL) {
    int col; 
    for (col = 0; col < col_num; ++col) {
      if (column[col] != NULL) {
        column[col]->resize(new_row_num); 
      } 
    }
  }
  dummy_zero.resize(new_row_num); 
  row_num = new_row_num; 
}

/*-------------------------------------------------------------*/
void AzSvect::resize(int new_row_num) 
{
  if (new_row_num < row_num) {
    throw new AzException("AzSvect::resize", "no support for shrinking"); 
  }
  row_num = new_row_num; 
}

/*-------------------------------------------------------------*/
void AzSmat::set(const AzReadOnlyMatrix *inp) 
{
  _release(); 
  bool asDense = false; 
  initialize(inp->rowNum(), inp->colNum(), asDense); 
  int cx; 
  for (cx = 0; cx < col_num; ++cx) {
    delete column[cx]; column[cx] = NULL; 
    const AzReadOnlyVector *v = inp->col(cx); 
    if (!v->isZero()) {
      column[cx] = new AzSvect(v); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzSmat::set(const AzSmat *inp)  
{
  if (inp->row_num != this->row_num ||