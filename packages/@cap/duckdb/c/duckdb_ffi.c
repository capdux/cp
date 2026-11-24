// @cap/duckdb — Minimal heap-allocated ABI shim for duckdb_result (~35 lines)
// Avoids mirroring large duckdb_result layout in CP; links against libduckdb.so.
#include <stdlib.h>
#include <stdint.h>
#include "duckdb.h"

void *cp_duckdb_query(void *conn, const char *sql, int32_t *out_state) {
    duckdb_result *res = (duckdb_result *)calloc(1, sizeof(duckdb_result));
    if (!res) {
        if (out_state) *out_state = (int32_t)DuckDBError;
        return NULL;
    }
    duckdb_state st = duckdb_query((duckdb_connection)conn, sql, res);
    if (out_state) *out_state = (int32_t)st;
    if (st != DuckDBSuccess) {
        duckdb_destroy_result(res);
        free(res);
        return NULL;
    }
    return (void *)res;
}

uint64_t cp_duckdb_row_count(void *res) {
    return res ? (uint64_t)duckdb_row_count((duckdb_result *)res) : 0;
}

uint64_t cp_duckdb_col_count(void *res) {
    return res ? (uint64_t)duckdb_column_count((duckdb_result *)res) : 0;
}

int64_t cp_duckdb_val_i64(void *res, uint64_t col, uint64_t row) {
    return res ? duckdb_value_int64((duckdb_result *)res, (idx_t)col, (idx_t)row) : 0;
}

double cp_duckdb_val_f64(void *res, uint64_t col, uint64_t row) {
    return res ? duckdb_value_double((duckdb_result *)res, (idx_t)col, (idx_t)row) : 0.0;
}

char *cp_duckdb_val_str(void *res, uint64_t col, uint64_t row) {
    return res ? duckdb_value_varchar((duckdb_result *)res, (idx_t)col, (idx_t)row) : NULL;
}

void cp_duckdb_free_str(char *ptr) {
    if (ptr) duckdb_free((void *)ptr);
}

void cp_duckdb_result_free(void *res) {
    if (res) {
        duckdb_destroy_result((duckdb_result *)res);
        free(res);
    }
}
