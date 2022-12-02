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
                            