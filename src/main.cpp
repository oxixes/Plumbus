/*  Copyright 2022 Pretendo Network contributors <pretendo.network>
    Copyright 2026 Oxixes <oxixes>
    Copyright 2022 Ash Logan <ash@heyquark.com>
    Copyright 2019 Maschell

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "export.h"
#include "iosu_url_patches.h"
#include "config.h"
#include "Notification.h"
#include "patches/olv_urls.h"
#include "patches/game_matchmaking.h"
#include "plumbus_config.h"

#include <wums.h>

#include <coreinit/dynload.h>
#include <coreinit/mcp.h>

#include <notifications/notifications.h>
#include <utils/logger.h>

#include <string>
#include <optional>

#include <cstring>
#include <cstdint>

#include "ca_pem.h"

#define PLUMBUS_VERSION "v3.0.0"

/**
    Mandatory plugin information.
    If not set correctly, the loader will refuse to use the plugin.
**/
WUMS_MODULE_EXPORT_NAME(PROJECT_MODULE_NAME);
WUMS_MODULE_DESCRIPTION("Custom Network Patcher");
WUMS_MODULE_VERSION(PLUMBUS_VERSION);
WUMS_MODULE_AUTHOR("Pretendo contributors & Oxixes");
WUMS_MODULE_LICENSE("GPLv3");

WUMS_DEPENDS_ON(homebrew_functionpatcher);
WUMS_DEPENDS_ON(homebrew_kernel);
WUMS_DEPENDS_ON(homebrew_notifications);

WUMS_USE_WUT_DEVOPTAB();

#include <mocha/mocha.h>
#include <function_patcher/function_patching.h>
#include "patches/account_settings.h"
#include "patches/dns_hooks.h"
#include "patches/eshop_applet.h"
#include "patches/olv_applet.h"
#include "patches/game_peertopeer.h"
#include "sysconfig.h"
#include "lang.h"

//thanks @Gary#4139 :p
static void write_string(uint32_t addr, const char *str) {
    int len = strlen(str) + 1;
    int remaining = len % 4;
    int num = len - remaining;

    for (int i = 0; i < (num / 4); i++) {
        Mocha_IOSUKernelWrite32(addr + i * 4, *(uint32_t * )(str + i * 4));
    }

    if (remaining > 0) {
        uint8_t buf[4];
        Mocha_IOSUKernelRead32(addr + num, (uint32_t * ) & buf);

        for (int i = 0; i < remaining; i++) {
            buf[i] = *(str + num + i);
        }

        Mocha_IOSUKernelWrite32(addr + num, *(uint32_t * ) & buf);
    }
}

static bool is555(MCPSystemVersion version) {
    return (version.major == 5) && (version.minor == 5) && (version.patch >= 5);
}

static const char *get_nintendo_network_message() {
    // TL note: "Nintendo Network" is a proper noun - "Network" is part of the name
    // TL note: "Using" instead of "Connected" is deliberate - we don't know if a successful connection exists, we are
    // only specifying what we'll *attempt* to connect to
    return get_config_strings(get_system_language()).using_nintendo_network.data();
}

static const char *get_custom_message() {
    // TL note: "Custom Network" is a proper noun in this context.
    // TL note: "Using" instead of "Connected" is deliberate - we don't know if a successful connection exists, we are
    // only specifying what we'll *attempt* to connect to
    return get_config_strings(get_system_language()).using_custom_network.data();
}

static void Plumbus_SetPluginRunning() {
    Config::plugin_is_loaded = true;
}

static PlumbusStatus Plumbus_GetStatus() {
    if (!Config::initialized)
        return PlumbusStatus::Uninitialized;

    if (Config::connect_to_network) {
        return PlumbusStatus::Custom;
    } else {
        return PlumbusStatus::Nintendo;
    }
}

