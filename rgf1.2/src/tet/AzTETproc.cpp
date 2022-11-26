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
    trainer->apply(&td, &v_p, &info, &ens); 

    writePrediction(model_fn_prefix, &v_p, seq_no, pred_fn_suffix, out); 
    writeModelInfo(model_fn_prefix, seq_no, info_fn_suffix, &info, out); 

    if (ret == AzTETrainer_Ret_Exit || 
        !doSaveLastModelOnly) {
      writeModel(&ens, seq_no, model_fn_prefix, NULL, &s_model_names, out); 
      ++model_num; 
    }
    if (ret == AzTETrainer_Ret_Exit) {
      break;   
    }
    ++seq_no; 
  }
  end_of_saving_models(model_num, s_model_names, "", out); 
}

/*------------------------------------------------------------------*/
void AzTETproc::train_predict2(const AzOut &out, 
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
  AzTimeLog::print("train_predict2 ... ", log_out); 
  int model_num = 0; 
  AzBytArr s_model_names; 
  trainer->startup(out, config, m_train_x, v_train_y, featInfo, v_fixed_dw, inp_ens); 
  int seq_no = 1; 
  for ( ; ; ) {
    /*---  proceed with training  ---*/
    AzTETrainer_Ret ret = trainer->proceed_until(); 

    AzTreeEnsemble ens; 
    trainer->copy_to(&ens); 
    AzDvect v_p; 
    ens.apply(m_test_x, &v_p);  
    AzTE_ModelInfo info; 
    ens.info(&info); 

    writePrediction(model_fn_prefix, &v_p, seq_no, pred_fn_suffix, out); 
    writeModelInfo(model_fn_prefix, seq_no, info_fn_suffix, &info, out); 

    if (ret == AzTETrainer_Ret_Exit || 
        !doSaveLastModelOnly) {
      writeModel(&ens, seq_no, model_fn_prefix, NULL, &s_model_names, out); 
      ++model_num; 
    }
    if (ret == AzTETrainer_Ret_Exit) {
      break;   
    }
    ++seq_no; 
  }
  end_of_saving_models(model_num, s_model_names, "", out); 
}

/*------------------------------------------------------------------*/
void AzTETproc::writePrediction(
                           const char *fn_stem, 
                           const AzDvect *v_p, 
                           int seq_no, 
                           const char *pred_suffix, 
                           const AzOut &out)
{
  AzTimeLog::print("Writing prediction: seq#=", seq_no, out); 

  AzBytArr s_fn; 
  gen_model_fn(fn_stem, seq_no, &s_fn); 
  AzBytArr s_pfn; 
  s_pfn.concat(&s_fn); s_pfn.concat(pred_suffix); 
  if (s_pfn.compare(&s_fn) == 0) {
    throw new AzException(AzInputError, "AzTETproc::writePrediction", 
                          "No suffix for prediction filenames is given"); 
  }

  writePrediction(s_pfn.c_str(), v_p); 
}

/*------------------------------------------------------------------*/
void AzTETproc::writePrediction(const char *fn, 
                                const AzDvect *v_p)
{
  /* one prediction value per line */
  int width = 8; 
  AzBytArr s; 
  int dx; 
  for (dx = 0; dx < v_p->rowNum(); ++dx) {
    double val = v_p->get(dx); 
    s.concatFloat(val, width); 
    s.nl(); /* new line */
  } 
  AzFile file(fn); 
  file.open("wb"); 
  s.writeText(&file); 
}

/*------------------------------------------------------------------*/
void AzTETproc::writeModelInfo(
                           const char *fn_stem, 
                           int seq_no, 
                           const char *info_suffix, 
                           const AzTE_ModelInfo *info, 
                           const AzOut &out)
{
  AzBytArr s_fn; 
  gen_model_fn(fn_stem, seq_no, &s_fn); 
  AzBytArr s_ifn; 
  s_ifn.concat(&s_fn); s_ifn.concat(info_suffix); 

  writeModelInfo(s_ifn.c_str(), s_fn.c_str(), info, out); 
}
 
/*------------------------------------------------------------------*/
void AzTETproc::writeModelInfo(const char *info_fn, 
                           const char *model_fn, 
                           const AzTE_ModelInfo *info, 
                           const AzOut &out)
{
  AzTimeLog::print("Writing model info", out); 

  AzBytArr s; 
  /*---  info  ---*/
  AzBytArr s_cfg(&info->s_config); 
  s_cfg.replace(',', ';'); 

  s.reset(); 
  s.c("#tree,"); s.cn(info->tree_num); 
  s.c(",#leaf,"); s.cn(info->leaf_num); 
  s.c(",sign,"); s.c(&info->s_sign); 
  s.c(",cfg,"); s.c(&s_cfg); 
  s.c(","); s.c(model_fn); 
  s.nl(); 

  AzFile ifile(info_fn); 
  ifile.open("wb"); 
  s.writeText(&ifile); 
  ifile.close(true); 
}

/*------------------------------------------------------------------*/
void AzTETproc::xv(const AzOut &out, 
                   int xv_num, 
                   const char *xv_fn, 
                   bool doShuffle, 
                   AzTETrainer *trainer, 
                   const char *config, 
                   AzSmat *m_x, 
                   AzDvect *v_y, 
                   const AzSvFeatInfo *featInfo,
                   /*---  data point weights  ---*/
                   AzDvect *v_dw) /* may be NULL */
{
  const char *eyec = "AzTETproc::xv"; 

  int n