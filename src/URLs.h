/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#if EL_USE_LOCAL_AUTH
 #define EL_URL_HOME                    "http://local.kushview.net"
 #define EL_URL_AUTH                    "http://local.kushview.net/edd-cp"
#else
 #define EL_URL_HOME                    "https://kushview.net"
 #define EL_URL_AUTH                    "https://kushview.net/authorize"
#endif

#define EL_URL_HELP_HOME                "https://help.kushview.net"
#define EL_URL_HELP_ELEMENT             EL_URL_HELP_HOME "/collection/10-element"
#define EL_URL_HELP_LICENSE_MANAGEMENT  EL_URL_HELP_HOME "/article/35-license-management"
#define EL_URL_HELP_ACTIVATION          EL_URL_HELP_HOME "/article/61-online-activation"

#define EL_URL_MY_ACCOUNT               EL_URL_HOME "/account"
#define EL_URL_MY_LICENSES              EL_URL_HOME "/account/licenses"
#define EL_URL_ELEMENT_PURCHASE         EL_URL_HOME "/element/purchase"
#define EL_URL_ELEMENT_GET_TRIAL        EL_URL_HOME "/element/trial"

#define EL_URL_LICENSE_UPGRADE          EL_URL_HOME "/checkout/?edd_action=sl_license_upgrade"
#define EL_URL_LICENSE_UPGRADES         EL_URL_HOME "/account/purchases/?view=upgrades&license_id=LICENSE_ID&action=manage_licenses&payment_id=PAYMENT_ID"
#define EL_URL_TRIAL_ACTIVATION         EL_URL_HOME "/edd-cp/?kv_action=activate_trial"
