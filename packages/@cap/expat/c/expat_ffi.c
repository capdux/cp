// @cap/expat thin C FFI bridge for CP
// License: MIT

#include "expat.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
    size_t count;
} cp_expat_counter_t;

static void XMLCALL cp_expat_start_elem(void *user_data, const XML_Char *name, const XML_Char **atts) {
    (void)name;
    (void)atts;
    cp_expat_counter_t *ctx = (cp_expat_counter_t *)user_data;
    ctx->count += 1;
}

const char *cp_expat_version(void) {
    return XML_ExpatVersion();
}

int32_t cp_expat_count_elements(const uint8_t *xml, size_t len, size_t *out_elem_count) {
    if (!out_elem_count) {
        return -1;
    }
    *out_elem_count = 0;
    if (!xml && len > 0) {
        return -1;
    }

    XML_Parser parser = XML_ParserCreate(NULL);
    if (!parser) {
        return -2;
    }

    cp_expat_counter_t ctx;
    ctx.count = 0;
    XML_SetUserData(parser, &ctx);
    XML_SetElementHandler(parser, cp_expat_start_elem, NULL);

    enum XML_Status st = XML_Parse(parser, (const char *)xml, (int)len, 1);
    XML_ParserFree(parser);

    if (st != XML_STATUS_OK) {
        return -3;
    }
    *out_elem_count = ctx.count;
    return 0;
}
