/******************************************************************************
 * Copyright (c) 2016 The Khronos Group
 * Copyright (c) 2016 Valve Corporation
 * Copyright (c) 2016 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you man not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is destributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language govering permissions and
 * limitations under the License.
 *
 * Author: Lenny Komow <lenny@lunarg.com>
 *
 *****************************************************************************/

/* 
 * This program is used by the Vulkan Runtime Installer/Uninstaller to:
 * - Copy the most recent vulkan<majorabi>-*.dll in C:\Windows\System32
 *   to vulkan<majorabi>.dll
 * - Copy the most recent version of vulkaninfo-<abimajor>-*.exe in
 *   C:\Windows\System32 to vulkaninfo.exe
 * - The same thing is done for those files in C:\Windows\SysWOW64, but
 *   only on a 64-bit target
 * - Set the layer registry entried to point to the layer json files in
 *   the Vulkan SDK associated with the most recent vulkan*.dll
 *
 * The program must be called with the following parameters:
 *     --major-abi: A single number specifying the major abi version
 */

// Compile with: `cl.exe configure_runtime.c /link advapi32.lib`
// Be sure to use the x86 version of cl.exe

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

// This hack gets Visual Studio 2013 to handle C99 stuff properly
// If we drop support for 2013, it would be a good idea to remove this
#if _MSC_VER < 1900
#define inline __inline
#define snprintf _snprintf
#endif

#if defined(_WIN64)
#error "This program is designed only as a 32-bit program. It should not be built as 64-bit."
#endif

#define COPY_BUFFER_SIZE (1024)
#define CHECK_ERROR(statement) { int error = (statement); if(error) return error; }
#define CHECK_ERROR_HANDLED(statement, handler) { int error = (statement); if(error) { { handler } return error; } }
#define SDK_VERSION_BUFFER_SIZE (64)

enum Platform
{
    PLATFORM_X64,
    PLATFORM_X86,
};

#pragma pack(1)
struct SDKVersion
{
    long major;
    long minor;
    long patch;
    long build;
    char extended[SDK_VERSION_BUFFER_SIZE];
};

const char* FLAG_ABI_MAJOR = "--abi-major";
const char* FLAG_API_NAME = "--api-name";
const char* PATH_SYSTEM32 = "\\SYSTEM32\\";
const char* PATH_SYSWOW64 = "\\SysWOW64\\";

inline size_t max_s(size_t a, size_t b) { return a > b ? a : b; }
inline size_t min_s(size_t a, size_t b) { return a > b ? a : b; }

// Add the registry entries for all explicit layers
//
// log (input) - Logging file stream
// install_path (input) - The installation path of the SDK which provides the layers
// platform (input) - The platform to set the installation for (x64 or x86)
// api_name (input) - The api name to use when working with registries
// Returns: Zero on success, an error code on failure
int add_explicit_layers(FILE* log, const char* install_path, enum Platform platform, const char* api_name);

// Compare two sdk versions
//
// Returns: Zero if they are equal, below zero if a predates b, greater than zero if b predates a
int compare_versions(const struct SDKVersion* a, const struct SDKVersion* b);

// Locate all of the SDK installations
//
// api_name (input) - The api name to use when working with registries
// install_paths (output) - A poiner to an array of the installations paths
// install_versions (output) - A pointer to an array of the SDK versions
// count (output) - A pointer to the number of items in each array
// Returns: Zero on success, an error code on failure
//
// Both install_paths and install_versions are allocated on the heap. To free them properly,
// call free_installations(), even if this function returned an error code. The orders of
// install_paths and install_versions match, so (*install_paths)[2] is guaranteed to match
// (*install_versions)[2]
int find_installations(const char* api_name, char*** install_paths, struct SDKVersion** install_versions,
    size_t* count);

// Free the memory allocated by find_installations()
void free_installations(char** install_paths, struct SDKVersion* install_versions, size_t count);

// Parse command line arguments for the program
//
// log (input) - Logging file stream
// argc (input) - The argument count
// argv (input) - An array of argument strings
// abi_major (output) - The major abi version from the arguments
// api_name (output) - The api name to use when working with registries and system files
// Returns: Zero on success, an error code on failure
int parse_arguments(FILE* log, int argc, char** argv, long* abi_major, const char** api_name);

