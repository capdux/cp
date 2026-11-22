// cap/test/test_sql_pg_extended_production_deep.cp
// Deep production tests for cap:sql PostgreSQL v3 Extended Query Protocol Codec (pg_extended.cp)

import { test, expect_eq, expect_true, expect_false } from "cap:test";
import {
    PG_PARSE_TAG,
    PG_DESCRIBE_TAG,
    PG_EXECUTE_TAG,
    PG_CLOSE_TAG,
    PG_SYNC_TAG,
    PG_FLUSH_TAG,
    PG_TARGET_PORTAL,
    PG_TARGET_STATEMENT,
    PgTargetKind,
    PgTargetRef,
    PgWireReader,
    encodePgParse,
    decodePgParse,
    encodePgDescribe,
    decodePgDescribe,
    encodePgExecute,
    decodePgExecute,
    encodePgClose,
    decodePgClose,
    encodePgSync,
    decodePgSync,
    encodePgFlush,
    decodePgFlush,
} from "cap:sql";

test("pg_extended: Parse named & unnamed roundtrip with parameter OIDs", () => {
    let buf: [256]u8 = [0; 256];
    const oids: [3]i32 = [23, 25, 16]; // int4, text, bool
    const wire = encodePgParse(
        buf[0..],
        "stmt_find_user",
        "SELECT id, name FROM users WHERE id = $1 AND email = $2 AND active = $3",
        oids[0..],
    ) catch {
        expect_true(false);
        return;
    };

    // Tag + big-endian length header verification
    expect_eq(wire[0], PG_PARSE_TAG);
    const declared_len: usize = (@as(usize, wire[1]) << 24) |
        (@as(usize, wire[2]) << 16) |
        (@as(usize, wire[3]) << 8) |
        @as(usize, wire[4]);
    expect_eq(declared_len + 1, wire.len);

    let decoded = decodePgParse(wire) catch {
        expect_true(false);
        return;
    };
    expect_false(decoded.isUnnamed());
    expect_eq(decoded.name, "stmt_find_user");
    expect_eq(decoded.query, "SELECT id, name FROM users WHERE id = $1 AND email = $2 AND active = $3");
    expect_eq(decoded.paramCount(), 3);
    expect_eq(decoded.params.remaining(), 3);

    const o0 = decoded.params.next() catch { expect_true(false); return; };
    const o1 = decoded.params.next() catch { expect_true(false); return; };
    const o2 = decoded.params.next() catch { expect_true(false); return; };
    const o3 = decoded.params.next() catch { expect_true(false); return; };
    expect_true(o0 != null);
    expect_true(o1 != null);
    expect_true(o2 != null);
    expect_true(o3 == null);
    if (v0 := o0) { expect_eq(v0, 23); }
    if (v1 := o1) { expect_eq(v1, 25); }
    if (v2 := o2) { expect_eq(v2, 16); }
    expect_eq(decoded.params.remaining(), 0);

    // Unnamed statement with zero parameter OIDs
    let buf2: [128]u8 = [0; 128];
    const empty_oids: [0]i32 = [];
    const unnamed_wire = encodePgParse(buf2[0..], "", "SELECT 1", empty_oids[0..]) catch {
        expect_true(false);
        return;
    };
    let decoded2 = decodePgParse(unnamed_wire) catch {
        expect_true(false);
        return;
    };
    expect_true(decoded2.isUnnamed());
    expect_eq(decoded2.name, "");
    expect_eq(decoded2.query, "SELECT 1");
    expect_eq(decoded2.paramCount(), 0);
    const none_oid = decoded2.params.next() catch { expect_true(false); return; };
    expect_true(none_oid == null);
});

test("pg_extended: Describe portal and prepared statement roundtrip", () => {
    let buf: [128]u8 = [0; 128];

    const stmt_ref = PgTargetRef.statement("ps_orders");
    expect_eq(stmt_ref.tag(), PG_TARGET_STATEMENT);
    expect_false(stmt_ref.isUnnamed());

    const wire_stmt = encodePgDescribe(buf[0..], stmt_ref) catch {
        expect_true(false);
        return;
    };
    expect_eq(wire_stmt[0], PG_DESCRIBE_TAG);
    expect_eq(wire_stmt[5], PG_TARGET_STATEMENT);

    const dec_stmt = decodePgDescribe(wire_stmt) catch {
        expect_true(false);
        return;
    };
    expect_true(dec_stmt.kind.isStatement());
    expect_false(dec_stmt.kind.isPortal());
    expect_eq(dec_stmt.name, "ps_orders");

    // Unnamed portal
    let buf2: [64]u8 = [0; 64];
    const portal_ref = PgTargetRef.portal("");
    expect_eq(portal_ref.tag(), PG_TARGET_PORTAL);
    expect_true(portal_ref.isUnnamed());

    const wire_portal = encodePgDescribe(buf2[0..], portal_ref) catch {
        expect_true(false);
        return;
    };
    // 'D' + 4B len(6) + 'P' + '\0' = 7 bytes
    expect_eq(wire_portal.len, 7);
    const dec_portal = decodePgDescribe(wire_portal) catch {
        expect_true(false);
        return;
    };
    expect_true(dec_portal.kind.isPortal());
    expect_true(dec_portal.isUnnamed());
    expect_eq(dec_portal.name, "");
});

