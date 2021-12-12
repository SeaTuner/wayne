
"""
__file__

    generate_ensemble_submission.py

__description__

    This file generates submission via ensemble selection.

__author__

    Chenglong Chen < c.chenglong@gmail.com >

"""

import os
import sys
import numpy as np
import pandas as pd
from utils import *
from ensemble_selection import *
from model_library_config import feat_folders, feat_na