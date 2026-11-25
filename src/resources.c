#include "rcedit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int ru_set_icon_lang(ResourceUpdater* ru, const wchar_t* path, WORD lang, UINT iconBundle);
static BOOL CALLBACK on_enum_lang(HMODULE hModule, LPCWSTR lpszType, LPCWSTR lpszName, WORD wIDLanguage,
                                  LONG_PTR lParam);
static BOOL CALLBACK on_enum_name(HMODULE hModule, LPCWSTR lpszType, LPWSTR lpszName, LONG_PTR lParam);
static BOOL CALLBACK on_enum_manifest(HMODULE hModule, LPCTSTR lpType, LPWSTR lpName, LONG_PTR lParam);

void ru_init(ResourceUpdater* ru) {
    ru->module = NULL;
    ru->filename = NULL;
    ru->executionLevel = NULL;
    ru->originalExecutionLevel = NULL;
    ru->applicationManifestPath = NULL;
    ru->manifestString = NULL;
    vsm_init(&ru->versionStampMap);
    stm_init(&ru->stringTableMap);
    itable_init(&ru->iconBundleMap);
    rcdmap_init(&ru->rcDataLngMap);
}

void ru_free(ResourceUpdater* ru) {
    if (ru->module) {
        FreeLibrary(ru->module);
        ru->module = NULL;
    }
    free(ru->filename);
    free(ru->executionLevel);
    free(ru->originalExecutionLevel);
    free(ru->applicationManifestPath);
    free(ru->manifestString);
    vsm_free(&ru->versionStampMap);
    stm_free(&ru->stringTableMap);
    itable_free(&ru->iconBundleMap);
    rcdmap_free(&ru->rcDataLngMap);
}

int ru_set_file_version(ResourceUpdater* ru, unsigned short v1, unsigned short v2, unsigned short v3,
                        unsigned short v4) {
    WORD lang = (ru->versionStampMap.count == 0) ? 1033 : ru->versionStampMap.entries[0].wLanguage;
    VersionStampMapEntry* e = vsm_get_or_create(&ru->versionStampMap, lang);
    if (!e)
        return 0;
    e->hasFixed = 1;
    e->fixedInfo.dwSignature = 0xFEEF04BD;
    e->fixedInfo.dwFileType = VFT_APP;
    e->fixedInfo.dwFileVersionMS = (v1 << 16) | v2;
    e->fixedInfo.dwFileVersionLS = (v3 << 16) | v4;
    return 1;
}

static int ru_set_file_version_lang(ResourceUpdater* ru, WORD lang, unsigned short v1, unsigned short v2,
                                    unsigned short v3, unsigned short v4) {
    VersionStampMapEntry* e = vsm_get_or_create(&ru->versionStampMap, lang);
    if (!e)
        return 0;
    e->hasFixed = 1;
    e->fixedInfo.dwSignature = 0xFEEF04BD;
    e->fixedInfo.dwFileType = VFT_APP;
    e->fixedInfo.dwFileVersionMS = (v1 << 16) | v2;
    e->fixedInfo.dwFileVersionLS = (v3 << 16) | v4;
    return 1;
}

static UNUSED int ru_set_file_version_lang_id(ResourceUpdater* ru, WORD lang, UINT id, unsigned short v1,
                                              unsigned short v2, unsigned short v3, unsigned short v4) {
    (void)id;
    return ru_set_file_version_lang(ru, lang, v1, v2, v3, v4);
}

int ru_set_product_version(ResourceUpdater* ru, unsigned short v1, unsigned short v2, unsigned short v3,
                           unsigned short v4) {
    WORD lang = (ru->versionStampMap.count == 0) ? 1033 : ru->versionStampMap.entries[0].wLanguage;
    VersionStampMapEntry* e = vsm_get_or_create(&ru->versionStampMap, lang);
    if (!e)
        return 0;
    e->hasFixed = 1;
    e->fixedInfo.dwSignature = 0xFEEF04BD;
    e->fixedInfo.dwFileType = VFT_APP;
    e->fixedInfo.dwProductVersionMS = (v1 << 16) | v2;
    e->fixedInfo.dwProductVersionLS = (v3 << 16) | v4;
    return 1;
}

static int ru_set_product_version_lang(ResourceUpdater* ru, WORD lang, unsigned short v1, unsigned short v2,
                                       unsigned short v3, unsigned short v4) {
    VersionStampMapEntry* e = vsm_get_or_create(&ru->versionStampMap, lang);
    if (!e)
        return 0;
    e->hasFixed = 1;
    e->fixedInfo.dwSignature = 0xFEEF04BD;
    e->fixedInfo.dwFileType = VFT_APP;
    e->fixedInfo.dwProductVersionMS = (v1 << 16) | v2;
    e->fixedInfo.dwProductVersionLS = (v3 << 16) | v4;
    return 1;
}

static UNUSED int ru_set_product_version_lang_id(ResourceUpdater* ru, WORD lang, UINT id, unsigned short v1,
                                                 unsigned short v2, unsigned short v3, unsigned short v4) {
    (void)id;
    return ru_set_product_version_lang(ru, lang, v1, v2, v3, v4);
}