test("pg_extended: Execute unlimited and bounded max_rows roundtrip", () => {
    let buf: [128]u8 = [0; 128];
    const wire_unlim = encodePgExecute(buf[0..], "", 0) catch {
        expect_true(false);
        return;
    };
    // 'E' + 4B len(9) + '\0' + 4B max_rows(0) = 10 bytes
    expect_eq(wire_unlim.len, 10);
    expect_eq(wire_unlim[0], PG_EXECUTE_TAG);

    const dec_unlim = decodePgExecute(wire_unlim) catch {
        expect_true(false);
        return;
    };
    expect_true(dec_unlim.isUnnamedPortal());
    expect_true(dec_unlim.isUnlimited());
    expect_eq(dec_unlim.max_rows, 0);

    let buf2: [128]u8 = [0; 128];
    const wire_paged = encodePgExecute(buf2[0..], "cursor_page_1", 250) catch {
        expect_true(false);
        return;
    };
    const dec_paged = decodePgExecute(wire_paged) catch {
        expect_true(false);
        return;
    };
    expect_false(dec_paged.isUnnamedPortal());
    expect_false(dec_paged.isUnlimited());
    expect_eq(dec_paged.portal, "cursor_page_1");
    expect_eq(dec_paged.max_rows, 250);
});

test("pg_extended: Close portal and prepared statement roundtrip", () => {
    let buf: [128]u8 = [0; 128];
    const wire_s = encodePgClose(buf[0..], PgTargetRef.statement("s1")) catch {
        expect_true(false);
        return;
    };
    // 'C' + 4B len(8) + 'S' + "s1\0" = 9 bytes
    expect_eq(wire_s.len, 9);
    expect_eq(wire_s[0], PG_CLOSE_TAG);
    const dec_s = decodePgClose(wire_s) catch {
        expect_true(false);
        return;
    };
    expect_true(dec_s.kind.isStatement());
    expect_eq(dec_s.name, "s1");

    let buf2: [128]u8 = [0; 128];
    const wire_p = encodePgClose(buf2[0..], PgTargetRef.portal("p_cursor")) catch {
        expect_true(false);
        return;
    };
    const dec_p = decodePgClose(wire_p) catch {
        expect_true(false);
        return;
    };
    expect_true(dec_p.kind.isPortal());
    expect_eq(dec_p.name, "p_cursor");
});

