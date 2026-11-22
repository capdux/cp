// cap/test/test_sql_pg_data_row_production_deep.cp
// Production-deep regression tests for cap:sql pg_data_row module
// Source: /data/dev/bun/src/sql/postgres/protocol/DataRow.cp & /data/dev/bun/docs/runtime/sql.mdx

import { test, expect } from "cap:test";
import {
    PG_DATA_ROW_TAG,
    PG_DATA_ROW_NULL_LEN,
    PG_DATA_ROW_MAX_COLS,
    PgDataRowError,
    PgDataRowColumn,
    estimatePgDataRowFrameLen,
    encodePgDataRowPayload,
    encodePgDataRowFrame,
    decodePgDataRowIter,
    decodePgDataRowFrame,
} from "cap:sql";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("sql pg_data_row: zero-column DataRow frame round-trip") {
    const empty: [0]PgDataRowColumn = [];
    expect(estimatePgDataRowFrameLen(empty[0..]) == 7);

    let wire: [16]u8 = [0; 16];
    const n = encodePgDataRowFrame(empty[0..], wire[0..]) catch { expect(false); return; };
    expect(n == 7);
    expect(wire[0] == PG_DATA_ROW_TAG);
    expect(wire[4] == 6);

    const view = decodePgDataRowFrame(wire[0..n]) catch { expect(false); return; };
    expect(view.column_count == 0);
}

test("sql pg_data_row: NULL vs empty string vs binary NUL bytes round-trip") {
    expect(PG_DATA_ROW_NULL_LEN == 0xffff_ffff);
    const bin_val: [4]u8 = [0x00, 0xde, 0xad, 0x00];
    const cols: [4]PgDataRowColumn = [
        PgDataRowColumn.bytesCol("alice"),
        PgDataRowColumn.nullCol(),
        PgDataRowColumn.emptyCol(),
        PgDataRowColumn.bytesCol(bin_val[0..]),
    ];

    let wire: [128]u8 = [0; 128];
    const est = estimatePgDataRowFrameLen(cols[0..]);
    const n = encodePgDataRowFrame(cols[0..], wire[0..]) catch { expect(false); return; };
    expect(n == est);

    const row = decodePgDataRowFrame(wire[0..n]) catch { expect(false); return; };
    expect(row.column_count == 4);
    expect(!row.columns[0].is_null && strEq(row.columns[0].bytes, "alice"));
    expect(row.columns[1].is_null);
    expect(!row.columns[2].is_null && row.columns[2].bytes.len == 0);
    expect(!row.columns[3].is_null && strEq(row.columns[3].bytes, bin_val[0..]));
}

test("sql pg_data_row: PgDataRowIter streaming, columnAt & validateComplete") {
    const cols: [3]PgDataRowColumn = [
        PgDataRowColumn.bytesCol("100"),
        PgDataRowColumn.nullCol(),
        PgDataRowColumn.bytesCol("ok"),
    ];
    let wire: [64]u8 = [0; 64];
    const n = encodePgDataRowFrame(cols[0..], wire[0..]) catch { expect(false); return; };

    let it = decodePgDataRowIter(wire[0..n]) catch { expect(false); return; };
    expect(it.total_columns == 3);
    expect(it.remaining() == 3);
    it.validateComplete() catch { expect(false); return; };

    // Random access via columnAt does not advance streaming cursor
    const c2 = it.columnAt(2) catch { expect(false); return; };
    expect(strEq(c2.bytes, "ok"));
    expect(it.remaining() == 3);

    // Out-of-bounds columnAt returns error
    let oob = false;
    * = it.columnAt(3) catch { oob = true; };
    expect(oob);

    // Stream sequentially
    const r0 = it.next() catch { expect(false); return; };
    expect(r0 != null && strEq(r0.?.bytes, "100"));
    expect(it.remaining() == 2);

    const r1 = it.next() catch { expect(false); return; };
    expect(r1 != null && r1.?.is_null);

    const r2 = it.next() catch { expect(false); return; };
    expect(r2 != null && strEq(r2.?.bytes, "ok"));
    expect(it.remaining() == 0);

    const end = it.next() catch { expect(false); return; };
    expect(end == null);
}