static int ru_set_version_string_lang(ResourceUpdater* ru, WORD lang, const wchar_t* name, const wchar_t* value) {
    VersionStampMapEntry* e = vsm_get_or_create(&ru->versionStampMap, lang);
    if (!e)
        return 0;
    e->wLanguage = lang;

    if (e->tables == NULL) {
        e->tables = malloc(sizeof(VersionStringTable));
        e->count = 1;
        e->capacity = 1;
        vst_init(&e->tables[0]);
    }
    for (size_t ti = 0; ti < e->count; ++ti) {
        VersionStringTable* table = &e->tables[ti];
        if (table->encoding.wLanguage == 0) {
            table->encoding.wLanguage = lang;
            table->encoding.wCodePage = 1200;
        }

        for (size_t si = 0; si < table->count; ++si) {
            if (wcscmp(table->strings[si].key, name) == 0) {
                free(table->strings[si].value);
                table->strings[si].value = _wcsdup(value);
                return 1;
            }
        }
        vst_push(table, name, value);
    }
    return 1;
}

int ru_set_version_string(ResourceUpdater* ru, const wchar_t* name, const wchar_t* value) {
    WORD lang = (ru->versionStampMap.count == 0) ? 1033 : ru->versionStampMap.entries[0].wLanguage;
    return ru_set_version_string_lang(ru, lang, name, value);
}

int ru_set_icon(ResourceUpdater* ru, const wchar_t* path) {
    WORD lang = (ru->iconBundleMap.count == 0) ? MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)
                                               : ru->iconBundleMap.entries[0].lang;
    return ru_set_icon_lang(ru, path, lang, 0);
}

static int ru_set_icon_lang(ResourceUpdater* ru, const wchar_t* path, WORD lang, UINT iconBundle) {
    size_t ico_size;
    unsigned char* ico_data = read_file_bytes(path, &ico_size);
    if (!ico_data)
        return 0;
    ICOHEADER* hdr = (ICOHEADER*)ico_data;
    if (hdr->reserved != 0 || hdr->type != 1) {
        free(ico_data);
        return 0;
    }
    UINT count = hdr->count;
    ICONDIRENTRY* entries = (ICONDIRENTRY*)(ico_data + sizeof(ICOHEADER));
    IconsValue* icon = malloc(sizeof(IconsValue));
    memset(icon, 0, sizeof(IconsValue));
    icon->header.reserved = hdr->reserved;
    icon->header.type = hdr->type;
    icon->header.count = hdr->count;
    icon->header.entries = malloc(count * sizeof(IconEntry));
    icon->images = malloc(count * sizeof(ByteVec));
    for (UINT i = 0; i < count; ++i) {
        icon->header.entries[i].width = entries[i].width;
        icon->header.entries[i].height = entries[i].height;
        icon->header.entries[i].colorCount = entries[i].colorCount;
        icon->header.entries[i].reserved = entries[i].reserved;
        icon->header.entries[i].planes = entries[i].planes;
        icon->header.entries[i].bitCount = entries[i].bitCount;
        icon->header.entries[i].bytesInRes = entries[i].bytesInRes;
        icon->header.entries[i].imageOffset = entries[i].imageOffset;
        bytevec_init(&icon->images[i]);
        bytevec_reserve(&icon->images[i], entries[i].bytesInRes);
        memcpy(icon->images[i].data, ico_data + entries[i].imageOffset, entries[i].bytesInRes);
        icon->images[i].size = entries[i].bytesInRes;
    }

    size_t grpSize = 3 * sizeof(WORD) + count * sizeof(GRPICONENTRY);
    bytevec_init(&icon->grpHeader);
    bytevec_reserve(&icon->grpHeader, grpSize);
    icon->grpHeader.size = grpSize;
    memset(icon->grpHeader.data, 0, grpSize);
    GRPICONHEADER* pGrp = (GRPICONHEADER*)icon->grpHeader.data;
    pGrp->reserved = 0;
    pGrp->type = 1;
    pGrp->count = (WORD)count;
    for (UINT i = 0; i < count; ++i) {
        GRPICONENTRY* e = &pGrp->entries[i];
        e->width = icon->header.entries[i].width;
        e->height = icon->header.entries[i].height;
        e->colourCount = icon->header.entries[i].colorCount;
        e->reserved = icon->header.entries[i].reserved;
        e->planes = icon->header.entries[i].planes;
        e->bitCount = icon->header.entries[i].bitCount;
        e->bytesInRes = (DWORD)icon->header.entries[i].bytesInRes;
        e->id = 0;
    }

    IconTableEntry* found = NULL;
    for (size_t i = 0; i < ru->iconBundleMap.count; ++i)
        if (ru->iconBundleMap.entries[i].lang == lang) {
            found = &ru->iconBundleMap.entries[i];
            break;
        }
    if (!found) {
        size_t newcap = (ru->iconBundleMap.capacity == 0) ? 4 : ru->iconBundleMap.capacity * 2;
        IconTableEntry* tmp =
            realloc(ru->iconBundleMap.entries, (ru->iconBundleMap.capacity == 0) ? newcap * sizeof(IconTableEntry)
                                                                                 : newcap * sizeof(IconTableEntry));
        if (!tmp) {
            free(ico_data);
            return 0;
        }
        ru->iconBundleMap.entries = tmp;
        ru->iconBundleMap.capacity = newcap;
        IconTableEntry* e2 = &ru->iconBundleMap.entries[ru->iconBundleMap.count++];
        e2->lang = lang;
        e2->info.count = 0;
        e2->info.bundleIds = NULL;
        e2->info.bundles = NULL;
        e2->info.capacity = 0;
        e2->info.maxIconId = 0;
        found = e2;
    }
    IconResInfo* info = &found->info;
    if (info->count + 1 > info->capacity) {
        size_t newcap = (info->capacity == 0) ? 4 : info->capacity * 2;
        UINT* newIds = realloc(info->bundleIds, newcap * sizeof(UINT));
        IconsValue** newB = realloc(info->bundles, newcap * sizeof(IconsValue*));
        if (!newIds || !newB) {
            free(ico_data);
            return 0;
        }
        info->bundleIds = newIds;
        info->bundles = newB;
        info->capacity = newcap;
    }

    UINT assignedBundleId = iconBundle;
    if (assignedBundleId == 0) {
        assignedBundleId = (info->count > 0) ? info->bundleIds[0] : 0;
    }
    info->bundleIds[info->count] = assignedBundleId;
    info->bundles[info->count] = icon;
    info->count++;
    if (assignedBundleId + count - 1 > info->maxIconId)
        info->maxIconId = assignedBundleId + count - 1;
    free(ico_data);
    return 1;
}

