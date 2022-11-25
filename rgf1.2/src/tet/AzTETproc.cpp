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

  trainer->startup(out, config, m_train_x, v_train_y, featInfo, v_fixed_dw, inp_ens); 
  eval->begin(config, trainer->lossType()); 
  int seq_no = 1; 
  for ( ; ; ) {
    /*---  proceed with training  ---*/
    AzTETrainer_Ret ret = trainer->proceed_until(); 

    /*---  test the current model  ---*/
    AzDvect v_p; 
    AzTE_ModelInfo info; 
    trainer->apply(&td, &v_p, &info); 
    eval->evaluate(&v_p, &info); 

    if (ret == AzTETrainer_Ret_Exit) {
      break;   
    }
  }
  eval->end(); 
}

/*------------------------------------------------------------------*/
void AzTETproc::train_test_save(const AzOut &out, 
                        AzTETrainer *trainer, 
                        const char *config, 
                        AzSmat *m_train_x, 
                        AzDvect *v_train_y, 
                        const AzSvFeatInfo *featInfo,
                        /*---  for evaluation  ---*/
                        AzSmat *m_test_x, 
                        AzTET_Eval *eval, 
                        /*---  for saving models  ---*/
                        bool doSaveLastModelOnly, 
                        const char *out_model_fn, 
                        const char *out_model_names_fn, /* may be NULL */
                        /*---  data point weights  ---*/
                        AzDvect *v_fixed_dw, /* may be NULL */
                        /*---  for warm start  ---*/
                        AzTreeEnsemble *inp_ens) /* may be NULL */
{
  AzTETrainer_TestData td(out, m_test_x); 

  trainer->startup(out, config, m_train_x, v_train_y, featInfo, v_fixed_dw, inp_ens); 
  eval->begin(config, trainer->lossType()); 
  int seq_no = 1; 
  int model_num = 0; 
  AzBytArr s_model_names; 
  for ( ; ; ) {
    /*---  proceed with training  ---*/
    AzTETrainer_Ret ret = trainer->proceed_until(); 

    AzTreeEnsemble ens; 
    AzDvect v_p; 
    AzTE_ModelInfo info; 
    trainer->apply(&td, &v_p, &info, &ens); 
    AzBytArr s_model_fn; 
    const char *model_fn = NULL; 
    if (!doSaveLastModelOnly || ret == AzTETrainer_Ret_Exit) {
      writeModel(&ens, seq_no, out_model_fn, &s_model_fn, &s_model_names, out); 
      ++model_num; 
      model_fn = s_model_fn.c_str(); 
    }
    eval->evaluate(&v_p, &info, model_fn); 
    ++seq_no; 

    if (ret == AzTETrainer_Ret_Exit) {
      break;   
    }
  }
  eval->end(); 

  end_of_saving_models(model_num, s_model_names, out_model_names_fn, out); 
}

/*------------------------------------------------------------------*/
void AzTETproc::end_of_saving_models(int model_num, 
                                     const AzBytArr &s_model_names, 
                                     const char *out_model_names_fn, 
                                     const AzOut &out)
{
  if (isSpecified(out_model_names_fn)) {
    AzFile file(out_model_names_fn); 
    file.open("wb"); 
    s_model_names.writeText(&file); 
    file.close(true); 
  }
  if (!out.isNull()) {
    AzBytArr s("Generated "); s.cn(model_num); s.c(" model file(s): "); 
    AzPrint::writeln(out, ""); 
    AzPrint::writeln(out, s); 
    AzPrint::writeln(out, s_model_names); 
  }
}

/*------------------------------------------------------------------*/
void AzTETproc::writeModel(AzTreeEnsemble *ens, 
                           int seq_no, 
                           const char *fn_stem, 
                           AzBytArr *s_model_fn, 
                           AzBytArr *s_model_names, 
                           const AzOut &out)
{
  AzBytArr s; 
  gen_model_fn(fn_stem, seq_no, &s); 
  AzTimeLog::print("Writing model: seq#=", seq_no, out); 
  ens->write(s.c_str()); 
  if (s_model_fn != NULL) s_model_fn->concat(&s); 
  s.nl(); 
  s_model_names->concat(&s); 
}

/*------------------------------------------------------------------*/
void AzTETproc::gen_model_fn(const char *fn_stem, 
                             int seq_no, 
                             AzBytArr *s) /* output */
{
  s->reset(fn_stem); 
  s->c("-"); s->cn(seq_no, 2, true); /* width 2, fill with zero */
}

/*------------------------------------------------------------------*/
void AzTETproc::train_predict(const AzOut &out, 
                        AzTETrainer *trainer, 
                        const char *config, 
                        AzSmat *m_train_x, 
                        AzDvect *v_train_y, 
                        const AzSvFeatInfo *featInfo,
                        bool doSaveLastModelOnly, 
                        /*---  for prediction  ---*/
                        AzSmat *m_test_x, 
                        const char *model_fn_prefix, 
                        const char *pred_fn_suffix,
                        const char *info_fn_suffix,
                        /*---  data point weights  ---*/
                        AzDvect *v_fixed_dw, /* may be NULL */
                        /*---  for warm start  ---*/
                        AzTreeEnsemble *inp_ens) /* may be NULL */
{
  AzTETrainer_TestData td(out, m_test_x); 

  int model_num = 0; 
  AzBytArr s_model_names; 
  trainer->startup(out, config, m_train_x, v_train_y, featInfo, v_fixed_dw, inp_ens); 
  int seq_no = 1; 
  for ( ; ; ) {
    /*---  proceed with training  ---*/
    AzTETrainer_Ret ret = trainer->proceed_until(); 

    AzTreeEnsemble ens; 
    AzDvect v_p; 
    AzTE_ModelInfo info; 
    trainer->apply(&t