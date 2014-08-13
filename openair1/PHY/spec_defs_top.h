/*******************************************************************************
    OpenAirInterface 
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is 
   included in this distribution in the file called "COPYING". If not, 
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr
  
  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
#ifndef __PHY_SPEC_DEFS_TOP_H__
#define __PHY_SPEC_DEFS_TOP_H__

#include "types.h"



/*! \brief Extension Type */
typedef enum {
   CYCLIC_PREFIX,
   CYCLIC_SUFFIX,
   ZEROS,
   NONE
 } Extension_t;


/// mod_sym_t is the type of txdataF
#ifdef IFFT_FPGA
typedef unsigned char mod_sym_t; 
#else
typedef int mod_sym_t;
#endif //IFFT_FPGA


#endif /*__PHY_SPEC_DEFS_TOP_H__ */ 















