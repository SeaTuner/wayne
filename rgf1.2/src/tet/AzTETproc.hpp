
/* * * * *
 *  AzTETproc.hpp 
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

#ifndef _AZ_TET_PROC_HPP_
#define _AZ_TET_PROC_HPP_

#include <ctime>

#include "AzUtil.hpp"
#include "AzIntPool.hpp"
#include "AzTETrainer.hpp"
#include "AzTET_Eval.hpp"

//! Call tree ensemble trainer.
class AzTETproc {
public:
  static void train(const AzOut &out, 
                    AzTETrainer *trainer, 
                    const char *config, 
                    AzSmat *m_train_x, 
                    AzDvect *v_train_y, 
                    const AzSvFeatInfo *featInfo,
                    /*---  for writing model to file  ---*/
                    const char *out_model_fn, 
                    const char *out_model_list_fn=NULL, 
                    /*---  data point weights  ---*/
                    AzDvect *v_fixed_dw=NULL, /* may be NULL */
                    /*---  for warm start  ---*/
                    AzTreeEnsemble *inp_ens=NULL); /* may be NULL */

  static void train_test(const AzOut &out, 
                        AzTETrainer *trainer, 
                        const char *config, 
                        AzSmat *m_train_x, 
                        AzDvect *v_train_y, 
                        const AzSvFeatInfo *featInfo,
                        /*---  for evaluation  ---*/
                        AzSmat *m_test_x, 
                        AzTET_Eval *eval, 
                        /*---  data point weights  ---*/
                        AzDvect *v_fixed_dw=NULL, /* may be NULL */
                        /*---  for warm start  ---*/
                        AzTreeEnsemble *inp_ens=NULL); /* may be NULL */

  static void train_test_save(const AzOut &out, 
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
                        const char *out_model_names_fn=NULL, 
                        /*---  data point weights  ---*/
                        AzDvect *v_fixed_dw=NULL, /* may be NULL */
                        /*---  for warm start  ---*/
                        AzTreeEnsemble *inp_ens=NULL); /* may be NULL */

  static void train_predict(const AzOut &out, 
                        AzTETrainer *trainer, 
                        const char *config, 
                        AzSmat *m_train_x, 
                        AzDvect *v_train_y, 
                        const AzSvFeatInfo *featInfo,
                        bool doSaveLastModelOnly, 
                        /*---  for prediction  ---*/
                        AzSmat *m_test_x, 
                        /*---  for saving predictions, model info, and the last model ---*/
                        const char *model_fn_prefix, 
                        const char *pred_fn_suffix,
                        const char *info_fn_suffix,
                        /*---  data point weights  ---*/
                        AzDvect *v_fixed_dw, /* may be NULL */
                        /*---  for warm start  ---*/
                        AzTreeEnsemble *inp_ens); /* may be NULL */

  static void train_predict2(const AzOut &out, 
                        AzTETrainer *trainer, 
                        const char *config, 
                        AzSmat *m_train_x, 
                        AzDvect *v_train_y, 
                        const AzSvFeatInfo *featInfo,
                        bool doSaveLastModelOnly, 
                        /*---  for prediction  ---*/
                        AzSmat *m_test_x, 
                        /*---  for saving predictions, model info, and the last model ---*/
                        const char *model_fn_prefix, 
                        const char *pred_fn_suffix,
                        const char *info_fn_suffix,
                        /*---  data point weights  ---*/
                        AzDvect *v_fixed_dw, /* may be NULL */
                        /*---  for warm start  ---*/
                        AzTreeEnsemble *inp_ens); /* may be NULL */