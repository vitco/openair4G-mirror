/*******************************************************************************

  Eurecom OpenAirInterface
  Copyright(c) 1999 - 2012 Eurecom

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information
  Openair Admin: openair_admin@eurecom.fr
  Openair Tech : openair_tech@eurecom.fr
  Forums       : http://forums.eurecom.fr/openairinterface
  Address      : EURECOM, Campus SophiaTech, 450 Route des Chappes
                 06410 Biot FRANCE

*******************************************************************************/

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "log.h"

/* mme log */
int log_enabled = 0;

int log_init(const mme_config_t *mme_config_p,
             log_specific_init_t specific_init)
{
    if (mme_config_p->verbosity_level == 1) {
        log_enabled = 1;
    } else if (mme_config_p->verbosity_level == 2) {
        log_enabled = 1;
    } else {
        log_enabled = 0;
    }
    return specific_init(mme_config_p->verbosity_level);
}