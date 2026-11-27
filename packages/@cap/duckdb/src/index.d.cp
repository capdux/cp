// Type definitions for CP

import {  } from "std";
export fn duckdb_open(path: [*:0]&u8!?, out_database: **auto!?!): i32;
export fn duckdb_close(database: **auto!?!);
export fn duckdb_connect(database: *auto!?, out_connection: **auto!?!): i32;
export fn duckdb_disconnect(connection: **auto!?!);
export fn duckdb_query(connection: *auto!?, query: [*:0]&u8!, out_result: *auto!?): i32;
export fn duckdb_destroy_result(result: *auto!?);
export fn duckdb_row_count(result: *auto!?): u64;
export fn duckdb_column_count(result: *auto!?): u64;
export fn duckdb_value_int64(result: *auto!?, col: u64, row: u64): i64;
export fn duckdb_value_double(result: *auto!?, col: u64, row: u64): f64;
export fn duckdb_value_varchar(result: *auto!?, col: u64, row: u64): [*:0]u8!?;
export fn duckdb_free(ptr: *auto!?);
export fn duckdb_library_version(): [*:0]&u8!;
export fn cp_duckdb_query(conn: *auto!?, sql: [*:0]&u8!, out_state: *i32!): *auto!?;
export fn cp_duckdb_row_count(res: *auto!?): u64;
export fn cp_duckdb_col_count(res: *auto!?): u64;
export fn cp_duckdb_val_i64(res: *auto!?, col: u64, row: u64): i64;
export fn cp_duckdb_val_f64(res: *auto!?, col: u64, row: u64): f64;
export fn cp_duckdb_val_str(res: *auto!?, col: u64, row: u64): [*:0]u8!?;
export fn cp_duckdb_free_str(ptr: [*:0]u8!?);
export fn cp_duckdb_result_free(res: *auto!?);
export error DuckError { OpenFailed, ConnectFailed, QueryFailed, ClosedHandle, OutOfMemory }

export fn libraryVersion(): string;
export class DuckString {
    ptr: [*:0]u8!?,
    fn asSlice(): string;
    fn __cp_deinit(_: &mut DuckString);
}

export class DuckResult {
    res_ptr: *auto!?,
    fn rowCount(): u64;
    fn colCount(): u64;
    fn valueI64(col: u64, row: u64): i64;
    fn valueF64(col: u64, row: u64): f64;
    fn valueString(col: u64, row: u64): DuckString;
    fn __cp_deinit(_: &mut DuckResult);
}

export class DuckConn {
    conn_ptr: *auto!?,
    fn query(sql: string): !DuckResult;
    fn exec(sql: string): !void;
    fn __cp_deinit(_: &mut DuckConn);
}

export class DuckDatabase {
    db_ptr: *auto!?,
    fn open(path: string): !DuckDatabase;
    fn connect(): !DuckConn;
    fn __cp_deinit(_: &mut DuckDatabase);
}

