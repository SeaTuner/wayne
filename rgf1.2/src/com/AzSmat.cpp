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
      inp->col_num != this->col_num) {
    reform(inp->row_num, inp->col_num); 
  }
  int cx; 
  for (cx = 0; cx < this->col_num; ++cx) {
    if (inp->column[cx] == NULL) {
      delete column[cx]; 
      column[cx] = NULL; 
    }
    else {
      if (column[cx] == NULL) {
        column[cx] = new AzSvect(row_num); 
      }
      column[cx]->set(inp->column[cx]); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzSmat::set(const AzSmat *inp, int col0, int col1)  
{
  if (col0 < 0 || col1 < 0 || col1 > inp->col_num || col0 >= col1) {
    throw new AzException("AzSmat::set(inp,col0,col1)", "out of range"); 
  }
  if (inp->row_num != this->row_num || 
      inp->col_num != col1-col0) {
    reform(inp->row_num, col1-col0); 
  }
  int cx; 
  for (cx = col0; cx < col1; ++cx) {
    int my_cx = cx - col0; 
    if (inp->column[cx] == NULL) {
      delete column[my_cx]; 
      column[my_cx] = NULL; 
    }
    else {
      if (column[my_cx] == NULL) {
        column[my_cx] = new AzSvect(row_num); 
      }
      column[my_cx]->set(inp->column[cx]); 
    }
  }
}

/*-------------------------------------------------------------*/
int AzSmat::set(const AzSmat *inp, const int *cols, int cnum,  /* new2old */
                bool do_zero_negaindex) 
{
  if (row_num != inp->row_num || col_num != cnum) {
    reform(inp->row_num, cnum); 
  }
  int negaindex = 0; 
  int my_col; 
  for (my_col = 0; my_col < cnum; ++my_col) {
    int col = cols[my_col]; 
    if (col < 0 && do_zero_negaindex) {
      delete column[my_col]; 
      column[my_col] = NULL;     
      ++negaindex; 
      continue; 
    }   
    if (col < 0 || col >= inp->col_num) {
      throw new AzException("AzSmat::set(inp,cols,cnum)", "invalid col#"); 
    }
    if (inp->column[col] == NULL) {
      delete column[my_col]; 
      column[my_col] = NULL; 
    }
    else {
      if (column[my_col] == NULL) {
        column[my_col] = new AzSvect(row_num); 
      }
      column[my_col]->set(inp->column[col]); 
    }
  }
  return negaindex; 
}

/*-------------------------------------------------------------*/
void AzSmat::set(int col0, int col1, const AzSmat *inp, int i_col0)
{
  const char *eyec = "AzSmat::set(col0,col1,inp)"; 
  if (col0 < 0 || col1-col0 <= 0 || col1 > col_num) {
    throw new AzException(eyec, "requested columns are out of range"); 
  }
  int i_col1 = i_col0 + (col1-col0); 
  if (i_col0 < 0 || i_col1 > inp->col_num) {
    throw new AzException(eyec, "requested columns are out of range in the input matrix"); 
  }
  if (row_num != inp->row_num) {
    throw new AzException(eyec, "#rows mismatch"); 
  }
  int i_col = i_col0; 
  int col; 
  for (col = col0; col < col1; ++col, ++i_col) {
    if (inp->column[i_col] == NULL) {
      delete column[col]; column[col] = NULL; 
    }
    else {
      if (column[col] == NULL) {
        column[col] = new AzSvect(row_num); 
      }
      column[col]->set(inp->column[i_col]);       
    }
  }
}             
                
/*-------------------------------------------------------------*/
void AzSmat::reduce(const int *cols, int cnum)  /* new2old; must be sorted */
{
  if (column == NULL) {
    reform(row_num, cnum); 
    return; 
  }

  int negaindex = 0; 
  int new_col; 
  for (new_col = 0; new_col < cnum; ++new_col) {
    int old_col = cols[new_col]; 
    if (old_col < 0 || old_col >= col_num) {
      throw new AzException("AzSmat::reduce(cols,cnum)", "invalid col#"); 
    }
    if (new_col > 0 && old_col <= cols[new_col-1]) {
      throw new AzException("AzSmat::reduce(cols,cnum)", "column# must be sorted"); 
    }
    
    if (old_col == new_col) {}
    else if (column[old_col] == NULL) {
      delete column[new_col]; 
      column[new_col] = NULL; 
    }
    else {
      if (column[new_col] == NULL) {
        column[new_col] = new AzSvect(row_num); 
      }
      column[new_col]->set(column[old_col]); 
    }
  }
  resize(cnum); 
}

/*-------------------------------------------------------------*/
void AzSmat::transpose(AzSmat *m_out, 
                       int col_begin, int col_end) const
{
  int col_b = col_begin, col_e = col_end; 
  if (col_b < 0) {
    col_b = 0; 
    col_e = col_num; 
  }
  else {
    if (col_b >= col_num || 
        col_e < 0 || col_e > col_num || 
        col_e - col_b <= 0) {
      throw new AzException("AzSmat::transpose", "column range error"); 
    }
  }

  _transpose(m_out, col_b, col_e); 
}

/*-------------------------------------------------------------*/
void AzSmat::_transpose(AzSmat *m_out, 
                        int col_begin, 
                        int col_end) const
{
  int row_num = rowNum(); 

  m_out->reform(col_end - col_begin, row_num); 

  AzIntArr ia_row_count; 
  ia_row_count.reset(row_num, 0); 
  int *row_count = ia_row_count.point_u(); 

  int cx; 
  for (cx = col_begin; cx < col_end; ++cx) {
    /* rewind(cx); */
    AzCursor cursor; 
    for ( ; ; ) {
      double val; 
      int rx = next(cursor, cx, val); 
      if (rx < 0) break;

      ++row_count[rx]; 
    }
  }
  int rx; 
  for (rx = 0; rx < row_num; ++rx) {
    if (row_count[rx] > 0) {
      m_out->col_u(rx)->clear_prepare(row_count[rx]); 
    }
  }

  for (cx = col_begin; cx < col_end; ++cx) {
    /* rewind(cx); */
    AzCursor cursor; 
    for ( ; ; ) {
      double val; 
      int rx = next(cursor, cx, val); 
      if (rx 