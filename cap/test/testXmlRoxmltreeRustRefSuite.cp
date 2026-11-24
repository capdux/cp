// Reference-hardened test suite against RazrFalcon/roxmltree (MIT/Apache-2.0) & W3C XML 1.0 (5th Ed)
// Covers compliant XML 1.0 documents (BOM, decl, namespaces, CDATA, comments, PIs, 5 entities,
// decimal/hex char refs, path queries & serialization roundtrip), strict malformed-input & limit guards,
// 16-lane @Vector(16, u8) SIMD vs scalar 0-diff parity, and 1,600 deterministic PRNG fuzz cases.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    XML_MAX_DEPTH,
    XML_MAX_ATTRS_PER_ELEMENT,
    XmlError,
    XmlParser,
    XmlSerializer,
    simdFindXmlTextDelim,
    scalarFindXmlTextDelim,
    simdHasInvalidXmlControl,
    scalarHasInvalidXmlControl,
    simdFindAttrQuoteOrAmp,
    scalarFindAttrQuoteOrAmp,
    validateXmlStrict,
} from "cap:xml";

struct XorShift64 {
    state: u64;

    static fn init(seed: u64): XorShift64 {
        return XorShift64{ state: if (seed == 0) 1 else seed };
    }

    fn next(*_): u64 {
        let x = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }

    fn nextByte(*_): u8 {
        return @as(u8, @truncate(_.next()));
    }

    fn fillBytes(*_, dst: []u8) {
        let i: usize = 0;
        while (i < dst.len) : (i += 1) {
            dst[i] = _.nextByte();
        }
    }
}

fn checkCompliantXml10(alloc: std.mem.Allocator): u32 {
    const doc_xml =
        "\xEF\xBB\xBF" ++
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n" ++
        "<!DOCTYPE feed SYSTEM \"feed.dtd\">\n" ++
        "<!-- valid comment with single - dashes - ok -->\n" ++
        "<ns:feed xmlns:ns=\"https://example.org/ns\" id='f-1' title=\"A &amp; B &lt;C&gt; &quot;D&quot; &apos;E&apos;\">\n" ++
        "  <?app-hint mode=\"fast\"?>\n" ++
        "  <ns:item idx=\"0\">\n" ++
        "    <ns:text>Dec: &#65;&#20013; Hex: &#x42;&#x1f600; Brackets: ] ]> ok</ns:text>\n" ++
        "    <ns:raw><![CDATA[raw <tag> &amp; -- inside cdata]]></ns:raw>\n" ++
        "    <ns:empty flag='true'/>\n" ++
        "  </ns:item>\n" ++
        "</ns:feed>\n" ++
        "<!-- trailing comment -->\n" ++
        "<?trailing-pi ok?>";

    validateXmlStrict(doc_xml) catch return 1;

    let doc = XmlParser.parse(alloc, doc_xml) catch return 2;
    defer doc.deinit();

    if (!std.mem.eql(u8, doc.version, "1.0")) return 3;
    if (!std.mem.eql(u8, doc.encoding, "UTF-8")) return 4;
    if (!std.mem.eql(u8, doc.root.tag, "ns:feed")) return 5;
    if (!std.mem.eql(u8, doc.root.getAttr("title") ?? "", "A & B <C> \"D\" 'E'")) return 6;

    const text_node = doc.root.findByPath("ns:item/ns:text") ?? return 7;
    const text_val = text_node.innerText(alloc) catch return 8;
    defer alloc.free(text_val);
    if (!std.mem.eql(u8, text_val, "Dec: A\xE4\xB8\xAD Hex: B\xF0\x9F\x98\x80 Brackets: ] ]> ok")) return 9;

    const raw_node = doc.root.findByPath("ns:item/ns:raw") ?? return 10;
    const raw_val = raw_node.innerText(alloc) catch return 11;
    defer alloc.free(raw_val);
    if (!std.mem.eql(u8, raw_val, "raw <tag> &amp; -- inside cdata")) return 12;

    // Compact + pretty serialization roundtrip
    const compact = XmlSerializer.stringify(alloc, doc.root, false) catch return 13;
    defer alloc.free(compact);
    validateXmlStrict(compact) catch return 14;

    const pretty = XmlSerializer.stringify(alloc, doc.root, true) catch return 15;
    defer alloc.free(pretty);
    validateXmlStrict(pretty) catch return 16;

    let doc2 = XmlParser.parse(alloc, pretty) catch return 17;
    defer doc2.deinit();
    const compact2 = XmlSerializer.stringify(alloc, doc2.root, false) catch return 18;
    defer alloc.free(compact2);
    if (!std.mem.eql(u8, compact, compact2)) return 19;

    return 0;
}

