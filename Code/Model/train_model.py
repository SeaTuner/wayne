
"""
__file__

    train_model.py

__description__

    This file trains various models.

__author__

    Chenglong Chen < c.chenglong@gmail.com >

"""

import sys
import csv
import os
import cPickle
import numpy as np
import pandas as pd
import xgboost as xgb
from scipy.sparse import hstack
## sklearn
from sklearn.base import BaseEstimator
from sklearn.datasets import load_svmlight_file, dump_svmlight_file
from sklearn.preprocessing import StandardScaler
from sklearn.linear_model import LogisticRegression, LinearRegression
from sklearn.linear_model import Ridge, Lasso, LassoLars, ElasticNet
from sklearn.ensemble import RandomForestClassifier, RandomForestRegressor
from sklearn.ensemble import ExtraTreesClassifier, ExtraTreesRegressor
from sklearn.ensemble import GradientBoostingClassifier, GradientBoostingRegressor
from sklearn.svm import SVR
from sklearn.pipeline import Pipeline
## hyperopt
from hyperopt import hp
from hyperopt import fmin, tpe, hp, STATUS_OK, Trials
## keras
from keras.models import Sequential
from keras.layers.core import Dense, Dropout, Activation
from keras.layers.normalization import BatchNormalization
from keras.layers.advanced_activations import PReLU
from keras.utils import np_utils, generic_utils
## cutomized module
from model_library_config import feat_folders, feat_names, param_spaces, int_feat
sys.path.append("../")
from param_config import config
from ml_metrics import quadratic_weighted_kappa
from utils import *


global trial_counter
global log_handler


## libfm
libfm_exe = "../../libfm-1.40.windows/libfm.exe"

## rgf
call_exe = "../../rgf1.2/test/call_exe.pl"
rgf_exe = "../../rgf1.2/bin/rgf.exe"

output_path = "../../Output"

### global params
## you can use bagging to stabilize the predictions
bootstrap_ratio = 1
bootstrap_replacement = False
bagging_size= 1

ebc_hard_threshold = False
verbose_level = 1


#### warpper for hyperopt for logging the training reslut
# adopted from
#
def hyperopt_wrapper(param, feat_folder, feat_name):
    global trial_counter
    global log_handler
    trial_counter += 1

    # convert integer feat
    for f in int_feat:
        if param.has_key(f):
            param[f] = int(param[f])

    print("------------------------------------------------------------")
    print "Trial %d" % trial_counter

    print("        Model")
    print("              %s" % feat_name)
    print("        Param")
    for k,v in sorted(param.items()):
        print("              %s: %s" % (k,v))
    print("        Result")
    print("                    Run      Fold      Bag      Kappa      Shape")

    ## evaluate performance
    kappa_cv_mean, kappa_cv_std = hyperopt_obj(param, feat_folder, feat_name, trial_counter)

    ## log
    var_to_log = [
        "%d" % trial_counter,
        "%.6f" % kappa_cv_mean, 
        "%.6f" % kappa_cv_std
    ]
    for k,v in sorted(param.items()):
        var_to_log.append("%s" % v)
    writer.writerow(var_to_log)
    log_handler.flush()

    return {'loss': -kappa_cv_mean, 'attachments': {'std': kappa_cv_std}, 'status': STATUS_OK}
    