// Read the version from a string
//
// version_string (input) - A string in the format <abi>.<major>.<minor>.<patch>.<build>.<extended>
// version (output) - The version indicated by the input string
// Returns: Zero on success, an error code on failure
int read_version(const char* version_string, struct SDKVersion* version);

// Read the version from a filename
//
// filename (input) - The name of a .dll or .exe file, in the format
//     somename-<abi>-<major>-<minor>-<path>-<build>-<extended>.dll
// version (output) - The versions indicated by the input string
// Returns: Zero on success, an error code on failure
int read_version_from_filename(const char* filename, struct SDKVersion* version);

// Remove explicit layers from the Windows registry
//
// log (input) - Loggin file stream
// install_paths (input) - An array of every vulkan installation path
// count (input) - The number of vulkan installations
// platform (input) - The platform (x64 or x86) of the registry to use (both exist on x64)
// api_name (input) - The api name to use when working with registries
// Returns: Zero on success, an error code on failure
int remove_explicit_layers(FILE* log, const char** install_paths, size_t count, enum Platform platform,
    const char* api_name);

// Update all explicity layers in the windows registry
//
// log (input) - Logging file stream
// platform (input) - The platform of the OS (both registries will be modified if this is x64)
// version (input) - The version that should be set to current (if it exists)
// api_name (input) - The api name to use when working with registries
// Returns: Zero on success, an error code on failure
int update_registry_layers(FILE* log, enum Platform platform, const struct SDKVersion* version,
    const char* api_name);

// Update a single vulkan system file (vulkan.dll or vulkaninfo.exe)
//
// log (input) - Loggin file stream
// name (input) - The name (excuding file extension) of the file to be updated
// extension (input) - The file extensions of the file to be updated
// path (input) - The directory of the file (usually System32 or SysWOW64)
// abi_major (input) - The ABI major version to be updated
// append_abi_major (input) - Whether or not the ABI number should be appended to the filename
// latest_version (output) - The version of the runtime which the file was updated to
// Returns: Zero on success, an error code on failure
int update_system_file(FILE* log, const char* name, const char* extension, const char* path,
    long abi_major, bool append_abi_major, struct SDKVersion* latest_version);

// Update vulkan.dll and vulkaninfo.exe in all of the windows directories (System32 and SysWOW64)
//
// log (input) - Loging file stream
// abi_major (input) - The ABI major version of the files that should be used
// platform (input) - The platform for the current OS
// api_name (input) - The api name to use when working with system files
// latest_runtime_version (output) - The version that the runtime files were updated to
int update_windows_directories(FILE* log, long abi_major, enum Platform platform, const char* api_name,
    struct SDKVersion* latest_runtime_version);

int main(int argc, char** argv)
{    
    // Get the OS platform (x86 or x64)
    BOOL is_64_bit;
    IsWow64Process(GetCurrentProcess(), &is_64_bit);
    enum Platform platform = is_64_bit ? PLATFORM_X64 : PLATFORM_X86;

    FILE* log = fopen("configure_rt.log", "w");
    if(log == NULL) {
        return 10;
    }

    // Parse the arguments to get the abi version and the number of bits of the OS
    long abi_major;
    const char* api_name;
    CHECK_ERROR_HANDLED(parse_arguments(log, argc, argv, &abi_major, &api_name), { fclose(log); });
    
    fprintf(log, "API Name: %s\n", api_name);
    
    // This makes System32 and SysWOW64 not do any redirection (well, until 128-bit is a thing)
    PVOID useless;
    Wow64DisableWow64FsRedirection(&useless);
    
    // Update System32 (on all systems) and SysWOW64 on 64-bit system
    struct SDKVersion latest_runtime_version;
    CHECK_ERROR_HANDLED(update_windows_directories(log, abi_major, platform, api_name,
        &latest_runtime_version), { fclose(log); });

    // Update the explicit layers that are set in the windows registry
    CHECK_ERROR_HANDLED(update_registry_layers(log, platform, &latest_runtime_version, api_name),
        { fclose(log); });

    fclose(log);
    return 0;
}