fn expectBothFail(alloc: std.mem.Allocator, xml: []const u8, expected_err: XmlError): bool {
    let strict_ok = false;
    validateXmlStrict(xml) catch |e| {
        if (e == expected_err) strict_ok = true;
    };
    if (!strict_ok) return false;

    let parse_ok = false;
    * = XmlParser.parse(alloc, xml) catch |e| {
        if (e == @as(anyerror, expected_err)) parse_ok = true;
    };
    return parse_ok;
}

fn checkStrictRejectionGuards(alloc: std.mem.Allocator): u32 {
    // 1. Duplicate attributes on same element
    if (!expectBothFail(alloc, "<root a=\"1\" b=\"2\" a=\"3\"/>", XmlError.DuplicateAttribute)) return 1;
    if (!expectBothFail(alloc, "<root ns:k='x' ns:k='y'></root>", XmlError.DuplicateAttribute)) return 2;

    // 2. Mismatched open/close tags
    if (!expectBothFail(alloc, "<root><child></other></root>", XmlError.MismatchedTag)) return 3;

    // 3. Forbidden XML 1.0 control bytes (0x00..0x08, 0x0B, 0x0C, 0x0E..0x1F)
    const bad_ctrl_cases: [6]string = [
        "<r>\x00</r>",
        "<r>\x01</r>",
        "<r>\x08</r>",
        "<r>\x0B</r>",
        "<r>\x0C</r>",
        "<r>\x1F</r>",
    ];
    for (bc of bad_ctrl_cases) {
        if (!expectBothFail(alloc, bc, XmlError.InvalidControlChar)) return 4;
    }
    // Allowed whitespace controls (\t, \n, \r) must pass
    validateXmlStrict("<r>\t\n\r</r>") catch return 5;

    // 4. Comment containing "--" or ending with "--->"
    if (!expectBothFail(alloc, "<r><!-- a--b --></r>", XmlError.MalformedTag)) return 6;
    if (!expectBothFail(alloc, "<r><!-- a---></r>", XmlError.MalformedTag)) return 7;
    if (!expectBothFail(alloc, "<!-- bad--comment --><r/>", XmlError.MalformedTag)) return 8;

    // 5. Bare "]]>" inside text content
    if (!expectBothFail(alloc, "<r>hello ]]> world</r>", XmlError.MalformedTag)) return 9;
    if (!expectBothFail(alloc, "<r>]]> </r>", XmlError.MalformedTag)) return 10;

    // 6. Invalid numeric & named entities (0, control, surrogates, FFFE/FFFF, >10FFFF, uppercase X, overflow)
    const bad_entities: [11]string = [
        "<r>&#0;</r>",
        "<r>&#11;</r>",
        "<r>&#xD800;</r>",
        "<r>&#xDFFF;</r>",
        "<r>&#xFFFE;</r>",
        "<r>&#xFFFF;</r>",
        "<r>&#x110000;</r>",
        "<r>&#99999999999999999;</r>",
        "<r>&#X41;</r>",
        "<r>&unknown;</r>",
        "<r>&amp</r>",
    ];
    for (be of bad_entities) {
        if (!expectBothFail(alloc, be, XmlError.InvalidEntity)) return 11;
    }

    // 7. Unclosed tag, attribute, comment, CDATA, PI
    const unclosed_cases: [5]string = [
        "<root><child>",
        "<root a=\"unclosed>",
        "<root><!-- unclosed",
        "<root><![CDATA[unclosed",
        "<root><?pi unclosed",
    ];
    for (uc of unclosed_cases) {
        if (!expectBothFail(alloc, uc, XmlError.UnexpectedEof)) return 12;
    }

    // 8. Empty tag, invalid NameStartChar, multiple roots, misplaced/case-wrong XML decl
    const malformed_cases: [7]string = [
        "<>",
        "<1bad/>",
        "<a/><b/>",
        " <?xml version=\"1.0\"?><a/>",
        "<?XML version=\"1.0\"?><a/>",
        "<a/><?xml version=\"1.0\"?>",
        "<?xml encoding=\"UTF-8\"?><a/>",
    ];
    for (mc of malformed_cases) {
        if (!expectBothFail(alloc, mc, XmlError.MalformedTag)) return 13;
    }

    // 9. Nesting depth boundary: XML_MAX_DEPTH passes, XML_MAX_DEPTH + 1 fails with MaxDepthExceeded
    {
        let ok_buf: std.ArrayList(u8) = .empty;
        defer ok_buf.deinit(alloc);
        let i: usize = 0;
        while (i < XML_MAX_DEPTH) : (i += 1) {
            ok_buf.appendSlice(alloc, "<a>") catch return 14;
        }
        i = 0;
        while (i < XML_MAX_DEPTH) : (i += 1) {
            ok_buf.appendSlice(alloc, "</a>") catch return 15;
        }
        validateXmlStrict(ok_buf.items) catch return 16;

        let bad_buf: std.ArrayList(u8) = .empty;
        defer bad_buf.deinit(alloc);
        i = 0;
        while (i < XML_MAX_DEPTH + 1) : (i += 1) {
            bad_buf.appendSlice(alloc, "<a>") catch return 17;
        }
        i = 0;
        while (i < XML_MAX_DEPTH + 1) : (i += 1) {
            bad_buf.appendSlice(alloc, "</a>") catch return 18;
        }
        if (!expectBothFail(alloc, bad_buf.items, XmlError.MaxDepthExceeded)) return 19;
    }

    // 10. Per-element attribute count limit (> XML_MAX_ATTRS_PER_ELEMENT)
    {
        let attr_xml: std.ArrayList(u8) = .empty;
        defer attr_xml.deinit(alloc);
        attr_xml.appendSlice(alloc, "<r") catch return 20;
        let num_buf: [16]u8 = [0; 16];
        let i: usize = 0;
        while (i <= XML_MAX_ATTRS_PER_ELEMENT) : (i += 1) {
            const part = std.fmt.bufPrint(&num_buf, " a{d}=\"1\"", .{i}) catch return 21;
            attr_xml.appendSlice(alloc, part) catch return 22;
        }
        attr_xml.appendSlice(alloc, "/>") catch return 23;
        if (!expectBothFail(alloc, attr_xml.items, XmlError.MaxAttrsExceeded)) return 24;
    }

    return 0;
}