static void Plumbus_Initialize(bool apply_patches) {
    if (Config::initialized)
        return;

    if (Config::block_initialize) {
        ShowNotification("Cannot load " PROJECT_DISPLAY_NAME " while the system is running. Please restart the console");
        return;
    }

    // if using custom network then (try to) apply the ssl patches
    if (apply_patches) {
        Config::connect_to_network = true;

        if (is555(get_console_os_version())) {
            Mocha_IOSUKernelWrite32(0xE1019F78, 0xE3A00001); // mov r0, #1
        } else {
            Mocha_IOSUKernelWrite32(0xE1019E84, 0xE3A00001); // mov r0, #1
        }

        for (const auto &patch: url_patches) {
            write_string(patch.address, patch.url);
        }

        // IOS-NIM-BOSS GlobalPolicyList->state: poking this forces a refresh after we changed the url
        Mocha_IOSUKernelWrite32(0xE24B3D90, 4);

        DEBUG_FUNCTION_LINE_VERBOSE("Custom URL and NoSSL patches applied successfully.");

        ShowNotification(get_custom_message());
        Config::initialized = true;
    } else {
        DEBUG_FUNCTION_LINE_VERBOSE("Custom URL and NoSSL patches skipped.");

        ShowNotification(get_nintendo_network_message());
        Config::initialized = true;
        return;
    }

    if (FunctionPatcher_InitLibrary() == FUNCTION_PATCHER_RESULT_SUCCESS) {
        patchDNS();
        patchEshop();
        patchOlvApplet();
        patchAccountSettings();
        install_matchmaking_patches();
    } else {
        DEBUG_FUNCTION_LINE("FunctionPatcher_InitLibrary failed");
    }
}

WUMS_INITIALIZE() {
    WHBLogCafeInit();
    WHBLogUdpInit();

    if (const auto res = Mocha_InitLibrary(); res != MOCHA_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE("Mocha init failed with code %d!", res);
        return;
    }

    if (NotificationModule_InitLibrary() != NOTIFICATION_MODULE_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE("NotificationModule_InitLibrary failed");
    }
}

WUMS_DEINITIALIZE() {
    unpatchDNS();
    unpatchEshop();
    unpatchOlvApplet();
    unpatchAccountSettings();
    remove_matchmaking_patches();

    Mocha_DeInitLibrary();
    NotificationModule_DeInitLibrary();
    FunctionPatcher_DeInitLibrary();

    WHBLogCafeDeinit();
    WHBLogUdpDeinit();
}

WUMS_APPLICATION_STARTS() {
    DEBUG_FUNCTION_LINE_VERBOSE(PROJECT_DISPLAY_NAME " " PLUMBUS_VERSION " starting up...\n");

    // Reset plugin loaded flag
    Config::plugin_is_loaded = false;
}

WUMS_ALL_APPLICATION_STARTS_DONE() {
    // we need to do the patches here because otherwise the Config::connect_to_network flag might be set yet
    setup_olv_libs();
    peertopeer_patch();
    matchmaking_notify_titleswitch();
    hotpatchAccountSettings();

    if (Config::initialized && !Config::plugin_is_loaded) {
        DEBUG_FUNCTION_LINE(PROJECT_DISPLAY_NAME " is running but the plugin got unloaded");
        if (!Config::block_initialize) {
            ShowNotification(PROJECT_DISPLAY_NAME " module is still running. Please restart the console");
        }
        Config::shown_warning = true;
    } else if (!Config::initialized && !Config::shown_warning) {
        DEBUG_FUNCTION_LINE(PROJECT_DISPLAY_NAME " module not initialized");
        ShowNotification(PROJECT_DISPLAY_NAME " module was not initialized. Ensure you have the " PROJECT_DISPLAY_NAME " plugin loaded");
        Config::shown_warning = true;
    }
    if (!Config::initialized) {
        Config::block_initialize = true;
    }
}

WUMS_APPLICATION_ENDS() {
}

WUMS_EXPORT_FUNCTION(Plumbus_Initialize);
WUMS_EXPORT_FUNCTION(Plumbus_GetStatus);
WUMS_EXPORT_FUNCTION(Plumbus_SetPluginRunning);
