
/* * * * *
 *  AzUtil.cpp 
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

#include <time.h>
#include <ctype.h>
#include "AzUtil.hpp"
#include "AzPrint.hpp"

static int th_autoSqueeze = 1024; 

/***************************************************************/
/***************************************************************/
/*-------------------------------------------------------------*/
AzFile::AzFile(const char *fn)
{
  fp = NULL; 
  str_fn = new AzBytArr(fn); 
}

/*-------------------------------------------------------------*/
AzFile::~AzFile()
{
  if (fp != NULL) {
    fclose(fp); 
    fp = NULL; 
  }
  delete str_fn; str_fn = NULL; 
}

/*-------------------------------------------------------------*/
void AzFile::reset(const char *fn)
{
  if (fp != NULL) {
    fclose(fp); 
    fp = NULL; 
  }
  delete str_fn; str_fn = NULL; 
  str_fn = new AzBytArr(fn); 
}

/*-------------------------------------------------------------*/
void AzFile::open(const char *inp_flags) 
{
  const char *eyec = "AzFile::open"; 

  const char *flags = inp_flags; 

  if (str_fn == NULL || str_fn->length() <= 0) {
    throw new AzException(eyec, "No filename"); 
  }

  if (strcmp(flags, "X") == 0) { /* for compatibility .. */
    flags = "rb";  
  }
  if ((fp = fopen(pointFileName(), flags)) == NULL) {
    throw new AzException(AzFileIOError, eyec, pointFileName(), "fopen"); 
  }
}

/*-------------------------------------------------------------*/
bool AzFile::isExisting(const char *fn)
{
  FILE *temp_fp = NULL; 
  if ((temp_fp = fopen(fn, "rb")) == NULL) {
    return false; 
  }
  else {
    fclose(temp_fp); 
    return true; 
  }
}

/*-------------------------------------------------------------*/
const char *AzFile::pointFileName() const
{
  if (str_fn == NULL) return ""; 
  return str_fn->c_str(); 
}

/*-------------------------------------------------------------*/
void AzFile::close(bool doCheckError) 
{
  const char *eyec = "AzFile::close"; 
  if (fp != NULL) {
    if (fclose(fp) != 0 && doCheckError) {
      throw new AzException(AzFileIOError, eyec, pointFileName(), "fclose"); 
    }
    fp = NULL; 
  }
}
 
/*-------------------------------------------------------------*/
AZint8 AzFile::size() 
{
  const char *eyec = "AzFile::size";

  if (fp == NULL) {
    throw new AzException("AzFile::size()", "file must be opened first"); 
  }

  /*-----  keep current offset  -----*/
  AZint8 offs = ftell(fp); 

  /*-----  seek to eof  -----*/
  if (fseek(fp, 0, SEEK_END) != 0) {
    throw new AzException(AzFileIOError, eyec, pointFileName(), "seek to end"); 
  }
  
  AZint8 size = ftell(fp); 
  if (size == -1L) {
    throw new AzException(AzFileIOError, eyec, pointFileName(), "ftell"); 
  }

  /*-----  seek it back  -----*/
  if (fseek(fp, offs, SEEK_SET) != 0) {
    throw new AzException(AzFileIOError, eyec, pointFileName(), "seek"); 
  }  

  return size; 
}

/*-------------------------------------------------------------*/
int AzFile::gets(AzByte *buff, int buffsize) 
{
  const char *eyec = "AzFile::gets";
  if (fgets((char *)buff, buffsize, fp) == NULL) {
    if (feof(fp)) {
      return 0; 
    }
    throw new AzException(AzFileIOError, eyec, pointFileName(), "fgets"); 
  }

  int len = Az64::cstrlen((char *)buff, "AzFile::gets"); 
  return len; 
}

