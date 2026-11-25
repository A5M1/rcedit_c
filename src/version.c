#include "rcedit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void versioninfo_init(VersionInfo* v) {
    v->has_fixed = 0;
    v->stringTables = NULL;
    v->tableCount = 0;
    v->supportedTranslations = NULL;
    v->supportedCount = 0;
}
void versioninfo_free(VersionInfo* v) {
    for (size_t i = 0; i < v->tableCount; ++i)
        vst_free(&v->stringTables[i]);
    free(v->stringTables);
    v->stringTables = NULL;
    for (size_t i = 0; i < v->supportedCount; ++i) {
    }
    free(v->supportedTranslations);
    v->supportedTranslations = NULL;
    v->tableCount = 0;
    v->supportedCount = 0;
}

static void versioninfo_fill_default(VersionInfo* v) {
    if (v->tableCount == 0) {
        v->stringTables = malloc(sizeof(VersionStringTable));
        v->tableCount = 1;
        vst_init(&v->stringTables[0]);
        v->stringTables[0].encoding.wLanguage = 1033;
        v->stringTables[0].encoding.wCodePage = 1200;
    }
    if (v->supportedCount == 0) {
        v->supportedTranslations = malloc(sizeof(Translate));
        v->supportedCount = 1;
        v->supportedTranslations[0].wLanguage = 1033;
        v->supportedTranslations[0].wCodePage = 1200;
    }
    if (!v->has_fixed) {
        memset(&v->fixedInfo, 0, sizeof(v->fixedInfo));
        v->fixedInfo.dwSignature = 0xFEEF04BD;
        v->fixedInfo.dwFileType = VFT_APP;
        v->has_fixed = 1;
    }
}

static void get_children_data(const unsigned char* entryData, const unsigned char** out_ptr, size_t* out_size) {
    const VS_VERSION_STRING_C* entry = (const VS_VERSION_STRING_C*)entryData;
    const unsigned char* headerOffset = entryData;
    size_t headerSize = sizeof(VS_VERSION_HEADER_C);
    size_t keySize = (wcslen(entry->szKey) + 1) * sizeof(wchar_t);
    size_t childrenOffset = round4(headerSize + keySize);
    const unsigned char* pChildren = headerOffset + childrenOffset;
    size_t childrenSize = entry->Header.wLength - (WORD)childrenOffset;
    *out_ptr = pChildren;
    *out_size = childrenSize;
}

static void versioninfo_deserialize_stringtable(const unsigned char* offset, size_t length,
                                                VersionStringTable** out_tables, size_t* out_table_count);

static void versioninfo_deserialize_varfileinfo(const unsigned char* offset, VersionInfo* vi) {
    const unsigned char* ptr;
    size_t size;
    get_children_data(offset, &ptr, &size);
    const DWORD* top = (const DWORD*)ptr;
    size_t cnt = size / sizeof(DWORD);
    vi->supportedTranslations = malloc(cnt * sizeof(Translate));
    vi->supportedCount = cnt;
    for (size_t i = 0; i < cnt; ++i) {
        DWORD var = top[i];
        vi->supportedTranslations[i].wLanguage = (WORD)(var & 0xFFFF);
        vi->supportedTranslations[i].wCodePage = (WORD)(var >> 16);
    }
}

static void versioninfo_deserialize_versioninfo(const unsigned char* data, size_t size, VersionInfo* vi) {
    const VS_VERSION_ROOT_C* pVersionInfo = (const VS_VERSION_ROOT_C*)data;
    WORD fixedFileInfoSize = pVersionInfo->Header.wValueLength;
    if (fixedFileInfoSize > 0) {
        vi->has_fixed = 1;
        memcpy(&vi->fixedInfo, &pVersionInfo->Info.Info, sizeof(VS_FIXEDFILEINFO));
    }
    const unsigned char* fixedFileInfoEndOffset = (const unsigned char*)(&pVersionInfo->Info.szKey) +
                                                  (wcslen(pVersionInfo->Info.szKey) + 1) * sizeof(wchar_t) +
                                                  fixedFileInfoSize;
    const unsigned char* pVersionInfoChildren = (const unsigned char*)round4((ptrdiff_t)fixedFileInfoEndOffset);
    size_t versionInfoChildrenOffset = pVersionInfoChildren - data;
    size_t versionInfoChildrenSize = pVersionInfo->Header.wLength - versionInfoChildrenOffset;
    const unsigned char* childrenEnd = pVersionInfoChildren + versionInfoChildrenSize;
    for (const unsigned char* p = pVersionInfoChildren; p < childrenEnd && p < data + size;) {
        const wchar_t* key = ((const VS_VERSION_STRING_C*)p)->szKey;
        const unsigned char* childPtr;
        size_t childSize;
        get_children_data(p, &childPtr, &childSize);
        if (wcscmp(key, L"StringFileInfo") == 0) {

            versioninfo_deserialize_stringtable(childPtr, childSize, &vi->stringTables, &vi->tableCount);
        } else if (wcscmp(key, L"VarFileInfo") == 0) {
            versioninfo_deserialize_varfileinfo(childPtr, vi);
        }
        WORD wLength = ((const VS_VERSION_STRING_C*)p)->Header.wLength;
        p += round4(wLength);
    }
}