int ru_set_application_manifest(ResourceUpdater* ru, const wchar_t* path) {

    DWORD attrs = GetFileAttributesW(path);
    if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY))
        return 0;
    if (ru->applicationManifestPath)
        free(ru->applicationManifestPath);
    ru->applicationManifestPath = _wcsdup(path);
    return 1;
}

int ru_set_requested_execution_level(ResourceUpdater* ru, const wchar_t* level) {
    if (ru->applicationManifestPath) {

        return 1;
    }
    if (ru->executionLevel)
        free(ru->executionLevel);

    if (wcscmp(level, L"asInvoker") != 0 && wcscmp(level, L"highestAvailable") != 0 &&
        wcscmp(level, L"requireAdministrator") != 0) {
        return 0;
    }
    ru->executionLevel = _wcsdup(level);
    return 1;
}

int ru_change_rcdata(ResourceUpdater* ru, unsigned int id, const wchar_t* path) {
    size_t rsize;
    unsigned char* rbuf = read_file_bytes(path, &rsize);
    if (!rbuf)
        return 0;

    for (size_t i = 0; i < ru->rcDataLngMap.count; ++i) {
        RcDataLangEntry* langEntry = &ru->rcDataLngMap.entries[i];
        for (size_t j = 0; j < langEntry->count; ++j) {
            RcDataEntry* entry = &langEntry->entries[j];
            if (entry->id == (ptrdiff_t)id) {
                bytevec_free(&entry->data);
                bytevec_init(&entry->data);
                bytevec_reserve(&entry->data, rsize);
                memcpy(entry->data.data, rbuf, rsize);
                entry->data.size = rsize;
                free(rbuf);
                return 1;
            }
        }
    }
    free(rbuf);
    fwprintf(stderr, L"Cannot find RCDATA with id '%u'\n", id);
    return 0;
}

int ru_change_string(ResourceUpdater* ru, unsigned int id, const wchar_t* value) {
    WORD lang = (ru->stringTableMap.count == 0) ? 1033 : ru->stringTableMap.entries[0].lang;
    StringTableMapEntry* langEntry = stm_get_or_create_lang(&ru->stringTableMap, lang);
    if (!langEntry)
        return 0;
    UINT blockId = id / 16;
    if (blockId >= langEntry->count) {
        size_t newcap = langEntry->capacity == 0 ? 8 : langEntry->capacity * 2;
        while (newcap <= blockId)
            newcap *= 2;
        StringTableEntry* tmp = realloc(langEntry->entries, newcap * sizeof(StringTableEntry));
        if (!tmp)
            return 0;

        for (size_t j = langEntry->count; j < newcap; ++j) {
            tmp[j].strings = NULL;
            tmp[j].blockId = 0;
        }
        langEntry->entries = tmp;
        langEntry->capacity = newcap;
        langEntry->count = blockId + 1;
    }
    StringTableEntry* table = &langEntry->entries[blockId];
    if (!table->strings) {
        table->strings = malloc(16 * sizeof(wchar_t*));
        for (int i = 0; i < 16; i++)
            table->strings[i] = NULL;
        table->blockId = blockId;
    }
    UINT idx = id % 16;
    if (table->strings[idx])
        free(table->strings[idx]);
    table->strings[idx] = _wcsdup(value);
    return 1;
}

