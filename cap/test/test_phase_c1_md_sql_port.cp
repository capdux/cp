// Source: /data/dev/bun/src/md/ & /data/dev/bun/src/sql/
// cap/test/test_phase_c1_md_sql_port.cp
// Comprehensive Phase C-1 Integration Test Suite for Direct Bun Port:
//   - cap:md (Full 16-file MD4C Engine: Blocks, Inlines, Delimiter Stack, Entities, Unicode CaseFold, HTML & ANSI Renderers)
//   - cap:sql (Full Postgres/MySQL/SQLite Protocol & Codec Engine: PgBinaryCodec, PgTypeTag, PgCommandTag, PgProtocol,
//              MySqlAuthNative/CachingSha2, MySqlPreparedProtocol, MySqlBinaryRowDecoder, SqlData, SqliteRows Bindings)

const std = @import("std");
import { test, expect_eq } from "cap:test";

import {
    MarkdownOptions,
    MdTableAlign,
    MdTable,
    Markdown,
    MdEngineOptions,
    AnsiTheme,
    ImageUrlCollector,
    renderWithRenderer,
} from "cap:md";

import {
    SqlData,
    ColumnIdentifier,
    SQLQueryResultMode,
    SqlParam,
    PgTypeTag,
    PgCommandKind,
    PgCommandTag,
    PgErrorCategory,
    PgErrorClassifier,
    PgBinaryCodec,
    PgParameterDescription,
    PgNotificationResponse,
    PgNoticeResponse,
    PgNegotiateProtocolVersion,
    PgCopyResponse,
    PgProtocol,
    MySqlAuthMethod,
    MySqlCapabilityFlags,
    MySqlStatusFlags,
    MySqlFieldType,
    MySqlErrorClassifier,
    MySqlValueDateTime,
    MySqlValueTime,
    MySqlAuthNative,
    MySqlAuthCachingSha2,
    MySqlFastAuthStatus,
    MySqlAuthSwitchRequest,
    MySqlSSLRequest,
    MySqlColumnFlags,
    MySqlColumnDefinition41,
    MySqlResultSetHeader,
    MySqlPrepareOK,
    MySqlPreparedProtocol,
    MySqlDecodedKind,
    MySqlDecodedCell,
    MySqlBinaryRowDecoder,
    MySqlCommandType,
    MySqlPacketHeader,
    SqliteDatabase,
} from "cap:sql";

