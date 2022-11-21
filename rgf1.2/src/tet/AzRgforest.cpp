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
  warmupEnsemble(az_param, max_tree_num, inp_ens); /* v_p is set */

  reg_depth->reset(az_param, out);  /* init regularizer on node depth */

  AzTimeLog::print("Warming-up the optimizer ... ", log_out); 
  opt->warm_start(loss_type, data, reg_depth, /* initialize optimizer */
                  az_param, v_y, v_fixed_dw, out, 
                  ens, &v_p); 

  initTarget(v_y, v_fixed_dw); 

  fs->reset(az_param, reg_depth, out); /* initialize node search */
  az_param.check(out); 
  l_num = ens->leafNum();  /* warm-up #leaf */

  if (!beVerbose) { 
    out.deactivate(); /* shut up after printing everyone's config */
  }

  time_init(); /* initialize time measure ment */
  end_of_initialization(); 
  AzTimeLog::print("End of warming-up ... ", log_out); 
}

/*------------------------------------------------------------------*/
/* Update test_timer, opt_timer */
void AzRgforest::warmup_timer(const AzTreeEnsemble *inp_ens, 
                              int max_tree_num)
{
  const char *eyec = "AzRgforest::warmup_timer"; 
  /*---  check consistency and adjust intervals for warm start ---*/
  int inp_leaf_num = inp_ens->leafNum(); 
  if (lmax_timer.reachedMax(inp_leaf_num) || 
      max_tree_num < inp_ens->size()) {
    AzBytArr s("The model given for warm-start is already over "); 
    s.concat("the requested maximum size of the models: #leaf="); 
    s.cn(inp_leaf_num); s.c(", #tree="); s.cn(inp_ens->size()); 
    throw new AzException(AzInputError, eyec, s.c_str()); 
  }

  int inp_leaf_num_test = inp_leaf_num; 
  if (inp_leaf_num_test % test_timer.inc == 1) --inp_leaf_num_test; 
  test_timer.chk += inp_leaf_num_test; 

  int inp_leaf_num_opt = inp_leaf_num; 
  if (inp_leaf_num_opt % opt_timer.inc == 1) --inp_leaf_num_opt; 
  opt_timer.chk += inp_leaf_num_opt; 
}

/*------------------------------------------------------------------*/
/* output: ens, output: v_p */
void AzRgforest::warmupEnsemble(AzParam &az_param, int max_tree_num, 
                                const AzTreeEnsemble *inp_ens)
{
  ens->warm_start(inp_ens, data, az_param, &s_temp_for_trees, 
                  out, max_tree_num, s_tree_num, &v_p); 

  /*---  always have one unsplit root: represent the next tree  ---*/
  rootonly_tree->reset(az_param); 
  rootonly_tree->makeRoot(data); 

  rootonly_tx = max_tree_num + 1;  /* any number that doesn't overlap other trees */
}

/*-------------------------------------------------------------------*/
/* input: v_p */
void AzRgforest::initTarget(const AzDvect *v_y, 
                            const AzDvect *v_fixed_dw)
{
  target.reset(v_y, v_fixed_dw);
  resetTarget(); 
}

/*-------------------------------------------------------------------*/
void AzRgforest::setInput(AzParam &p, 
                          const AzSmat *m_x, 
                          const AzSvFeatInfo *featInfo)
{
  dflt_data.reset_data(out, m_x, p, beTight, featInfo); 
  data = &dflt_data; 

  f_pick = -1; 
  if (f_ratio > 0) {
    f_pick = (int)((double)data->featNum() * f_ratio); 
    f_pick = MAX(1, f_pick); 
    AzPrint::writeln(out, "#feature to be sampled = ", f_pick); 
  }
}

/*------------------------------------------------------------------*/
void AzRgforest::initEnsemble(AzParam &az_param, int max_tree_num)
{
  const char *eyec = "AzRgforest::initEnsemble"; 

  if (max_tree_num < 1) {
    throw new AzException(AzInputNotValid, "AzRgforest::initEnsemble", 
                          "max# must be positive"); 
  }

  ens->cold_start(az_param, &s_temp_for_trees, data->dataNum(), 
                  out, max_tree_num, data->featNum()); 

  /*---  always have one unsplit root: represent the next tree  ---*/
  rootonly_tree->reset(az_param); 
  rootonly_tree->makeRoot(data); 

  rootonly_tx = max_tree_num + 1;  /* any number that doesn't overlap other trees */
}

/*-------------------------------------------------------------------*/
AzRgfTree *AzRgforest::tree_to_grow(int &best_tx,  /* inout */
                                   int &best_nx,  /* inout */
                                   bool *isNewTree) /* output */
{
  if (best_tx != rootonly_tx) {
    *isNewTree = false; 
    return ens->tree_u(best_tx); 
  }
  else {
    AzRgfTree *tree = ens->new_tree(&best_tx); /* best_tx is updated */
    tree->reset(out); 
    best_nx = tree->makeRoot(data); 
    *isNewTree = true; 
    return tree; 
  }
}

/*-------------------------------------------------------------------*/
AzTETrainer_Ret AzRgforest::proceed_until()
{
  AzTETrainer_Ret ret = AzTETrainer_Ret_Exit; 
  for ( ; ; ) {
    /*---  grow the forest  ---*/
    bool doExit = growForest(); 
    if (doExit) break; 

    /*---  optimize weights  ---*/
    if (opt_timer.ringing(false, l_num)) {
      optimize_resetTarget(); 
      show_tree_info(); 
    }

    /*---  time to test?  ---*/
    bool doTestNow = test_timer.ringing(false, l_num); 
    if (doTestNow) {  
      ret = AzTETrainer_Ret_TestNow; 
      break; /* time to test */
    }
  }

  if (ret == AzTETrainer_Ret_Exit) {
    if (!isOpt) {
      optimize_resetTarget(); 
    }
    time_show(); 
    end_of_training(); 
  }

  return ret; 
}

/*------------------------------------------------------------------*/
void AzRgforest::time_show()
{
  if (doTime) {
    AzOut my_out = out; 
    my_out.activate(); 
    if (my_out.isNull()) return; 
    AzPrint o(my_out); 
    o.printBegin("", "