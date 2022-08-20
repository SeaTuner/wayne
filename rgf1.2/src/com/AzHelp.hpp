/* * * * *
 *  AzHelp.hpp 
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

#ifndef _AZ_HELP_HPP_
#define _AZ_HELP_HPP_

#include "AzPrint.hpp"

/*----------------------------------------------------------------*/
//! Display help. 
class AzHelp {
protected:
  AzOut out; 
  int kw_width, ind; 
  static const int ind_dflt = 3; 
  static const int kw_width_dflt = 20; 
  static const int line_width = 78; 
public:
  AzHelp(const AzOut &inp_out, 
         int inp_kw_width=kw_width_dflt) {
    out = inp_out; 
    kw_width = inp_kw_width; 
    ind = ind_dflt; 
  }

  inline void set_kw_width() {
    kw_width = kw_width_dflt; 
  }
  inline void set_kw_width(int inp_kw_width) {
    kw_width = inp_kw_width; 
  }
  inline void set_indent(int inp_ind) {
    ind = inp_ind; 
  }

  /*-------------------*/
  void toplevel_header(const char *desc, AzByte dlm='*') 
  {
    if (out.isNull()) return; 

    newline(); 

    AzBytArr s; 
    s.fill(dlm, 3); 
    int dlm_len = Az64::cstrlen(desc) + s.length()*2; 
    dlm_len = MIN(line_width, dlm_len); 

    AzBytArr s_long; 
    s_long.fill(dlm, dlm_len); 

    AzPrint::writeln(out, s_long.c_str()); 
    AzPrint::write(out, s.c_str()); 
    AzPrint::write(out, desc); 
    AzPrint::writeln(out, s.c_str()); 
    AzPrint::writeln(out, s_long.c_str()); 
  }

  int getLevel() const {
    return out.getLevel(); 
  }

  /*-------------------*/
  void begin(const char *compo, const char *name, const char *desc=NULL) 
  {
    if (out.getLevel() == 0) return; 

    if (out.isNull()) return; 

    AzBytArr s(desc); 
    if (s.length() > 0) {
      s.c(" ("); s.c(name); 