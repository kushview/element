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
