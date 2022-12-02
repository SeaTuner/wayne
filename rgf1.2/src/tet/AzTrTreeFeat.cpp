/* * * * *
 *  AzTrTreeFeat.cpp 
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

#include "AzTrTreeFeat.hpp"
#include "AzHelp.hpp"
#include "AzRgf_kw.hpp"

/*------------------------------------------------------------------*/
void AzTrTreeFeat::reset(const AzDataForTrTree *data, 
                         AzParam &param, 
                         const AzOut &out_req, 
                         bool inp_doAllowZeroWeightLeaf)
{
  out = out_req; 
  org_featInfo.reset(data->featInfo()); 
  ip_featDef.reset(); 
  int init_size=10000, avg_len=32; 
  pool_rules.reset(init_size, avg_len); 
  pool_rules_rmved.reset(init_size, avg_len); 
  sp_desc.reset(init_size, avg_len); 
  f_inf.reset(); 
  doAllowZeroWeightLeaf = inp_doAllowZeroWeightLeaf; 

  bool beVerbose = resetParam(param); 
  printParam(out); 

  if (!beVerbose) {
    out.deactivate(); 
  }
}

/*------------------------------------------------------------------*/
void AzTrTreeFeat::reset(const AzTrTreeFeat *inp)
{
  doAllowZeroWeightLeaf = inp->doAllowZeroWeightLeaf; 

  org_featInfo.reset((AzSvFeatInfo *)&inp->org_featInfo); 
  ip_featDef.reset(&inp->ip_featDef); 
  sp_desc.reset(&inp->sp_desc); 
  f_inf.reset(&inp->f_inf); 
  pool_rules.reset(&inp->pool_rules); 
  pool_rules_rmved.reset(&inp->pool_rules_rmved); 

  doCountRules = inp->doCountRules; 
  doCheckConsistency = inp->doCheckConsistency; 
  out = inp->out; 
}

/*------------------------------------------------------------------*/
void AzTrTreeFeat::featIds(int tx, 
                          AzIIarr *iia_nx_fx) const 
{
  const char *eyec = "AzTrTreeFeat::featIds"; 
  if (tx < 0 || tx >= ip_featDef.size()) {
    throw new AzException(eyec, "tx is out of range"); 
  }
  int len; 
  const int *def = ip_featDef.point(tx, &len); 
  int nx; 
  for (nx = 0; nx < len; ++nx) {
    if (def[nx] >= 0) {
      iia_nx_fx->put(nx, def[nx]); 
    }
  } 
}

/*! features are only added  -- never removed here. */
/*------------------------------------------------------------------*/
int AzTrTreeFeat::update_with_new_tree(const AzTrTree_ReadOnly *dtree, 
                                       int tx)
{
  const char *eyec = "AzTrTreeFeat::update"; 
  int old_t_num = ip_featDef.size(); 
  if (tx != old_t_num) {
    throw new AzException(eyec, "tree# conflict"); 
  }

  AzIntArr ia_isActiveNode; 
  int added_feat_num = _update(dtree, tx, &ia_isActiveNode); 
  ip_featDef.put(&ia_isActiveNode); 
  if (ip_featDef.size()-1 != tx) {
    throw new AzException(eyec, "ip_featDef conflict"); 
  }

  updateRulePools(); 

  return added_feat_num; 
}

/*! features may be removed and added */
/*------------------------------------------------------------------*/
int AzTrTreeFeat::update_with_ens(const AzTrTreeEnsemble_ReadOnly *ens, 
                                  AzIntArr *ia_rmv_fx) /* output */
{
  int old_t_num = ip_featDef.size(); 
  int added_from_new = 0, added_from_old = 0; 
  added_from_old = _update_with_existing_trees(old_t_num, ens, ia_rmv_fx); 
  added_from_new = _update_with_new_trees(old_t_num, ens);  

  updateRulePools(); 

  if (!out.isNull()) {
    AzPrint o(out); 
    o.printBegin("", ",", ","); 
    o.print("#tree", treeNum()); 
    o.print("#added(from existing trees)", added_from_old); 
    o.print("#added(from new trees)", added_from_new); 
    o.print("#removed", ia_rmv_fx->size()); 
    o.printEnd(); 
  }

  if (doCheckConsistency) {
    checkConsistency(ens); 
  }

  return added_from_new+added_from_old; 
}

