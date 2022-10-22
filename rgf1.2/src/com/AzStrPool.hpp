/* * * * *
 *  AzStrPool.hpp 
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

#ifndef _AZ_STR_POOL_HPP_
#define _AZ_STR_POOL_HPP_

#include "AzUtil.hpp"
#include "AzStrArray.hpp"

typedef struct {
public:
  AZint8 offs; 
  int len; 
  AZint8 count;
  int value; 
  const AzByte *bytes; /* we need this for qsort */
} AzSpEnt; 

typedef struct {
  int begin; 
  int end; 
  int min_len; 
  int max_len; 
} AzSpIndex; 

//! Store byte arrays or strings.  Searchable after committed. 
//  max(#entry): 2^31-1
//  max(total length of data): 2^63-1 (since June 2014)
class AzStrPool : public virtual AzStrArray {
protected:
  AzSpEnt *ent; 
  AzBaseArray<AzSpEnt> a_ent; 
  int ent_num; 

  AzByte *data; 
  AzBaseArray<AzByte,AZint8> a_data; 
  AZint8 data_len; 

  bool isCommitted;  

  AzSpIndex *my_idx; 
  AzBaseArray<AzSpIndex> a_index; 

  int init_ent_num;
  AZint8 init_data_len; 
public:
  AzStrPool(); 
  AzStrPool(int init_num, AZint8 avg_data_len); 
  AzStrPool(const AzStrPool *inp_sp); /* copy */
  AzStrPool(const AzStrArray *inp_sp); /* copy */
  AzStrPool(AzFile *file); 
  ~AzStrPool() {}

  void reset(); 
  inline void reset(int init_num, AZint8 avg_data_len) {
    reset(); 
    init_ent_num = MAX(init_num, 64); 
    init_data_len = init_ent_num * MAX(1,avg_data_len); 
  }

  void read(AzFile *file) {
    reset(); 
    _read(file); 
  }

  void write(AzFile *file); 
  void write(const char *fn) {
    AzFile::write(fn, this); 
  }
  
  void reset(const AzStrPool *inp) {
    reset(); 
    _copy(inp); 
  }

  void copy(AzStrArray *sp2) {
    reset(); 
    _copy(sp2); 
  }

  int put(const AzByte *bytes, int bytes_len, 
          AZint8 count=1, 
          int value=-1); 
  virtual int put(const char *str, AZint8 count=1) {
    return put((AzByte *)str, Az64::cstrlen(str), count);     
  }
  int put(const AzBytArr *byteq, AZint8 count=1) {
    return put(byteq->point(), byteq->getLen(), count); 
  }

  inline int putv(const AzBytArr *bq, int value) {
    AZint8 count = 1; 
    return put(bq->point(), bq->getLen(), count, value); 
  }        
  inline virtual int putv(const char *str, int value) {
    AZint8 count = 1; 
    return put((AzByte *)str, Az64::cstrlen(str), count, value); 
  }  
  inline int getValue(int ent_no) const {
    checkRange(ent_no, "AzStrPool::getValue"); 
    return ent[ent_no].value; 
  }
  inline void setValue(int ent_no, int value) {
    checkRange(ent_no, "AzStrPool::setValue"); 
    ent[ent_no].value = value; 
  }
  void add_to_value(int added) {
    int ex; 
    for (ex = 0; ex < ent_num; ++ex) ent[ex].value += added; 
  }

  void setCount(int ent_no, AZint8 count) {
    checkRange(ent_no, "AzStrPool::setCount"); 
    ent[ent_no].count = count; 
  }

  void put(AzDataArray<AzBytArr> *aStr) {
    int num = aStr->size(); 
    int ix; 
    for (ix = 0; ix < num; ++ix) {
      put(aStr->point(ix)); 
    }
  }
  void put(const AzStrPool *inp) {
    int num = inp->size(); 
    int ix; 
    for (ix = 0; ix < num; ++ix) {
      int len; 
      const AzByte *str = inp->point(ix, &len); 
      AZint8 count = inp->getCount(ix); 
      int value = inp->getValue(ix); 
      put(str, len, count, value); 
    }
  }
  inline void append(const AzStrPool *inp) {
    put(inp); 
  }
  virtual void add_prefix(const char *str) {
    AzBytArr s_str(str); 
    add_prefix(&s_str); 
  }
  virtual void add_prefix(const AzBytArr *s_str) {
    AzStrPool sp_tmp(this); 
    reset();
    reset(sp_tmp.size(), sp_tmp.data_len / sp_tmp.size() + s_str->length()); 
    int ix; 
    for (ix = 0; ix < sp_tmp.size(); ++ix) {
      AzBytArr s(s_str->c_str(), sp_tmp.c_str(ix)); 
      put(s.point(), s.length(), sp_tmp.getCount(i