#include "rcedit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

enum {
    OPTION_OK = 0,
    OPTION_ERROR = 1,
    OPTION_CHANGED = 2,
};

typedef int (*OptionHandler)(ResourceUpdater* ru, wchar_t* args[]);

typedef struct {
    const wchar_t* name;
    const wchar_t* alias;
    int argCount;
    const wchar_t* usage;
    OptionHandler handler;
} OptionSpec;

static int is_help(const wchar_t* arg) { return wcscmp(arg, L"-h") == 0 || wcscmp(arg, L"--help") == 0; }

static int parse_uint(const wchar_t* value, unsigned int* result) {
    if (!value || !*value || value[0] == L'-')
        return 0;

    wchar_t* end = NULL;
    unsigned long parsed = wcstoul(value, &end, 10);
    if (end == value || *end != L'\0')
        return 0;

    *result = (unsigned int)parsed;
    return 1;
}

static int handle_set_file_version(ResourceUpdater* ru, wchar_t* args[]) {
    unsigned short v1, v2, v3, v4;
    if (!parse_version_string(args[0], &v1, &v2, &v3, &v4)) {
        fwprintf(stderr, L"Unable to parse version string for FileVersion\n");
        return OPTION_ERROR;
    }
    if (!ru_set_file_version(ru, v1, v2, v3, v4)) {
        fwprintf(stderr, L"Unable to change file version\n");
        return OPTION_ERROR;
    }
    if (!ru_set_version_string(ru, L"FileVersion", args[0])) {
        fwprintf(stderr, L"Unable to change FileVersion string\n");
        return OPTION_ERROR;
    }
    return OPTION_CHANGED;
}

static int handle_set_product_version(ResourceUpdater* ru, wchar_t* args[]) {
    unsigned short v1, v2, v3, v4;
    if (!parse_version_string(args[0], &v1, &v2, &v3, &v4)) {
        fwprintf(stderr, L"Unable to parse version string for ProductVersion\n");
        return OPTION_ERROR;
    }
    if (!ru_set_product_version(ru, v1, v2, v3, v4)) {
        fwprintf(stderr, L"Unable to change product version\n");
        return OPTION_ERROR;
    }
    if (!ru_set_version_string(ru, L"ProductVersion", args[0])) {
        fwprintf(stderr, L"Unable to change ProductVersion string\n");
        return OPTION_ERROR;
    }
    return OPTION_CHANGED;
}

static int handle_set_version_string(ResourceUpdater* ru, wchar_t* args[]) {
    const wchar_t* key = args[0];
    const wchar_t* value = args[1];

    unsigned short v1, v2, v3, v4;
    if (parse_version_string(value, &v1, &v2, &v3, &v4)) {
        if (wcscmp(key, L"FileVersion") == 0) {
            if (!ru_set_file_version(ru, v1, v2, v3, v4)) {
                fwprintf(stderr, L"Unable to change file version\n");
                return OPTION_ERROR;
            }
        } else if (wcscmp(key, L"ProductVersion") == 0) {
            if (!ru_set_product_version(ru, v1, v2, v3, v4)) {
                fwprintf(stderr, L"Unable to change product version\n");
                return OPTION_ERROR;
            }
        }
    }

    if (!ru_set_version_string(ru, key, value)) {
        fwprintf(stderr, L"Unable to change version string\n");
        return OPTION_ERROR;
    }
    return OPTION_CHANGED;
}

static int handle_get_version_string(ResourceUpdater* ru, wchar_t* args[]) {
    wchar_t* result = (wchar_t*)ru_get_version_string(ru, args[0]);
    if (!result) {
        fwprintf(stderr, L"Unable to get version string\n");
        return OPTION_ERROR;
    }
    fwprintf(stdout, L"%s", result);
    free(result);
    return OPTION_OK;
}

static int handle_set_icon(ResourceUpdater* ru, wchar_t* args[]) {
    if (!ru_set_icon(ru, args[0])) {
        fwprintf(stderr, L"Unable to set icon\n");
        return OPTION_ERROR;
    }
    return OPTION_CHANGED;
}

static int handle_application_manifest(ResourceUpdater* ru, wchar_t* args[]) {
    if (ru_is_execution_level_set(ru))
        print_warning("--set-requested-execution-level is ignored if --application-manifest is set");
    if (!ru_set_application_manifest(ru, args[0])) {
        fwprintf(stderr, L"Unable to set application manifest\n");
        return OPTION_ERROR;
    }
    return OPTION_CHANGED;
}