static void versioninfo_deserialize_stringtable(const unsigned char* offset, size_t length,
                                                VersionStringTable** out_tables, size_t* out_table_count) {

    const unsigned char* end = offset + length;
    size_t count = 0;
    size_t cap = 0;
    VersionStringTable* tables = NULL;
    for (const unsigned char* p = offset; p < end;) {
        const VS_VERSION_STRING_C* strTable = (const VS_VERSION_STRING_C*)p;
        const unsigned char* stringData;
        size_t stringDataSize;
        get_children_data(p, &stringData, &stringDataSize);

        if (count + 1 > cap) {
            size_t newcap = (cap == 0) ? 4 : cap * 2;
            VersionStringTable* tmp = realloc(tables, newcap * sizeof(VersionStringTable));
            if (!tmp)
                return;
            tables = tmp;
            cap = newcap;
        }
        VersionStringTable* t = &tables[count++];
        vst_init(t);
        {

            unsigned long keyValue = wcstoul(strTable->szKey, NULL, 16);
            t->encoding.wLanguage = (WORD)(keyValue >> 16);
            t->encoding.wCodePage = (WORD)(keyValue & 0xFFFF);
        }

        const unsigned char* pstr = stringData;
        const unsigned char* pstrEnd = stringData + stringDataSize;
        while (pstr < pstrEnd) {
            const VS_VERSION_STRING_C* sEntry = (const VS_VERSION_STRING_C*)pstr;
            const unsigned char* sData;
            size_t sDataLen;
            get_children_data(pstr, &sData, &sDataLen);
            wchar_t keyBuf[256];
            wcscpy(keyBuf, sEntry->szKey);
            wchar_t* val = malloc((sEntry->Header.wValueLength + 1) * sizeof(wchar_t));
            if (sEntry->Header.wValueLength > 0) {
                memcpy(val, sData, sEntry->Header.wValueLength * sizeof(wchar_t));
                val[sEntry->Header.wValueLength] = L'\0';
            } else {
                val[0] = L'\0';
            }
            vst_push(t, keyBuf, val);
            free(val);
            pstr += round4(sEntry->Header.wLength);
        }
        p += round4(strTable->Header.wLength);
    }
    *out_tables = tables;
    *out_table_count = count;
}

VersionInfo* vinfo_create_from_module(HMODULE hModule, WORD lang) {
    HRSRC hRsrc = FindResourceExW(hModule, RT_VERSION, MAKEINTRESOURCEW(1), lang);
    if (!hRsrc)
        return NULL;
    HGLOBAL hGlobal = LoadResource(hModule, hRsrc);
    if (!hGlobal)
        return NULL;
    void* p = LockResource(hGlobal);
    if (!p) {
        FreeResource(hGlobal);
        return NULL;
    }
    DWORD size = SizeofResource(hModule, hRsrc);
    if (size == 0) {
        FreeResource(hGlobal);
        return NULL;
    }
    VersionInfo* vi = malloc(sizeof(VersionInfo));
    versioninfo_init(vi);
    versioninfo_deserialize_versioninfo((const unsigned char*)p, size, vi);
    versioninfo_fill_default(vi);
    UnlockResource(hGlobal);
    FreeResource(hGlobal);
    return vi;
}
typedef struct VersionStampValue {
    WORD valueLength;
    WORD type;
    wchar_t* key;
    ByteVec value;
    struct VersionStampValue* children;
    size_t childCount;
    size_t childCapacity;
} VersionStampValue;

static void vsv_init(VersionStampValue* v) {
    v->valueLength = 0;
    v->type = 0;
    v->key = NULL;
    bytevec_init(&v->value);
    v->children = NULL;
    v->childCount = 0;
    v->childCapacity = 0;
}
static void vsv_free(VersionStampValue* v) {
    free(v->key);
    bytevec_free(&v->value);
    if (v->children) {
        for (size_t i = 0; i < v->childCount; ++i)
            vsv_free(&v->children[i]);
        free(v->children);
    }
    v->children = NULL;
    v->childCount = 0;
    v->childCapacity = 0;
}
static int vsv_push_child(VersionStampValue* parent, VersionStampValue* child) {
    if (parent->childCount + 1 > parent->childCapacity) {
        size_t newcap = (parent->childCapacity == 0) ? 4 : parent->childCapacity * 2;
        VersionStampValue* tmp = realloc(parent->children, newcap * sizeof(VersionStampValue));
        if (!tmp)
            return 0;
        parent->children = tmp;
        parent->childCapacity = newcap;
    }
    parent->children[parent->childCount++] = *child;
    return 1;
}

