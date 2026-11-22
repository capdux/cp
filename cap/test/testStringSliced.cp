// cap/test/testStringSliced.cp
// Production-grade deep tests for cap:string sliced.cp (Bun install_types/SlicedString)

import { test, assert, assertEq } from "cap:test";
import {
    SlicedStringError,
    SlicedSpan,
    SlicedString,
} from "cap:string";

test("SlicedString: fromBuffer / fromRange / subRange & offset preservation", () => {
    const raw = "registry:https://registry.npmjs.org/@bun/core@1.3.14";
    const full = SlicedString.fromBuffer(raw);
    assertEq(full.offsetInBuffer(), @as(usize, 0));
    assertEq(full.length(), raw.len);
    assert(!full.isEmpty());
    assert(full.eql(raw));

    const url_view = SlicedString.fromRange(raw, 9, raw.len) catch SlicedString.fromBuffer("");
    assertEq(url_view.offsetInBuffer(), @as(usize, 9));
    assert(url_view.eql("https://registry.npmjs.org/@bun/core@1.3.14"));

    // Secondary subRange keeps exact offset inside original host buffer
    const host = url_view.subRange(8, 26) catch SlicedString.fromBuffer("");
    assertEq(host.offsetInBuffer(), @as(usize, 17));
    assertEq(host.slice(), "registry.npmjs.org");
    assertEq(host.buffer().len, raw.len);
});

test("SlicedString: compact SlicedSpan round-trip & resolve", () => {
    const raw = "alpha:beta:gamma";
    const beta = SlicedString.fromRange(raw, 6, 10) catch SlicedString.fromBuffer("");
    const span = beta.toSpan();
    assertEq(span.offset, @as(u32, 6));
    assertEq(span.len, @as(u32, 4));
    assertEq(span.end(), @as(usize, 10));
    assert(!span.isEmpty());

    const resolved = span.resolve(raw) catch "";
    assertEq(resolved, "beta");

    const restored = SlicedString.fromSpan(raw, span) catch SlicedString.fromBuffer("");
    assert(restored.eql("beta"));
    assertEq(restored.offsetInBuffer(), @as(usize, 6));
});

test("SlicedString: trimAscii / stripPrefix / stripSuffix", () => {
    const raw = " \t\n  prefix_value_suffix \r\n ";
    const trimmed = SlicedString.fromBuffer(raw).trimAscii();
    assertEq(trimmed.slice(), "prefix_value_suffix");
    assertEq(trimmed.offsetInBuffer(), @as(usize, 5));

    if (no_pre := trimmed.stripPrefix("prefix_")) {
        assertEq(no_pre.slice(), "value_suffix");
        assertEq(no_pre.offsetInBuffer(), @as(usize, 12));
        if (core := no_pre.stripSuffix("_suffix")) {
            assertEq(core.slice(), "value");
            assertEq(core.offsetInBuffer(), @as(usize, 12));
        } else {
            assert(false);
        }
    } else {
        assert(false);
    }

    assert(trimmed.stripPrefix("missing") == null);
    assert(trimmed.stripSuffix("missing") == null);

    const all_ws = SlicedString.fromBuffer("   \t\r\n").trimAscii();
    assert(all_ws.isEmpty());
    assertEq(all_ws.slice(), "");
});

test("SlicedString: splitOnceByte preserves host buffer offsets", () => {
    const raw = "  @scope/pkg@1.2.3  ";
    const trimmed = SlicedString.fromBuffer(raw).trimAscii();
    if (s1 := trimmed.splitOnceByte('@')) {
        assert(s1.left.isEmpty());
        assertEq(s1.right.slice(), "scope/pkg@1.2.3");
        assertEq(s1.right.offsetInBuffer(), @as(usize, 3));

        if (s2 := s1.right.splitOnceByte('@')) {
            assertEq(s2.left.slice(), "scope/pkg");
            assertEq(s2.left.offsetInBuffer(), @as(usize, 3));
            assertEq(s2.right.slice(), "1.2.3");
            assertEq(s2.right.offsetInBuffer(), @as(usize, 13));
            const ver_span = s2.right.toSpan();
            assertEq(ver_span.resolve(raw) catch "", "1.2.3");
        } else {
            assert(false);
        }
    } else {
        assert(false);
    }

    assert(trimmed.splitOnceByte('#') == null);
});

test("SlicedString: hashFnv64 deterministic equality across distinct buffers", () => {
    const buf1 = "xxx_token_yyy";
    const buf2 = "token";
    const v1 = SlicedString.fromRange(buf1, 4, 9) catch SlicedString.fromBuffer("");
    const v2 = SlicedString.fromBuffer(buf2);
    const v3 = SlicedString.fromBuffer("tokeN");

    assertEq(v1.hashFnv64(), v2.hashFnv64());
    assert(v1.hashFnv64() != v3.hashFnv64());
});

test("SlicedString: out-of-bounds & invalid-range error guards", () => {
    const raw = "abcdef";
    let got_invalid = false;
    * = SlicedString.fromRange(raw, 4, 2) catch |e| {
        if (e == SlicedStringError.InvalidRange) {
            got_invalid = true;
        }
        SlicedString.fromBuffer("")
    };
    assert(got_invalid);

    let got_oob = false;
    * = SlicedString.fromRange(raw, 2, 10) catch |e| {
        if (e == SlicedStringError.OutOfBounds) {
            got_oob = true;
        }
        SlicedString.fromBuffer("")
    };
    assert(got_oob);

    const view = SlicedString.fromRange(raw, 1, 4) catch SlicedString.fromBuffer("");
    let got_sub_oob = false;
    * = view.subRange(0, 4) catch |e| {
        if (e == SlicedStringError.OutOfBounds) {
            got_sub_oob = true;
        }
        SlicedString.fromBuffer("")
    };
    assert(got_sub_oob);

    const bad_span = SlicedSpan{ offset: 4, len: 5 };
    let got_span_oob = false;
    * = bad_span.resolve(raw) catch |e| {
        if (e == SlicedStringError.OutOfBounds) {
            got_span_oob = true;
        }
        ""
    };
    assert(got_span_oob);
});
