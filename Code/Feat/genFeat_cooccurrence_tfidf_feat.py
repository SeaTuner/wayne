
"""
__file__

    genFeat_cooccurrence_tfidf.py

__description__

    This file generates the following features for each run and fold, and for the entire training and testing set.

        1. tfidf for the following cooccurrence terms
            - query unigram/bigram & title unigram/bigram
            - query unigram/bigram & description unigram/bigram
            - query id & title unigram/bigram
            - query id & description unigram/bigram

        2. corresponding lsa (svd) version features

__author__

    Chenglong Chen < c.chenglong@gmail.com >

"""

import re
import sys
import cPickle
import ngram
from feat_utils import dump_feat_name
from sklearn.decomposition import TruncatedSVD
from nlp_utils import stopwords, english_stemmer, stem_tokens, getTFV
sys.path.append("../")
from param_config import config

######################
## Pre-process data ##
######################
token_pattern = r"(?u)\b\w\w+\b"
#token_pattern = r'\w{1,}'
#token_pattern = r"\w+"
#token_pattern = r"[\w']+"
def preprocess_data(line,
                    token_pattern=token_pattern,
                    exclude_stopword=config.cooccurrence_word_exclude_stopword,
                    encode_digit=False):
    token_pattern = re.compile(token_pattern, flags = re.UNICODE | re.LOCALE)
    ## tokenize
    tokens = [x.lower() for x in token_pattern.findall(line)]
    ## stem
    tokens_stemmed = stem_tokens(tokens, english_stemmer)
    if exclude_stopword:
        tokens_stemmed = [x for x in tokens_stemmed if x not in stopwords]
    return tokens_stemmed


########################
## Cooccurrence terms ##
########################
def cooccurrence_terms(lst1, lst2, join_str):
    terms = [""] * len(lst1) * len(lst2)
    cnt =  0
    for item1 in lst1:
        for item2 in lst2:
            terms[cnt] = item1 + join_str + item2
            cnt += 1
    res = " ".join(terms)
    return res


##################
## Extract feat ##
##################
def extract_feat(df):
    ## unigram
    print "generate unigram"
    df["query_unigram"] = list(df.apply(lambda x: preprocess_data(x["query"]), axis=1))
    df["title_unigram"] = list(df.apply(lambda x: preprocess_data(x["product_title"]), axis=1))
    df["description_unigram"] = list(df.apply(lambda x: preprocess_data(x["product_description"]), axis=1))
    ## bigram
    print "generate bigram"
    join_str = "_"
    df["query_bigram"] = list(df.apply(lambda x: ngram.getBigram(x["query_unigram"], join_str), axis=1))
    df["title_bigram"] = list(df.apply(lambda x: ngram.getBigram(x["title_unigram"], join_str), axis=1))
    df["description_bigram"] = list(df.apply(lambda x: ngram.getBigram(x["description_unigram"], join_str), axis=1))
    # ## trigram
    # join_str = "_"
    # df["query_trigram"] = list(df.apply(lambda x: ngram.getTrigram(x["query_unigram"], join_str), axis=1))
    # df["title_trigram"] = list(df.apply(lambda x: ngram.getTrigram(x["title_unigram"], join_str), axis=1))
    # df["description_trigram"] = list(df.apply(lambda x: ngram.getTrigram(x["description_unigram"], join_str), axis=1))

    ## cooccurrence terms
    join_str = "X"
    # query unigram
    df["query_unigram_title_unigram"] = list(df.apply(lambda x: cooccurrence_terms(x["query_unigram"], x["title_unigram"], join_str), axis=1))
    df["query_unigram_title_bigram"] = list(df.apply(lambda x: cooccurrence_terms(x["query_unigram"], x["title_bigram"], join_str), axis=1))
    df["query_unigram_description_unigram"] = list(df.apply(lambda x: cooccurrence_terms(x["query_unigram"], x["description_unigram"], join_str), axis=1))
    df["query_unigram_description_bigram"] = list(df.apply(lambda x: cooccurrence_terms(x["query_unigram"], x["description_bigram"], join_str), axis=1))
    # query bigram
    df["query_bigram_title_unigram"] = list(df.apply(lambda x: cooccurrence_terms(x["query_bigram"], x["title_unigram"], join_str), axis=1))
    df["query_bigram_title_bigram"] = list(df.apply(lambda x: cooccurrence_terms(x["query_bigram"], x["title_bigram"], join_str), axis=1))
    df["query_bigram_description_unigram"] = list(df.apply(lambda x: cooccurrence_terms(x["query_bigram"], x["description_unigram"], join_str), axis=1))
    df["query_bigram_description_bigram"] = list(df.apply(lambda x: cooccurrence_terms(x["query_bigram"], x["description_bigram"], join_str), axis=1))
    # query id
    df["query_id_title_unigram"] = list(df.apply(lambda x: cooccurrence_terms(["qid"+str(x["qid"])], x["title_unigram"], join_str), axis=1))
    df["query_id_title_bigram"] = list(df.apply(lambda x: cooccurrence_terms(["qid"+str(x["qid"])], x["title_bigram"], join_str), axis=1))
    df["query_id_description_unigram"] = list(df.apply(lambda x: cooccurrence_terms(["qid"+str(x["qid"])], x["description_unigram"], join_str), axis=1))
    df["query_id_description_bigram"] = list(df.apply(lambda x: cooccurrence_terms(["qid"+str(x["qid"])], x["description_bigram"], join_str), axis=1))

        

if __name__ == "__main__":

    ############
    ## Config ##
    ############
    ## cooccurrence terms column names
    column_names = [
        "query_unigram_title_unigram",
        "query_unigram_title_bigram",
        "query_unigram_description_unigram",
        "query_unigram_description_bigram",
        "query_bigram_title_unigram",
        "query_bigram_title_bigram",
        "query_bigram_description_unigram",
        "query_bigram_description_bigram",
        "query_id_title_unigram",
        "query_id_title_bigram",
        "query_id_description_unigram",
        "query_id_description_bigram",
    ]
    ## feature names
    feat_names = [ name+"_tfidf" for name in column_names ]
    ## file to save feat names
    feat_name_file = "%s/intersect_tfidf.feat_name" % config.feat_folder

    ngram_range = config.cooccurrence_tfidf_ngram_range

    svd_n_components = 100

    ###############
    ## Load Data ##
    ###############
    ## load data
    with open(config.processed_train_data_path, "rb") as f:
        dfTrain = cPickle.load(f)
    with open(config.processed_test_data_path, "rb") as f:
        dfTest = cPickle.load(f)
    ## load pre-defined stratified k-fold index
    with open("%s/stratifiedKFold.%s.pkl" % (config.data_folder, config.stratified_label), "rb") as f:
            skf = cPickle.load(f)

    #######################
    ## Generate Features ##
    #######################
    print("==================================================")
    print("Generate co-occurrence tfidf features...")

    ## get cooccurrence terms
    extract_feat(dfTrain)
    extract_feat(dfTest)

    ######################
    ## Cross validation ##
    ######################
    print("For cross-validation...")
    for run in range(config.n_runs):
        ## use 33% for training and 67 % for validation
        ## so we switch trainInd and validInd
        for fold, (validInd, trainInd) in enumerate(skf[run]):
            print("Run: %d, Fold: %d" % (run+1, fold+1))
            path = "%s/Run%d/Fold%d" % 