static int handle_set_execution_level(ResourceUpdater* ru, wchar_t* args[]) {
    if (ru_is_application_manifest_set(ru))
        print_warning("--set-requested-execution-level is ignored if --application-manifest is set");
    if (!ru_set_requested_execution_level(ru, args[0])) {
        fwprintf(stderr, L"Unable to set execution level\n");
        return OPTION_ERROR;
    }
    return OPTION_CHANGED;
}

static int handle_set_resource_string(ResourceUpdater* ru, wchar_t* args[]) {
    unsigned int id = 0;
    if (!parse_uint(args[0], &id)) {
        fwprintf(stderr, L"Unable to parse id\n");
        return OPTION_ERROR;
    }
    if (!ru_change_string(ru, id, args[1])) {
        fwprintf(stderr, L"Unable to change string\n");
        return OPTION_ERROR;
    }
    return OPTION_CHANGED;
}

static int handle_get_resource_string(ResourceUpdater* ru, wchar_t* args[]) {
    unsigned int id = 0;
    if (!parse_uint(args[0], &id)) {
        fwprintf(stderr, L"Unable to parse id\n");
        return OPTION_ERROR;
    }
    const wchar_t* result = ru_get_resource_string(ru, id);
    if (!result) {
        fwprintf(stderr, L"Unable to get resource string\n");
        return OPTION_ERROR;
    }
    fwprintf(stdout, L"%s", result);
    return OPTION_OK;
}

static int handle_set_rcdata(ResourceUpdater* ru, wchar_t* args[]) {
    unsigned int id = 0;
    if (!parse_uint(args[0], &id)) {
        fwprintf(stderr, L"Unable to parse id\n");
        return OPTION_ERROR;
    }
    if (!ru_change_rcdata(ru, id, args[1])) {
        fwprintf(stderr, L"Unable to change RCDATA\n");
        return OPTION_ERROR;
    }
    return OPTION_CHANGED;
}

static const OptionSpec kOptions[] = {
    {L"--set-file-version", L"-sfv", 1, L"a version string", handle_set_file_version},
    {L"--set-product-version", L"-spv", 1, L"a version string", handle_set_product_version},
    {L"--set-version-string", L"-svs", 2, L"a key and a value", handle_set_version_string},
    {L"--get-version-string", L"-gvs", 1, L"a key", handle_get_version_string},
    {L"--set-icon", L"-si", 1, L"a path to an icon", handle_set_icon},
    {L"--set-requested-execution-level", L"-srel", 1, L"a level string", handle_set_execution_level},
    {L"--application-manifest", L"-am", 1, L"a local path", handle_application_manifest},
    {L"--set-resource-string", L"--srs", 2, L"an id and a value", handle_set_resource_string},
    {L"--get-resource-string", L"-grs", 1, L"an id", handle_get_resource_string},
    {L"--set-rcdata", NULL, 2, L"an id and a path", handle_set_rcdata},
};

static const OptionSpec* find_option(const wchar_t* arg) {
    for (size_t i = 0; i < ARRAY_SIZE(kOptions); ++i) {
        if (wcscmp(arg, kOptions[i].name) == 0 || (kOptions[i].alias && wcscmp(arg, kOptions[i].alias) == 0))
            return &kOptions[i];
    }
    return NULL;
}

int wmain(int argc, wchar_t* argv[]) {
    if (argc == 1 || is_help(argv[1])) {
        print_help(argv[0]);
        return 0;
    }

    ResourceUpdater ru;
    ru_init(&ru);
    if (!ru_load(&ru, argv[1])) {
        fwprintf(stderr, L"Unable to load file: \"%ls\"\n", argv[1]);
        ru_free(&ru);
        return 1;
    }

    int dirty = 0;
    for (int i = 2; i < argc;) {
        if (is_help(argv[i])) {
            print_help(argv[0]);
            ru_free(&ru);
            return 0;
        }

        const OptionSpec* option = find_option(argv[i]);
        if (!option) {
            fwprintf(stderr, L"Unrecognized argument: %ls\n", argv[i]);
            ru_free(&ru);
            return 1;
        }

        if (i + option->argCount >= argc) {
            fwprintf(stderr, L"%ls requires %ls\n", option->name, option->usage);
            ru_free(&ru);
            return 1;
        }

        int status = option->handler(&ru, &argv[i + 1]);
        if (status == OPTION_ERROR) {
            ru_free(&ru);
            return 1;
        }
        if (status == OPTION_CHANGED)
            dirty = 1;

        i += 1 + option->argCount;
    }

    if (dirty && !ru_commit(&ru)) {
        fwprintf(stderr, L"Failed to commit updates\n");
        ru_free(&ru);
        return 1;
    }

    ru_free(&ru);
    return 0;
}