/*-------------------------------------------------------------*/
void AzFile::seekReadBytes(AZint8 offs, AZint8 len, void *buff) 
{
  const char *eyec = "AzFile::seekReadBytes"; 
  /*-----  seek  -----*/
  if (offs >= 0) {
    if (fseek(fp, offs, SEEK_SET) != 0) { 
      throw new AzException(AzFileIOError, eyec, pointFileName(), "seek"); 
    }
  }

  /*-----  read  -----*/
  if (len > 0) {
    if (fread(buff, len, 1, fp) != 1) {
      throw new AzException(AzFileIOError, eyec, pointFileName(), "fread"); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzFile::seekReadBytes(AZint8 offs, AZint8 sz, AZint8 count, void *buff) 
{
  const char *eyec = "AzFile::seekReadBytes(offs,sz,count,buff)"; 
  /*-----  seek  -----*/
  if (offs >= 0) {
    if (fseek(fp, offs, SEEK_SET) != 0) {
      throw new AzException(AzFileIOError, eyec, pointFileName(), "seek"); 
    }
  }

  /*-----  read  -----*/   
  check_overflow(sz, "AzFile::seekReadBytes,sz"); 
  check_overflow(count, "AzFile::seekReadBytes,count");   
  if (sz > 0 && count > 0) {
    if (fread(buff, sz, count, fp) != count) {
      throw new AzException(AzFileIOError, eyec, pointFileName(), "fread"); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzFile::seek(AZint8 offs) 
{
  seekReadBytes(offs, 0, NULL); 
}

/*-------------------------------------------------------------*/
#define AZ_BIN_MARKER_INT 1
#define AZ_BIN_MARKER_DBL 1.0
void AzFile::writeBinMarker() 
{
  int int_val = AZ_BIN_MARKER_INT; 
  writeInt(int_val); 
  double dbl_val = AZ_BIN_MARKER_DBL; 
  writeDouble(dbl_val); 
}
void AzFile::checkBinMarker() 
{
  int int_val = 0;  
  double dbl_val = 0; 
  try {
    int_val = readInt(); 
    dbl_val = readDouble(); 
  }
  catch (AzException *e) {
    delete e; 
  }
  if (int_val != AZ_BIN_MARKER_INT || 
      dbl_val != AZ_BIN_MARKER_DBL) {
    throw new AzException(AzInputError, "AzFile::checkBinMarker", 
              "Binary file marker check failed: a broken file or endian mismatch:", 
              pointFileName()); 
  }
}

/* static */
/*------------------------------------------------------------------*/
void AzFile::scan(const char *fn, 
                  int buff_size, 
                  AzIntArr *ia_line_len, 
                  int max_line_num) /* -1: means don't care; added 06/15/2013 */
{
  AzFile file(fn); 
  file.open("rb"); 
  AzBytArr ba_buff; 
  AzByte *buff = ba_buff.reset(buff_size, 0); 
  
  /*---  find the number of lines and the maximum length of the lines  ---*/
  ia_line_len->reset(); 
  ia_line_len->prepare(1000000); 
  bool do_exit = false; 
  int remain = 0; 
  for ( ; ; ) {
    if (do_exit) break; 
    int len = file._readBytes(buff, buff_size); 
    if (len <= 0) break; 
    const char *wp = (char *)buff, *buff_end = (char *)(buff+len); 
    while(wp < buff_end) {
      const char *next_wp = strchr(wp, '\n'); 
      if (next_wp == NULL) {
        remain += (int)(buff_end - wp); 
        break; 
      }
      ++next_wp;  /* +1 for '\n' */
      
      int line_len = (int)(next_wp - wp) + remain; 
      ia_line_len->put(line_len);   
      remain = 0; 
      wp = next_wp; 
      if (max_line_num > 0 && ia_line_len->size() >= max_line_num) {
        do_exit = true; 
        break; 
      }  
    }
  }
  if (remain > 0) {
    ia_line_len->put(remain); 
  }
}

/***************************************************************/
/*              AzIIFarr (Int1, Int2, Floating-point)           */
/***************************************************************/
int az_compare_IIFarr_IntInt_A(const void *v1, const void *v2); 
int az_compare_IIFarr_IntInt_D(const void *v1, const void *v2); 
int az_compare_IIFarr_Int2Int1_A(const void *v1, const void *v2); 
int az_compare_IIFarr_Int2Int1_D(const void *v1, const void *v2); 
int az_compare_IIFarr_Float_A(const void *v1, const void *v2); 
int az_compare_IIFarr_Float_D(const void *v1, const void *v2); 
int az_compare_IIFarr_FloatInt1Int2_A(const void *v1, const void *v2); 
int az_compare_IIFarr_FloatInt1Int2_D(const void *v1, const void *v2); 
int az_compare_IIFarr_Int1_A(const void *v1, const void *v2);

/*------------------------------------------------------------------*/
int AzIIFarr::bsearch_Float(double key, bool isAscending) const
{
  if (ent_num <= 0 || ent == NULL) return -1; 

  AzIIFarrEnt *ptr = NULL; 
  if (isAscending) {
    ptr = (AzIIFarrEnt *)bsearch(&key, ent, ent_num, sizeof(ent[0]), 
                               az_compare_IIFarr_Float_A); 
  }
  else {
    ptr = (AzIIFarrEnt *)bsearch(&key, ent, ent_num, sizeof(ent[0]), 
                               az_compare_IIFarr_Float_D); 
  }
  if (ptr == NULL) return -1; 
  int ent_no = Az64::ptr_diff(ptr - ent, "AzIIFarr::bsearch_Float"); 
  return ent_no; 
}

/*------------------------------------------------------------------*/
double AzIIFarr::sum_Fval() const
{
  double sum = 0; 
  int ix; 
  for (ix = 0; ix < ent_num; ++ix) {
    sum += ent[ix].val; 
  }
  return sum; 
}

/*------------------------------------------------------------------*/
bool AzIIFarr::isSame(const AzIIFarr *iifq) const
{
  if (iifq->ent_num != ent_num) return false; 
  int ix; 
  for (ix = 0; ix < ent_num; ++ix) {
    if (ent[ix].int1 != iifq->ent[ix].int1 || 
        ent[ix].int2 != iifq->ent[ix].int2 || 
        ent[ix].val != iifq->ent[ix].val) {
      return false; 
    }
  }
  return true; 
}

/*------------------------------------------------------------------*/
int AzIIFarr::find(int int1, int int2, int first_ix) const
{
  int ix; 
  for (ix = first_ix; ix < ent_num; ++ix) {
    if (ent[ix].int1 == int1 && ent[ix].int2 == int2) {
      return ix; 
    }
  }
  return AzNone; 
}

/*-------------------------------------------------------------*/
double AzIIFarr::findMin(int *out_idx) const
{
  if (ent_num == 0) {
    if (out_idx != NULL) {
      *out_idx = AzNone; 
    }
    return -1; 
  }

  double min_val = ent[0].val; 
  int min_idx = 0; 
  int ix; 
  for (ix = 1; ix < ent_num; ++ix) {
    if (ent[ix].val < min_val) {
      min_idx = ix; 
      min_val = ent[ix].val; 
    }
  }
  if (out_idx != NULL) {
    *out_idx = min_idx; 
  }
  return min_val; 
}

/*-------------------------------------------------------------*/
double AzIIFarr::findMax(int *out_idx) const
{
  if (ent_num == 0) {
    if (out_idx != NULL) {
      *out_idx = AzNone; 
    }
    return -1; 
  }

  double max_val = ent[0].val; 
  int max_idx = 0; 
  int ix; 
  for (ix = 1; ix < ent_num; ++ix) {
    if (ent[ix].val > max_val) {
      max_idx = ix; 
      max_val = ent[ix].val; 
    }
  }
  if (out_idx != NULL) {
    *out_idx = max_idx; 
  }
  return max_val; 
}

/*-------------------------------------------------------------*/
void AzIIFarr::concat(const AzIIFarr *iifq2) 
{
  const char *eyec = "AzIIFarr::concat"; 

  if (iifq2 == NULL || iifq2->ent_num <= 0) {
    return; 
  }

  int new_num = ent_num + iifq2->ent_num; 
  int ent_num_max = a.size(); 
  if (new_num > ent_num_max) {
    ent_num_max = new_num; 
    a.realloc(&ent, ent_num_max, eyec, "ent realloc"); 
  }

  memcpy(ent + ent_num, iifq2->ent, sizeof(ent[0]) * iifq2->ent_num);  
  ent_num = new_num; 
}

/*-------------------------------------------------------------*/
int AzIIFarr::getNum(double req_val) const
{
  int count = 0; 
  int ix; 
  for (ix = 0; ix < ent_num; ++ix) {
    if (ent[ix].val == req_val) ++count; 
  }
  return count; 
}

/*-------------------------------------------------------------*/
void AzIIFarr::concat(const AzIIFarr *iifq2, double req_val) 
{
  const char *eyec = "AzIIFarr::concat(req_val)"; 

  if (iifq2 == NULL || iifq2->ent_num <= 0) {
    return; 
  }

  int new_num = ent_num + iifq2->ent_num; 
  int ent_num_max = a.size(); 
  if (new_num > ent_num_max) {
    ent_num_max = new_num; 
    a.realloc(&ent, ent_num_max, eyec, "ent realloc"); 
  }
  int ix; 
  for (ix = 0; ix < iifq2->ent_num; ++ix) {
    if (iifq2->ent[ix].val == req_val) {
      ent[ent_num] = iifq2->ent[ix]; 
      ++ent_num; 
    }
  }
}

/*-------------------------------------------------------------*/
void AzIIFarr::update(int idx, 
                     int int1, int int2, 
                     double val)
{
  if (idx >= 0 && idx < ent_num) {
    ent[idx].int1 = int1; 
    ent[idx].int2 = int2; 
    ent[idx].val = val; 
  }
}

/*-------------------------------------------------------------*/
void AzIIFarr::_read(AzFile *file) 
{
  const char *eyec = "AzIIFarr::_read (file)"; 
  ent_type = (AzIIFarrType)file->readInt(); 
  ent_num = file->readInt(); 
  a.alloc(&ent, ent_num, eyec, "ent"); 
  int ex; 
  for (ex = 0; ex < ent_num; ++ex) {
    AzIIFarrEnt *ep = &ent[ex]; 

    if (ent_type == AzIIFarr_II) {
      ep->int1 = file->readInt(); 
      ep->int2 = file->readInt(); 
      ep->val = 0; 
    }
    else if (ent_type == AzIIFarr_IF) {
      ep->int1 = file->readInt(); 
      ep->int2 = AzNone; 
      ep->val = file->readDouble(); 
    }
    else if (ent_type == AzIIFarr_IIF) {
      ep->int1 = file->readInt(); 
      ep->int2 = file->readInt(); 
      ep->val = file->readDouble(); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzIIFarr::write(AzFile *file) 
{
  file->writeInt((int)ent_type); 
  file->writeInt(ent_num); 
  int ex; 
  for (ex = 0; ex < ent_num; ++ex) {
    const AzIIFarrEnt *ep = &ent[ex]; 
    if (ent_type == AzIIFarr_II) {
      file->writeInt(ep->int1); 
      file->writeInt(ep->int2); 
    }
    else if (ent_type == AzIIFarr_IF) {
      file->writeInt(ep->int1); 
      file->writeDouble(ep->val); 
    }
    else if (ent_type == AzIIFarr_IIF) {
      file->writeInt(ep->int1); 
      file->writeInt(ep->int2); 
      file->writeDouble(ep->val); 
    }
  }
}

/*-------------------------------------------------------------*/
void AzIIFarr::reset(int num, 
                    int int1, int int2, double val) 
{
  reset(); 
  prepare(num); 
  int ex; 
  for (ex = 0; ex < num; ++ex) {
    put(int1, int2, val); 
  }
}

/*-------------------------------------------------------------*/
void AzIIFarr::prepare(int num) 
{
  const char *eyec = "AzIIFarr::prepare";
  int ent_num_max = a.size(); 
  if (num >= ent_num && 
      num != ent_num_max) {
    ent_num_max = num; 
    a.realloc(&ent, ent_num_max, eyec, "ent"); 
  }
}

/*-------------------------------------------------------------*/
void AzIIFarr::cut(int new_num) 
{
  if (new_num < 0 || 
      ent_num <= new_num) {
    return; 
  }
  ent_num = new_num; 
  return; 
}

/*-------------------------------------------------------------*/
void AzIIFarr::put(int int1, int int2, double val) 
{
  const char *eyec = "AzIIFarr::put";

  int ent_num_max = a.size(); 
  if (ent_num >= ent_num_max) {
    if (ent_num_max <= 0) 
      ent_num_max = 32; 
    else if (ent_num_max < 1024*1024) /* changed from 1024: 1/12/2014 */
      ent_num_max *= 2; 
    else
      ent_num_max += 1024*1024; /* changed from 1024: 1/12/2014 */

    a.realloc(&ent, ent_num_max, eyec, "2"); 
  }
  ent[ent_num].int1 = int1; 
  ent[ent_num].int2 = int2; 
  ent[ent_num].val = val; 
  ++ent_num; 
}

/*-------------------------------------------------------------*/
void AzIIFarr::insert(int index, 
                    int int1, int int2, double fval)
{
  if (index < 0 || index > ent_num) {
    throw new AzException("AzIIFarr::insert", "invalid index"); 
  }

  /*---  append to the end to allocate memory if necessary  ---*/
  put(int1, int2, fval); 
  if (index == ent_num - 1) {
    return; /* already in position */
  }

  AzIIFarrEnt my_ent = ent[ent_num-1]; 

  /*---  make room for [index]  ---*/
  int ex; 
  for (ex = ent_num - 2; ex >= index; --ex) {
    ent[ex+1] = ent[ex]; 
  }

  /*---  insert  ---*/
  ent[index] = my_ent; 
}

/*-------------------------------------------------------------*/
void AzIIFarr::int1(AzIntArr *ia_int1) const
{
  ia_int1->prepare(ent_num); 
  int ex; 
  for (ex = 0; ex < ent_num; ++ex) {
    ia_int1->put(ent[ex].int1); 
  }
}

/*-------------------------------------------------------------*/
void AzIIFarr::int2(AzIntArr *ia_int2) const
{
  ia_int2->prepare(ent_num); 
  int ex; 
  for (ex = 0; ex < ent_num; ++ex) {
    ia_int2->put(ent[ex].int2); 
  }
}

/*-------------------------------------------------------------*/
double AzIIFarr::get(int idx, int *int1, int *int2) const 
{
  if (idx < 0 || idx >= ent_num) {
    throw new AzException("AzIIFarr::get", "out of range"); 
  }
  if (int1 != NULL) {
    *int1 = ent[idx].int1; 
  }
  if (int2 != NULL) {
    *int2 = ent[idx].int2; 
  }

  return ent[idx].val; 
}

/*-------------------------------------------------------------*/
void AzIIFarr::squeeze_Sum()
{
  if (ent_num <= 1) {
    return; 
  }

  qsort(ent, ent_num, sizeof(ent[0]), az_compare_IIFarr_IntInt_A); 

  int ex1 = 0; 
  int ex; 
  for (ex = 0; ex < ent_num; ) {
    if (ex != ex1) {
      ent[ex1] = ent[ex]; 
    }
    for (++ex; ex < ent_num; ++ex) {
      if (ent[ex].int1 != ent[ex1].int1 || 
          ent[ex].int2 != ent[ex1].int2) {
        break; 
      }
      ent[ex1].val += ent[ex].val; 
    }
    ++ex1; 
  }
  ent_num = ex1; 
}
 
/*-------------------------------------------------------------*/
void AzIIFarr::squeeze_Max()
{
  if (ent_num <= 1) {
    return; 
  }

  qsort(ent, ent_num, sizeof(ent[0]), az_compare_IIFarr_IntInt_A); 

  int ex1 = 0; 
  int ex; 
  for (ex = 0; ex < ent_num; ) {
    if (ex != ex1) {
      ent[ex1] = ent[ex]; 
    }  
    for (++ex; ex < ent_num; ++ex) {
      if (ent[ex].int1 != ent[ex1].int1 || 
          ent[ex].int2 != ent[ex1].int2)  
        break; 

      ent[ex1].val = MAX(ent[ex1].val, ent[ex].val); 
    }
    ++ex1; 
  }
  ent_num = ex1; 
}

/*-------------------------------------------------------------*/
void AzIIFarr::squeeze_Int1_Max()
{
  if (ent_num <= 1) {
    return; 
  }

  qsort(ent, ent_num, sizeof(ent[0]), az_compare_IIFarr_Int1_A); 

  int ex1 = 0; 
  int ex; 
  for (ex = 0; ex < ent_num; ) {
    if (ex != ex1) {
      ent[ex1] = ent[ex]; 
    }  
    for (++ex; ex < ent_num; ++ex) {
      if (ent[ex].int1 != ent[ex1].int1) 
        break; 

      if (ent[ex].val > ent[ex1].val) {
        ent[ex1] = ent[ex]; 
      }
    }
    ++ex1; 
  }
  ent_num = ex1; 
}

/*-------------------------------------------------------------*/
void AzIIFarr::squeeze_Int1_Sum()
{
  if (ent_num <= 1) {
    return; 
  }

  qsort(ent, ent_num, sizeof(ent[0]), az_compare_IIFarr_Int1_A); 

  int ex1 = 0; 
  int ex; 
  for (ex = 0; ex < ent_num; ) {
    if (ex != ex1) {
      ent[ex1] = ent[ex]; 
    }  
    for (++ex; ex < ent_num; ++ex) {
      if (ent[ex].int1 != ent[ex1].int1) 
        break; 

      if (ent[ex].val > ent[ex1].val) {
        ent[ex1].int2 = ent[ex].int2; 
        ent[ex1].val += ent[ex].val; 
      }
    }
    ++ex1; 
  }
  ent_num = ex1; 
}

/*-------------------------------------------------------------*/
void AzIIFarr::sort_IntInt(bool isAscending) 
{
  if (ent_num <= 1) {
    return; 
  }

  if (isAscending) {
    qsort(ent, ent_num, sizeof(ent[0]), az_compare_IIFarr_IntInt_A); 
  }
  else {
    qsort(ent, ent_num, sizeof(ent[0]), az_compare_IIFarr_IntInt_D); 
  }
}

/*-------------------------------------------------------------*/
void AzIIFarr::sort_Int2Int1(bool isAscending) 
{
  if (ent_num <= 1) {
    return; 
  }

  if (isAscending) {
    qsort(ent, ent_num, sizeof(ent[0]), az_compare_IIFarr_Int2Int1_A); 
  }
  else {
    qsort(ent, ent_num, sizeof(ent[0]), az_compare_IIFarr_Int2Int1_D); 
  }
}

/*-------------------------------------------------------------*/
void AzIIFarr::sort_Float(bool isAscending) 
{
  if (ent_num <= 1) {
    return; 
  }

  if (isAscending) {
    qsort(ent, ent_num, sizeof(ent[0]), az_compare_IIFarr_Float_A); 
  }
  else {
    qsort(ent, ent_num, sizeof(ent[0]), az_compare_IIFarr_Float_D); 
  }
}

/*-------------------------------------------------------------*/
void AzIIFarr::sort_FloatInt1Int2(bool isAscending) 
{
  if (ent_num <= 1) {
    return; 
  }

  if (isAscending) {
    qsort(ent, ent_num, sizeof(ent[0]), az_compare_IIFarr_FloatInt1Int2_A); 
  }
  else {
    qsort(ent, ent_num, sizeof(ent[0]), az_compare_IIFarr_FloatInt1Int2_D); 
  }
}

/*-------------------------------------------------------------*/
int az_compare_IIFarr_IntInt_A(const void *v1, const void *v2) 
{
  AzIIFarrEnt *p1 = (AzIIFarrEnt *)v1; 
  AzIIFarrEnt *p2 = (AzIIFarrEnt *)v2; 
  if (p1->int1 < p2->int1) return -1; 
  if (p1->int1 > p2->int1) return 1; 
  if (p1->int2 < p2->int2) return -1; 
  if (p1->int2 > p2->int2) return 1; 
  return 0; 
}

/*-------------------------------------------------------------*/
int az_compare_IIFarr_IntInt_D(const void *v1, const void *v2) 
{
  AzIIFarrEnt *p1 = (AzIIFarrEnt *)v1; 
  AzIIFarrEnt *p2 = (AzIIFarrEnt *)v2; 
  if (p1->int1 > p2->int1) return -1; 
  if (p1->int1 < p2->int1) return 1; 
  if (p1->int2 > p2->int2) return -1; 
  if (p1->int2 < p2->int2) return 1; 
  return 0; 
}

/*-------------------------------------------------------------*/
int az_compare_IIFarr_Int2Int1_A(const void *v1, const void *v2) 
{
  AzIIFarrEnt *p1 = (AzIIFarrEnt *)v1; 
  AzIIFarrEnt *p2 = (AzIIFarrEnt *)v2; 
  if (p1->int2 < p2->int2) return -1; 
  if (p1->int2 > p2->int2) return 1; 
  if (p1->int1 < p2->int1) return -1; 
  if (p1->int1 > p2->int1) return 1; 
  return 0; 
}

/*-------------------------------------------------------------*/
int az_compare_IIFarr_Int2Int1_D(const void *v1, const void *v2) 
{
  AzIIFarrEnt *p1 = (AzIIFarrEnt *)v1; 
  AzIIFarrEnt *p2 = (AzIIFarrEnt *)v2; 
  if (p1->int2 > p2->int2) return -1; 
  if (p1->int2 < p2->int2) return 1; 
  if (p1->int1 > p2->int1) return -1; 
  if (p1->int1 < p2->int1) return 1; 
  return 0; 
}


/*-------------------------------------------------------------*/
int az_compare_IIFarr_Float_A(const void *v1, const void *v2) 
{
  AzIIFarrEnt *p1 = (AzIIFarrEnt *)v1; 
  AzIIFarrEnt *p2 = (AzIIFarrEnt *)v2; 
  if (p1->val < p2->val) return -1; 
  if (p1->val > p2->val) return 1; 
  return 0; 
}

/*-------------------------------------------------------------*/
int az_compare_IIFarr_Float_D(const void *v1, const void *v2) 
{
  AzIIFarrEnt *p1 = (AzIIFarrEnt *)v1; 
  AzIIFarrEnt *p2 = (AzIIFarrEnt *)v2; 
  if (p1->val > p2->val) return -1; 
  if (p1->val < p2->val) return 1; 
  return 0; 
}

/*-------------------------------------------------------------*/
int az_compare_IIFarr_FloatInt1Int2_A(const void *v1, const void *v2) 
{
  AzIIFarrEnt *p1 = (AzIIFarrEnt *)v1; 
  AzIIFarrEnt *p2 = (AzIIFarrEnt *)v2; 
  if (p1->val < p2->val) return -1; 
  if (p1->val > p2->val) return 1; 
  if (p1->int1 < p2->int1) return -1; 
  if (p1->int1 > p2->int1) return 1; 
  if (p1->int2 < p2->int2) return -1; 
  if (p1->int2 > p2->int2) return 1; 
  return 0; 
}

/*-------------------------------------------------------------*/
int az_compare_IIFarr_FloatInt1Int2_D(const void *v1, const void *v2) 
{
  AzIIFarrEnt *p1 = (AzIIFarrEnt *)v1; 
  AzIIFarrEnt *p2 = (AzIIFarrEnt *)v2; 
  if (p1->val > p2->val) return -1; 
  if (p1->val < p2->val) return 1; 
  if (p1->int1 > p2->int1) return -1; 
  if (p1->int1 < p2->int1) return 1; 
  if (p1->int2 > p2->int2) return -1; 
  if (p1->int2 < p2->int2) return 1;
  return 0; 
}

/*-------------------------------------------------------------*/
int az_compare_IIFarr_Int1_A(const void *v1, const void *v2) 
{
  AzIIFarrEnt *p1 = (AzIIFarrEnt *)v1; 
  AzIIFarrEnt *p2 = (AzIIFarrEnt *)v2; 
  if (p1->int1 < p2->int1) return -1; 
  if (p1->int1 > p2->int1) return 1; 
  return 0; 
}

/********************************************************************/
/*------------------------------------------------------------------*/
/*                             AzIIarr                              */
/*------------------------------------------------------------------*/
void AzIIarr::read(AzFile *file) 
{ 
  iifq.read(file); 
  if (iifq.get_ent_type() != AzIIFarr_II) {
    throw new AzException("AzIIarr::AzIIarr(file)", "type conflict");
  }
}

/*------------------------------------------------------------------*/
void AzIIarr::reset(const int pairs[][2], int pair_num)
{
  reset(); 
  prepare(pair_num); 
  int ix; 
  for (ix = 0; ix < pair_num; ++ix) {
    put(pairs[ix][0], pairs[ix][1]); 
  }
}

/*------------------------------------------------------------------*/
/*                             AzIFarr                              */
/*------------------------------------------------------------------*/
void AzIFarr::read(AzFile *file) 
{ 
  iifq.read(file); 
  if (iifq.get_ent_type() != AzIIFarr_IF) {
    throw new AzException("AzIFarr::_read", "type conflict");
  }
}

/********************************************************************/
/*                          AzIntArr                                */
/********************************************************************/
int az_compareInt_A(const void *v1, const void *v2); 
int az_compareInt_A(const void *v1, const void *v2); 

/*------------------------------------------------------------------*/
void AzIntArr::transfer_from(AzIntArr *inp)
{
  a.transfer_from(&inp->a, &ints, &inp->ints, "AzIntArr::transfer_from"); 
  num = inp->num; 
  inp->num = 0; 
}

/*------------------------------------------------------------------*/
void AzIntArr::initialize(int inp_num, int initial_value) 
{
  const char *eyec = "AzIntArr::initialize"; 

  if (inp_num <= 0) {
    return; 
  }
  a.alloc(&ints, inp_num, eyec, "ints"); 
  num = inp_num; 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    ints[ix] = initial_value; 
  }
}

/*------------------------------------------------------------------*/
void AzIntArr::put(int int_val) 
{
  int num_max = a.size(); 
  if (num >= num_max) {
    _realloc(); 
  }
  ints[num] = int_val; 
  ++num; 
}

/*------------------------------------------------------------------*/
bool AzIntArr::toOnOff(const int *inp, int inp_num)
{
  bool isThereNegative = false; 
  int max_idx = -1; 
  int ix; 
  for (ix = 0; ix < inp_num; ++ix) {
    if (ix == 0 || inp[ix] > max_idx) max_idx = inp[ix]; 
    if (inp[ix] < 0) isThereNegative = true; 
  }
  if (max_idx < 0) {
    reset(); 
    return isThereNegative; 
  }

  reset(max_idx+1, false); 
  for (ix = 0; ix < inp_num; ++ix) {
    int idx = inp[ix]; 
    if (idx >= 0) {
      ints[idx] = true; 
    }
  }
  return isThereNegative; 
}

/*------------------------------------------------------------------*/
bool AzIntArr::toCount(AzIntArr *out_iq) const
{
  bool isThereNegative = false; 
  int max_idx = max(); 
  out_iq->reset(max_idx+1, 0); 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    int idx = ints[ix]; 
    if (idx < 0) {
      isThereNegative = true; 
      continue; 
    } 
    ++out_iq->ints[idx]; 
  }
  return isThereNegative; 
}

/*------------------------------------------------------------------*/
int AzIntArr::compare(const AzIntArr *iq1, const AzIntArr *iq2, 
                         int first_k)
{
  int cmp_num = MIN(iq1->num, iq2->num); 
  if (first_k > 0) {
    cmp_num = MIN(cmp_num, first_k); 
  }

  int ix; 
  for (ix = 0; ix < cmp_num; ++ix) {
    int cmp = iq1->ints[ix] - iq2->ints[ix]; 
    if (cmp != 0) {
      if (cmp < 0) return -1; 
      else if (cmp > 0) return 1; 
      return 0; 
    }
  }
  if (first_k > 0 && 
      cmp_num == first_k) { 
    return 0; 
  }
  int cmp = iq1->num - iq2->num; 
  if (cmp < 0) return -1; 
  else if (cmp > 0) return 1; 
  return 0; 
}

/*------------------------------------------------------------------*/
void AzIntArr::changeOrder(const AzIntArr *ia_old2new) 
{
  int old2new_num = ia_old2new->num; 
  const int *old2new = ia_old2new->ints; 
  if (old2new_num != num) {
    throw new AzException("AzIntArr::changeOrder", "number mismatch"); 
  }

  AzIntArr ia_org(this); 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    ints[ix] = -1; 
  }
  for (ix = 0; ix < num; ++ix) {
    int new_ix = old2new[ix]; 
    ints[new_ix] = ia_org.ints[ix]; 
  }
}

/*------------------------------------------------------------------*/
double AzIntArr::average() const {
  if (num <= 0) return 0; 
  double sum = 0; 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    sum += ints[ix]; 
  }
  return sum / (double)num; 
}

/*------------------------------------------------------------------*/
void AzIntArr::prepare(int prep_num)
{
  const char *eyec = "AzIntArr::prepare"; 
  int num_max = a.size(); 
  if (num_max < prep_num) {
    num_max = prep_num; 
    a.realloc(&ints, num_max, eyec, "ints"); 
  }
}

/*------------------------------------------------------------------*/
void AzIntArr::initialize(const AzIntArr *inp_ia) 
{
  if (inp_ia == NULL) return; 

  int inp_num; 
  const int *inp_ints = inp_ia->point(&inp_num); 
  initialize(inp_ints, inp_num); 
}

/*------------------------------------------------------------------*/
void AzIntArr::initialize(const int *inp_ints, int inp_ints_num) 
{
  if (inp_ints_num <= 0) {
    return; 
  }

  num = inp_ints_num; 
  if (num > 0) {
    a.alloc(&ints, num, "AzIntArr::initialize(array,num)", "ints"); 
  }
  memcpy(ints, inp_ints, sizeof(inp_ints[0]) * inp_ints_num); 
}

/*------------------------------------------------------------------*/
void AzIntArr::initialize(AzFile *file)  
{
  num = file->readInt(); 
  initialize(num, 0); 

  file->seekReadBytes(-1, sizeof(ints[0])*num, ints); 
  _swap(); 
}

/*------------------------------------------------------------------*/
void AzIntArr::_swap()
{
  if (!isSwapNeeded) return; 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    AzFile::swap_int4(&ints[ix]); 
  }
}

/*------------------------------------------------------------------*/
void AzIntArr::write(AzFile *file) 
{