static int ru_change_string_lang(ResourceUpdater* ru, WORD lang, unsigned int id, const wchar_t* value) {
    StringTableMapEntry* langEntry = stm_get_or_create_lang(&ru->stringTableMap, lang) UNUSED;
    if (!langEntry)
        return 0;
    UINT blockId = id / 16;
    if (blockId >= langEntry->count) {
        size_t newcap = langEntry->capacity == 0 ? 8 : langEntry->capacity * 2;
        while (newcap <= blockId)
            newcap *= 2;
        StringTableEntry* tmp = realloc(langEntry->entries, newcap * sizeof(StringTableEntry));
        if (!tmp)
            return 0;
        for (size_t j = langEntry->count; j < newcap; ++j) {
            tmp[j].strings = NULL;
            tmp[j].blockId = 0;
        }
        langEntry->entries = tmp;
        langEntry->capacity = newcap;
        langEntry->count = blockId + 1;
    }
    StringTableEntry* table = &langEntry->entries[blockId];
    if (!table->strings) {
        table->strings = malloc(16 * sizeof(wchar_t*));
        for (int i = 0; i < 16; i++)
            table->strings[i] = NULL;
        table->blockId = blockId;
    }
    UINT idx = id % 16;
    if (table->strings[idx])
        free(table->strings[idx]);
    table->strings[idx] = _wcsdup(value);
    return 1;
}

static const wchar_t* ru_get_resource_string_lang(ResourceUpdater* ru, WORD lang, unsigned int id) {
    for (size_t i = 0; i < ru->stringTableMap.count; ++i)
        if (ru->stringTableMap.entries[i].lang == lang)
            UNUSED {
                StringTableMapEntry* langEntry = &ru->stringTableMap.entries[i];
                UINT blockId = id / 16;
                if (blockId >= langEntry->count)
                    return NULL;
                StringTableEntry* table = &langEntry->entries[blockId];
                if (!table->strings)
                    return NULL;
                UINT idx = id % 16;
                return table->strings[idx];
            }
    return NULL;
}

int ru_is_execution_level_set(ResourceUpdater* ru) { return ru->executionLevel != NULL; }
int ru_is_application_manifest_set(ResourceUpdater* ru) { return ru->applicationManifestPath != NULL; }