test("pg_extended: Sync & Flush 5-byte frames and pipelined multi-frame stream", () => {
    let sync_buf: [16]u8 = [0; 16];
    const sync_wire = encodePgSync(sync_buf[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(sync_wire.len, 5);
    expect_eq(sync_wire[0], PG_SYNC_TAG);
    expect_eq(sync_wire[4], 4);
    const sync_ok = decodePgSync(sync_wire) catch {
        expect_true(false);
        return;
    };
    expect_true(sync_ok);

    let flush_buf: [16]u8 = [0; 16];
    const flush_wire = encodePgFlush(flush_buf[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(flush_wire.len, 5);
    expect_eq(flush_wire[0], PG_FLUSH_TAG);
    expect_eq(flush_wire[4], 4);
    const flush_ok = decodePgFlush(flush_wire) catch {
        expect_true(false);
        return;
    };
    expect_true(flush_ok);

    // Concatenate Parse + Describe + Execute + Flush + Close + Sync into a pipelined buffer
    let pipe: [512]u8 = [0; 512];
    let pos: usize = 0;

    let tmp: [128]u8 = [0; 128];
    const oids: [1]i32 = [23];
    const f1 = encodePgParse(tmp[0..], "p1", "SELECT $1::int", oids[0..]) catch { expect_true(false); return; };
    let i: usize = 0;
    while (i < f1.len) { pipe[pos] = f1[i]; pos += 1; i += 1; }

    const f2 = encodePgDescribe(tmp[0..], PgTargetRef.statement("p1")) catch { expect_true(false); return; };
    i = 0;
    while (i < f2.len) { pipe[pos] = f2[i]; pos += 1; i += 1; }

    const f3 = encodePgExecute(tmp[0..], "", 10) catch { expect_true(false); return; };
    i = 0;
    while (i < f3.len) { pipe[pos] = f3[i]; pos += 1; i += 1; }

    const f4 = encodePgFlush(tmp[0..]) catch { expect_true(false); return; };
    i = 0;
    while (i < f4.len) { pipe[pos] = f4[i]; pos += 1; i += 1; }

    const f5 = encodePgClose(tmp[0..], PgTargetRef.statement("p1")) catch { expect_true(false); return; };
    i = 0;
    while (i < f5.len) { pipe[pos] = f5[i]; pos += 1; i += 1; }

    const f6 = encodePgSync(tmp[0..]) catch { expect_true(false); return; };
    i = 0;
    while (i < f6.len) { pipe[pos] = f6[i]; pos += 1; i += 1; }

    // Stream-read all 6 frames back via PgWireReader
    let reader = PgWireReader.init(pipe[0..pos]);
    const r1 = reader.readTaggedFrame() catch { expect_true(false); return; };
    const r2 = reader.readTaggedFrame() catch { expect_true(false); return; };
    const r3 = reader.readTaggedFrame() catch { expect_true(false); return; };
    const r4 = reader.readTaggedFrame() catch { expect_true(false); return; };
    const r5 = reader.readTaggedFrame() catch { expect_true(false); return; };
    const r6 = reader.readTaggedFrame() catch { expect_true(false); return; };
    expect_eq(r1.tag, PG_PARSE_TAG);
    expect_eq(r2.tag, PG_DESCRIBE_TAG);
    expect_eq(r3.tag, PG_EXECUTE_TAG);
    expect_eq(r4.tag, PG_FLUSH_TAG);
    expect_eq(r5.tag, PG_CLOSE_TAG);
    expect_eq(r6.tag, PG_SYNC_TAG);
    expect_eq(reader.remaining(), 0);
});

test("pg_extended: malformed frames and boundary error guards", () => {
    // 1. BufferTooSmall on encoders
    let tiny: [3]u8 = [0; 3];
    const empty_oids: [0]i32 = [];
    let failed_buf = false;
    * = encodePgParse(tiny[0..], "a", "SELECT 1", empty_oids[0..]) catch { failed_buf = true; };
    expect_true(failed_buf);

    let failed_sync_buf = false;
    * = encodePgSync(tiny[0..]) catch { failed_sync_buf = true; };
    expect_true(failed_sync_buf);

    // 2. Negative max_rows rejected on encode and decode
    let ok_buf: [64]u8 = [0; 64];
    let failed_neg = false;
    * = encodePgExecute(ok_buf[0..], "", -1) catch { failed_neg = true; };
    expect_true(failed_neg);

    // Crafted Execute frame with max_rows = -5 (0xFFFFFFFB)
    const bad_exec: [10]u8 = ['E', 0, 0, 0, 9, 0, 0xff, 0xff, 0xff, 0xfb];
    let failed_dec_neg = false;
    * = decodePgExecute(bad_exec[0..]) catch { failed_dec_neg = true; };
    expect_true(failed_dec_neg);

    // 3. Invalid target kind byte ('X') in Describe and Close
    const bad_desc: [7]u8 = ['D', 0, 0, 0, 6, 'X', 0];
    let failed_kind = false;
    * = decodePgDescribe(bad_desc[0..]) catch { failed_kind = true; };
    expect_true(failed_kind);

    const bad_close: [7]u8 = ['C', 0, 0, 0, 6, 'Z', 0];
    let failed_close_kind = false;
    * = decodePgClose(bad_close[0..]) catch { failed_close_kind = true; };
    expect_true(failed_close_kind);

    // 4. Invalid tag mismatch
    let sync_buf: [16]u8 = [0; 16];
    const sync_wire = encodePgSync(sync_buf[0..]) catch { expect_true(false); return; };
    let failed_tag = false;
    * = decodePgFlush(sync_wire) catch { failed_tag = true; };
    expect_true(failed_tag);

    // 5. Missing NUL terminator in Describe
    const no_nul_desc: [7]u8 = ['D', 0, 0, 0, 6, 'S', 'a'];
    let failed_nul = false;
    * = decodePgDescribe(no_nul_desc[0..]) catch { failed_nul = true; };
    expect_true(failed_nul);

    // 6. Sync frame with non-zero payload length rejected
    const bad_sync: [6]u8 = ['S', 0, 0, 0, 5, 0];
    let failed_sync_len = false;
    * = decodePgSync(bad_sync[0..]) catch { failed_sync_len = true; };
    expect_true(failed_sync_len);

    // 7. Trailing bytes after valid frame rejected
    const trailing_sync: [6]u8 = ['S', 0, 0, 0, 4, 0xaa];
    let failed_trailing = false;
    * = decodePgSync(trailing_sync[0..]) catch { failed_trailing = true; };
    expect_true(failed_trailing);

    // 8. Truncated Parse parameter OID table
    const trunc_parse: [12]u8 = ['P', 0, 0, 0, 11, 0, 'Q', 0, 0, 1, 0x00, 0x17];
    let failed_trunc = false;
    * = decodePgParse(trunc_parse[0..]) catch { failed_trunc = true; };
    expect_true(failed_trunc);

    // 9. PgTargetKind.fromByte direct guard
    let failed_enum = false;
    * = PgTargetKind.fromByte('?') catch { failed_enum = true; };
    expect_true(failed_enum);
});