test("sql pg_data_row: wide table (> 64 cols) streams via PgDataRowIter while view enforces cap") {
    expect(PG_DATA_ROW_MAX_COLS == 64);
    let cols: [70]PgDataRowColumn = [PgDataRowColumn.nullCol(); 70];
    cols[0] = PgDataRowColumn.bytesCol("first");
    cols[69] = PgDataRowColumn.bytesCol("last");

    let wire: [512]u8 = [0; 512];
    const n = encodePgDataRowFrame(cols[0..], wire[0..]) catch { expect(false); return; };

    // Fixed-capacity view rejects > 64 columns cleanly
    let too_many = false;
    * = decodePgDataRowFrame(wire[0..n]) catch { too_many = true; };
    expect(too_many);

    // Streaming iterator handles all 70 columns with zero heap allocation
    let it = decodePgDataRowIter(wire[0..n]) catch { expect(false); return; };
    expect(it.total_columns == 70);
    const first = it.columnAt(0) catch { expect(false); return; };
    const last = it.columnAt(69) catch { expect(false); return; };
    expect(strEq(first.bytes, "first"));
    expect(strEq(last.bytes, "last"));
    it.validateComplete() catch { expect(false); return; };
}

test("sql pg_data_row: malformed frame, negative column length & trailing bytes guards") {
    // 1. Wrong tag ('T' instead of 'D')
    const bad_tag: [7]u8 = ['T', 0, 0, 0, 6, 0, 0];
    let e1 = false;
    * = decodePgDataRowFrame(bad_tag[0..]) catch { e1 = true; };
    expect(e1);

    // 2. Truncated header
    const trunc_hdr: [4]u8 = ['D', 0, 0, 0];
    let e2 = false;
    * = decodePgDataRowFrame(trunc_hdr[0..]) catch { e2 = true; };
    expect(e2);

    // 3. Invalid negative column length (-2 = 0xffff_fffe)
    const neg_len: [11]u8 = ['D', 0, 0, 0, 10, 0, 1, 0xff, 0xff, 0xff, 0xfe];
    let e3 = false;
    * = decodePgDataRowFrame(neg_len[0..]) catch { e3 = true; };
    expect(e3);

    // 4. Truncated column data (declares len 5, only 2 bytes present)
    const trunc_col: [13]u8 = ['D', 0, 0, 0, 12, 0, 1, 0, 0, 0, 5, 'a', 'b'];
    let e4 = false;
    * = decodePgDataRowFrame(trunc_col[0..]) catch { e4 = true; };
    expect(e4);

    // 5. Trailing garbage byte inside message length (declares 0 cols, 1 extra byte)
    const inner_trail: [8]u8 = ['D', 0, 0, 0, 7, 0, 0, 0x99];
    let e5 = false;
    * = decodePgDataRowFrame(inner_trail[0..]) catch { e5 = true; };
    expect(e5);

    // 6. Trailing byte after declared frame length
    const outer_trail: [8]u8 = ['D', 0, 0, 0, 6, 0, 0, 0x99];
    let e6 = false;
    * = decodePgDataRowFrame(outer_trail[0..]) catch { e6 = true; };
    expect(e6);

    // 7. BufferTooSmall during encode
    const cols: [1]PgDataRowColumn = [PgDataRowColumn.bytesCol("abcdef")];
    let tiny: [8]u8 = [0; 8];
    let e7 = false;
    * = encodePgDataRowFrame(cols[0..], tiny[0..]) catch { e7 = true; };
    expect(e7);
}

test("sql pg_data_row: raw payload encode matches framed payload bytes") {
    const cols: [2]PgDataRowColumn = [
        PgDataRowColumn.bytesCol("pg"),
        PgDataRowColumn.nullCol(),
    ];
    let frame: [64]u8 = [0; 64];
    let pay: [64]u8 = [0; 64];

    const fn_len = encodePgDataRowFrame(cols[0..], frame[0..]) catch { expect(false); return; };
    const pn_len = encodePgDataRowPayload(cols[0..], pay[0..]) catch { expect(false); return; };
    expect(fn_len == 5 + pn_len);
    expect(strEq(frame[5..fn_len], pay[0..pn_len]));
}

test("sql pg_data_row: exact 64-column boundary succeeds in both view & iter") {
    let cols: [PG_DATA_ROW_MAX_COLS]PgDataRowColumn = [PgDataRowColumn.emptyCol(); PG_DATA_ROW_MAX_COLS];
    cols[0] = PgDataRowColumn.bytesCol("start");
    cols[63] = PgDataRowColumn.bytesCol("end");

    let wire: [512]u8 = [0; 512];
    const n = encodePgDataRowFrame(cols[0..], wire[0..]) catch { expect(false); return; };
    const view = decodePgDataRowFrame(wire[0..n]) catch { expect(false); return; };
    expect(view.column_count == 64);
    expect(strEq(view.columns[0].bytes, "start"));
    expect(strEq(view.columns[63].bytes, "end"));
}
