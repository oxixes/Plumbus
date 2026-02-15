//
// Created by ash on 17/12/24.
//

#ifndef PLUMBUS_CONFIG_H
#define PLUMBUS_CONFIG_H

#ifdef __has_include
    #if __has_include("plumbus_config.local.h")
        #include "plumbus_config.local.h"
        #define PLUMBUS_CUSTOM 1
    #elif __has_include("inkay_config.local.h")
        #include "inkay_config.local.h"
        #define PLUMBUS_CUSTOM 1
    #endif
#endif

#ifndef PROJECT_DISPLAY_NAME
    #define PROJECT_DISPLAY_NAME "Plumbus"
#endif

#ifndef PROJECT_MODULE_NAME
    #define PROJECT_MODULE_NAME "plumbus"
#endif

#ifndef REPLACEMENT_DOMAIN
    #define REPLACEMENT_DOMAIN "change.me"
#endif

#define NETWORK_BASEURL REPLACEMENT_DOMAIN
#define MAX_REPLACEMENT_DOMAIN_LEN (sizeof("nintendo.net") - 1)
#define REPLACEMENT_DOMAIN_LEN (sizeof(NETWORK_BASEURL) - 1)

#if defined(__cplusplus)
static_assert(REPLACEMENT_DOMAIN_LEN > 0,
              "REPLACEMENT_DOMAIN must not be empty.");
static_assert(REPLACEMENT_DOMAIN_LEN <= MAX_REPLACEMENT_DOMAIN_LEN,
              "REPLACEMENT_DOMAIN too long. Max length is 12 (nintendo.net).");
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
_Static_assert(REPLACEMENT_DOMAIN_LEN > 0,
               "REPLACEMENT_DOMAIN must not be empty.");
_Static_assert(REPLACEMENT_DOMAIN_LEN <= MAX_REPLACEMENT_DOMAIN_LEN,
               "REPLACEMENT_DOMAIN too long. Max length is 12 (nintendo.net).");
#endif

#endif //PLUMBUS_CONFIG_H
