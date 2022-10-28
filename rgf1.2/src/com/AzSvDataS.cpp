/* * * * *
 *  AzSvDataS.cpp 
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

#include "AzSvDataS.hpp"
#include "AzTools.hpp"

/*------------------------------------------------------------------*/
void AzSvDataS::reset()
{
  m_feat.reform(0,0);  
  sp_f_dic.reset(); 
  v_y.reform(0); 
}

/*------------------------------------------------------------------*/
void AzSvDataS::destroy()
{
  reset(); 
}

/*------------------------------------------------------------------*/
void AzSvDataS::checkIfReady(const char *msg) const
{
  if (m_feat.colNum() <= 0 || 
      m_feat.colNum() != v_y.rowNum() || 
      sp_f_dic.size() > 0 && sp_f_dic.size() != m_feat.rowNum()) {
    throw new AzException("AzSvDataS::checkIfReady", "failed", msg); 
  }
}

/*------------------------------------------------------------------*/
void AzSvDataS::concatDesc(int ex, AzBytArr *str_desc) const
{
  if (ex < 0 || ex >= m_feat.rowNum()) {
    str_desc->concatInt(ex); 
    str_desc->concat("?"); 
    return; 
  }
  
  if (sp_f_dic.size() > 0) {
    str_desc->concat(sp_f_dic.c_str(ex)); 
  }
  else {
    str_desc->concat("T"); 
    str_desc->concatInt(ex); 
  }
}

/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/
void AzSvDataS::_read(const char *feat_fn, 
                     const char *y_fn, 
                     const char *fdic_fn,
                     int max_data_num)
{
  reset(); 

  read_feat(feat_fn, fdic_fn, &m_feat, &sp_f_dic, max_data_num); 
  read_target(y_fn, &v_y, max_data_num); 

  /*---  check the dimensionalty  ---*/
  int f_data_num = m_feat.colNum(); 
  int y_data_num = v_y.rowNum(); 
  if (f_data_num != y_data_num) {
    AzBytArr s("Data conflict: "); 
    s.c(feat_fn); s.c(" has "); s.cn(f_data_num); s.c(" data points, whereas "); 
    s.c(y_fn);    s.c(" has "); s.cn(y_data_num); s.c(" data points."); 
    throw new AzException(AzInputNotValid, "AzSvDataS::read", s.c_str()); 
  }
}

/*------------------------------------------------------------------*/
void AzSvDataS::read_features_only(const char *feat_fn, 
                                   const char *fdic_fn,
                                   int max_data_num)
{
  reset(); 
  read_feat(feat_fn, fdic_fn, &m_feat, &sp_f_dic, max_data_num); 
  int data_num = m_feat.colNum(); 
  v_y.reform(data_num); /* fill with dummy value zero */
}

/*------------------------------------------------------------------*/
void AzSvDataS::read_targets_only(const char *y_fn, 
                                  int max_data_num)
{
  reset(); 
  read_target(y_fn, &v_y, max_data_num); 
  int data_num = v_y.rowNum(); 
  m_feat.reform(1, data_num); /* dummy features */
}

/*------------------------------------------------------------------*/
/* static */
void AzSvDataS::read_feat(const char *feat_fn, 
                          const char *fdic_fn, 
                          /*---  output  ---*/
                          AzSmat *m_feat, 
                          AzStrPool *sp_f_dic, 
                          int max_data_num)
{
  /*---  read feature names  ---*/
  int f_num = -1; 
  if (fdic_fn != NULL && strlen(fdic_fn) > 0) {
    AzTools::readList(fdic_fn, sp_f_dic); 
    if (sp_f_dic->size() > 0) {
      f_num = sp_f_dic->size(); 
      int fx; 
      for (fx = 0; fx < sp_f_dic->size(); ++fx) {
        if (sp_f_dic->getLen(fx) <= 0) {
          AzBytArr s("No blank line is allowed in feature name files: "); s.c(fdic_fn); 
          throw new AzException(AzInputNotValid, "AzSvDataS::read_feat", s.c_str()); 
        }
      }
    }
  }

  /*---  read feature file  ---*/
  readData(feat_fn, -1, m_feat, max_data_num); 

  if (f_num > 0 && f_num != m_feat->rowNum()) {
    AzBytArr s("Conflict in #feature: "); s.c(feat_fn); s.c(" vs. "); s.c(fdic_fn); 
    throw new AzException(AzInputNotValid, "AzSvDataS::read_feat", s.c_str()); 
  }
}

/*------------------------------------------------------------------*/
/* static */
void AzSvDataS::read_target(const char *y_fn, AzDvect *v_y, int max_data_num)
{
  AzSmat m_y; 
  readData(y_fn, 1, &m_y, max_data_num); 
  int y_data_num = m_y.colNum(); 
  v_y->reform(y_data_num); 