int add_explicit_layers(FILE* log, const char* install_path, enum Platform platform, const char* api_name)
{
    switch(platform)
    {
    case PLATFORM_X64:
        fprintf(log, "Updating x64 explicit layers to path: %s\n", install_path);
        break;
    case PLATFORM_X86:
        fprintf(log, "Updating x86 explicit layers to path: %s\n", install_path);
        break;
    }
    
    const char* registry_pattern = "SOFTWARE\\Khronos\\%s\\ExplicitLayers";
    int registry_size = snprintf(NULL, 0, registry_pattern, api_name) + 1;
    char* registry_key = malloc(registry_size);
    snprintf(registry_key, registry_size, registry_pattern, api_name);

    // If this is a 32 bit system, we allow redirection to point this at the 32-bit registries.
    // If not, we add the flag KEY_WOW64_64KEY, to disable redirection for this node.
    HKEY hKey;
    REGSAM flags = KEY_ALL_ACCESS;
    if(platform == PLATFORM_X64) {
        flags |= KEY_WOW64_64KEY;
    }
    
    // Create (if needed) and open the explicit layer key
    if(RegCreateKeyEx(HKEY_LOCAL_MACHINE, registry_key, 0, NULL, REG_OPTION_NON_VOLATILE, flags,
         NULL, &hKey, NULL) != ERROR_SUCCESS) {
        free(registry_key);
        return 20;
    }

    const char* pattern = platform == PLATFORM_X64 ? "%s\\Bin\\VkLayer*.json" : "%s\\Bin32\\VkLayer*.json";
    int filter_size = snprintf(NULL, 0, pattern, install_path) + 1;
    if(filter_size < 0) {
        return 30;
    }
    char* filter = malloc(filter_size);
    snprintf(filter, filter_size, pattern, install_path);

    WIN32_FIND_DATA find_data;
    HANDLE find = FindFirstFile(filter, &find_data);
    free(filter);
    for(bool at_end = (find != INVALID_HANDLE_VALUE); at_end;
        at_end = FindNextFile(find, &find_data)) {
        
        const char* layer_pattern = platform == PLATFORM_X64 ? "%s\\Bin\\%s" : "%s\\Bin32\\%s";
        int layer_size = snprintf(NULL, 0, layer_pattern, install_path, find_data.cFileName) + 1;
        if(layer_size < 0) {
            free(registry_key);
            return 40;
        }
        char* layer = malloc(layer_size);
        snprintf(layer, layer_size, layer_pattern, install_path, find_data.cFileName);

        fprintf(log, "Adding explicit layer: %s\n", layer);

        DWORD zero = 0;
        LSTATUS err = RegSetValueEx(hKey, layer, zero, REG_DWORD, (BYTE*) &zero, sizeof(DWORD));
        free(layer);
        if(err != ERROR_SUCCESS) {
            free(registry_key);
            return 50;
        }
    }

    RegCloseKey(hKey);
    free(registry_key);
    return 0;
}

int compare_versions(const struct SDKVersion* a, const struct SDKVersion* b)
{
    // Compare numerical versions
    for(int i = 0; i < 4; ++i) {
        long* a_current = ((long*) a) + i;
        long* b_current = ((long*) b) + i;
        
        if(*a_current < *b_current) {
            return -4 + i;
        } else if(*b_current < *a_current) {
            return 4 - i;
        }
    }
    
    // An empty string should be considered greater (and therefore more recent) than one with test
    if(a->extended[0] == '\0' && b->extended[0] != '\0') {
        return 1;
    } else if(b->extended[0] == '\0' && a->extended[0] != '\0') {
        return -1;
    }

    // Otherwise, just do a strncmp
    return strncmp(a->extended, b->extended, SDK_VERSION_BUFFER_SIZE);
}

