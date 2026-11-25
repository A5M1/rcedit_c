#ifndef RCEDIT_H
#define RCEDIT_H

#include <stddef.h>
#include <stdint.h>
#include <wchar.h>
#include <windows.h>
#include <winver.h>

#ifndef UNUSED
#define UNUSED
#endif

#define RCEDIT_LANG_EN_US 1033
#define RCEDIT_CODEPAGE_EN_US 1200

typedef struct {
    uint8_t* data;
    size_t size;
    size_t capacity;
} ByteVec;

void bytevec_init(ByteVec* v);
void bytevec_free(ByteVec* v);
int bytevec_reserve(ByteVec* v, size_t sz);

#pragma pack(push, 1)
typedef struct {
    WORD reserved;
    WORD type;
    WORD count;
} ICOHEADER;

typedef struct {
    BYTE width;
    BYTE height;
    BYTE colorCount;
    BYTE reserved;
    WORD planes;
    WORD bitCount;
    DWORD bytesInRes;
    DWORD imageOffset;
} ICONDIRENTRY;
#pragma pack(pop)

#pragma pack(push, 2)
typedef struct {
    BYTE width;
    BYTE height;
    BYTE colourCount;
    BYTE reserved;
    WORD planes;
    WORD bitCount;
    DWORD bytesInRes;
    WORD id;
} GRPICONENTRY;

typedef struct {
    WORD reserved;
    WORD type;
    WORD count;
    GRPICONENTRY entries[1];
} GRPICONHEADER;
#pragma pack(pop)

typedef struct {
    wchar_t* key;
    wchar_t* value;
} VersionString;

typedef struct {
    WORD wLanguage;
    WORD wCodePage;
} Translate;

typedef struct {
    Translate encoding;
    VersionString* strings;
    size_t count;
    size_t capacity;
} VersionStringTable;

typedef struct {
    WORD wLanguage;
    UINT id;
    int hasFixed;
    VS_FIXEDFILEINFO fixedInfo;
    VersionStringTable* tables;
    size_t count;
    size_t capacity;
    Translate* supportedTranslations;
    size_t supportedCount;
} VersionStampMapEntry;

typedef struct {
    VersionStampMapEntry* entries;
    size_t count;
    size_t capacity;
} VersionStampMap;

typedef struct {
    int has_fixed;
    VS_FIXEDFILEINFO fixedInfo;
    VersionStringTable* stringTables;
    size_t tableCount;
    Translate* supportedTranslations;
    size_t supportedCount;
} VersionInfo;

#pragma pack(push, 1)
typedef struct {
    WORD wLength;
    WORD wValueLength;
    WORD wType;
} VS_VERSION_HEADER_C;

typedef struct {
    VS_VERSION_HEADER_C Header;
    wchar_t szKey[1];
} VS_VERSION_STRING_C;

typedef struct {
    wchar_t szKey[16];
    WORD Padding1[1];
    VS_FIXEDFILEINFO Info;
} VS_VERSION_ROOT_INFO_C;

typedef struct {
    VS_VERSION_HEADER_C Header;
    VS_VERSION_ROOT_INFO_C Info;
} VS_VERSION_ROOT_C;
#pragma pack(pop)

static inline size_t round4(size_t v) { return v + ((v % 4) ? (4 - (v % 4)) : 0); }

typedef struct {
    UINT blockId;
    wchar_t** strings;
} StringTableEntry;

typedef struct {
    WORD lang;
    StringTableEntry* entries;
    size_t count;
    size_t capacity;
} StringTableMapEntry;

typedef struct {
    StringTableMapEntry* entries;
    size_t count;
    size_t capacity;
} StringTableMap;

typedef struct {
    ptrdiff_t id;
    ByteVec data;
} RcDataEntry;

typedef struct {
    WORD lang;
    RcDataEntry* entries;
    size_t count;
    size_t capacity;
} RcDataLangEntry;

typedef struct {
    RcDataLangEntry* entries;
    size_t count;
    size_t capacity;
} RcDataLangMap;