fn checkSimdDifferential(): u32 {
    // 1. All 256 byte values individually & in 32-byte padded buffers for simdHasInvalidXmlControl
    let single: [1]u8 = [0; 1];
    let padded: [32]u8 = ['a'; 32];
    let bv: usize = 0;
    while (bv < 256) : (bv += 1) {
        const b = @as(u8, @intCast(bv));
        single[0] = b;
        if (simdHasInvalidXmlControl(single[0..]) != scalarHasInvalidXmlControl(single[0..])) return 1;
        padded[15] = b;
        if (simdHasInvalidXmlControl(padded[0..]) != scalarHasInvalidXmlControl(padded[0..])) return 2;
        padded[15] = 'a';
        padded[16] = b;
        if (simdHasInvalidXmlControl(padded[0..]) != scalarHasInvalidXmlControl(padded[0..])) return 3;
        padded[16] = 'a';
        padded[31] = b;
        if (simdHasInvalidXmlControl(padded[0..]) != scalarHasInvalidXmlControl(padded[0..])) return 4;
        padded[31] = 'a';
    }

    // 2. Full 0..257 length sweep with PRNG data and controlled delimiters at start/mid/end/16B boundaries
    let prng = XorShift64.init(0x584d_4c53_494d_4431);
    let buf: [260]u8 = ['x'; 260];
    let len: usize = 0;
    while (len <= 257) : (len += 1) {
        let i: usize = 0;
        while (i < len) : (i += 1) {
            // Safe printable ASCII without <, &, ], ", '
            const r = prng.nextByte() % 26;
            buf[i] = @as(u8, 'a') + r;
        }
        const slice = buf[0..len];

        // No delimiters / no controls baseline
        if (simdHasInvalidXmlControl(slice) != scalarHasInvalidXmlControl(slice)) return 5;
        if (simdFindXmlTextDelim(slice, 0) != scalarFindXmlTextDelim(slice, 0)) return 6;
        if (simdFindAttrQuoteOrAmp(slice, 0, '"') != scalarFindAttrQuoteOrAmp(slice, 0, '"')) return 7;
        if (simdFindAttrQuoteOrAmp(slice, 0, '\'') != scalarFindAttrQuoteOrAmp(slice, 0, '\'')) return 8;

        if (len > 0) {
            const targets: [4]usize = [ 0, len / 2, len - 1, if (len > 17) 17 else 0 ];
            const text_delims: [3]u8 = [ '<', '&', ']' ];
            for (pos of targets) {
                const orig = buf[pos];
                for (td of text_delims) {
                    buf[pos] = td;
                    const starts: [3]usize = [ 0, if (pos > 0) pos - 1 else 0, pos ];
                    for (st of starts) {
                        if (simdFindXmlTextDelim(buf[0..len], st) != scalarFindXmlTextDelim(buf[0..len], st)) return 9;
                    }
                }
                const attr_delims: [4]u8 = [ '"', '\'', '&', '<' ];
                for (ad of attr_delims) {
                    buf[pos] = ad;
                    if (simdFindAttrQuoteOrAmp(buf[0..len], 0, '"') != scalarFindAttrQuoteOrAmp(buf[0..len], 0, '"')) return 10;
                    if (simdFindAttrQuoteOrAmp(buf[0..len], 0, '\'') != scalarFindAttrQuoteOrAmp(buf[0..len], 0, '\'')) return 11;
                }
                buf[pos] = 0x0B;
                if (simdHasInvalidXmlControl(buf[0..len]) != scalarHasInvalidXmlControl(buf[0..len])) return 12;
                buf[pos] = orig;
            }
        }
    }

    return 0;
}