int find_installations(const char* api_name, char*** install_paths, struct SDKVersion** install_versions, size_t* count)
{
    *install_paths = malloc(sizeof(char*) * 64);
    *install_versions = malloc(sizeof(struct SDKVersion) * 64);
    *count = 0;

    // We want the 64-bit registries on 64-bit windows, and the 32-bit registries on 32-bit Windows.
    // KEY_WOW64_64KEY accomplishes this because it gets ignored on 32-bit Windows.
    HKEY hKey;
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        0, KEY_READ | KEY_WOW64_64KEY, &hKey) != ERROR_SUCCESS) {
        return 90;
    }
    
    size_t api_len = strlen(api_name);
    char* sdk_id = malloc(api_len + 4);
    strcpy(sdk_id, api_name);
    strcpy(sdk_id + api_len, "SDK");
    
    DWORD keyCount, keyLen;
    RegQueryInfoKey(hKey, NULL, NULL, NULL, &keyCount, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    for(int i = 0; i < keyCount; ++i) {
        TCHAR name[COPY_BUFFER_SIZE];
        DWORD nameSize = COPY_BUFFER_SIZE;
        RegEnumKeyEx(hKey, i, name, &nameSize, NULL, NULL, NULL, NULL);
        
        if(strncmp(sdk_id, name, api_len + 3)) {
            continue;
        }
        
        HKEY subKey;
        if(RegOpenKeyEx(hKey, name, 0, KEY_READ | KEY_WOW64_64KEY, &subKey) != ERROR_SUCCESS) {
            continue;
        }
        
        bool found_installation = false, found_version = false;
        DWORD valueCount;
        RegQueryInfoKey(subKey, NULL, NULL, NULL, NULL, NULL, NULL, &valueCount, NULL, NULL, NULL, NULL);
        for(int j = 0; j < valueCount; ++j) {

            TCHAR name[COPY_BUFFER_SIZE], value[COPY_BUFFER_SIZE];
            DWORD type, nameSize = COPY_BUFFER_SIZE, valSize = COPY_BUFFER_SIZE;
            LSTATUS ret = RegEnumValue(subKey, j, name, &nameSize, NULL, &type, value, &valSize);
            if(type == REG_SZ && !strcmp("InstallDir", name)) {
                *install_paths = realloc(*install_paths, sizeof(char*) * ((*count) + 1));
                (*install_paths)[*count] = malloc(sizeof(char) * COPY_BUFFER_SIZE);
                strcpy((*install_paths)[*count], value);
                found_installation = true;
            } else if(type == REG_SZ && !strcmp("DisplayVersion", name)) {
                *install_versions = realloc(*install_versions, sizeof(struct SDKVersion) * ((*count) + 1));
                CHECK_ERROR(read_version(value, (*install_versions) + *count));
                found_version = true;
            }
            
            if(found_installation && found_version) {
                ++(*count);
                break;
            }
        }
        RegCloseKey(subKey);
        
        if(!(found_installation && found_version)) {
            RegCloseKey(hKey);
            free(sdk_id);
            return 100;
        }
    }
    RegCloseKey(hKey);
    free(sdk_id);
    
    return 0;
}

void free_installations(char** install_paths, struct SDKVersion* install_versions, size_t count)
{
    for(size_t i = 0; i < count; ++i) {
        free(install_paths[i]);
    }
    free(install_paths);
    free(install_versions);
}

int parse_arguments(FILE* log, int argc, char** argv, long* abi_major, const char** api_name)
{
    *abi_major = 0;
    *api_name = "Vulkan";

    // Parse arguments
    for(int i = 0; i < argc; ++i) {
        if(!strcmp(argv[i], FLAG_ABI_MAJOR)) {
            if(i + 1 == argc) {
                fprintf(log, "ERROR: No value given for flag %s.\n", FLAG_ABI_MAJOR);
                return 110;
            }
            *abi_major = strtol(argv[++i], NULL, 10);
            if(*abi_major == 0) {
                fprintf(log, "ERROR: Unable to parse ABI major version as integer.\n");
                return 120;
            }
        }
        if(!strcmp(argv[i], FLAG_API_NAME)) {
            if(i + 1 == argc) {
                fprintf(log, "ERROR: No value given for flag %s.\n", FLAG_API_NAME);
                return 124;
            }
            *api_name = argv[++i];
        }
    }
    
    // Check that we have everything we need
    if(*abi_major == 0 ) {
        fprintf(log, "ERROR: Flag %s must be provided.\n", FLAG_ABI_MAJOR);
        return 130;
    }

    // It all worked fine
    fprintf(log, "Found ABI: %ld\n\n", *abi_major);
    return 0;
}

int read_version(const char* version_string, struct SDKVersion* version)
{    
    size_t borders[4], dot_count = 0, i;
    for(i = 0; dot_count < 3 && version_string[i] != '\0'; ++i) {
        if(version_string[i] == '.') {
            borders[dot_count++] = i + 1;
        }
    }
    borders[3] = i + 1;
    
    if(dot_count < 3) {
        return 140;
    }
    
    // Read the version number
    version->major = strtol(version_string,              NULL, 10);
    version->minor = strtol(version_string + borders[0], NULL, 10);
    version->patch = strtol(version_string + borders[1], NULL, 10);
    version->build = strtol(version_string + borders[2], NULL, 10);

    strncpy(version->extended, version_string + borders[3] + 1,
        min_s(SDK_VERSION_BUFFER_SIZE - 1, strlen(version_string + borders[3] + 1)));
    
    return 0;
}

