import { test, expect_eq } from "cap:test";
import {
    DEFAULT_MAX_HEADERS_PER_PART,
    MultipartError,
    MultipartDocument,
    MultipartBuilder,
    scalarMpFindDashOrCrlf,
    simdMpFindDashOrCrlf,
    isValidMultipartBoundary,
    extractBoundary,
    parse,
    parseWithLimits,
} from "../../lib/web/multipart.cp";
import {
    URI_TEMPLATE_DEFAULT_MAX_OUTPUT,
    UriTemplateError,
    AssocPair,
    TemplateVars,
    UriTemplate,
    scalarUriFindBrace,
    simdUriFindBrace,
} from "../../lib/web/uritemplate.cp";

const std = @import("std");

fn expect_true(cond: bool): void! {
    expect_eq(cond, true);
}

fn expect_false(cond: bool): void! {
    expect_eq(cond, false);
}

fn tryParseMp(alloc: std.mem.Allocator, bnd: []const u8, body: []const u8): MultipartDocument? {
    return parseWithLimits(alloc, bnd, body, 16, 4096) catch return null;
}

fn tryParseUri(alloc: std.mem.Allocator, pat: []const u8): UriTemplate? {
    return UriTemplate.parse(alloc, pat) catch return null;
}

struct XorShift64 {
    state: u64 = 0x1E3779B97F4A7C15,

    fn next(*_): u64 {
        let x = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }
}

// ---------------------------------------------------------------------------
// Test 1: RFC 7578 Multipart Strict Boundaries, Fake Prefix & Injection Guard
// ---------------------------------------------------------------------------

fn runTestMultipartStrict(): void! {
    const alloc = std.heap.page_allocator;

    // 1. Boundary validation (1..=70 bytes, bchars, no trailing space, no CRLF/CTL)
    expect_true(isValidMultipartBoundary("----WebKitFormBoundary7MA4YWxkTrZu0gW"))?;
    expect_false(isValidMultipartBoundary(""))?;
    expect_false(isValidMultipartBoundary("trailing_space "))?;
    expect_false(isValidMultipartBoundary("bad\r\nboundary"))?;
    const too_long_71: [71]u8 = ['a'; 71];
    expect_false(isValidMultipartBoundary(too_long_71[0..]))?;
    const exact_70: [70]u8 = ['a'; 70];
    expect_true(isValidMultipartBoundary(exact_70[0..]))?;

    const b1 = extractBoundary("multipart/form-data; boundary=\"abc-123\"" ) catch unreachable;
    expect_true(std.mem.eql(u8, b1, "abc-123"))?;

    let bad_ct = false;
    * = extractBoundary("multipart/form-data; boundary=abc\r\nX-Injected: 1") catch { bad_ct = true; };
    expect_true(bad_ct)?;

    // 2. Builder -> Parser roundtrip including fake prefix "--bnd_extra" inside payload & binary NUL
    let mb = MultipartBuilder.init(alloc, "bnd") catch unreachable;
    defer mb.deinit();
    mb.addField("note", "line1\r\n--bnd_extra\r\nline3") catch unreachable;
    mb.addFile("upload", "my \"file\".bin", "application/octet-stream", "\x00\x01\x02--bnd_not_end\xFF") catch unreachable;

    // Injection attempt in builder rejected
    let inj_err = false;
    mb.addField("bad\r\nname", "val") catch |e| {
        if (e == MultipartError.InvalidHeaderValue) inj_err = true;
    };
    expect_true(inj_err)?;

    const body = mb.finish(alloc) catch unreachable;
    defer alloc.free(body);

    let doc = parse(alloc, "bnd", body) catch unreachable;
    defer doc.deinit(alloc);
    expect_eq(doc.parts.len, 2 as usize);
    const f_note = doc.findField("note").?;
    expect_true(std.mem.eql(u8, f_note.data, "line1\r\n--bnd_extra\r\nline3"))?;
    const f_up = doc.findField("upload").?;
    expect_true(std.mem.eql(u8, f_up.filename.?, "my \"file\".bin"))?;
    expect_true(std.mem.eql(u8, f_up.content_type, "application/octet-stream"))?;
    expect_true(std.mem.eql(u8, f_up.data, "\x00\x01\x02--bnd_not_end\xFF"))?;

    // 3. Missing closing boundary "--bnd--" rejected
    const no_close = "--bnd\r\nContent-Disposition: form-data; name=\"a\"\r\n\r\nhello\r\n";
    let got_missing = false;
    * = parse(alloc, "bnd", no_close) catch |e| {
        if (e == MultipartError.MissingEndBoundary) got_missing = true;
    };
    expect_true(got_missing)?;

    // 4. Max parts limit & max headers per part limit
    let got_too_many = false;
    * = parseWithLimits(alloc, "bnd", body, 1, 1024) catch |e| {
        if (e == MultipartError.TooManyParts) got_too_many = true;
    };
    expect_true(got_too_many)?;

    // Exceed DEFAULT_MAX_HEADERS_PER_PART (64)
    let hdr_flood = std.ArrayList(u8).empty;
    defer hdr_flood.deinit(alloc);
    hdr_flood.appendSlice(alloc, "--bnd\r\nContent-Disposition: form-data; name=\"x\"\r\n") catch unreachable;
    let hi: usize = 0;
    while (hi <= DEFAULT_MAX_HEADERS_PER_PART) : (hi += 1) {
        hdr_flood.appendSlice(alloc, "X-H: 1\r\n") catch unreachable;
    }
    hdr_flood.appendSlice(alloc, "\r\nbody\r\n--bnd--\r\n") catch unreachable;
    let got_hdr_limit = false;
    * = parse(alloc, "bnd", hdr_flood.items) catch |e| {
        if (e == MultipartError.MalformedPartHeader) got_hdr_limit = true;
    };
    expect_true(got_hdr_limit)?;
}

