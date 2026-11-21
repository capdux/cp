// cap/test/test_stdlib_phase_b2_c.cp
// Comprehensive E2E Regression Suite for Phase B2 & Phase C Standard Library Modules:
// cap:elf, cap:md, cap:html, cap:ffi, cap:sql, cap:http, cap:net, cap:mem, cap:string

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { ExeFormatKind, ElfMachine, ExeInspector } from "cap:elf";
import { MdOptions, Markdown } from "cap:md";
import { CssSelector, HtmlRewriter } from "cap:html";
import { FFIType, FFISignature, DynLib, DlFlags, TccOutputFormat } from "cap:ffi";
import { SqlDialect, SqlType, SqlStatement, PostgresWire, MySqlPacketHeader } from "cap:sql";
import { CookieMap, CookieEntry, CookieSameSite, FormData } from "cap:http";
import { SocketAddress, UdpSocket } from "cap:net";
import { CowSlice, TaggedPtr } from "cap:mem";
import { Utf8Grapheme } from "cap:string";

fn main() {
    test("1. cap:elf — Binary format detection & ELF64 / PE header inspection", () => {
        expect_eq(ExeInspector.detectFormat("\x7FELF\x02\x01") == ExeFormatKind.Elf, true);
        expect_eq(ExeInspector.detectFormat("\xCF\xFA\xED\xFE") == ExeFormatKind.MachO64, true);
        expect_eq(ExeInspector.detectFormat("\xCA\xFE\xBA\xBE") == ExeFormatKind.MachOFat, true);
        expect_eq(ExeInspector.detectFormat("\x00asm\x01\x00\x00\x00") == ExeFormatKind.Wasm, true);
        expect_eq(ExeInspector.detectFormat("MZ\x90\x00") == ExeFormatKind.Pe, true);

        let elf_hdr: [64]u8 = [0; 64];
        elf_hdr[0] = 0x7F;
        elf_hdr[1] = 'E';
        elf_hdr[2] = 'L';
        elf_hdr[3] = 'F';
        elf_hdr[4] = 2; // 64-bit
        elf_hdr[5] = 1; // little-endian
        elf_hdr[16] = 2; // ET_EXEC
        elf_hdr[18] = 62; // EM_X86_64
        elf_hdr[24] = 0x40; // entry_point = 0x40

        const parsed = ExeInspector.parseElf64Header(elf_hdr[0..64]).?;
        expect_eq(parsed.is_64bit, true);
        expect_eq(parsed.is_little_endian, true);
        expect_eq(parsed.machine, (ElfMachine.X86_64 as u16));
        expect_eq(parsed.entry_point, 0x40 as u64);
    });

    test("2. cap:md — CommonMark/GFM HTML & terminal ANSI rendering", () => {
        const gfm = MdOptions.github();
        expect_eq(gfm.tables, true);
        expect_eq(gfm.tasklists, true);

        let html_buf: [256]u8 = [0; 256];
        const html = Markdown.renderHtml("# Title\n- item **bold**\nHello `cp`!\n", html_buf[0..256]);
        expect_eq(std.mem.eql(u8, html, "<h1>Title</h1>\n<li>item <strong>bold</strong></li>\n<p>Hello <code>cp</code>!</p>\n"), true);

        let ansi_buf: [128]u8 = [0; 128];
        const ansi = Markdown.renderAnsi("# Heading\nBody\n", ansi_buf[0..128]);
        expect_eq(std.mem.indexOf(u8, ansi, "\x1b[1;36mHeading\x1b[0m") != null, true);
    });

    test("3. cap:html — CSS Selector parser & streaming HtmlRewriter", () => {
        const sel1 = CssSelector.parse("div#app");
        expect_eq(std.mem.eql(u8, sel1.tag_name, "div"), true);
        expect_eq(std.mem.eql(u8, sel1.id_name, "app"), true);
        expect_eq(sel1.matchesTag("div"), true);
        expect_eq(sel1.matchesTag("span"), false);

        const sel2 = CssSelector.parse("input[type=\"password\"]");
        expect_eq(std.mem.eql(u8, sel2.tag_name, "input"), true);
        expect_eq(std.mem.eql(u8, sel2.attr_name, "type"), true);
        expect_eq(std.mem.eql(u8, sel2.attr_val, "password"), true);

        let out1: [128]u8 = [0; 128];
        const stripped = HtmlRewriter.stripTags("<div>Hello <b>World</b>!</div>", out1[0..128]);
        expect_eq(std.mem.eql(u8, stripped, "Hello World!"), true);

        let out2: [128]u8 = [0; 128];
        const renamed = HtmlRewriter.renameTag("<b>Bold</b>", "b", "strong", out2[0..128]);
        expect_eq(std.mem.eql(u8, renamed, "<strong>Bold</strong>"), true);
    });

    test("4. cap:ffi — FFIType, FFISignature, DynLib & TinyCC bindings", () => {
        expect_eq(TccOutputFormat.Memory as i32, 1);
        expect_eq(FFIType.byteSize(FFIType.I32), 4 as usize);
        expect_eq(FFIType.byteSize(FFIType.F64), 8 as usize);
        expect_eq(FFIType.isFloatingPoint(FFIType.F64), true);

        let sig = FFISignature.init(FFIType.I64);
        expect_eq(sig.addArg(FFIType.I32), true);
        expect_eq(sig.addArg(FFIType.Ptr), true);
        expect_eq(sig.totalArgBytes(), 12 as usize);

        let lib = DynLib.open("libc.so.6", DlFlags.RTLD_LAZY);
        expect_eq(lib.is_open, true);
        lib.close();
        expect_eq(lib.is_open, false);
    });

    test("5. cap:sql — SqlStatement parameter binding, PostgresWire & MySqlPacketHeader", () => {
        let stmt = SqlStatement.prepare("SELECT * FROM users WHERE id = $1 AND role = $2", SqlDialect.Postgres);
        expect_eq(stmt.bindInt(101), true);
        expect_eq(stmt.bindText("admin"), true);
        expect_eq(stmt.bindNull(), true);
        expect_eq(stmt.param_count, 3 as usize);
        expect_eq(stmt.params[0].kind == SqlType.Int64, true);
        expect_eq(stmt.params[0].int_val, 101 as i64);

        let pg_buf: [64]u8 = [0; 64];
        const pg_len = PostgresWire.encodeSimpleQuery(pg_buf[0..64], "SELECT 1");
        expect_eq(pg_len, 14 as usize);
        expect_eq(pg_buf[0], 'Q' as u8);
        expect_eq(PostgresWire.parseFrameLength(pg_buf[0..5]).?, 13 as u32);

        let my_buf: [8]u8 = [0; 8];
        expect_eq(MySqlPacketHeader.encode(my_buf[0..8], 258, 3), 4 as usize);
        const my_hdr = MySqlPacketHeader.decode(my_buf[0..4]).?;
        expect_eq(my_hdr.payload_length, 258 as u32);
        expect_eq(my_hdr.sequence_id, 3 as u8);
    });

    test("6. cap:http — CookieMap RFC 6265 parser/serializer & FormData", () => {
        const cookies = CookieMap.parse("session_id=\"abc123xyz\"; theme=dark; lang=zh-CN");
        expect_eq(cookies.has("session_id"), true);
        expect_eq(std.mem.eql(u8, cookies.get("session_id").?, "abc123xyz"), true);
        expect_eq(std.mem.eql(u8, cookies.get("theme").?, "dark"), true);

        let set_cookie_buf: [128]u8 = [0; 128];
        const sc = CookieMap.formatSetCookie(CookieEntry {
            name: "token",
            value: "xyz",
            path: "/",
            http_only: true,
            secure: true,
            same_site: CookieSameSite.Strict,
        }, set_cookie_buf[0..128]);
        expect_eq(std.mem.eql(u8, sc, "token=xyz; Path=/; HttpOnly; Secure; SameSite=Strict"), true);

        const form = FormData.parseUrlEncoded("user=alice&role=architect");
        expect_eq(std.mem.eql(u8, form.get("user").?, "alice"), true);
        expect_eq(std.mem.eql(u8, form.get("role").?, "architect"), true);
        const boundary = FormData.extractBoundary("multipart/form-data; boundary=---WebKitFormBoundary7MA4YWxkTrZu0gW").?;
        expect_eq(std.mem.eql(u8, boundary, "---WebKitFormBoundary7MA4YWxkTrZu0gW"), true);
    });

    test("7. cap:net — SocketAddress & UdpSocket datagram lifecycle", () => {
        const local = SocketAddress.parseIpv4("127.0.0.1", 19000).?;
        const peer = SocketAddress.parseIpv4("127.0.0.1", 19001).?;
        let sock = UdpSocket.bind(local);
        defer sock.~init();

        expect_eq(sock.boundPort(), 19000 as u16);
        expect_eq(sock.connectPeer(peer), true);
        expect_eq(sock.send("ping"), 4 as usize);
        const pkt = sock.simulateReceive(4, peer);
        expect_eq(pkt.payload_len, 4 as usize);
        expect_eq(pkt.truncated, false);
    });

    test("8. cap:mem — CowSlice copy-on-write & TaggedPtr 16-bit tag packing", () => {
        let cow = CowSlice.initBorrowed("hello");
        defer cow.~init();
        expect_eq(cow.isOwned(), false);
        expect_eq(cow.len(), 5 as usize);
        expect_eq(cow.setByte(0, 'H' as u8), true);
        expect_eq(cow.isOwned(), true);
        expect_eq(std.mem.eql(u8, cow.slice(), "Hello"), true);

        const tp = TaggedPtr.pack(0x12345678, 0xCAFE);
        expect_eq(tp.address(), 0x12345678 as usize);
        expect_eq(tp.tag(), 0xCAFE as u16);
    });

    test("9. cap:string — Utf8Grapheme validation, codepoint & UAX #29 grapheme counting", () => {
        expect_eq(Utf8Grapheme.isValidUtf8("Hello 世界 🚀"), true);
        expect_eq(Utf8Grapheme.countCodepoints("Hello"), 5 as usize);
        expect_eq(Utf8Grapheme.countCodepoints("世界"), 2 as usize);
        // 'e' + U+0301 (combining acute accent) = 2 codepoints, 1 grapheme cluster
        const accented = "e\xCC\x81";
        expect_eq(Utf8Grapheme.isValidUtf8(accented), true);
        expect_eq(Utf8Grapheme.countCodepoints(accented), 2 as usize);
        expect_eq(Utf8Grapheme.countGraphemes(accented), 1 as usize);
    });
}