int read_version_from_filename(const char* filename, struct SDKVersion* version)
{
    size_t borders[5], dash_count = 0;

    // Locate all of the dashes that divides different version numbers
    size_t i;
    for(i = 0; dash_count < 5; ++i) {
        if(filename[i] == '-' && dash_count == 0) {
            ++dash_count;
        } else if(filename[i] == '-') {
            borders[dash_count++ - 1] = i + 1;
        } else if(filename[i] == '\0') {
            return 150;
        }
    }
    borders[4] = i + 1;
    
    // Read the version number
    version->major = strtol(filename + borders[0], NULL, 10);
    version->minor = strtol(filename + borders[1], NULL, 10);
    version->patch = strtol(filename + borders[2], NULL, 10);
    version->build = strtol(filename + borders[3], NULL, 10);

    if(strcmp(filename + borders[4] + 1, "dll") && strcmp(filename + borders[4] + 1, "exe")) {
        strncpy(version->extended, filename + borders[4] + 1, SDK_VERSION_BUFFER_SIZE - 1);
        size_t file_len = strlen(filename + borders[4] + 1);
        if(file_len - 4 < SDK_VERSION_BUFFER_SIZE) {
            version->extended[file_len - 4] = '\0';
        }
    } else {
        version->extended[0] = '\0';
    }

    for(size_t i = 0; version->extended[i] != '\0' && i < SDK_VERSION_BUFFER_SIZE; ++i) {
        if(version->extended[i] == '-') {
            version->extended[i] = '.';
        }
    }
    
    return 0;
}

int remove_explicit_layers(FILE* log, const char** install_paths, size_t count, enum Platform platform,
    const char* api_name)
{
    switch(platform)
    {
    case PLATFORM_X64:
        fprintf(log, "Removing x64 explicit layers from registry\n");
        break;
    case PLATFORM_X86:
        fprintf(log, "Removing x86 explicit layers from registry\n");
        break;
    }
    
    const char* pattern = "SOFTWARE\\Khronos\\%s\\ExplicitLayers";
    int registry_size = snprintf(NULL, 0, pattern, api_name) + 1;
    char* registry_key = malloc(registry_size);
    snprintf(registry_key, registry_size, pattern, api_name);

    bool removed_one;
    do {
        // If this is a 32 bit system, we allow redirection to point this at the 32-bit registries.
        // If not, we add the flag KEY_WOW64_64KEY, to disable redirection for this node.
        HKEY hKey;
        REGSAM flags = KEY_ALL_ACCESS;
        if(platform == PLATFORM_X64) {
            flags |= KEY_WOW64_64KEY;
        }

        // Create (if needed) and open the explicit layer key
        if(RegCreateKeyEx(HKEY_LOCAL_MACHINE, registry_key, 0, NULL, REG_OPTION_NON_VOLATILE, flags,
            NULL, &hKey, NULL) != ERROR_SUCCESS) {
            free(registry_key);
            return 160;
        }

        removed_one = false;
        DWORD valueCount;
        RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &valueCount, NULL, NULL, NULL, NULL);
        for(DWORD i = 0; i < valueCount; ++i) {
            TCHAR name[COPY_BUFFER_SIZE];
            DWORD type, buffSize = COPY_BUFFER_SIZE;
            RegEnumValue(hKey, i, name, &buffSize, NULL, &type, NULL, NULL);

            for(size_t j = 0; j < count; ++j) {
                if(strncmp(install_paths[j], name, strlen(install_paths[j])) == 0) {
                    fprintf(log, "Removing explicit layer entry: %s\n", name);
                    LSTATUS err = RegDeleteValue(hKey, name);
                    if(err != ERROR_SUCCESS) {
                        free(registry_key);
                        return 170;
                    }
                    removed_one = true;
                    break;
                }
            }
            if(removed_one) {
                break;
            }
        }

        RegCloseKey(hKey);
    } while(removed_one);
    
    free(registry_key);
    return 0;
}