// ---------------------------------------------------------------------------
// Test 2: RFC 6570 Level 1-4 Official Vectors, Modifiers & Output Cap
// ---------------------------------------------------------------------------

fn assertExpand(alloc: std.mem.Allocator, vars: *TemplateVars, pattern: []const u8, expected: []const u8): void! {
    let tpl = UriTemplate.parse(alloc, pattern) catch unreachable;
    defer tpl.deinit();
    const out = tpl.expand(alloc, vars) catch unreachable;
    defer alloc.free(out);
    expect_true(std.mem.eql(u8, out, expected))?;
}

fn runTestUriTemplateRfc6570(): void! {
    const alloc = std.heap.page_allocator;
    let vars = TemplateVars.init(alloc);
    defer vars.deinit();

    vars.set("var", "value") catch unreachable;
    vars.set("hello", "Hello World!") catch unreachable;
    vars.set("path", "/foo/bar") catch unreachable;
    vars.set("x", "1024") catch unreachable;
    vars.set("y", "768") catch unreachable;
    vars.set("empty", "") catch unreachable;
    const list_vals: [3][]const u8 = ["red", "green", "blue"];
    vars.setList("list", list_vals[0..]) catch unreachable;
    const key_pairs: [3]AssocPair = [
        AssocPair{ key: "semi", value: ";" },
        AssocPair{ key: "dot", value: "." },
        AssocPair{ key: "comma", value: "," },
    ];
    vars.setAssoc("keys", key_pairs[0..]) catch unreachable;

    // Level 1-4 official RFC 6570 Section 3.2 vectors across all 7 operators + Simple
    assertExpand(alloc, &vars, "{var}", "value")?;
    assertExpand(alloc, &vars, "{hello}", "Hello%20World%21")?;
    assertExpand(alloc, &vars, "{+path}/here", "/foo/bar/here")?;
    assertExpand(alloc, &vars, "{+hello}", "Hello%20World!")?;
    assertExpand(alloc, &vars, "{#hello}", "#Hello%20World!")?;
    assertExpand(alloc, &vars, "X{.var}", "X.value")?;
    assertExpand(alloc, &vars, "{/var,x}/here", "/value/1024/here")?;
    assertExpand(alloc, &vars, "{;x,y,empty}", ";x=1024;y=768;empty")?;
    assertExpand(alloc, &vars, "{?x,y,empty}", "?x=1024&y=768&empty=")?;
    assertExpand(alloc, &vars, "?fixed=yes{&x}", "?fixed=yes&x=1024")?;
    // Level 4 prefix :N and explode *
    assertExpand(alloc, &vars, "{var:3}", "val")?;
    assertExpand(alloc, &vars, "{/list*}", "/red/green/blue")?;
    assertExpand(alloc, &vars, "{?list*}", "?list=red&list=green&list=blue")?;
    assertExpand(alloc, &vars, "{?keys*}", "?semi=%3B&dot=.&comma=%2C")?;

    // Reject malformed templates: unclosed, stray close, nested, invalid op, invalid prefix modifier
    const bad_pats: [8][]const u8 = [
        "/{unclosed",
        "/stray}",
        "/{{nested}}",
        "/{}",
        "/{=var}",
        "/{var:0}",
        "/{var:10000}",
        "/{var:3*}",
    ];
    let bi: usize = 0;
    while (bi < bad_pats.len) : (bi += 1) {
        if (t := tryParseUri(alloc, bad_pats[bi])) {
            let tm = t;
            tm.deinit();
            expect_true(false)?;
        }
    }

    // Output limit blowup protection
    let tpl_cap = UriTemplate.parse(alloc, "{hello}/{hello}/{hello}") catch unreachable;
    defer tpl_cap.deinit();
    let got_too_large = false;
    * = tpl_cap.expandWithLimit(alloc, &vars, 16) catch |e| {
        if (e == UriTemplateError.OutputTooLarge) got_too_large = true;
    };
    expect_true(got_too_large)?;
}

// ---------------------------------------------------------------------------
// Test 3: 16-Lane SIMD vs Scalar 0-Diff Verification
// ---------------------------------------------------------------------------

