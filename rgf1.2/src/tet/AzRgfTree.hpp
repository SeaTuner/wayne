/* * * * *
 *  AzRgfTree.hpp 
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

#ifndef _AZ_RGF_TREE_HPP_
#define _AZ_RGF_TREE_HPP_

#include "AzUtil.hpp"
#include "AzTrTree.hpp"
#include "AzDataForTrTree.hpp"
#include "AzTrTtarget.hpp"
#include "AzRgf_FindSplit.hpp"
#include "AzParam.hpp"

class AzRgfTreeTemp {
public:
  AzFile *file;  
  int offs