fn canParseXml(alloc: std.mem.Allocator, data: []const u8): bool {
    let doc = XmlParser.parse(alloc, data) catch return false;
    doc.deinit();
    return true;
}

fn checkDeterministicFuzz(alloc: std.mem.Allocator): u32 {
    let prng = XorShift64.init(0x2026_1124_584d_4c46);
    const seed_xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" ++
        "<!-- c --><root a=\"1 &amp; &#65;\" b='ok'><c><![CDATA[xyz]]></c><e/></root>";

    let rand_buf = std.mem.zeroes([192]u8);

    // 1,600 deterministic fuzz cases: 800 mutated valid XML + 800 raw random byte streams
    // Also asserts strict equivalence between validateXmlStrict and XmlParser.parse!
    let iter: usize = 0;
    while (iter < 1600) : (iter += 1) {
        if (iter % 2 == 0) {
            const cut_len = @as(usize, @intCast(prng.next() % (seed_xml.len + 1)));
            let mutated = alloc.dupe(u8, seed_xml[0..cut_len]) catch return 1;
            defer alloc.free(mutated);
            if (mutated.len > 0) {
                const edits = 1 + @as(usize, @intCast(prng.next() % 3));
                let ei: usize = 0;
                while (ei < edits) : (ei += 1) {
                    const idx = @as(usize, @intCast(prng.next() % mutated.len));
                    mutated[idx] = prng.nextByte();
                }
            }
            let strict_passed = true;
            validateXmlStrict(mutated) catch {
                strict_passed = false;
            };
            const parse_passed = canParseXml(alloc, mutated);
            if (strict_passed != parse_passed) return 2;
        } else {
            const n = @as(usize, @intCast(prng.next() % 192));
            prng.fillBytes(rand_buf[0..n]);
            const slice = rand_buf[0..n];

            let strict_passed = true;
            validateXmlStrict(slice) catch {
                strict_passed = false;
            };
            const parse_passed = canParseXml(alloc, slice);
            if (strict_passed != parse_passed) return 3;
        }
    }

    return 0;
}

fn main() {
    test("1. W3C XML 1.0 compliant document (BOM, decl, NS, CDATA, entities, queries & roundtrip)", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        expect_eq(checkCompliantXml10(alloc), 0 as u32);
    });

    test("2. Strict roxmltree/W3C guards (depth, dup-attr, ctrl-bytes, comment --, bare ]]>, entities)", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        expect_eq(checkStrictRejectionGuards(alloc), 0 as u32);
    });

    test("3. 16-lane @Vector(16, u8) SIMD vs scalar 0-diff parity (all 256 bytes & 0..257 lengths)", () => {
        expect_eq(checkSimdDifferential(), 0 as u32);
    });

    test("4. Deterministic PRNG fuzzing (1,600 mutated/random XML inputs + strict/parse parity)", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        expect_eq(checkDeterministicFuzz(alloc), 0 as u32);
    });
}
