// Production deep test suite for cap:pgwire Extended Query Parse ('P') & 5-byte control signal frames.

import { test, expect } from "cap:test";
import {
    PG_TAG_PARSE,
    PG_TAG_SYNC,
    PG_TAG_FLUSH,
    PG_TAG_PARSE_COMPLETE,
    PG_TAG_BIND_COMPLETE,
    PG_TAG_NO_DATA,
    PG_TAG_EMPTY_QUERY,
    PG_TAG_PORTAL_SUSPENDED,
    PG_EMPTY_FRAME_LEN,
    PG_PARSE_MAX_PARAM_OIDS,
    PgEmptySignalKind,
    parseEmptySignalKind,
    PgParseFrame,
    PgEmptySignalFrame,
} from "cap:pgwire";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("pgwire PgParseFrame: named prepared statement with typed OIDs roundtrip", fn() {
    let p = PgParseFrame.init("stmt_get_order", "SELECT * FROM orders WHERE tenant_id = $1 AND status = $2");
    const i0 = p.addParamOid(20) catch unreachable; // INT8
    const i1 = p.addParamOid(25) catch unreachable; // TEXT
    const i2 = p.addParamOid(0) catch unreachable;  // Unspecified (server-inferred)
    expect(i0 == 0 && i1 == 1 && i2 == 2);
    expect(!p.isUnnamed());
    expect(p.paramCount() == 3);

    let buf: [256]u8 = [0; 256];
    const n = p.encodeFrame(buf[0..]) catch unreachable;
    expect(n == p.encodedLen());
    expect(buf[0] == PG_TAG_PARSE);

    const decoded = PgParseFrame.decodeFrame(buf[0..n]) catch unreachable;
    expect(decoded.eql(p));
    expect(strEql(decoded.statement_name, "stmt_get_order"));
    expect(strEql(decoded.query, "SELECT * FROM orders WHERE tenant_id = $1 AND status = $2"));
    expect(decoded.paramCount() == 3);
    if (o0 := decoded.paramOidAt(0)) { expect(o0 == 20); } else { expect(false); }
    if (o1 := decoded.paramOidAt(1)) { expect(o1 == 25); } else { expect(false); }
    if (o2 := decoded.paramOidAt(2)) { expect(o2 == 0); } else { expect(false); }
    expect(decoded.paramOidAt(3) == null);
});

test("pgwire PgParseFrame: unnamed statement with zero params roundtrip", fn() {
    const p = PgParseFrame.unnamed("SELECT 1");
    expect(p.isUnnamed());
    expect(p.paramCount() == 0);

    let buf: [64]u8 = [0; 64];
    const n = p.encodeFrame(buf[0..]) catch unreachable;
    expect(n == 5 + 1 + 9 + 2);
    const decoded = PgParseFrame.decodeFrame(buf[0..n]) catch unreachable;
    expect(decoded.eql(p));
    expect(decoded.isUnnamed());
    expect(strEql(decoded.query, "SELECT 1"));
    expect(decoded.paramCount() == 0);
});

test("pgwire PgEmptySignalFrame: all 7 extended query 5-byte control signals roundtrip", fn() {
    expect(PG_TAG_SYNC == 'S' && PG_TAG_FLUSH == 'H');
    expect(PG_TAG_PARSE_COMPLETE == '1' && PG_TAG_BIND_COMPLETE == '2');
    expect(PG_TAG_NO_DATA == 'n' && PG_TAG_EMPTY_QUERY == 'I' && PG_TAG_PORTAL_SUSPENDED == 's');
    expect(parseEmptySignalKind('X') == null);

    const kinds: [7]PgEmptySignalKind = [
        PgEmptySignalKind.Sync,
        PgEmptySignalKind.Flush,
        PgEmptySignalKind.ParseComplete,
        PgEmptySignalKind.BindComplete,
        PgEmptySignalKind.NoData,
        PgEmptySignalKind.EmptyQueryResponse,
        PgEmptySignalKind.PortalSuspended,
    ];

    let buf: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < 7) {
        const sig = PgEmptySignalFrame.init(kinds[i]);
        expect(sig.encodedLen() == PG_EMPTY_FRAME_LEN);
        const n = sig.encodeFrame(buf[0..]) catch unreachable;
        expect(n == 5);
        expect(buf[1] == 0 && buf[2] == 0 && buf[3] == 0 && buf[4] == 4);

        const any_dec = PgEmptySignalFrame.decodeAny(buf[0..n]) catch unreachable;
        expect(any_dec.eql(sig));

        const strict_dec = PgEmptySignalFrame.decodeFrame(buf[0..n], kinds[i]) catch unreachable;
        expect(strict_dec.eql(sig));
        i += 1;
    }
});