/*------------------------------------------------------------------*/
int AzTrTreeFeat::_update_with_new_trees(int old_t_num, 
                  const AzTrTreeEnsemble_ReadOnly *ens)
{
  const char *eyec = "AzTrTreeFeat::_update_with_new_trees"; 
  if (old_t_num != treeNum()) {
    throw new AzException(eyec, "tree# conflict1"); 
  }

  /*---  add those from new trees  ---*/
  int added_f_num1 = 0; 
  int tree_num = ens->size(); 
  int tx; 
  for (tx = old_t_num; tx < tree_num; ++tx) {
    AzIntArr ia_isActiveNode; 
    added_f_num1 += _update(ens->tree(tx), tx, &ia_isActiveNode); 
    ip_featDef.put(&ia_isActiveNode); 
  }

  /*---  just checking ... ---*/
  if (treeNum() != tree_num) {
    throw new AzException(eyec, "tree# conflict2"); 
  }
  return added_f_num1; 
}

/*------------------------------------------------------------------*/
int AzTrTreeFeat::_update_with_existing_trees(int old_t_num, 
                   const AzTrTreeEnsemble_ReadOnly *ens, 
                   AzIntArr *ia_rmv_fx) /* output */
{
  /*---  add features from existing trees  ---*/
  int added_f_num2 = 0; 
  int tx; 
  for (tx = 0; tx < old_t_num; ++tx) {
    int featDef_len; 
    const int *featDef = ip_featDef.point(tx, &featDef_len);     
    if (ens->tree(tx)->nodeNum() == featDef_len) {
      continue; /* no change */
    }

    AzIntArr ia_new_featDef;    
    added_f_num2 += _update(ens->tree(tx), tx, &ia_new_featDef, 
                            featDef, featDef_len, 
                            ia_rmv_fx); 
    ip_featDef.update(tx, &ia_new_featDef); 
  }
  return added_f_num2; 
}

/*------------------------------------------------------------------*/
void AzTrTreeFeat::_updateMatrix(const AzDataForTrTree *data, 
                          const AzTrTreeEnsemble_ReadOnly *ens, 
                          int old_f_num, 
                          /*---  output  ---*/
                          AzBmat *b_tran) const
{
  int data_num = data->dataNum(); 
  int f_num = featNum(); 
  if (old_f_num == 0) {
    b_tran->reform(data_num, f_num); 
  }
  else {
    if (b_tran->rowNum() != data_num || 
        b_tran->colNum() != old_f_num) {
      throw new AzException("AzTrTreeFeat::_updateMatrix", 
                            "b_tran has a wrong shape"); 
    }
    b_tran->resize(f_num); 
  }

  /*---  which trees are referred in the new features?  ---*/
  AzIntArr ia_tx; 
  int fx; 
  for (fx = old_f_num; fx < f_num; ++fx) {
    ia_tx.put(f_inf.point(fx)->tx); 
  }
  ia_tx.unique(); /* remove duplication */
  int tx_num; 
  const int *txs = ia_tx.point(&tx_num); 

  /*---  generate features  ---*/
  AzDataArray<AzIntArr> aia_fx_dx(f_num-old_f_num); 
  int xx; 
  for (xx = 0; xx < tx_num; ++xx) {
    int tx = txs[xx]; 
    int dx; 
    for (dx = 0; dx < data_num; ++dx) {
      genFeats(ens->tree(tx), tx, data, dx, 
               old_f_num, &aia_fx_dx); 
    }
  }

  /*---  load into the matrix  ---*/
  for (fx = old_f_num; fx < f_num; ++fx) {
    b_tran->load(fx, aia_fx_dx.point(fx-old_f_num)); 
  }  
}

/*------------------------------------------------------------------*/
void AzTrTreeFeat::genFeats(const AzTrTree_ReadOnly *dtree, 
                        int tx, 
                        const AzDataForTrTree *data, 
                        int dx, 
                        int fx_offs, 
                        /*---  output  ---*/
                        AzDataArray<AzIntArr> *aia_fx_dx)