int update_registry_layers(FILE* log, enum Platform platform, const struct SDKVersion* version,
    const char* api_name)
{
    char** install_paths;
    struct SDKVersion* install_versions;
    size_t count;
    CHECK_ERROR_HANDLED(find_installations(api_name, &install_paths, &install_versions, &count),
        { free_installations(install_paths, install_versions, count); });
    for(size_t i = 0; i < count; ++i) {
        fprintf(log, "Found installation of %ld.%ld.%ld.%ld in: %s\n", install_versions[i].major,
            install_versions[i].minor, install_versions[i].patch, install_versions[i].build, install_paths[i]);
    }
    fprintf(log, "\n");
    if(platform == PLATFORM_X64) {
        CHECK_ERROR_HANDLED(remove_explicit_layers(log, install_paths, count, PLATFORM_X64, api_name),
            { free_installations(install_paths, install_versions, count); });
        fprintf(log, "\n");
    }
    CHECK_ERROR_HANDLED(remove_explicit_layers(log, install_paths, count, PLATFORM_X86, api_name),
        { free_installations(install_paths, install_versions, count); });
    fprintf(log, "\n");

    if(version->major == 0 && version->minor == 0 && version->patch == 0 && version->build == 0) {
        free_installations(install_paths, install_versions, count);
        return 0;
    }

    size_t preferred_sdk = count;
    for(size_t i = 0; i < count; ++i) {
        int cmp = compare_versions(install_versions + i, version);
        if(cmp <= 0 && cmp >= -2) {
            if(preferred_sdk == count ||
                (compare_versions(install_versions + i, install_versions + preferred_sdk) > 0)) {
                preferred_sdk = i;
            }
        }
    }
    
    if(preferred_sdk < count) {
        if(platform == PLATFORM_X64) {
            CHECK_ERROR_HANDLED(add_explicit_layers(log, install_paths[preferred_sdk], PLATFORM_X64, api_name),
                { free_installations(install_paths, install_versions, count); });
            fprintf(log, "\n");
        }
        CHECK_ERROR_HANDLED(add_explicit_layers(log, install_paths[preferred_sdk], PLATFORM_X86, api_name),
            { free_installations(install_paths, install_versions, count); });
    }
    
    free_installations(install_paths, install_versions, count);
    return 0;
}

//int update_system_file(FILE* log, const char* name, const char* extension, const char* path,
//    long abi_major, bool append_abi_major, struct SDKVersion* latest_version)
int update_system_file(FILE* log, const char* name, const char* extension, const char* path,
    long abi_major, bool leave_abi_major, struct SDKVersion* latest_version)
{
    // Generate the filter string
    const char* pattern = "%s%s-%ld-*-*-*-*%s";
    int filter_size = snprintf(NULL, 0, pattern, path, name, abi_major, extension) + 1;
    if(filter_size < 0) {
        return 180;
    }
    char* filter = malloc(filter_size);
    snprintf(filter, filter_size, pattern, path, name, abi_major, extension);
    
    // Find all of the files that match the pattern
    char* latest_filename = malloc(64);
    memset(latest_version, 0, sizeof(struct SDKVersion));
    WIN32_FIND_DATA find_data;
    HANDLE find = FindFirstFile(filter, &find_data);
    free(filter);
    for(bool at_end = (find != INVALID_HANDLE_VALUE); at_end;
        at_end = FindNextFile(find, &find_data)) {
        
        struct SDKVersion version;
        CHECK_ERROR_HANDLED(read_version_from_filename(find_data.cFileName, &version), { free(latest_filename); });
        
        // Decide if this is the latest file
        if(compare_versions(latest_version, &version) < 0) {
            *latest_version = version;
            const char* latestPattern = "%s%s";
            int size = snprintf(NULL, 0, latestPattern, path, find_data.cFileName) + 1;
            if(size < 0) {
                free(latest_filename);
                return 200;
            }
            latest_filename = realloc(latest_filename, size);
            snprintf(latest_filename, size, latestPattern, path, find_data.cFileName);
        }
    }
    FindClose(find);
    
    // Make sure something was found
    if(latest_version->major == 0 && latest_version->minor == 0 && latest_version->patch == 0 &&
        latest_version->build == 0) {
        fprintf(log, "Didn't find any version of %s%s\n", name, extension);
        return 0;
    }
    
    fprintf(log, "Found latest version of %s%s: %ld.%ld.%ld.%ld\n", name, extension, latest_version->major,
        latest_version->minor, latest_version->patch, latest_version->build);
        
    // Generate output filename
    char* output_filename;
    if(leave_abi_major) {
        const char* outPattern = "%s%s-%ld%s";
        int out_size = snprintf(NULL, 0, outPattern, path, name, abi_major, extension) + 1;
        if(out_size < 0) {
            free(latest_filename);
            return 205;
        }
        output_filename = malloc(out_size);
        snprintf(output_filename, out_size, outPattern, path, name, abi_major, extension);
    } else {
        const char* outPattern = "%s%s%s";
        int out_size = snprintf(NULL, 0, outPattern, path, name, extension) + 1;
        if(out_size < 0) {
            free(latest_filename);
            return 210;
        }
        output_filename = malloc(out_size);
        snprintf(output_filename, out_size, outPattern, path, name, extension);
    }
    
    // Remove any older version of the output file
    if(remove(output_filename) == 0) {
        fprintf(log, "Removed file %s\n", output_filename);
    } else {
        fprintf(log, "Did not remove file %s\n", output_filename);
    }
    
    fprintf(log, "Attempting to copy file %s to %s\n", latest_filename, output_filename);
    if(CopyFile(latest_filename, output_filename, false) == 0) {
        free(latest_filename);
        free(output_filename);
        return 215;
    }
    
    free(latest_filename);
    free(output_filename);
    return 0;
}

