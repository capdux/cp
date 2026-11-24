// Thin non-inline FFI shim exporting yyjson.h static inline helpers for CP @c FFI
#include "yyjson.h"
#include <stdlib.h>

yyjson_doc *cp_yyjson_read(const char *dat, size_t len, yyjson_read_flag flg) {
    return yyjson_read(dat, len, flg);
}
void cp_yyjson_doc_free(yyjson_doc *doc) { yyjson_doc_free(doc); }
yyjson_val *cp_yyjson_doc_get_root(yyjson_doc *doc) { return yyjson_doc_get_root(doc); }
size_t cp_yyjson_doc_get_read_size(yyjson_doc *doc) { return yyjson_doc_get_read_size(doc); }
size_t cp_yyjson_doc_get_val_count(yyjson_doc *doc) { return yyjson_doc_get_val_count(doc); }
char *cp_yyjson_write(const yyjson_doc *doc, yyjson_write_flag flg, size_t *len) {
    return yyjson_write(doc, flg, len);
}

bool cp_yyjson_is_null(yyjson_val *val) { return yyjson_is_null(val); }
bool cp_yyjson_is_bool(yyjson_val *val) { return yyjson_is_bool(val); }
bool cp_yyjson_is_int(yyjson_val *val) { return yyjson_is_int(val); }
bool cp_yyjson_is_real(yyjson_val *val) { return yyjson_is_real(val); }
bool cp_yyjson_is_str(yyjson_val *val) { return yyjson_is_str(val); }
bool cp_yyjson_is_arr(yyjson_val *val) { return yyjson_is_arr(val); }
bool cp_yyjson_is_obj(yyjson_val *val) { return yyjson_is_obj(val); }

bool cp_yyjson_get_bool(yyjson_val *val) { return yyjson_get_bool(val); }
int64_t cp_yyjson_get_sint(yyjson_val *val) { return yyjson_get_sint(val); }
double cp_yyjson_get_real(yyjson_val *val) { return yyjson_get_real(val); }
const char *cp_yyjson_get_str(yyjson_val *val) { return yyjson_get_str(val); }
size_t cp_yyjson_get_len(yyjson_val *val) { return yyjson_get_len(val); }

size_t cp_yyjson_obj_size(yyjson_val *obj) { return yyjson_obj_size(obj); }
yyjson_val *cp_yyjson_obj_getn(yyjson_val *obj, const char *key, size_t key_len) {
    return yyjson_obj_getn(obj, key, key_len);
}
size_t cp_yyjson_arr_size(yyjson_val *arr) { return yyjson_arr_size(arr); }
yyjson_val *cp_yyjson_arr_get(yyjson_val *arr, size_t idx) { return yyjson_arr_get(arr, idx); }
void cp_yyjson_free_str(void *ptr) { free(ptr); }
