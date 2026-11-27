import { test, expect_eq } from "cap:test";
import {
    SniffOptions,
    sniffHtmlEncoding,
    normalizeEncodingLabel,
} from "../../packages/html-encoding-sniffer/src/index.cp";

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn main() {
    test("1. @cap/html-encoding-sniffer — BOM sniffing overrides transport layer", () => {
        const opts = SniffOptions{ transport_layer_encoding: "latin1", default_encoding: "windows-1252" };

        let utf8_bom: [6]u8 = undefined;
        utf8_bom[0] = 0xEF; utf8_bom[1] = 0xBB; utf8_bom[2] = 0xBF;
        utf8_bom[3] = '<'; utf8_bom[4] = 'p'; utf8_bom[5] = '>';
        expect_eq(strEq(sniffHtmlEncoding(utf8_bom[0..6], opts), "UTF-8"), true);

        let be_bom: [4]u8 = undefined;
        be_bom[0] = 0xFE; be_bom[1] = 0xFF; be_bom[2] = 0x00; be_bom[3] = 0x3C;
        expect_eq(strEq(sniffHtmlEncoding(be_bom[0..4], opts), "UTF-16BE"), true);

        let le_bom: [4]u8 = undefined;
        le_bom[0] = 0xFF; le_bom[1] = 0xFE; le_bom[2] = 0x3C; le_bom[3] = 0x00;
        expect_eq(strEq(sniffHtmlEncoding(le_bom[0..4], opts), "UTF-16LE"), true);
    });

    test("2. @cap/html-encoding-sniffer — Transport layer encoding takes priority over <meta>", () => {
        const html = "<html><head><meta charset=\"gbk\"></head></html>";
        const opts = SniffOptions{ transport_layer_encoding: "utf8", default_encoding: "" };
        expect_eq(strEq(sniffHtmlEncoding(html, opts), "UTF-8"), true);
    });

    test("3. @cap/html-encoding-sniffer — <meta charset> & <meta http-equiv='content-type'> prescan", () => {
        const empty_opts = SniffOptions{ transport_layer_encoding: "", default_encoding: "" };

        const h1 = "<!doctype html><html><head><meta charset=\"utf-8\"></head></html>";
        expect_eq(strEq(sniffHtmlEncoding(h1, empty_opts), "UTF-8"), true);

        const h2 = "<html><head><META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=gb2312\"></head></html>";
        expect_eq(strEq(sniffHtmlEncoding(h2, empty_opts), "GBK"), true);

        // UTF-16 declared in <meta> maps to UTF-8 per WHATWG
        const h3 = "<html><head><meta charset='utf-16le'></head></html>";
        expect_eq(strEq(sniffHtmlEncoding(h3, empty_opts), "UTF-8"), true);
    });

    test("4. @cap/html-encoding-sniffer — Skips <meta> inside HTML comments", () => {
        const empty_opts = SniffOptions{ transport_layer_encoding: "", default_encoding: "" };
        const html = "<!-- <meta charset=\"gbk\"> --><html><head><meta charset=\"shift_jis\"></head></html>";
        expect_eq(strEq(sniffHtmlEncoding(html, empty_opts), "Shift_JIS"), true);
    });

    test("5. @cap/html-encoding-sniffer — Default encoding fallback (windows-1252 or custom)", () => {
        const plain = "<html><body>Hello world</body></html>";
        const default_opts = SniffOptions{ transport_layer_encoding: "", default_encoding: "" };
        expect_eq(strEq(sniffHtmlEncoding(plain, default_opts), "windows-1252"), true);

        const custom_opts = SniffOptions{ transport_layer_encoding: "", default_encoding: "utf-8" };
        expect_eq(strEq(sniffHtmlEncoding(plain, custom_opts), "UTF-8"), true);
    });

    test("6. @cap/html-encoding-sniffer — normalizeEncodingLabel WHATWG aliases", () => {
        expect_eq(strEq(normalizeEncodingLabel("  UTF8 "), "UTF-8"), true);
        expect_eq(strEq(normalizeEncodingLabel("latin1"), "windows-1252"), true);
        expect_eq(strEq(normalizeEncodingLabel("ISO-8859-1"), "windows-1252"), true);
        expect_eq(strEq(normalizeEncodingLabel("us-ascii"), "windows-1252"), true);
        expect_eq(strEq(normalizeEncodingLabel("gb2312"), "GBK"), true);
        expect_eq(strEq(normalizeEncodingLabel("sjis"), "Shift_JIS"), true);
        expect_eq(strEq(normalizeEncodingLabel("unknown-xyz"), ""), true);
    });
}