fn runTestSimdZeroDiff(): void! {
    let buf: [80]u8 = [0; 80];
    let len: usize = 0;
    while (len <= 80) : (len += 1) {
        let i: usize = 0;
        while (i < len) : (i += 1) {
            buf[i] = (65 as u8) + ((i % 26) as u8);
        }
        expect_eq(simdMpFindDashOrCrlf(buf[0..len]), scalarMpFindDashOrCrlf(buf[0..len]));
        expect_eq(simdUriFindBrace(buf[0..len]), scalarUriFindBrace(buf[0..len]));

        if (len > 0 and (len <= 34 or len == 48 or len == 64 or len == 80)) {
            const mp_chars: [3]u8 = ['-', '\r', '\n'];
            let ci: usize = 0;
            while (ci < mp_chars.len) : (ci += 1) {
                let p: usize = 0;
                while (p < len) : (p += 1) {
                    const saved = buf[p];
                    buf[p] = mp_chars[ci];
                    expect_eq(simdMpFindDashOrCrlf(buf[0..len]), scalarMpFindDashOrCrlf(buf[0..len]));
                    expect_eq(simdMpFindDashOrCrlf(buf[0..len]).?, p);
                    buf[p] = saved;
                }
            }

            const br_chars: [2]u8 = ['{', '}'];
            ci = 0;
            while (ci < br_chars.len) : (ci += 1) {
                let p: usize = 0;
                while (p < len) : (p += 1) {
                    const saved = buf[p];
                    buf[p] = br_chars[ci];
                    expect_eq(simdUriFindBrace(buf[0..len]), scalarUriFindBrace(buf[0..len]));
                    expect_eq(simdUriFindBrace(buf[0..len]).?, p);
                    buf[p] = saved;
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Test 4: 1,600-Case Deterministic PRNG Fuzz (800 Multipart + 800 URI Template)
// ---------------------------------------------------------------------------

fn runTestWebMpUriFuzz1600(): void! {
    const alloc = std.heap.page_allocator;
    let rng = XorShift64 { state: 0x3708AB20261124 };
    let buf: [160]u8 = [0; 160];

    // Part A: 800 Multipart fuzz cases
    let mi: usize = 0;
    while (mi < 800) : (mi += 1) {
        const flen = (rng.next() as usize) % 160;
        let j: usize = 0;
        while (j < flen) : (j += 1) {
            const r = rng.next() % 8;
            if (r == 0) {
                buf[j] = '-';
            } else if (r == 1) {
                buf[j] = '\r';
            } else if (r == 2) {
                buf[j] = '\n';
            } else if (r == 3) {
                buf[j] = ':';
            } else if (r == 4) {
                buf[j] = '"';
            } else {
                buf[j] = (rng.next() & 0x7F) as u8;
            }
        }
        const sl = buf[0..flen];
        expect_eq(simdMpFindDashOrCrlf(sl), scalarMpFindDashOrCrlf(sl));
        if (doc := tryParseMp(alloc, "bnd", sl)) {
            let dm = doc;
            dm.deinit(alloc);
        }
    }

    // Part B: 800 URI Template fuzz cases
    let vars = TemplateVars.init(alloc);
    defer vars.deinit();
    vars.set("a", "hello world") catch unreachable;
    vars.set("b", "/x/y") catch unreachable;

    let ui: usize = 0;
    while (ui < 800) : (ui += 1) {
        const flen = (rng.next() as usize) % 96;
        let j: usize = 0;
        while (j < flen) : (j += 1) {
            const r = rng.next() % 10;
            if (r == 0) {
                buf[j] = '{';
            } else if (r == 1) {
                buf[j] = '}';
            } else if (r == 2) {
                buf[j] = '+';
            } else if (r == 3) {
                buf[j] = '?';
            } else if (r == 4) {
                buf[j] = ':';
            } else if (r == 5) {
                buf[j] = '*';
            } else if (r == 6) {
                buf[j] = 'a';
            } else if (r == 7) {
                buf[j] = ',';
            } else {
                buf[j] = (rng.next() & 0x7F) as u8;
            }
        }
        const sl = buf[0..flen];
        expect_eq(simdUriFindBrace(sl), scalarUriFindBrace(sl));
        if (tpl := tryParseUri(alloc, sl)) {
            let tm = tpl;
            if (expanded := tryExpandUriLimit(tm, alloc, &vars, 256)) {
                alloc.free(expanded);
            }
            tm.deinit();
        }
    }
}

fn tryExpandUriLimit(tm: UriTemplate, alloc: std.mem.Allocator, vars: *TemplateVars, maxOut: usize): ([]u8)? {
    const out = tm.expandWithLimit(alloc, vars, maxOut) catch {
        return null;
    };
    return out;
}

fn main() {
    test("RFC 7578 Multipart boundary validation, fake prefix, limits & injection guard", () => {
        runTestMultipartStrict()?;
    });
    test("RFC 6570 Level 1-4 URI Template vectors, modifiers, errors & output limit", () => {
        runTestUriTemplateRfc6570()?;
    });
    test("16-lane SIMD vs Scalar 0-diff differential verification", () => {
        runTestSimdZeroDiff()?;
    });
    test("1600-case deterministic PRNG Fuzz (800 Multipart + 800 URI Template)", () => {
        runTestWebMpUriFuzz1600()?;
    });
}