typedef struct {
    BYTE width;
    BYTE height;
    BYTE colorCount;
    BYTE reserved;
    WORD planes;
    WORD bitCount;
    DWORD bytesInRes;
    DWORD imageOffset;
} IconEntry;

typedef struct {
    WORD reserved;
    WORD type;
    WORD count;
    IconEntry* entries;
} IconHeader;

typedef struct {
    IconHeader header;
    ByteVec* images;
    ByteVec grpHeader;
} IconsValue;

typedef struct {
    UINT maxIconId;
    UINT* bundleIds;
    IconsValue** bundles;
    size_t count;
    size_t capacity;
} IconResInfo;

typedef struct {
    WORD lang;
    IconResInfo info;
} IconTableEntry;

typedef struct {
    IconTableEntry* entries;
    size_t count;
    size_t capacity;
} IconTableMap;

typedef struct ResourceUpdater {
    HMODULE module;
    wchar_t* filename;
    wchar_t* executionLevel;
    wchar_t* originalExecutionLevel;
    wchar_t* applicationManifestPath;
    wchar_t* manifestString;
    WORD manifestLang;
    UINT manifestId;
    VersionStampMap versionStampMap;
    StringTableMap stringTableMap;
    IconTableMap iconBundleMap;
    RcDataLangMap rcDataLngMap;
} ResourceUpdater;

void vst_init(VersionStringTable* t);
void vst_free(VersionStringTable* t);
int vst_push(VersionStringTable* t, const wchar_t* key, const wchar_t* value);
void vsm_init(VersionStampMap* m);
void vsm_free(VersionStampMap* m);
VersionStampMapEntry* vsm_get_or_create(VersionStampMap* m, WORD lang);
void stm_init(StringTableMap* m);
void stm_free(StringTableMap* m);
StringTableMapEntry* stm_get_or_create_lang(StringTableMap* m, WORD lang);
void rcdmap_init(RcDataLangMap* m);
void rcdmap_free(RcDataLangMap* m);
void itable_init(IconTableMap* m);
void itable_free(IconTableMap* m);

void versioninfo_init(VersionInfo* v);
void versioninfo_free(VersionInfo* v);
unsigned char* versioninfo_serialize(const VersionInfo* vi, size_t* outSize);
VersionInfo* vinfo_create_from_module(HMODULE hModule, UINT id, WORD lang);

void ru_init(ResourceUpdater* ru);
void ru_free(ResourceUpdater* ru);
int ru_set_file_version(ResourceUpdater* ru, unsigned short v1, unsigned short v2, unsigned short v3,
                        unsigned short v4);
int ru_set_product_version(ResourceUpdater* ru, unsigned short v1, unsigned short v2, unsigned short v3,
                           unsigned short v4);
int ru_set_version_string(ResourceUpdater* ru, const wchar_t* name, const wchar_t* value);
int ru_set_icon(ResourceUpdater* ru, const wchar_t* path);
int ru_set_application_manifest(ResourceUpdater* ru, const wchar_t* path);
int ru_set_requested_execution_level(ResourceUpdater* ru, const wchar_t* level);
int ru_change_rcdata(ResourceUpdater* ru, unsigned int id, const wchar_t* path);
int ru_change_string(ResourceUpdater* ru, unsigned int id, const wchar_t* value);
const wchar_t* ru_get_resource_string(ResourceUpdater* ru, unsigned int id);
const wchar_t* ru_get_version_string(ResourceUpdater* ru, const wchar_t* key);
int ru_is_execution_level_set(ResourceUpdater* ru);
int ru_is_application_manifest_set(ResourceUpdater* ru);
int ru_commit(ResourceUpdater* ru);
int ru_load(ResourceUpdater* ru, const wchar_t* filename);

unsigned char* read_file_bytes(const wchar_t* path, size_t* out_size);
void print_help(const wchar_t* argv0);
int parse_version_string(const wchar_t* str, unsigned short* v1, unsigned short* v2, unsigned short* v3,
                         unsigned short* v4);
int print_error(const char* message);
int print_warning(const char* message);

#endif
