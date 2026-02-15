/*  Copyright 2024 Pretendo Network contributors <pretendo.network>
    Copyright 2026 Oxixes <oxixes>

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

#include "module.h"

#include "Notification.h"
#include "utils/logger.h"
#include "sysconfig.h"
#include "lang.h"
#include "plumbus_config.h"

#include <coreinit/dynload.h>

static OSDynLoad_Module module;
static void (*moduleInitialize)(bool) = nullptr;
static PlumbusStatus (*moduleGetStatus)() = nullptr;
static void (*moduleSetPluginRunning)() = nullptr;

static const char *get_module_not_found_message() {
    return get_config_strings(get_system_language()).module_not_found.data();
}

static const char *get_module_init_not_found_message() {
    return get_config_strings(get_system_language()).module_init_not_found.data();
}

void Plumbus_Initialize(bool apply_patches) {
    if (module) {
        return;
    }

    if (OSDynLoad_Acquire(PROJECT_MODULE_NAME, &module) != OS_DYNLOAD_OK) {
        DEBUG_FUNCTION_LINE("Failed to acquire module");
        ShowNotification(get_module_not_found_message());
        return;
    }

    if (OSDynLoad_FindExport(module, OS_DYNLOAD_EXPORT_FUNC, "Plumbus_Initialize", reinterpret_cast<void * *>(&moduleInitialize)) != OS_DYNLOAD_OK) {
        DEBUG_FUNCTION_LINE("Failed to find initialization function");
        ShowNotification(get_module_init_not_found_message());
        OSDynLoad_Release(module);
        return;
    }

    moduleInitialize(apply_patches);
}

void Plumbus_Finalize() {
    if (module) {
        OSDynLoad_Release(module);
        moduleInitialize = nullptr;
        moduleGetStatus = nullptr;
        moduleSetPluginRunning = nullptr;
    }
}

PlumbusStatus Plumbus_GetStatus() {
    if (!module) {
        return PlumbusStatus::Error;
    }

    if (!moduleGetStatus && OSDynLoad_FindExport(module, OS_DYNLOAD_EXPORT_FUNC, "Plumbus_GetStatus", reinterpret_cast<void * *>(&moduleGetStatus)) != OS_DYNLOAD_OK) {
        DEBUG_FUNCTION_LINE("Failed to find status function");
        return PlumbusStatus::Error;
    }

    return moduleGetStatus();
}

void Plumbus_SetPluginRunning() {
    if (!module) {
        return;
    }

    if (!moduleSetPluginRunning && OSDynLoad_FindExport(module, OS_DYNLOAD_EXPORT_FUNC, "Plumbus_SetPluginRunning", reinterpret_cast<void * *>(&moduleSetPluginRunning)) != OS_DYNLOAD_OK) {
        DEBUG_FUNCTION_LINE("Failed to find \"Plumbus_SetPluginRunning\" function");
        return;
    }

    moduleSetPluginRunning();
}