int ru_commit(ResourceUpdater* ru) {
    if (!ru->filename)
        return 0;

    if (ru->module) {
        FreeLibrary(ru->module);
        ru->module = NULL;
    }
    HANDLE hUpdate = BeginUpdateResourceW(ru->filename, FALSE);
    if (!hUpdate)
        return 0;

    for (size_t i = 0; i < ru->versionStampMap.count; ++i) {
        VersionStampMapEntry* e = &ru->versionStampMap.entries[i];

        VersionInfo vi;
        versioninfo_init(&vi);
        if (e->hasFixed) {
            vi.has_fixed = 1;
            vi.fixedInfo = e->fixedInfo;
        }

        if (e->count > 0) {
            vi.stringTables = malloc(e->count * sizeof(VersionStringTable));
            vi.tableCount = e->count;
            for (size_t t = 0; t < e->count; ++t) {
                vst_init(&vi.stringTables[t]);
                vi.stringTables[t].encoding = e->tables[t].encoding;
                for (size_t s = 0; s < e->tables[t].count; ++s) {
                    vst_push(&vi.stringTables[t], e->tables[t].strings[s].key, e->tables[t].strings[s].value);
                }
            }
        }

        if (e->supportedCount > 0) {
            vi.supportedTranslations = malloc(e->supportedCount * sizeof(Translate));
            if (vi.supportedTranslations) {
                memcpy(vi.supportedTranslations, e->supportedTranslations, e->supportedCount * sizeof(Translate));
                vi.supportedCount = e->supportedCount;
            }
        }
        if (vi.supportedCount == 0) {
            vi.supportedTranslations = malloc(sizeof(Translate));
            vi.supportedCount = 1;
            vi.supportedTranslations[0].wLanguage = e->wLanguage;
            vi.supportedTranslations[0].wCodePage = 1200;
        }
        size_t outSize = 0;
        unsigned char* buf = versioninfo_serialize(&vi, &outSize);
        versioninfo_free(&vi);
        if (buf) {
            if (!UpdateResourceW(hUpdate, RT_VERSION, MAKEINTRESOURCEW(1), e->wLanguage, buf, (DWORD)outSize)) {
                EndUpdateResourceW(hUpdate, TRUE);
                free(buf);
                return 0;
            }
            free(buf);
        }
    }

    for (size_t i = 0; i < ru->stringTableMap.count; ++i) {
        StringTableMapEntry* lang = &ru->stringTableMap.entries[i];
        for (size_t j = 0; j < lang->count; ++j) {
            StringTableEntry* table = &lang->entries[j];
            if (!table->strings)
                continue;

            size_t size = 0;
            for (int k = 0; k < 16; k++) {
                WORD len = (WORD)(table->strings[k] ? wcslen(table->strings[k]) : 0);
                size += sizeof(WORD) + len * sizeof(wchar_t);
            }
            char* buf = malloc(size);
            char* p = buf;
            for (int k = 0; k < 16; k++) {
                WORD len = (WORD)(table->strings[k] ? wcslen(table->strings[k]) : 0);
                memcpy(p, &len, sizeof(WORD));
                p += sizeof(WORD);
                if (len > 0) {
                    memcpy(p, table->strings[k], len * sizeof(wchar_t));
                    p += len * sizeof(wchar_t);
                }
            }
            if (!UpdateResourceW(hUpdate, RT_STRING, MAKEINTRESOURCEW(table->blockId + 1), lang->lang, buf,
                                 (DWORD)size)) {
                EndUpdateResourceW(hUpdate, TRUE);
                free(buf);
                return 0;
            }
            free(buf);
        }
    }

    for (size_t i = 0; i < ru->rcDataLngMap.count; ++i) {
        RcDataLangEntry* le = &ru->rcDataLngMap.entries[i];
        for (size_t j = 0; j < le->count; ++j) {
            RcDataEntry* rde = &le->entries[j];
            if (!UpdateResourceW(hUpdate, RT_RCDATA, (LPWSTR)rde->id, le->lang, rde->data.data,
                                 (DWORD)rde->data.size)) {
                EndUpdateResourceW(hUpdate, TRUE);
                return 0;
            }
        }
    }

    for (size_t i = 0; i < ru->iconBundleMap.count; ++i) {
        IconTableEntry* ie = &ru->iconBundleMap.entries[i];
        for (size_t b = 0; b < ie->info.count; ++b) {
            UINT bundleId = ie->info.bundleIds[b];
            IconsValue* icon = ie->info.bundles[b];
            if (!icon)
                continue;

            unsigned char* grpbuf = malloc(icon->grpHeader.size);
            if (!grpbuf) {
                EndUpdateResourceW(hUpdate, TRUE);
                return 0;
            }
            memcpy(grpbuf, icon->grpHeader.data, icon->grpHeader.size);
            GRPICONHEADER* pGrp = (GRPICONHEADER*)grpbuf;
            for (int k = 0; k < pGrp->count; ++k) {
                pGrp->entries[k].id = (WORD)(k + 1);
            }
            if (!UpdateResourceW(hUpdate, RT_GROUP_ICON, MAKEINTRESOURCEW(bundleId), ie->lang, grpbuf,
                                 (DWORD)icon->grpHeader.size)) {
                free(grpbuf);
                EndUpdateResourceW(hUpdate, TRUE);
                return 0;
            }
            free(grpbuf);
            for (int k = 0; k < icon->header.count; ++k) {
                if (!UpdateResourceW(hUpdate, RT_ICON, MAKEINTRESOURCEW(k + 1), ie->lang, icon->images[k].data,
                                     (DWORD)icon->images[k].size)) {
                    EndUpdateResourceW(hUpdate, TRUE);
                    return 0;
                }
            }
        }
    }

    if (ru->applicationManifestPath) {
        size_t msize;
        unsigned char* mbuf = read_file_bytes(ru->applicationManifestPath, &msize);
        if (!mbuf) {
            EndUpdateResourceW(hUpdate, TRUE);
            return 0;
        }
        if (!UpdateResourceW(hUpdate, RT_MANIFEST, MAKEINTRESOURCEW(1), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                             mbuf, (DWORD)msize)) {
            free(mbuf);
            EndUpdateResourceW(hUpdate, TRUE);
            return 0;
        }
        free(mbuf);
    } else if (ru->executionLevel && ru->manifestString) {

        wchar_t* newManifest = _wcsdup(ru->manifestString);
        wchar_t* found = wcsstr(newManifest, L"requestedExecutionLevel");
        if (found) {
            wchar_t* levelAttr = wcsstr(found, L"level=");
            if (levelAttr) {
                wchar_t quote = levelAttr[6];
                if (quote == L'\'' || quote == L'\"') {
                    wchar_t* startval = levelAttr + 7;
                    wchar_t* endval = wcschr(startval, quote);
                    if (endval) {
                        size_t before = startval - newManifest;
                        size_t after_len = wcslen(endval);
                        size_t out_len = before + wcslen(ru->executionLevel) + after_len + 1;
                        wchar_t* outw = malloc(out_len * sizeof(wchar_t));
                        wcsncpy(outw, newManifest, before);
                        outw[before] = L'\0';
                        wcscat(outw, ru->executionLevel);
                        wcscat(outw, endval);

                        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, outw, -1, NULL, 0, NULL, NULL);
                        char* outbuf = malloc(utf8Len);
                        WideCharToMultiByte(CP_UTF8, 0, outw, -1, outbuf, utf8Len, NULL, NULL);
                        if (!UpdateResourceW(hUpdate, RT_MANIFEST, MAKEINTRESOURCEW(1),
                                             MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), outbuf,
                                             (DWORD)(utf8Len - 1))) {
                            free(outbuf);
                            free(outw);
                            free(newManifest);
                            EndUpdateResourceW(hUpdate, TRUE);
                            return 0;
                        }
                        free(outbuf);
                        free(outw);
                    }
                }
            }
        }
        free(newManifest);
    }
    BOOL ok = EndUpdateResourceW(hUpdate, FALSE);
    return ok ? 1 : 0;
}