fn main() {
    test("1. cap:md — MD4C CommonMark + GFM HTML Engine (Headings, Tables, Lists, Ref Links, Entities, XSS)", () => {
        const allocator: std.mem.Allocator = std.heap.page_allocator;
        const md_src =
            "# Hello World\n\n" ++
            "Hello World\n===========\n\n" ++
            "Emphasis: ***boldem***, foo_bar_baz, ~~strike~~, `a ` b`.\n\n" ++
            "Entities & Escapes: &copy; &mdash; &aelig; &#x2714; \\*lit\\*.\n\n" ++
            "Autolink: https://example.com/(foo) and [Ref Link][MyRef].\n\n" ++
            "- [x] Done task\n- [ ] Todo task\n\n" ++
            "| Name | Score |\n| :--- | ---: |\n| Alice | 100 |\n| Bob\\|Jr | 95 |\n\n" ++
            "<script>alert(1)</script>\n\n" ++
            "[myref]: https://bun.sh/docs \"Docs\"\n";

        const opts = MdEngineOptions{
            tables: true,
            strikethrough: true,
            tasklists: true,
            permissive_url_autolinks: true,
            heading_ids: true,
            autolink_headings: true,
            tag_filter: true,
            sanitize_html: true,
        };

        const html = Markdown.toHtmlEngine(allocator, md_src, opts) catch unreachable;
        defer allocator.free(html);

        expect_eq(std.mem.indexOf(u8, html, "<h1 id=\"hello-world\">") != null, true);
        expect_eq(std.mem.indexOf(u8, html, "<h1 id=\"hello-world-1\">") != null, true);
        expect_eq(std.mem.indexOf(u8, html, "<strong><em>boldem</em></strong>") != null or
            std.mem.indexOf(u8, html, "<em><strong>boldem</strong></em>") != null, true);
        expect_eq(std.mem.indexOf(u8, html, "foo_bar_baz") != null, true);
        expect_eq(std.mem.indexOf(u8, html, "<del>strike</del>") != null, true);
        expect_eq(std.mem.indexOf(u8, html, "\xc2\xa9") != null, true); // &copy;
        expect_eq(std.mem.indexOf(u8, html, "\xe2\x80\x94") != null, true); // &mdash;
        expect_eq(std.mem.indexOf(u8, html, "\xc3\xa6") != null, true); // &aelig;
        expect_eq(std.mem.indexOf(u8, html, "https://example.com/(foo)") != null, true);
        expect_eq(std.mem.indexOf(u8, html, "href=\"https://bun.sh/docs\"") != null, true);
        expect_eq(std.mem.indexOf(u8, html, "checked") != null, true);
        expect_eq(std.mem.indexOf(u8, html, "<table>") != null, true);
        expect_eq(std.mem.indexOf(u8, html, "Bob|Jr") != null, true);
        expect_eq(std.mem.indexOf(u8, html, "<script>alert(1)</script>") == null, true);
        expect_eq(std.mem.indexOf(u8, html, "&lt;script&gt;") != null, true);
    });

    test("2. cap:md — ANSI Terminal Renderer, W3C Entities, Unicode CaseFold & ImageUrlCollector", () => {
        const allocator: std.mem.Allocator = std.heap.page_allocator;
        const md_src =
            "# Title\n\n" ++
            "```zig\nconst x: i32 = 42;\n```\n\n" ++
            "| Col A | Col B |\n| :--- | :---: |\n| Cell 1 | **Bold** |\n\n" ++
            "![diagram](https://cdn.example.com/arch.png)\n";

        const opts = MdEngineOptions{ tables: true };
        const theme = AnsiTheme{ light: false, columns: 80, colors: true, hyperlinks: true };
        const ansi = Markdown.toAnsi(allocator, md_src, opts, theme) catch unreachable;
        defer allocator.free(ansi);

        expect_eq(std.mem.indexOf(u8, ansi, "\x1b[") != null, true);
        expect_eq(std.mem.indexOf(u8, ansi, "Title") != null, true);
        expect_eq(std.mem.indexOf(u8, ansi, "Cell 1") != null, true);

        let collector = ImageUrlCollector.init(allocator);
        defer collector.deinit();
        renderWithRenderer(md_src, allocator, opts, collector.renderer()) catch unreachable;
        expect_eq(collector.urls.items.len, 1);
        expect_eq(std.mem.eql(u8, collector.urls.items[0], "https://cdn.example.com/arch.png"), true);

        // W3C HTML5 Entity lookup & CommonMark Unicode CaseFold
        const ae = Markdown.lookupEntity("&AElig;").?;
        expect_eq(ae[0], 0x00C6);
        const fold_ss = Markdown.caseFold(0x00DF); // sharp s -> ss
        expect_eq(fold_ss.n_codepoints, 2);
        expect_eq(fold_ss.codepoints[0], 's');
        expect_eq(fold_ss.codepoints[1], 's');
    });

    test("3. cap:sql — SqlData SSO, PgTypeTag OIDs, PgCommandTag & PgErrorClassifier", () => {
        const allocator: std.mem.Allocator = std.heap.page_allocator;

        let short_d = SqlData.create("hello", allocator) catch unreachable;
        defer short_d.deinit(allocator);
        expect_eq(std.mem.eql(u8, short_d.slice(), "hello"), true);

        let long_d = SqlData.create("0123456789abcdef_long", allocator) catch unreachable;
        defer long_d.zdeinit(allocator);
        expect_eq(std.mem.eql(u8, long_d.slice(), "0123456789abcdef_long"), true);

        const col = ColumnIdentifier{ is_index: false, index: 0, name: SqlData.temporary("id") };
        expect_eq(col.index, 0);
        expect_eq(SQLQueryResultMode.objects != SQLQueryResultMode.values, true);

        expect_eq(PgTypeTag.fromOid(16) == PgTypeTag.pg_bool, true);
        expect_eq(PgTypeTag.fromOid(23) == PgTypeTag.int4, true);
        expect_eq(PgTypeTag.fromOid(2950) == PgTypeTag.uuid, true);
        expect_eq(PgTypeTag.fromOid(3802) == PgTypeTag.jsonb, true);
        expect_eq(PgTypeTag.fromOid(1007).isArray(), true);
        expect_eq(PgTypeTag.int4.formatCode(), 1);
        expect_eq(PgTypeTag.text.formatCode(), 0);

        const cmd = PgCommandTag.parse("INSERT 0 15\x00");
        expect_eq(cmd.kind == PgCommandKind.INSERT, true);
        expect_eq(cmd.rows, 15);

        expect_eq(PgErrorClassifier.classify("23505") == PgErrorCategory.IntegrityConstraintViolation, true);
        expect_eq(PgErrorClassifier.isRetryable("40001"), true);
        expect_eq(PgErrorClassifier.isRetryable("23505"), false);
    });

    test("4. cap:sql — PgBinaryCodec (Int/Float/Bool/Bytea/UUID/JSONB/Date/Timestamp/Numeric/Array)", () => {
        let buf: [128]u8 = [0; 128];

        expect_eq(PgBinaryCodec.encodeInt2(buf[0..128], -1234), 2);
        expect_eq(PgBinaryCodec.decodeInt2(buf[0..2]) catch unreachable, -1234);

        expect_eq(PgBinaryCodec.encodeInt4(buf[0..128], -987654), 4);
        expect_eq(PgBinaryCodec.decodeInt4(buf[0..4]) catch unreachable, -987654);

        expect_eq(PgBinaryCodec.encodeInt8(buf[0..128], 1234567890123), 8);
        expect_eq(PgBinaryCodec.decodeInt8(buf[0..8]) catch unreachable, 1234567890123);

        expect_eq(PgBinaryCodec.encodeFloat8(buf[0..128], 3.5), 8);
        expect_eq(PgBinaryCodec.decodeFloat8(buf[0..8]) catch unreachable, 3.5);

        expect_eq(PgBinaryCodec.encodeBool(buf[0..128], true), 1);
        expect_eq(PgBinaryCodec.decodeBool(buf[0..1]) catch unreachable, true);

        const hex_b = PgBinaryCodec.encodeByteaHex("\xDE\xAD\xBE\xEF", buf[0..128]) catch unreachable;
        expect_eq(std.mem.eql(u8, hex_b, "\\xdeadbeef"), true);
        let raw_b: [16]u8 = [0; 16];
        const dec_b = PgBinaryCodec.decodeByteaText(hex_b, raw_b[0..16]) catch unreachable;
        expect_eq(dec_b.len, 4);
        expect_eq(dec_b[0], 0xDE);

        let u16_bytes: [16]u8 = [0; 16];
        PgBinaryCodec.parseUuid("550e8400-e29b-41d4-a716-446655440000", &u16_bytes) catch unreachable;
        let u_str: [36]u8 = [0; 36];
        const u_out = PgBinaryCodec.formatUuid(u16_bytes[0..16], &u_str) catch unreachable;
        expect_eq(std.mem.eql(u8, u_out, "550e8400-e29b-41d4-a716-446655440000"), true);

        const jb = PgBinaryCodec.encodeJsonb("{\"ok\":true}", buf[0..128]) catch unreachable;
        expect_eq(jb[0], 0x01);
        const jb_txt = PgBinaryCodec.decodeJsonb(jb) catch unreachable;
        expect_eq(std.mem.eql(u8, jb_txt, "{\"ok\":true}"), true);

        expect_eq(PgBinaryCodec.encodeTimestampUnixMs(buf[0..128], 1700000000000), 8);
        expect_eq(PgBinaryCodec.decodeTimestampUnixMs(buf[0..8]) catch unreachable, 1700000000000);

        // NUMERIC binary: -123.4500 => ndigits=2, weight=0, sign=0x4000 (-), dscale=4, digits=[123, 4500]
        const num_raw: [12]u8 = [0, 2, 0, 0, 0x40, 0x00, 0, 4, 0, 123, 0x11, 0x94];
        const num_s = PgBinaryCodec.decodeNumericText(num_raw[0..12], buf[0..128]) catch unreachable;
        expect_eq(std.mem.eql(u8, num_s, "-123.4500"), true);

        // Int4 binary array: ndim=1, has_null=0, elem_oid=23, len=2, lbound=1, [10, 20]
        const arr_raw: [36]u8 = [
            0, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, 23,
            0, 0, 0, 2,  0, 0, 0, 1,
            0, 0, 0, 4,  0, 0, 0, 10,
            0, 0, 0, 4,  0, 0, 0, 20,
        ];
        let ints: [4]i32 = [0; 4];
        const arr_out = PgBinaryCodec.decodeInt4Array(arr_raw[0..36], ints[0..4]) catch unreachable;
        expect_eq(arr_out.len, 2);
        expect_eq(arr_out[0], 10);
        expect_eq(arr_out[1], 20);
    });

    test("5. cap:sql — PgProtocol Frames (SSLRequest, Cancel, StartupUTF8, Copy, ParameterDesc, Notification)", () => {
        let wire: [256]u8 = [0; 256];
        expect_eq(PgProtocol.encodeSSLRequest(wire[0..256]), 8);
        expect_eq(PgProtocol.encodeCancelRequest(wire[0..256], 101, 202), 16);

        const st_len = PgProtocol.encodeStartupWithClientEncoding(wire[0..256], "alice", "appdb", "cp_client");
        expect_eq(st_len > 20, true);
        expect_eq(std.mem.indexOf(u8, wire[0..st_len], "UTF8") != null, true);

        const cd_len = PgProtocol.encodeCopyData(wire[0..256], "row1\n");
        expect_eq(cd_len, 10);
        expect_eq(wire[0], 'd');

        // ParameterDescription: 2 params -> int4 (23), text (25)
        const pd_raw: [10]u8 = [0, 2, 0, 0, 0, 23, 0, 0, 0, 25];
        const pd = PgParameterDescription.parse(pd_raw[0..10]).?;
        expect_eq(pd.param_count, 2);
        expect_eq(pd.getTypeTag(0).? == PgTypeTag.int4, true);
        expect_eq(pd.getTypeTag(1).? == PgTypeTag.text, true);

        const notif_raw = "\x00\x00\x00\x2Aevents\x00hello\x00";
        const notif = PgNotificationResponse.parse(notif_raw).?;
        expect_eq(notif.process_id, 42);
        expect_eq(std.mem.eql(u8, notif.channel, "events"), true);
        expect_eq(std.mem.eql(u8, notif.payload, "hello"), true);

        const copy_raw: [7]u8 = [1, 0, 2, 0, 1, 0, 0];
        const cp_resp = PgCopyResponse.parse(copy_raw[0..7]).?;
        expect_eq(cp_resp.overall_format, 1);
        expect_eq(cp_resp.column_count, 2);
        expect_eq(cp_resp.column_formats[0], 1);
    });

    test("6. cap:sql — MySQL CachingSha2/Native Auth, COM_STMT_EXECUTE & Binary Row Decoder", () => {
        expect_eq(MySqlAuthMethod.fromPluginName("caching_sha2_password") == MySqlAuthMethod.caching_sha2_password, true);
        expect_eq(MySqlErrorClassifier.isRetryable(1213), true);

        const nonce = "12345678901234567890";
        const s1 = MySqlAuthNative.scramble("secret", nonce) catch unreachable;
        const s2 = MySqlAuthCachingSha2.scramble("secret", nonce) catch unreachable;
        expect_eq(s1.len, 20);
        expect_eq(s2.len, 32);

        const fast = MySqlAuthCachingSha2.parseFastAuthResponse("\x01\x03").?;
        expect_eq(fast == MySqlFastAuthStatus.success, true);

        const sw = MySqlAuthSwitchRequest.parse("\xFEmysql_native_password\x00abcdef\x00").?;
        expect_eq(std.mem.eql(u8, sw.plugin_name, "mysql_native_password"), true);
        expect_eq(std.mem.eql(u8, sw.plugin_data, "abcdef"), true);

        let pkt: [256]u8 = [0; 256];
        const ssl_len = MySqlSSLRequest.encode(pkt[0..256], MySqlCapabilityFlags.CLIENT_SSL, 16777216, 45, 1);
        expect_eq(ssl_len, 36);

        let params: [2]SqlParam = [
            SqlParam{ kind: .Int64, int_val: 42 },
            SqlParam{ kind: .Text, text_val: "cp" },
        ];
        const exec_len = MySqlPreparedProtocol.encodeExecute(pkt[0..256], 7, 0, params[0..2], 0);
        expect_eq(exec_len > 15, true);
        expect_eq(pkt[4], @intFromEnum(MySqlCommandType.COM_STMT_EXECUTE));

        // MySQL Binary Row: 2 cols (LONG id=99, VAR_STRING name="cp")
        let cols: [2]MySqlColumnDefinition41 = [
            MySqlColumnDefinition41{ name: "id", column_type: .MYSQL_TYPE_LONG },
            MySqlColumnDefinition41{ name: "name", column_type: .MYSQL_TYPE_VAR_STRING },
        ];
        const bin_row: [9]u8 = [0x00, 0x00, 99, 0, 0, 0, 2, 'c', 'p'];
        let cells: [2]MySqlDecodedCell = [MySqlDecodedCell{}, MySqlDecodedCell{}];
        const decoded_n = MySqlBinaryRowDecoder.decodeRow(bin_row[0..9], cols[0..2], cells[0..2]) catch unreachable;
        expect_eq(decoded_n, 2);
        expect_eq(cells[0].kind == MySqlDecodedKind.Int64, true);
        expect_eq(cells[0].int_val, 99);
        expect_eq(cells[1].kind == MySqlDecodedKind.String, true);
        expect_eq(std.mem.eql(u8, cells[1].bytes_val, "cp"), true);

        const dt = MySqlValueDateTime{ year: 2026, month: 11, day: 20, hour: 10, minute: 30, second: 5 };
        let dt_buf: [16]u8 = [0; 16];
        const dt_n = dt.toBinary(.MYSQL_TYPE_DATETIME, dt_buf[0..16]);
        expect_eq(dt_n, 8);
        const dt2 = MySqlValueDateTime.fromBinary(dt_buf[1..8]) catch unreachable;
        expect_eq(dt2.year, 2026);
        expect_eq(dt2.day, 20);
    });

    test("7. cap:sql — SQLite Parameterized Bindings (bindInt/Double/Text/Blob/Null & Transactions)", () => {
        let db = SqliteDatabase.open(":memory:") catch unreachable;
        defer db.close();

        db.exec("CREATE TABLE items (id INTEGER PRIMARY KEY, name TEXT, score REAL, raw BLOB, note TEXT);") catch unreachable;
        db.begin() catch unreachable;

        let ins = db.query("INSERT INTO items (id, name, score, raw, note) VALUES (?, ?, ?, ?, ?);") catch unreachable;
        defer ins.close();
        ins.bindInt(1, 7) catch unreachable;
        ins.bindText(2, "alpha") catch unreachable;
        ins.bindDouble(3, 9.5) catch unreachable;
        ins.bindBlob(4, "\x01\x02\x03") catch unreachable;
        ins.bindNull(5) catch unreachable;
        expect_eq(ins.next(), false);

        db.commit() catch unreachable;
        expect_eq(db.changes(), 1);
        expect_eq(db.lastInsertRowId(), 7);

        let sel = db.query("SELECT id, name, score, raw, note FROM items WHERE id = ?;") catch unreachable;
        defer sel.close();
        sel.bindInt(1, 7) catch unreachable;
        expect_eq(sel.next(), true);
        expect_eq(std.mem.eql(u8, sel.columnName(1), "name"), true);
        expect_eq(sel.columnInt(0), 7);
        expect_eq(std.mem.eql(u8, sel.columnText(1), "alpha"), true);
        expect_eq(sel.columnDouble(2), 9.5);
        expect_eq(sel.columnBlob(3).len, 3);
        expect_eq(sel.columnType(4), 5); // SQLITE_NULL
    });

    test("8. cap:md — Backward-Compatible AST (Markdown.parse, toHtml, MdTable)", () => {
        const allocator: std.mem.Allocator = std.heap.page_allocator;
        const md_src =
            "Setext H1\n===\n\n" ++
            "- [x] Done task\n\n" ++
            "| A | B |\n| :--- | ---: |\n| 1 | 2 |\n";
        const opts = MarkdownOptions.github();
        const html = Markdown.toHtml(allocator, md_src, opts) catch unreachable;
        defer allocator.free(html);
        expect_eq(std.mem.indexOf(u8, html, "<h1 id=\"setext-h1\">Setext H1</h1>") != null, true);
        expect_eq(std.mem.indexOf(u8, html, "checked /> Done task") != null, true);
        expect_eq(std.mem.indexOf(u8, html, "<table>") != null, true);

        let doc = Markdown.parse(allocator, md_src, opts) catch unreachable;
        defer doc.deinit();
        expect_eq(doc.blocks().len >= 3, true);

        let cells: [4][]const u8 = [""; 4];
        expect_eq(MdTable.splitRow("| :--- | ---: |", &cells), 2);
        expect_eq(MdTable.parseAlignSpec(cells[0]).? == MdTableAlign.Left, true);
        expect_eq(MdTable.parseAlignSpec(cells[1]).? == MdTableAlign.Right, true);
    });
}
