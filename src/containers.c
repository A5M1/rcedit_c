#include "rcedit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void bytevec_init(ByteVec* v) {
    v->data = NULL;
    v->size = 0;
    v->capacity = 0;
}
void bytevec_free(ByteVec* v) {
    free(v->data);
    v->data = NULL;
    v->size = 0;
    v->capacity = 0;
}
int bytevec_reserve(ByteVec* v, size_t sz) {
    if (v->capacity >= sz)
        return 1;
    size_t newcap = (v->capacity == 0) ? 64 : v->capacity * 2;
    while (newcap < sz)
        newcap *= 2;
    uint8_t* tmp = realloc(v->data, newcap);
    if (!tmp)
        return 0;
    v->data = tmp;
    v->capacity = newcap;
    return 1;
}
void vst_init(VersionStringTable* t) {
    t->strings = NULL;
    t->count = 0;
    t->capacity = 0;
}
void vst_free(VersionStringTable* t) {
    for (size_t i = 0; i < t->count; ++i) {
        free(t->strings[i].key);
        free(t->strings[i].value);
    }
    free(t->strings);
    t->strings = NULL;
    t->count = 0;
    t->capacity = 0;
}
int vst_push(VersionStringTable* t, const wchar_t* key, const wchar_t* value) {
    if (t->count + 1 > t->capacity) {
        size_t newcap = (t->capacity == 0) ? 8 : t->capacity * 2;
        VersionString* tmp = realloc(t->strings, newcap * sizeof(VersionString));
        if (!tmp)
            return 0;
        t->strings = tmp;
        t->capacity = newcap;
    }
    t->strings[t->count].key = _wcsdup(key);
    t->strings[t->count].value = _wcsdup(value);
    t->count++;
    return 1;
}

void vsm_init(VersionStampMap* m) {
    m->entries = NULL;
    m->count = 0;
    m->capacity = 0;
}
void vsm_free(VersionStampMap* m) {
    for (size_t i = 0; i < m->count; ++i) {
        for (size_t j = 0; j < m->entries[i].count; ++j)
            vst_free(&m->entries[i].tables[j]);
        free(m->entries[i].tables);
        free(m->entries[i].supportedTranslations);
    }
    free(m->entries);
    m->entries = NULL;
    m->count = 0;
    m->capacity = 0;
}
VersionStampMapEntry* vsm_get_or_create(VersionStampMap* m, WORD lang) {
    for (size_t i = 0; i < m->count; ++i)
        if (m->entries[i].wLanguage == lang)
            return &m->entries[i];
    if (m->count + 1 > m->capacity) {
        size_t newcap = (m->capacity == 0) ? 4 : m->capacity * 2;
        VersionStampMapEntry* tmp = realloc(m->entries, newcap * sizeof(VersionStampMapEntry));
        if (!tmp)
            return NULL;
        m->entries = tmp;
        m->capacity = newcap;
    }
    VersionStampMapEntry* e = &m->entries[m->count++];
    e->wLanguage = lang;
    e->count = 0;
    e->capacity = 0;
    e->tables = NULL;
    e->hasFixed = 0;
    memset(&e->fixedInfo, 0, sizeof(e->fixedInfo));
    e->supportedTranslations = NULL;
    e->supportedCount = 0;
    return e;
}
void stm_init(StringTableMap* m) {
    m->entries = NULL;
    m->count = 0;
    m->capacity = 0;
}
void stm_free(StringTableMap* m) {
    for (size_t i = 0; i < m->count; ++i) {
        for (size_t j = 0; j < m->entries[i].count; ++j) {
            if (m->entries[i].entries[j].strings) {
                for (int k = 0; k < 16; k++) {
                    free(m->entries[i].entries[j].strings[k]);
                }
                free(m->entries[i].entries[j].strings);
            }
        }
        free(m->entries[i].entries);
    }
    free(m->entries);
    m->entries = NULL;
    m->count = 0;
    m->capacity = 0;
}
StringTableMapEntry* stm_get_or_create_lang(StringTableMap* m, WORD lang) {
    for (size_t i = 0; i < m->count; ++i)
        if (m->entries[i].lang == lang)
            return &m->entries[i];
    if (m->count + 1 > m->capacity) {
        size_t newcap = (m->capacity == 0) ? 4 : m->capacity * 2;
        StringTableMapEntry* tmp = realloc(m->entries, newcap * sizeof(StringTableMapEntry));
        if (!tmp) {
            return NULL;
        }
        m->entries = tmp;
        m->capacity = newcap;
    }
    StringTableMapEntry* e = &m->entries[m->count++];
    e->lang = lang;
    e->entries = NULL;
    e->count = 0;
    e->capacity = 0;
    return e;
}
void rcdmap_init(RcDataLangMap* m) {
    m->entries = NULL;
    m->count = 0;
    m->capacity = 0;
}
void rcdmap_free(RcDataLangMap* m) {
    for (size_t i = 0; i < m->count; ++i) {
        for (size_t j = 0; j < m->entries[i].count; ++j)
            bytevec_free(&m->entries[i].entries[j].data);
        free(m->entries[i].entries);
    }
    free(m->entries);
    m->entries = NULL;
    m->count = 0;
    m->capacity = 0;
}
void itable_init(IconTableMap* m) {
    m->entries = NULL;
    m->count = 0;
    m->capacity = 0;
}
void itable_free(IconTableMap* m) {
    for (size_t i = 0; i < m->count; ++i) {
        for (size_t j = 0; j < m->entries[i].info.count; ++j) {
            if (m->entries[i].info.bundles[j]) {
                for (int k = 0; k < m->entries[i].info.bundles[j]->header.count; ++k)
                    bytevec_free(&m->entries[i].info.bundles[j]->images[k]);
                free(m->entries[i].info.bundles[j]->images);
                bytevec_free(&m->entries[i].info.bundles[j]->grpHeader);
                free(m->entries[i].info.bundles[j]);
            }
        }
        free(m->entries[i].info.bundleIds);
        free(m->entries[i].info.bundles);
    }
    free(m->entries);
    m->entries = NULL;
    m->count = 0;
    m->capacity = 0;
}