#### train CV and final model with a specified parameter setting
def hyperopt_obj(param, feat_folder, feat_name, trial_counter):

    kappa_cv = np.zeros((config.n_runs, config.n_folds), dtype=float)
    for run in range(1,config.n_runs+1):
        for fold in range(1,config.n_folds+1):
            rng = np.random.RandomState(2015 + 1000 * run + 10 * fold)

            #### all the path
            path = "%s/Run%d/Fold%d" % (feat_folder, run, fold)
            save_path = "%s/Run%d/Fold%d" % (output_path, run, fold)
            if not os.path.exists(save_path):
                os.makedirs(save_path)
            # feat
            feat_train_path = "%s/train.feat" % path
            feat_valid_path = "%s/valid.feat" % path
            # weight
            weight_train_path = "%s/train.feat.weight" % path
            weight_valid_path = "%s/valid.feat.weight" % path
            # info
            info_train_path = "%s/train.info" % path
            info_valid_path = "%s/valid.info" % path
            # cdf
            cdf_valid_path = "%s/valid.cdf" % path
            # raw prediction path (rank)
            raw_pred_valid_path = "%s/valid.raw.pred.%s_[Id@%d].csv" % (save_path, feat_name, trial_counter)
            rank_pred_valid_path = "%s/valid.pred.%s_[Id@%d].csv" % (save_path, feat_name, trial_counter)

            ## load feat
            X_train, labels_train = load_svmlight_file(feat_train_path)
            X_valid, labels_valid = load_svmlight_file(feat_valid_path)
            if X_valid.shape[1] < X_train.shape[1]:
                X_valid = hstack([X_valid, np.zeros((X_valid.shape[0], X_train.shape[1]-X_valid.shape[1]))])
            elif X_valid.shape[1] > X_train.shape[1]:
                X_train = hstack([X_train, np.zeros((X_train.shape[0], X_valid.shape[1]-X_train.shape[1]))])
            X_train = X_train.tocsr()
            X_valid = X_valid.tocsr()
            ## load weight
            weight_train = np.loadtxt(weight_train_path, dtype=float)
            weight_valid = np.loadtxt(weight_valid_path, dtype=float)

            ## load valid info
            info_train = pd.read_csv(info_train_path)
            numTrain = info_train.shape[0]
            info_valid = pd.read_csv(info_valid_path)
            numValid = info_valid.shape[0]
            Y_valid = info_valid["median_relevance"]
            ## load cdf
            cdf_valid = np.loadtxt(cdf_valid_path, dtype=float)

            ## make evalerror func
            evalerror_regrank_valid = lambda preds,dtrain: evalerror_regrank_cdf(preds, dtrain, cdf_valid)
            evalerror_softmax_valid = lambda preds,dtrain: evalerror_softmax_cdf(preds, dtrain, cdf_valid)
            evalerror_softkappa_valid = lambda preds,dtrain: evalerror_softkappa_cdf(preds, dtrain, cdf_valid)
            evalerror_ebc_valid = lambda preds,dtrain: evalerror_ebc_cdf(preds, dtrain, cdf_valid, ebc_hard_threshold)
            evalerror_cocr_valid = lambda preds,dtrain: evalerror_cocr_cdf(preds, dtrain, cdf_valid)

            ##############
            ## Training ##
            ##############
            ## you can use bagging to stabilize the predictions
            preds_bagging = np.zeros((numValid, bagging_size), dtype=float)
            for n in range(bagging_size):
                if bootstrap_replacement:
                    sampleSize = int(numTrain*bootstrap_ratio)
                    index_base = rng.randint(numTrain, size=sampleSize)
                    index_meta = [i for i in range(numTrain) if i not in index_base]
                else:
                    randnum = rng.uniform(size=numTrain)
                    index_base = [i for i in range(numTrain) if randnum[i] < bootstrap_ratio]
                    index_meta = [i for i in range(numTrain) if randnum[i] >= bootstrap_ratio]
                
                if param.has_key("booster"):
                    dvalid_base = xgb.DMatrix(X_valid, label=labels_valid, weight=weight_valid)
                    dtrain_base = xgb.DMatrix(X_train[index_base], label=labels_train[index_base], weight=weight_train[index_base])
                        
                    watchlist = []
                    if verbose_level >= 2:
                        watchlist  = [(dtrain_base, 'train'), (dvalid_base, 'valid')]
                    
                ## various models
                if param["task"] in ["regression", "ranking"]:
                    ## regression & pairwise ranking with xgboost
                    bst = xgb.train(param, dtrain_base, param['num_round'], watchlist, feval=evalerror_regrank_valid)
                    pred = bst.predict(dvalid_base)

                elif param["task"] in ["softmax"]:
                    ## softmax regression with xgboost
                    bst = xgb.train(param, dtrain_base, param['num_round'], watchlist, feval=evalerror_softmax_valid)
                    pred = bst.predict(dvalid_base)
                    w = np.asarray(range(1,numOfClass+1))
                    pred = pred * w[np.newaxis,:]
                    pred = np.sum(pred, axis=1)

                elif param["task"] in ["softkappa"]:
                    ## softkappa with xgboost
                    obj = lambda preds, dtrain: softkappaObj(preds, dtrain, hess_scale=param['hess_scale'])
                    bst = xgb.train(param, dtrain_base, param['num_round'], watchlist, obj=obj, feval=evalerror_softkappa_valid)
                    pred = softmax(bst.predict(dvalid_base))
                    w = np.asarray(range(1,numOfClass+1))
                    pred = pred * w[np.newaxis,:]
                    pred = np.sum(pred, axis=1)

                elif param["task"]  in ["ebc"]:
                    ## ebc with xgboost
                    obj = lambda preds, dtrain: ebcObj(preds, dtrain)
                    bst = xgb.train(param, dtrain_base, param['num_round'], watchlist, obj=obj, feval=evalerror_ebc_valid)
                    pred = sigmoid(bst.predict(dvalid_base))
                    pred = applyEBCRule(pred, hard_threshold=ebc_hard_threshold)

                elif param["task"]  in ["cocr"]:
                    ## cocr with xgboost
                    obj = lambda preds, dtrain: cocrObj(preds, dtrain)
                    bst = xgb.train(param, dtrain_base, param['num_round'], watchlist, obj=obj, feval=evalerror_cocr_valid)
                    pred = bst.predict(dvalid_base)
                    pred = applyCOCRRule(pred)

                elif param['task'] == "reg_skl_rf":
                    ## regression with sklearn random forest regressor
                    rf = RandomForestRegressor(n_estimators=param['n_estimators'],
                                               max_features=param['max_features'],
                                               n_jobs=param['n_jobs'],
                                               random_state=param['random_state'])
                    rf.fit(X_train[index_base], labels_train[index_base]+1, sample_weight=weight_train[index_base])
                    pred = rf.predict(X_valid)

                elif param['task'] == "reg_skl_etr":
                    ## regression with sklearn extra trees regressor
                    etr = ExtraTreesRegressor(n_estimators=param['n_estimators'],
                                              max_features=param['max_features'],
                                              n_jobs=param['n_jobs'],
                                              random_state=param['random_state'])
                    etr.fit(X_train[index_base], labels_train[index_base]+1, sample_weight=weight_train[index_base])
                    pred = etr.predict(X_valid)

                elif param['task'] == "reg_skl_gbm":
                    ## regression with sklearn gradient boosting regressor
                    gbm = GradientBoostingRegressor(n_estimators=param['n_estimators'],
                                                    max_features=param['max_features'],
                                                    learning_rate=param['learning_rate'],
                                                    max_depth=param['max_depth'],
                                                    subsample=param['subsample'],
                                                    random_state=param['random_state'])
                    gbm.fit(X_train.toarray()[index_base], labels_train[index_base]+1, sample_weight=weight_train[index_base])
                    pred = gbm.predict(X_valid.toarray())

                elif param['task'] == "clf_skl_lr":
                    ## classification with sklearn logistic regression
                    lr = LogisticRegression(penalty="l2", dual=True, tol=1e-5,
                                            C=param['C'], fit_intercept=True, intercept_scaling=1.0,
                                            class_weight='auto', random_state=param['random_state'])
                    lr.fit(X_train[index_base], labels_train[index_base]+1)
                    pred = lr.predict_proba(X_valid)
                    w = np.asarray(range(1,numOfClass+1))
                    pred = pred * w[np.newaxis,:]
                    pred = np.sum(pred, axis=1)

                elif param['task'] == "reg_skl_svr":
                    ## regression with sklearn support vector regression
                    X_train, X_valid = X_train.toarray(), X_valid.toarray()
                    scaler = StandardScaler()
                    X_train[index_base] = scaler.fit_transform(X_train[index_base])
                    X_valid = scaler.transform(X_valid)
                    svr = SVR(C=param['C'], gamma=param['gamma'], epsilon=param['epsilon'],
                                            degree=param['degree'], kernel=param['kernel'])
                    svr.fit(X_train[index_base], labels_train[index_base]+1, sample_weight=weight_train[index_base])
                    pred = svr.predict(X_valid)

                elif param['task'] == "reg_skl_ridge":
                    ## regression with sklearn ridge regression
                    ridge = Ridge(alpha=param["alpha"], normalize=True)
                    ridge.fit(X_train[index_base], labels_train[index_base]+1, sample_weight=weight_train[index_base])
                    pred = ridge.predict(X_valid)

                elif param['task'] == "reg_skl_lasso":
                    ## regression with sklearn lasso
                    lasso = Lasso(alpha=param["alpha"], normalize=True)
                    lasso.fit(X_train[index_base], labels_train[index_base]+1)
                    pred = lasso.predict(X_valid)

                elif param['task'] == 'reg_libfm':
                    ## regression with factorization machine (libfm)
                    ## to array
                    X_train = X_train.toarray()
                    X_valid = X_valid.toarray()

                    ## scale
                    scaler = StandardScaler()
                    X_train[index_base] = scaler.fit_transform(X_train[index_base])
                    X_valid = scaler.transform(X_valid)

                    ## dump feat
                    dump_svmlight_file(X_train[index_base], labels_train[index_base], feat_train_path+".tmp")
                    dump_svmlight_file(X_valid, labels_valid, feat_valid_path+".tmp")

                    ## train fm
                    cmd = "%s -task r -train %s -test %s -out %s -dim '1,1,%d' -iter %d > libfm.log" % ( \
                                libfm_exe, feat_train_path+".tmp", feat_valid_path+".tmp", raw_pred_valid_path, \
                                param['dim'], param['iter'])
                    os.system(cmd)
                    os.remove(feat_train_path+".tmp")
                    os.remove(feat_valid_path+".tmp")
                    
                    ## extract libfm prediction
                    pred = np.loadtxt(raw_pred_valid_path, dtype=float)
                    ## labels are in [0,1,2,3]
                    pred += 1

                elif param['task'] == "reg_keras_dnn":
                    ## regression with keras' deep neural networks
                    model = Sequential()
                    ## input layer
                    model.add(Dropout(param["input_dropout"]))
                    ## hidden layers
                    first = True
                    hidden_layers = param['hidden_layers']
                    while hidden_layers > 0:
                        if first:
                            dim = X_train.shape[1]
                            first = False
                        else:
                            dim = param["hidden_units"]
                        model.add(Dense(dim, param["hidden_units"], init='glorot_uniform'))
                        if param["batch_norm"]:
                            model.add(BatchNormalization((param["hidden_units"],)))
                        if param["hidden_activation"] == "prelu":
                            model.add(PReLU((param["hidden_units"],)))
                        else:
                            model.add(Activation(param['hidden_activation']))
                        model.add(Dropout(param["hidden_dropout"]))
                        hidden_layers -= 1

                    ## output layer
                    model.add(Dense(param["hidden_units"], 1, init='glorot_uniform'))
                    model.add(Activation('linear'))

                    ## loss
                    model.compile(loss='mean_squared_error', optimizer="adam")

                    ## to array
                    X_train = X_train.toarray()
                    X_valid = X_valid.toarray()

                    ## scale
                    scaler = StandardScaler()
                    X_train[index_base] = scaler.fit_transform(X_train[index_base])
                    X_valid = scaler.transform(X_valid)

                    ## train
                    model.fit(X_train[index_base], labels_train[index_base]+1,
                                nb_epoch=param['nb_epoch'], batch_size=param['batch_size'],
                                validation_split=0, verbose=0)

                    ##prediction
                    pred = model.predict(X_valid, verbose=0)
                    pred.shape = (X_valid.shape[0],)

                elif param['task'] == "reg_rgf":
                    ## regression with regularized greedy forest (rgf)
                    ## to array
                    X_train, X_valid = X_train.toarray(), X_valid.toarray()

                    train_x_fn = feat_train_path+".x"
                    train_y_fn = feat_train_path+".y"
                    valid_x_fn = feat_valid_path+".x"
                    valid_pred_fn = feat_valid_path+".pred"

                    model_fn_prefix = "rgf_model"

                    np.savetxt(train_x_fn, X_train[index_base], fmt="%.6f", delimiter='\t')
                    np.savetxt(train_y_fn, labels_train[index_base], fmt="%d", delimiter='\t')
                    np.savetxt(valid_x_fn, X_valid, fmt="%.6f", delimiter='\t')
                    # np.savetxt(valid_y_fn, labels_valid, fmt