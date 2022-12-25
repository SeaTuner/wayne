/* * * * *
 *  AzTreeEnsemble.cpp 
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

#include "AzTreeEnsemble.hpp"
#include "AzPrint.hpp"

static int reserved_length = 256; 

/*--------------------------------------------------------*/
void AzTreeEnsemble::info(AzTE_ModelInfo *out_info) const
{
  if (out_info == NULL) return; 

  out_info->leaf_num = leafNum(); 
  out_info->tree_num = size(); 
  out_info->s_sign.reset(&s_sign); 
  out_info->s_config.reset(&s_config); 
}

/*--------------------------------------------------------*/
void AzTreeEnsemble::transfer_from(AzTree *inp_tree[], 
                                   int inp_tree_num, 
                                   double inp_const_val, 
                                   int inp_org_dim, 
                                   const char *config, 
                                   const char *sign)
{
  a_tree.free(&t); t_num = 0;  
  a_tree.alloc(&t, inp_tree_num, "AzTreeEnsemble::reset");                           
  t_num = inp_tree_num; 
  const_val = inp_const_val; 
  org_dim = inp_org_dim; 
  int tx; 
  for (tx = 0; tx < t_num; ++tx) {
    if (inp_tree[tx] != NULL) {
      t[tx] = inp_tree[tx]; 
      inp_tree[tx] = NULL; 
    }
  }
  s_config.reset(config); 
  s_sign.reset(sign); 

  clean_up(); 
}

/*--------------------------------------------------------*/
void AzTreeEnsemble::write(AzFile *file)
{
  file->writeBinMarker(); 
  int ix; 
  for (ix = 0; ix < reserved_length; ++ix) file->writeByte(0); 
  file->writeInt(t_num); 
  file->writeDouble(const_val); 
  file->writeInt(org_dim); 
  s_config.write(file); 
  s_sign.write(file); 
  int tx; 
  for (tx = 0; tx < t_num; ++tx) {
    AzObjIOTools::write(t[tx],