int update_windows_directories(FILE* log, long abi_major, enum Platform platform, const char* api_name,
    struct SDKVersion* latest_runtime_version)
{
    size_t api_len = strlen(api_name);
    char* vulkan_name = malloc(api_len + 1);
    char* vulkan_info_name = malloc(api_len + 5);
    for(size_t i = 0; i < api_len; ++i) {
        vulkan_name[i] = tolower(api_name[i]);
    }
    vulkan_name[api_len] = '\0';
    strcpy(vulkan_info_name, vulkan_name);
    strcpy(vulkan_info_name + api_len, "info");
    
    struct SDKVersion version;
    unsigned windows_path_size = GetWindowsDirectory(NULL, 0); // Size includes null terminator
    char* system_path = malloc(windows_path_size +
        max_s(strlen(PATH_SYSTEM32), strlen(PATH_SYSWOW64)));
    GetWindowsDirectory(system_path, windows_path_size);

    strcpy(system_path + windows_path_size - 1, PATH_SYSTEM32);
    fprintf(log, "Updating system directory: %s\n", system_path);
    CHECK_ERROR_HANDLED(update_system_file(log, vulkan_name, ".dll", system_path, abi_major, true,
        latest_runtime_version), { free(system_path); });
    CHECK_ERROR_HANDLED(update_system_file(log, vulkan_info_name, ".exe", system_path, abi_major, false,
        &version), { free(system_path); free(vulkan_info_name); free(vulkan_name);});
    if(compare_versions(latest_runtime_version, &version) != 0) {
        free(system_path);
        free(vulkan_info_name);
        free(vulkan_name);
        return 220;
    }

    if(platform == PLATFORM_X64) {
        strcpy(system_path + windows_path_size - 1, PATH_SYSWOW64);
        fprintf(log, "\nUpdating system directory: %s\n", system_path);
        CHECK_ERROR_HANDLED(update_system_file(log, vulkan_name, ".dll", system_path, abi_major,
            true, &version), { free(system_path); free(vulkan_info_name); free(vulkan_name); });
        if(compare_versions(latest_runtime_version, &version) != 0) {
            free(system_path);
            free(vulkan_info_name);
            free(vulkan_name);
            return 230;
        }
        CHECK_ERROR_HANDLED(update_system_file(log, vulkan_info_name, ".exe", system_path, abi_major,
            false, &version), { free(system_path); free(vulkan_info_name); free(vulkan_name); });
        if(compare_versions(latest_runtime_version, &version) != 0) {
            free(system_path);
            free(vulkan_info_name);
            free(vulkan_name);
            return 240;
        }
    }

    free(system_path);
    free(vulkan_info_name);
    free(vulkan_name);
    fprintf(log, "\nUpdate of system directories succeeded.\n\n");
    return 0;
}
