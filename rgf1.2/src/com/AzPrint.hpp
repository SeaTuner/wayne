/* * * * *
 *  AzPrint.hpp 
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

#ifndef _AZ_PRINT_HPP
#define _AZ_PRINT_HPP

#include "AzUtil.hpp"
#if 0 
/*---  removed 08/01/2013  ---*/
#include "AzLoss.hpp"
#endif 

//! Printing tools
class AzPrint {
protected:
  ostream *o; /* may be NULL */
  const char *dlm, *name_dlm; 
  int count; 
  bool useDlm; 
  int level; 

public:
  inline AzPrint(const AzOut &out) : o(NULL), dlm(NULL), name_dlm(NULL), 
                                     count(0), useDlm(true), level(0) {
    if (!out.isNull()) {
      o = out.o; 
    }
    level = out.getLevel(); 
  }
  inline void reset(const AzOut &out) {
    o = NULL; 
    if (!out.isNull()) {
      o = out.o; 
    }
    level = out.getLevel(); 
    dlm = NULL; 
    count = 0; 
  }

  inline void resetDlm(const char *inp_dlm) {
    dlm = inp_dlm; 
  }

  /*--------*/
  inline void ppBegin(const char *caller, 
                      const char *desc=NULL, 
                      const char *inp_dlm=NULL) {
    AzBytArr s; 
    if (level > 0) {
      s.concat(caller); 
    }
    else {
      s.concat(desc); 
    }
    printBegin(s.c_str(), inp_dlm); 
  }
  inline void ppEnd() {
    printEnd(); 
  }
  /*--------*/

  inline void printBegin(const char *kw, 
                         const char *inp_dlm=NULL, 
                         const char *inp_name_dlm=NULL, 
                         int indent=0) {
    if (o == NULL) return; 
    dlm = inp_dlm; 
    name_dlm = inp_name_dlm; 
    if (name_dlm == NULL) name_dlm = dlm; 
    if (indent > 0) { /* indentation */
      AzBytArr s; s.fill(' ', indent);  
      *o<<s.c_str(); 
    }
    if (kw != NULL && strlen(kw) > 0) {   
      *o<<kw<<": "; 
    }
    count = 0; 
  }
  inline void printSw(const char *kw, bool val) {
    if (o == NULL) return; 
    if (val) {
      /*---  print only when it's on  ---*/
      itemBegin(); 
      *o<<kw<<":"; 
      if (val) *o<<"ON"; 
      else     *o<<"OFF"; 
    }
  }
  template<class T>
  inline void printV(const char *kw, T val) {
    if (o == NULL) return; 
    if (val != -1) {
      itemBegin();  
      *o<<kw<<val; 
    }
  }
  template<class T>
  inline void printV_posiOnly(const char *kw, T val) {
    if (o == NULL) return; 
    if (val > 0) {
      itemBegin();  
      *o<<kw<<val; 
    }
  }
  inline void printV(const char *kw, const char *val) {
    if (o == NULL) return; 
    itemBegin(); 
    *o<<kw<<val; 
  }
  inline void printV(const char *kw, const AzBytArr &s) {
    if (o == NULL) return; 
    itemBegin(); 
    *o<<kw<<s.c_str(); 
  }
  inline void printV_if_not_empty(const char *kw, const AzBytArr &s) {
    if (o == NULL) return; 
    if (s.length() <= 0) return; 
    itemBegin(); 
    *o<<kw<<s.c_str(); 
  }
#if 0  
  /*---  removed 08/01/2013  ---*/ 
  inline void printLoss(const char *kw, AzLossType loss_type) {
    if (o == NULL) return; 
    itemBegin(); 
    *o<<kw<<AzLoss::lossName(loss_type); 
  }
#endif   
  inline void printEnd(const char *str=NULL) {
    if (o == NULL) return; 
    if (str != NULL) *o<<str; 
    *o<<endl; 
   