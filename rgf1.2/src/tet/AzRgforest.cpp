/* * * * *
 *  AzRgforest.cpp 
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

#include "AzRgforest.hpp"
#include "AzHelp.hpp"
#include "AzRgf_kw.hpp"

/*-------------------------------------------------------------------*/
void AzRgforest::cold_start(const char *param, 
                        const AzSmat *m_x, 
                        const AzDvect *v_y, 
                        const AzSvFeatInfo *featInfo, 
                        const AzDvect *v_fixed_dw, 
                        const AzOut &out_req)
{
  out = out_req; 
  s_config.reset(param); 

  AzParam az_param(param); 
  int max_tree_num = resetParam(az_param); 
  setInput(az_param, m_x, featInfo);        
  reg_depth->reset(az_param, out);  /* init regularizer on node depth */
  v_p.reform(v_y->rowNum()); 
  opt->cold_start(loss_type, data, reg_depth, /* initialize optimizer */
                  az_param, v_y, v_fixed_dw, out, &v_p); 
  initTarget(v_y, v_fixed_dw);    
  initEnsemble(az_param, max_tree_num); /* initialize tree ensemble */
  fs->reset(az_param, reg_depth, out); /* initialize node search */
  az_param.check(out); 
  l_num = 0; /* initialize leaf node counter */

  if (!beVerbose) { 
    out.deactivate(); /* shut up after printing everyone's config */
  }

  time_init(); /* initialize time measure ment */
  end_of_initialization(); 
}

/*-------------------------------------------------------------------*/
void AzRgforest::warm_start(const char *param, 
                        const AzSmat *m_x, 
                        const AzDvect *v_y, 
                        const AzSvFeatInfo *featInfo, 
                        const AzDvect *v_fixed_dw, 
                        const AzTreeEnsemble *inp_ens, 
                        const AzOut &out_req)
{
  const char *eyec = "AzRgforest::warm_start"; 
  out = out_req; 
  if (inp_ens->orgdim() > 0 && 
      inp_ens->orgdim() != m_x->rowNum()) {
    AzBytArr s("Mismatch in feature dimensionality.  "); 
    s.cn(inp_ens->orgdim());s.c(" (tree ensemeble), ");s.cn(m_x->rowNum());s.c(" (dataset)."); 
    throw new AzException(AzInputError, eyec, s.c_str()); 
  }
  s_config.reset(param); 

  AzParam az_param(param); 
  int max_tree_num = resetParam(az_param); 

  warmup_timer(inp_ens, max_tree_num); /* timers are modified for warm-start */

  setInput(az_param, m_x, featInfo); 

  AzTimeLog::print("Warming-up trees ... ", log_out); 
  warmupEnsemble(az_param, max_tree_num,