static size_t vsv_get_length(const VersionStampValue* v) {
    size_t bytes = sizeof(VS_VERSION_HEADER_C);
    bytes += (wcslen(v->key) + 1) * sizeof(wchar_t);
    if (v->value.data && v->value.size > 0)
        bytes = round4(bytes) + v->value.size;
    for (size_t i = 0; i < v->childCount; ++i) {
        bytes = round4(bytes) + vsv_get_length(&v->children[i]);
    }
    return bytes;
}

static int vsv_serialize(const VersionStampValue* v, unsigned char* buf, size_t bufsize) {
    size_t offset = 0;
    VS_VERSION_HEADER_C header = {(WORD)bufsize, v->valueLength, v->type};
    memcpy(buf + offset, &header, sizeof(header));
    offset += sizeof(header);
    size_t keySize = (wcslen(v->key) + 1) * sizeof(wchar_t);
    memcpy(buf + offset, v->key, keySize);
    offset += keySize;
    if (v->value.data && v->value.size > 0) {
        offset = round4(offset);
        memcpy(buf + offset, v->value.data, v->value.size);
        offset += v->value.size;
    }
    for (size_t i = 0; i < v->childCount; ++i) {
        offset = round4(offset);
        size_t childLen = vsv_get_length(&v->children[i]);
        vsv_serialize(&v->children[i], buf + offset, childLen);
        offset += childLen;
    }
    return 1;
}

unsigned char* versioninfo_serialize(const VersionInfo* vi, size_t* outSize) {
    VersionStampValue root;
    vsv_init(&root);
    root.key = _wcsdup(L"VS_VERSION_INFO");
    root.type = 0;

    if (vi->has_fixed) {
        root.valueLength = sizeof(VS_FIXEDFILEINFO);
        bytevec_reserve(&root.value, root.valueLength);
        memcpy(root.value.data, &vi->fixedInfo, root.valueLength);
        root.value.size = root.valueLength;
    }

    VersionStampValue stringFileInfo;
    vsv_init(&stringFileInfo);
    stringFileInfo.key = _wcsdup(L"StringFileInfo");
    stringFileInfo.type = 1;
    stringFileInfo.valueLength = 0;
    for (size_t i = 0; i < vi->tableCount; ++i) {
        VersionStampValue stringTableRaw;
        vsv_init(&stringTableRaw);
        stringTableRaw.type = 1;
        stringTableRaw.valueLength = 0;

        wchar_t tableKey[64];
        swprintf(tableKey, 64, L"%08x",
                 (vi->stringTables[i].encoding.wLanguage << 16) | vi->stringTables[i].encoding.wCodePage);
        stringTableRaw.key = _wcsdup(tableKey);
        for (size_t j = 0; j < vi->stringTables[i].count; ++j) {
            VersionStampValue stringRaw;
            vsv_init(&stringRaw);
            stringRaw.type = 1;
            stringRaw.key = _wcsdup(vi->stringTables[i].strings[j].key);
            size_t strLen = wcslen(vi->stringTables[i].strings[j].value) + 1;
            stringRaw.valueLength = (WORD)strLen;
            bytevec_reserve(&stringRaw.value, strLen * sizeof(wchar_t));
            memcpy(stringRaw.value.data, vi->stringTables[i].strings[j].value, strLen * sizeof(wchar_t));
            stringRaw.value.size = strLen * sizeof(wchar_t);
            vsv_push_child(&stringTableRaw, &stringRaw);
        }
        vsv_push_child(&stringFileInfo, &stringTableRaw);
    }
    vsv_push_child(&root, &stringFileInfo);

    VersionStampValue varFileInfo;
    vsv_init(&varFileInfo);
    varFileInfo.key = _wcsdup(L"VarFileInfo");
    varFileInfo.type = 1;
    VersionStampValue varRaw;
    vsv_init(&varRaw);
    varRaw.key = _wcsdup(L"Translation");
    varRaw.type = 0;

    size_t transCount = vi->supportedCount;
    bytevec_reserve(&varRaw.value, transCount * sizeof(DWORD));
    varRaw.value.size = transCount * sizeof(DWORD);
    for (size_t i = 0; i < transCount; ++i) {
        DWORD var = (DWORD)vi->supportedTranslations[i].wCodePage << 16 | vi->supportedTranslations[i].wLanguage;
        memcpy(varRaw.value.data + i * sizeof(DWORD), &var, sizeof(DWORD));
    }
    varRaw.valueLength = (WORD)varRaw.value.size;
    vsv_push_child(&varFileInfo, &varRaw);
    vsv_push_child(&root, &varFileInfo);

    size_t totalLen = vsv_get_length(&root);
    unsigned char* out = malloc(totalLen);
    if (!out) {
        vsv_free(&root);
        return NULL;
    }
    vsv_serialize(&root, out, totalLen);
    *outSize = totalLen;
    vsv_free(&root);
    return out;
}
