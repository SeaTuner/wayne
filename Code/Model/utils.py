
"""
__file__

    utils.py

__description__

    This file provides functions for
        1. various customized objectives used together with XGBoost
        2. corresponding decoding method for various objectives
            - MSE
            - Pairwise ranking
            - Softmax
            - Softkappa
            - EBC
            - COCR

__author__

    Chenglong Chen < c.chenglong@gmail.com >

"""

import sys
import numpy as np
from ml_metrics import quadratic_weighted_kappa
sys.path.append("../")
from param_config import config


######################
## Helper Functions ##
######################
#### sigmoid
def sigmoid(score):
    p = 1. / (1. + np.exp(-score))
    return p

#### softmax
def softmax(score):
    score = np.asarray(score, dtype=float)
    score = np.exp(score-np.max(score))
    score /= np.sum(score, axis=1)[:,np.newaxis]
    return score


##########################
## Cutomized Objectives ##
##########################
#### Implement the method described in the paper:
# Ordinal Regression by Extended Binary Classification
# Ling Li, Hsuan-Tien Lin
def ebcObj(preds, dtrain):
    ## label are +1/-1
    labels = dtrain.get_label()
    weights = dtrain.get_weight()
    ## extended samples within the feature construction part
    if np.min(labels) == -1 and np.max(labels) == 1:
        s = np.exp(labels * preds)
        grad = - weights * labels / (1. + s)
        hess = weights * (labels**2) * s / ((1. + s)**2)
        ## TODO: figure out how to apply sample weights
    ## extended samples within the objective value computation part
    else:
        ## label are in [0,1,2,3]
        labels += 1
        M = preds.shape[0]
        N = preds.shape[1]
        grad = np.zeros((M,N), dtype=float)
        hess = np.zeros((M,N), dtype=float)
        ## we only use the first K-1 class for extended examples
        for c in range(N-1):
            k = c+1
            Y = 2. * np.asarray(labels > k, dtype=float) - 1.
            C_yk = np.power(Y - k, 2)
            C_yk1 = np.power(Y - (k+1), 2)
            w = np.abs(C_yk - C_yk1)
            p = preds[:,c]
            s = np.exp(Y * p)
            grad[:,c] = - w * Y / (1. + s)
            hess[:,c] = w * (Y**2) * s / ((1. + s)**2)
        ## apply sample weights
        grad *= weights[:,np.newaxis]
        hess *= weights[:,np.newaxis]
        grad.shape = (M*N)
        hess.shape = (M*N)
    return grad, hess

#### Implement the method described in the paper:
# Improving ranking performance with cost-sensitive ordinal classification via regression
# Yu-Xun Ruan, Hsuan-Tien Lin, and Ming-Feng Tsai
def cocrObj(preds, dtrain):
    ## label are in [0,1,2,3]
    Y = dtrain.get_label()
    Y = Y[:,np.newaxis]
    ## get sample weights
    weights = dtrain.get_weight()
    weights = weights[:,np.newaxis]
    ##
    M,N = preds.shape
    k = np.asarray(range(1,N+1))
    k = k[np.newaxis,:]
    b = np.asarray(Y >= k)
    C_yk = np.power(Y - k, 2)
    C_yk1 = np.power(Y - (k-1), 2)
    w = np.abs(C_yk - C_yk1)
    grad = 2 * w * (preds - b)
    hess = 2 * w

    ## apply sample weights
    grad *= weights
    hess *= weights
    grad.shape = (M*N)
    hess.shape = (M*N)
    return grad, hess

#### directly optimized kappa (old version)
def softkappaObj(preds, dtrain):
    ## label are in [0,1,2,3]
    labels = dtrain.get_label() + 1
    labels = np.asarray(labels, dtype=int)
    preds = softmax(preds)
    M = preds.shape[0]
    N = preds.shape[1]

    ## compute O (enumerator)
    O = 0.0
    for j in range(N):
        wj = (labels - (j+1.))**2
        O += np.sum(wj * preds[:,j])
    
    ## compute E (denominator)
    hist_label = np.bincount(labels)[1:]
    hist_pred = np.sum(preds, axis=0)
    E = 0.0
    for i in range(N):
        for j in range(N):
            E += pow(i - j, 2.0) * hist_label[i] * hist_pred[j]

    ## compute gradient and hessian
    grad = np.zeros((M, N))
    hess = np.zeros((M, N))
    for n in range(N):
        ## first-order derivative: dO / dy_mn
        dO = np.zeros((M))
        for j in range(N):
            indicator = float(n == j)
            dO += ((labels - (j+1.))**2) * preds[:,n] * (indicator - preds[:,j])
        ## first-order derivative: dE / dy_mn
        dE = np.zeros((M))
        for k in range(N):
            for l in range(N):
                indicator = float(n == k)
                dE += pow(k-l, 2.0) * hist_label[l] * preds[:,n] * (indicator - preds[:,k])
        ## the grad
        grad[:,n] = -M * (dO * E - O * dE) / (E**2)
        
        ## second-order derivative: d^2O / d (y_mn)^2
        d2O = np.zeros((M))
        for j in range(N):
            indicator = float(n == j)
            d2O += ((labels - (j+1.))**2) * preds[:,n] * (1 - 2.*preds[:,n]) * (indicator - preds[:,j])
       
        ## second-order derivative: d^2E / d (y_mn)^2
        d2E = np.zeros((M))
        for k in range(N):
            for l in range(N):
                indicator = float(n == k)
                d2E += pow(k-l, 2.0) * hist_label[l] * preds[:,n] * (1 - 2.*preds[:,n]) * (indicator - preds[:,k])
        ## the hess
        hess[:,n] = -M * ((d2O * E - O * d2E)*(E**2) - (dO *