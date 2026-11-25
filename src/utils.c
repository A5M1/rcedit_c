#include "rcedit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
unsigned char* read_file_bytes(const wchar_t* path, size_t* out_size) {
    FILE* f = _wfopen(path, L"rb");
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char* buf = malloc(size);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    if (fread(buf, 1, size, f) != (size_t)size) {
        free(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);
    *out_size = (size_t)size;
    return buf;
}
void print_help(const wchar_t* argv0) {
    fprintf(stdout,
            "Rcedit C port: Edit resources of exe.\n\n"
            "Usage: %ls <filename> [options...]\n\n"
            "Options:\n"
            "  -h, --help                                 Show this message\n"
            "  --set-file-version <version>               Set FileVersion\n"
            "  --set-product-version <version>            Set ProductVersion\n"
            "  --set-version-string <key> <value>         Set version string\n"
            "  --set-icon <path-to-icon>                  Set file icon\n"
            "  --set-requested-execution-level <level>    Pass nothing to see usage\n"
            "  --application-manifest <path-to-file>      Set manifest file\n"
            "  --set-resource-string <id> <value>         Set string table value\n"
            "  --set-rcdata <id> <path-to-file>           Replace RCDATA by integer id\n",
            argv0);
}

int parse_version_string(const wchar_t* str, unsigned short* v1, unsigned short* v2, unsigned short* v3,
                         unsigned short* v4) {
    *v1 = *v2 = *v3 = *v4 = 0;
    int matched = swscanf(str, L"%hu.%hu.%hu.%hu", v1, v2, v3, v4);
    if (matched >= 1)
        return 1;
    return 0;
}

int print_error(const char* message) UNUSED {
    fprintf(stderr, "Fatal error: %s\n", message);
    return 1;
}

int print_warning(const char* message) UNUSED {
    fprintf(stderr, "Warning: %s\n", message);
    return 1;
}