const wchar_t* ru_get_version_string(ResourceUpdater* ru, const wchar_t* key) {
    DWORD handle = 0;
    DWORD size = GetFileVersionInfoSizeW(ru->filename, &handle);
    if (size == 0)
        return NULL;
    unsigned char* data = malloc(size);
    if (!data)
        return NULL;
    if (!GetFileVersionInfoW(ru->filename, 0, size, data)) {
        free(data);
        return NULL;
    }

    WORD lang = 1033;
    WORD codepage = 1200;
    if (ru->versionStampMap.count > 0) {
        lang = ru->versionStampMap.entries[0].wLanguage;
        codepage = 1200;
    } else {
        LPVOID transbuf = NULL;
        UINT tlen = 0;
        if (VerQueryValueW(data, L"\\VarFileInfo\\Translation", &transbuf, &tlen) && tlen >= sizeof(DWORD)) {
            DWORD tt = ((DWORD*)transbuf)[0];
            lang = (WORD)(tt & 0xFFFF);
            codepage = (WORD)(tt >> 16);
        }
    }
    wchar_t subkey[64];
    swprintf(subkey, 64, L"\\StringFileInfo\\%08x\\%s", (lang << 16) | codepage, key);
    LPVOID buf = NULL;
    UINT len = 0;
    if (!VerQueryValueW(data, subkey, &buf, &len)) {
        free(data);
        return NULL;
    }
    const wchar_t* sval = (const wchar_t*)buf;
    wchar_t* out = _wcsdup(sval);
    free(data);
    return out;
}

static const wchar_t* ru_get_version_string_lang(ResourceUpdater* ru, WORD lang, const wchar_t* key) {

    for (size_t i = 0; i < ru->versionStampMap.count; ++i) {
        if (ru->versionStampMap.entries[i].wLanguage == lang) {
            VersionStampMapEntry* e = &ru->versionStampMap.entries[i];
            for (size_t t = 0; t < e->count; ++t) {
                VersionStringTable* table = &e->tables[t];
                for (size_t s = 0; s < table->count; ++s) {
                    if (wcscmp(table->strings[s].key, key) == 0) {
                        return table->strings[s].value;
                    }
                }
            }
        }
    }

    DWORD handle = 0;
    DWORD size = GetFileVersionInfoSizeW(ru->filename, &handle);
    if (size == 0)
        return NULL;
    unsigned char* data = malloc(size);
    if (!data)
        return NULL;
    if (!GetFileVersionInfoW(ru->filename, 0, size, data)) {
        free(data);
        return NULL;
    }
    wchar_t subkey[64];
    swprintf(subkey, 64, L"\\StringFileInfo\\%08x\\%s", (lang << 16) | 1200, key);
    LPVOID buf = NULL;
    UINT len = 0;
    if (!VerQueryValueW(data, subkey, &buf, &len)) {
        free(data);
        return NULL;
    }
    const wchar_t* sval = (const wchar_t*)buf;
    wchar_t* out = _wcsdup(sval);
    free(data);
    return out;
}

const wchar_t* ru_get_resource_string(ResourceUpdater* ru, unsigned int id) {
    if (ru->stringTableMap.count == 0)
        return NULL;

    StringTableMapEntry* langEntry = &ru->stringTableMap.entries[0];
    UINT blockId = id / 16;
    if (blockId >= langEntry->count)
        return NULL;
    StringTableEntry* table = &langEntry->entries[blockId];
    if (!table->strings)
        return NULL;
    UINT index = id % 16;
    return table->strings[index];
}

