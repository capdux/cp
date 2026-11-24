// Thin FFI shim over libyaml 0.2.5 to avoid large C struct ABI layout coupling in CP
#include "yaml.h"
#include <stdlib.h>

typedef struct cp_yaml_parser {
    yaml_parser_t parser;
    yaml_event_t event;
    int has_event;
    int initialized;
    int stream_ended;
} cp_yaml_parser;

cp_yaml_parser *cp_yaml_parse_events(const unsigned char *input, size_t size) {
    cp_yaml_parser *p = (cp_yaml_parser *)calloc(1, sizeof(cp_yaml_parser));
    if (!p) return NULL;
    if (!yaml_parser_initialize(&p->parser)) {
        free(p);
        return NULL;
    }
    p->initialized = 1;
    yaml_parser_set_input_string(&p->parser, input, size);
    return p;
}

void cp_yaml_parser_free(cp_yaml_parser *p) {
    if (!p) return;
    if (p->has_event) {
        yaml_event_delete(&p->event);
        p->has_event = 0;
    }
    if (p->initialized) {
        yaml_parser_delete(&p->parser);
        p->initialized = 0;
    }
    free(p);
}

// Returns: 1 = event available, 0 = stream already ended (EOF), -1 = parse error
int cp_yaml_next_event(cp_yaml_parser *p) {
    if (!p || !p->initialized) return -1;
    if (p->stream_ended) return 0;
    if (p->has_event) {
        yaml_event_delete(&p->event);
        p->has_event = 0;
    }
    if (!yaml_parser_parse(&p->parser, &p->event)) {
        return -1;
    }
    p->has_event = 1;
    if (p->event.type == YAML_STREAM_END_EVENT) {
        p->stream_ended = 1;
    }
    return 1;
}

int cp_yaml_event_type(const cp_yaml_parser *p) {
    if (!p || !p->has_event) return YAML_NO_EVENT;
    return (int)p->event.type;
}

const char *cp_yaml_event_scalar(const cp_yaml_parser *p, size_t *out_len) {
    if (out_len) *out_len = 0;
    if (!p || !p->has_event || p->event.type != YAML_SCALAR_EVENT) return NULL;
    if (out_len) *out_len = p->event.data.scalar.length;
    return (const char *)p->event.data.scalar.value;
}

const char *cp_yaml_error_problem(const cp_yaml_parser *p) {
    if (!p || !p->initialized) return NULL;
    return p->parser.problem;
}

const char *cp_yaml_version_string(void) {
    return yaml_get_version_string();
}
