/* * * * *
 *  AzTETproc.cpp 
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

#include "AzTETproc.hpp"
#include "AzTaskTools.hpp"

/*------------------------------------------------------------------*/
void AzTETproc::train(const AzOut &out, 
                      AzTETrainer *trainer, 
                      const char *config,                       
                      AzSmat *m_train_x, 
                      AzDvect *v_train_y, 
                      const AzSvFeatInfo *featInfo, 
                      /*---  for writing model to file  ---*/
                      const char *out_model_fn, 
                      const char *out_model_names_fn, /* may be NULL */
                      /*---  data point weights  ---*/
                      AzDvect *v_fixed_dw, /* may be NULL */
                      /*---  for warm start  ---*/
                      AzTreeEnsemble *inp_ens) /* may be NULL */
{
  trainer->startup(out, config, m_train_x, v_train_y, featInfo, v_fixed_dw, inp_ens); 

  AzBytArr s_model_names; 
  int seq_no = 1; 
  for ( ; ; ) {
    AzTETrainer_Ret ret = trainer->proceed_until(); 
    if (out_model_fn != NULL) {
      AzTreeEnsemble ens; 
      trainer->copy_to(&ens); 
      writeModel(&ens, seq_no, out_model_fn, NULL, &s_model_names, out); 
      ++seq_no; 
    }
    if (ret == AzTETrainer_Ret_Exit) {
      break;   
    }
  }
  int model_num = seq_no - 1; 
  end_of_saving_models(model_num, s_model_names, out_model_names_fn, out); 
}

/*------------------------------------------------------------------*/
void AzTETproc::train_test(const AzOut &out, 
                        AzTETrainer *trainer, 
                        const char *config, 
                        AzSmat *m_train_x, 
                        AzDvect *v_train_y, 
                        const AzSvFeatInfo *featInfo,
                        /*---  for evaluation  ---*/
                        AzSmat *m_test_x, 
                        AzTET_Eval *eval, 
                        /*---  data point weights  ---*/
                        AzDvect *v_fixed_dw, /* may be NULL */
                        /*---  for warm start  ---*/
                        AzTreeEnsemble *inp_ens) /* may be NULL */
{
  AzTETrainer_TestData td(out, m_test_x); 

  trainer->startup(out, config, m_train_x, v_train_y, fe