int ru_load(ResourceUpdater* ru, const wchar_t* filename) {
    wchar_t abspath[MAX_PATH] = {0};
    if (_wfullpath(abspath, filename, MAX_PATH))
        ru->module = LoadLibraryExW(abspath, NULL, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
    else
        ru->module = LoadLibraryExW(filename, NULL, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
    if (!ru->module)
        return 0;
    if (ru->filename)
        free(ru->filename);
    ru->filename = _wcsdup(filename);

    EnumResourceNamesW(ru->module, RT_STRING, on_enum_name, (LONG_PTR)ru);
    EnumResourceNamesW(ru->module, RT_VERSION, on_enum_name, (LONG_PTR)ru);
    EnumResourceNamesW(ru->module, RT_GROUP_ICON, on_enum_name, (LONG_PTR)ru);
    EnumResourceNamesW(ru->module, RT_ICON, on_enum_name, (LONG_PTR)ru);
    EnumResourceNamesW(ru->module, RT_MANIFEST, on_enum_manifest, (LONG_PTR)ru);
    EnumResourceNamesW(ru->module, RT_RCDATA, on_enum_name, (LONG_PTR)ru);
    return 1;
}

static BOOL CALLBACK on_enum_name(HMODULE hModule, LPCWSTR lpszType, LPWSTR lpszName, LONG_PTR lParam) {
    EnumResourceLanguagesW(hModule, lpszType, lpszName, on_enum_lang, lParam);
    return TRUE;
}

static BOOL CALLBACK on_enum_manifest(HMODULE hModule, LPCTSTR lpType, LPWSTR lpName, LONG_PTR lParam) {
    ResourceUpdater* instance = (ResourceUpdater*)lParam;
    HRSRC hResInfo = FindResourceW(hModule, lpName, lpType);
    if (!hResInfo)
        return TRUE;

    HGLOBAL hResData = LoadResource(hModule, hResInfo);
    const BYTE* pResource = (const BYTE*)LockResource(hResData);

    int len = (int)strlen((const char*)pResource);
    wchar_t* manifestStringLocal = malloc((len + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, (const char*)pResource, -1, manifestStringLocal, (int)(len + 1));

    wchar_t* start = wcsstr(manifestStringLocal, L"<?xml");
    if (start) {
        memmove(manifestStringLocal, start, (wcslen(start) + 1) * sizeof(wchar_t));
    }

    wchar_t* found = wcsstr(manifestStringLocal, L"requestedExecutionLevel");
    if (found) {
        wchar_t* levelAttr = wcsstr(found, L"level=\"");
        if (!levelAttr)
            levelAttr = wcsstr(found, L"level='");
        if (levelAttr) {
            wchar_t quote = (levelAttr[7] == L'\'' ? L'\'' : L'\"');
            wchar_t* startval = wcschr(levelAttr, quote);
            if (startval) {
                wchar_t* endval = wcschr(startval + 1, quote);
                if (endval) {
                    size_t lenval = endval - (startval + 1);
                    free(instance->originalExecutionLevel);
                    instance->originalExecutionLevel = malloc((lenval + 1) * sizeof(wchar_t));
                    wcsncpy(instance->originalExecutionLevel, startval + 1, lenval);
                    instance->originalExecutionLevel[lenval] = L'\0';
                }
            }
        }
    }
    free(instance->manifestString);
    instance->manifestString = manifestStringLocal;
    UnlockResource(hResData);
    FreeResource(hResData);
    return TRUE;
}

static BOOL CALLBACK on_enum_lang(HMODULE hModule, LPCWSTR lpszType, LPCWSTR lpszName, WORD wIDLanguage,
                                  LONG_PTR lParam) {
    ResourceUpdater* instance = (ResourceUpdater*)lParam;
    if (IS_INTRESOURCE(lpszName) && IS_INTRESOURCE(lpszType)) {
        intptr_t type = (intptr_t)lpszType;
        switch (type) {
        case (intptr_t)RT_VERSION: {
            VersionInfo* vi = vinfo_create_from_module(hModule, wIDLanguage);
            if (vi) {
                VersionStampMapEntry* e = vsm_get_or_create(&instance->versionStampMap, wIDLanguage);
                if (e) {

                    e->hasFixed = vi->has_fixed;
                    if (vi->has_fixed)
                        e->fixedInfo = vi->fixedInfo;
                    if (vi->tableCount > 0) {
                        e->tables = malloc(vi->tableCount * sizeof(VersionStringTable));
                        e->count = vi->tableCount;
                        e->capacity = vi->tableCount;
                        for (size_t ti = 0; ti < vi->tableCount; ++ti) {
                            vst_init(&e->tables[ti]);
                            e->tables[ti].encoding = vi->stringTables[ti].encoding;
                            for (size_t si = 0; si < vi->stringTables[ti].count; ++si) {
                                vst_push(&e->tables[ti], vi->stringTables[ti].strings[si].key,
                                         vi->stringTables[ti].strings[si].value);
                            }
                        }
                    }
                    if (vi->supportedCount > 0) {
                        e->supportedTranslations = malloc(vi->supportedCount * sizeof(Translate));
                        if (e->supportedTranslations) {
                            memcpy(e->supportedTranslations, vi->supportedTranslations,
                                   vi->supportedCount * sizeof(Translate));
                            e->supportedCount = vi->supportedCount;
                        }
                    }
                }
                versioninfo_free(vi);
                free(vi);
            }
            break;
        }
        case (intptr_t)RT_STRING: {
            UINT id = (UINT)((uintptr_t)lpszName) - 1;
            StringTableMapEntry* langEntry = stm_get_or_create_lang(&instance->stringTableMap, wIDLanguage);

            if (langEntry) {
                if (id + 1 > langEntry->count) {
                    size_t newcap = (langEntry->capacity == 0) ? 8 : langEntry->capacity * 2;
                    while (newcap <= id)
                        newcap *= 2;
                    StringTableEntry* tmp = realloc(langEntry->entries, newcap * sizeof(StringTableEntry));
                    if (tmp) {

                        for (size_t j = langEntry->count; j < newcap; j++)
                            tmp[j].strings = NULL, tmp[j].blockId = 0;
                        langEntry->entries = tmp;
                        langEntry->capacity = newcap;
                        langEntry->count = id + 1;
                    }
                }
                StringTableEntry* table = &langEntry->entries[id];
                if (table->strings == NULL) {
                    table->strings = malloc(16 * sizeof(wchar_t*));
                    for (int k = 0; k < 16; ++k)
                        table->strings[k] = NULL;
                    table->blockId = id;
                }
                for (int k = 0; k < 16; ++k) {
                    wchar_t buf[4096];
                    int len = LoadStringW(hModule, (int)(id * 16 + k), buf, 4096);
                    if (len > 0) {
                        if (table->strings[k])
                            free(table->strings[k]);
                        table->strings[k] = _wcsdup(buf);
                    }
                }
            }
            break;
        }
        case (intptr_t)RT_ICON: {

            IconTableEntry* found = NULL;
            for (size_t i = 0; i < instance->iconBundleMap.count; ++i)
                if (instance->iconBundleMap.entries[i].lang == wIDLanguage) {
                    found = &instance->iconBundleMap.entries[i];
                    break;
                }
            if (!found) {
                if (instance->iconBundleMap.count + 1 > instance->iconBundleMap.capacity) {
                    size_t newcap = (instance->iconBundleMap.capacity == 0) ? 4 : instance->iconBundleMap.capacity * 2;
                    IconTableEntry* tmp = realloc(instance->iconBundleMap.entries, newcap * sizeof(IconTableEntry));
                    if (!tmp)
                        break;
                    instance->iconBundleMap.entries = tmp;
                    instance->iconBundleMap.capacity = newcap;
                }
                IconTableEntry* e2 = &instance->iconBundleMap.entries[instance->iconBundleMap.count++];
                e2->lang = wIDLanguage;
                e2->info.count = 0;
                e2->info.bundles = NULL;
                e2->info.bundleIds = NULL;
                e2->info.maxIconId = 0;
                found = e2;
            }
            UINT iconId = (UINT)((uintptr_t)lpszName);
            if (iconId > found->info.maxIconId)
                found->info.maxIconId = iconId;
            break;
        }
        case (intptr_t)RT_GROUP_ICON: {
            UINT iconId = (UINT)((uintptr_t)lpszName);
            IconTableEntry* found = NULL;
            for (size_t i = 0; i < instance->iconBundleMap.count; ++i)
                if (instance->iconBundleMap.entries[i].lang == wIDLanguage) {
                    found = &instance->iconBundleMap.entries[i];
                    break;
                }
            if (!found) {
                if (instance->iconBundleMap.count + 1 > instance->iconBundleMap.capacity) {
                    size_t newcap = (instance->iconBundleMap.capacity == 0) ? 4 : instance->iconBundleMap.capacity * 2;
                    IconTableEntry* tmp = realloc(instance->iconBundleMap.entries, newcap * sizeof(IconTableEntry));
                    if (!tmp)
                        break;
                    instance->iconBundleMap.entries = tmp;
                    instance->iconBundleMap.capacity = newcap;
                }
                IconTableEntry* e2 = &instance->iconBundleMap.entries[instance->iconBundleMap.count++];
                e2->lang = wIDLanguage;
                e2->info.count = 0;
                e2->info.bundles = NULL;
                e2->info.bundleIds = NULL;
                e2->info.maxIconId = 0;
                found = e2;
            }

            IconResInfo* inf = &found->info;
            if (inf->count + 1 > inf->capacity) {
                size_t newcap = (inf->capacity == 0) ? 4 : inf->capacity * 2;
                UINT* newIds = realloc(inf->bundleIds, newcap * sizeof(UINT));
                IconsValue** newB = realloc(inf->bundles, newcap * sizeof(IconsValue*));
                if (!newIds || !newB)
                    break;
                inf->bundleIds = newIds;
                inf->bundles = newB;
                inf->capacity = newcap;
            }
            inf->bundleIds[inf->count] = iconId;
            inf->bundles[inf->count] = NULL;
            inf->count++;
            break;
        }
        case (intptr_t)RT_RCDATA: {
            HRSRC hResInfo = FindResource(hModule, lpszName, lpszType);
            if (!hResInfo)
                break;
            DWORD cbResource = SizeofResource(hModule, hResInfo);
            HGLOBAL hResData = LoadResource(hModule, hResInfo);
            const BYTE* pResource = (const BYTE*)LockResource(hResData);

            RcDataLangEntry* langEntry = NULL;
            for (size_t i = 0; i < instance->rcDataLngMap.count; ++i)
                if (instance->rcDataLngMap.entries[i].lang == wIDLanguage) {
                    langEntry = &instance->rcDataLngMap.entries[i];
                    break;
                }
            if (!langEntry) {
                if (instance->rcDataLngMap.count + 1 > instance->rcDataLngMap.capacity) {
                    size_t newcap = (instance->rcDataLngMap.capacity == 0) ? 4 : instance->rcDataLngMap.capacity * 2;
                    RcDataLangEntry* tmp = realloc(instance->rcDataLngMap.entries, newcap * sizeof(RcDataLangEntry));
                    if (!tmp)
                        break;
                    instance->rcDataLngMap.entries = tmp;
                    instance->rcDataLngMap.capacity = newcap;
                }
                RcDataLangEntry* e = &instance->rcDataLngMap.entries[instance->rcDataLngMap.count++];
                e->lang = wIDLanguage;
                e->entries = NULL;
                e->count = 0;
                e->capacity = 0;
                langEntry = e;
            }

            if (langEntry->count + 1 > langEntry->capacity) {
                size_t newcap = (langEntry->capacity == 0) ? 4 : langEntry->capacity * 2;
                RcDataEntry* tmp = realloc(langEntry->entries, newcap * sizeof(RcDataEntry));
                if (!tmp)
                    break;
                langEntry->entries = tmp;
                langEntry->capacity = newcap;
            }
            RcDataEntry* rde = &langEntry->entries[langEntry->count++];
            rde->id = (ptrdiff_t)lpszName;
            bytevec_init(&rde->data);
            bytevec_reserve(&rde->data, cbResource);
            memcpy(rde->data.data, pResource, cbResource);
            rde->data.size = cbResource;
            UnlockResource(hResData);
            FreeResource(hResData);
            break;
        }
        default:
            break;
        }
    }
    return TRUE;
}
