import {
    encodeFormComponent,
    decodeFormComponent,
    FormIter,
    getFormParam,
    appendFormParam,
} from "cap:http_form";
import { test, expect } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("encodeFormComponent and decodeFormComponent WHATWG rules", () => {
    let buf: [128]u8 = [0; 128];
    const enc = encodeFormComponent("hello world & 100%!", buf[0..]).?;
    expect(strEq(enc, "hello+world+%26+100%25%21"));

    let dec_buf: [128]u8 = [0; 128];
    expect(strEq(decodeFormComponent(enc, dec_buf[0..]).?, "hello world & 100%!"));
    // Malformed %ZZ preserved verbatim per WHATWG
    expect(strEq(decodeFormComponent("a+b%ZZc", dec_buf[0..]).?, "a b%ZZc"));
});

test("FormIter, appendFormParam, and getFormParam round-trip", () => {
    let out: [256]u8 = [0; 256];
    let pos: usize = 0;
    expect(appendFormParam(out[0..], &pos, "q", "cp lang"));
    expect(appendFormParam(out[0..], &pos, "tag+1", "a&b=c"));
    expect(appendFormParam(out[0..], &pos, "empty", ""));

    const encoded = out[0..pos];
    let vbuf: [64]u8 = [0; 64];
    expect(strEq(getFormParam(encoded, "q", vbuf[0..]).?, "cp lang"));
    expect(strEq(getFormParam(encoded, "tag+1", vbuf[0..]).?, "a&b=c"));
    expect(strEq(getFormParam(encoded, "empty", vbuf[0..]).?, ""));
    expect(getFormParam(encoded, "missing", vbuf[0..]) == null);

    // Leading '?' and valueless key
    let it = FormIter.init("?flag&&k=v");
    const p1 = it.next().?;
    expect(strEq(p1.raw_key, "flag") && p1.raw_value.len == 0);
    const p2 = it.next().?;
    expect(strEq(p2.raw_key, "k") && strEq(p2.raw_value, "v"));
    expect(it.next() == null);
});