test("pgwire PgParseFrame: malformed frame header, truncation, and trailing bytes guards", fn() {
    // 1. Truncated header (< 9 bytes)
    const short_f: [8]u8 = ['P', 0, 0, 0, 8, 0, 0, 0];
    let e1 = false;
    * = PgParseFrame.decodeFrame(short_f[0..]) catch { e1 = true; };
    expect(e1);

    // 2. Wrong tag
    const bad_tag: [9]u8 = ['B', 0, 0, 0, 8, 0, 0, 0, 0];
    let e2 = false;
    * = PgParseFrame.decodeFrame(bad_tag[0..]) catch { e2 = true; };
    expect(e2);

    // 3. Missing second NUL terminator
    const no_nul2: [9]u8 = ['P', 0, 0, 0, 8, 0, 'a', 'b', 'c'];
    let e3 = false;
    * = PgParseFrame.decodeFrame(no_nul2[0..]) catch { e3 = true; };
    expect(e3);

    // 4. Declared param_count = 1 but 0 OID bytes follow
    const trunc_oid: [9]u8 = ['P', 0, 0, 0, 8, 0, 0, 0, 1];
    let e4 = false;
    * = PgParseFrame.decodeFrame(trunc_oid[0..]) catch { e4 = true; };
    expect(e4);

    // 5. Trailing bytes after OIDs
    const trail_oid: [10]u8 = ['P', 0, 0, 0, 9, 0, 0, 0, 0, 0xFF];
    let e5 = false;
    * = PgParseFrame.decodeFrame(trail_oid[0..]) catch { e5 = true; };
    expect(e5);

    // 6. Declared param_count > PG_PARSE_MAX_PARAM_OIDS (65)
    const too_many: [9]u8 = ['P', 0, 0, 0, 8, 0, 0, 0, 65];
    let e6 = false;
    * = PgParseFrame.decodeFrame(too_many[0..]) catch { e6 = true; };
    expect(e6);
});

test("pgwire PgParseFrame: embedded NUL, capacity limit, and buffer guards", fn() {
    let out: [64]u8 = [0; 64];
    const bad: [3]u8 = ['a', 0, 'b'];

    // 1. Embedded NUL in statement_name
    let e1 = false;
    * = PgParseFrame.init(bad[0..], "SELECT 1").encodeFrame(out[0..]) catch { e1 = true; };
    expect(e1);

    // 2. Embedded NUL in query
    let e2 = false;
    * = PgParseFrame.init("s", bad[0..]).encodeFrame(out[0..]) catch { e2 = true; };
    expect(e2);

    // 3. Buffer too small
    let tiny: [8]u8 = [0; 8];
    let e3 = false;
    * = PgParseFrame.unnamed("SELECT 1").encodeFrame(tiny[0..]) catch { e3 = true; };
    expect(e3);

    // 4. Max capacity (> PG_PARSE_MAX_PARAM_OIDS)
    let full = PgParseFrame.unnamed("SELECT 1");
    let i: usize = 0;
    while (i < PG_PARSE_MAX_PARAM_OIDS) {
        * = full.addParamOid(23) catch unreachable;
        i += 1;
    }
    expect(full.paramCount() == 64);
    let e4 = false;
    * = full.addParamOid(23) catch { e4 = true; };
    expect(e4);
});

test("pgwire PgEmptySignalFrame: invalid tag, length, mismatch, and buffer guards", fn() {
    let tiny: [4]u8 = [0; 4];
    let e_buf = false;
    * = PgEmptySignalFrame.init(PgEmptySignalKind.Sync).encodeFrame(tiny[0..]) catch { e_buf = true; };
    expect(e_buf);

    const bad_tag: [5]u8 = ['Q', 0, 0, 0, 4];
    let e1 = false;
    * = PgEmptySignalFrame.decodeAny(bad_tag[0..]) catch { e1 = true; };
    expect(e1);

    const bad_len: [5]u8 = ['S', 0, 0, 0, 5];
    let e2 = false;
    * = PgEmptySignalFrame.decodeAny(bad_len[0..]) catch { e2 = true; };
    expect(e2);

    const sync_frame: [5]u8 = ['S', 0, 0, 0, 4];
    let e3 = false;
    * = PgEmptySignalFrame.decodeFrame(sync_frame[0..], PgEmptySignalKind.Flush) catch { e3 = true; };
    expect(e3);

    const trail: [6]u8 = ['1', 0, 0, 0, 4, 0];
    let e4 = false;
    * = PgEmptySignalFrame.decodeAny(trail[0..]) catch { e4 = true; };
    expect(e4);
});