const
{
  AzIntArr ia_nx; 
  dtree->apply(data, dx, &ia_nx); 

  int num; 
  const int *nx = ia_nx.point(&num); 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    int feat_no = (ip_featDef.point(tx))[nx[ix]]; 
    if (feat_no >= fx_offs) {     
      aia_fx_dx->point_u(feat_no-fx_offs)->put(dx); 
    }
  }
}

/* can be used for both frontier and non-frontier */
/*------------------------------------------------------------------*/
void AzTrTreeFeat::updateMatrix(const AzDataForTrTree *data, 
                                const AzTrTreeEnsemble_ReadOnly *ens, 
                                AzBmat *b_tran) /* inout */
                                const
{
  if (data == NULL) return; 

  const char *eyec = "AzTrTreeFeat::updateMatrix"; 
  int f_num = featNum(); 
  if (ens->size() != treeNum()) {
    throw new AzException(eyec, "size of tree ensemble and #feat should be the same"); 
  }
  int old_f_num = b_tran->colNum(); 
  if (old_f_num > f_num) {
    throw new AzException(eyec, "#col is bigger than #feat"); 
  }
  int fx; 
  for (fx = 0; fx < old_f_num; ++fx) {
    if (f_inf.point(fx)->isRemoved) {
      b_tran->clear(fx); 
    }
  }

  if (old_f_num == f_num) {
    if (old_f_num == 0) {
      /* for the rare case that no feature was generated */
      b_tran->reform(data->dataNum(), f_num);  /* added 9/13/2011 */
    }
    return; 
  }

  _updateMatrix(data, ens, old_f_num, b_tran); 
}

/*------------------------------------------------------------------*/
void AzTrTreeFeat::removeFeat(int removed_fx)
{
  AzTrTreeFeatInfo *fp = f_inf.point_u(removed_fx); 
  if (fp->isRemoved) {
    throw new AzException("AzTrTreeFeat::removeFeat", "want to remove a removed feature??"); 
  }
  fp->isRemoved = true; 
  if (doCountRules) {
    pool_rules_rmved.put(&fp->rule); 
  }
}
/*------------------------------------------------------------------*/
int AzTrTreeFeat::_update(const AzTrTree_ReadOnly *dtree, 
                      int tx, 
                      AzIntArr *ia_isActiveNode, /* output */
                      /*---  for updating old feature definition  ---*/
                      const int *prevDef, int prevDef_len, 
                      AzIntArr *ia_rmv_fx) 
{
  int added_feat_num = 0, rmv_num = 0; 
  dtree->isActiveNode(doAllowZeroWeightLeaf, ia_isActiveNode); 
  int node_num; 
  int *isActive = ia_isActiveNode->point_u(&node_num); 
  int nx; 
  for (nx = 0; nx < node_num; ++nx) {
    if (!isActive[nx]) {
      isActive[nx] = AzNone; 
      if (prevDef != NULL && 
          nx < prevDef_len && 
          prevDef[nx] >= 0) {
        /*---  this was a feature before, but not any more  ---*/
        /* dtree->setFeatNo(nx, AzNone); */
        removeFeat(prevDef[nx]);  
        if (ia_rmv_fx != NULL) {
          ia_rmv_fx->put(prevDef[nx]); 
        }
        ++rmv_num;  
      }
      continue; 
    }

    /*---  active node  ---*/
    if (prevDef != NULL && 
        nx < prevDef_len && 
        prevDef[nx] >= 0) {
      /*---  this is already defined  ---*/
      isActive[nx] = prevDef[nx];  /* set feat# as set before */
      continue; 
    }

    /*---  we need to assign new feat#  ---*/
    AzTreeRule rule; 
    dtree->getRule(nx, &rule); 

    int feat_no; 
    AzTrTreeFeatInfo *fp = f_inf.new_slot(&feat_no);  
    fp->tx = tx; 
    fp->nx = nx; 
    fp->rule.reset(rule.b