import {
    PreferIter,
    PreferHeader,
    ReturnPref,
    HandlingPref,
} from "cap:http";
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

test("PreferHeader parses standard RFC 7240 directives and first-wins", () => {
    const hdr = PreferHeader.parse("respond-async, return=minimal; foo=\"a,b\", wait=30, return=representation, handling=strict");
    expect(hdr.respond_async);
    expect(hdr.return_pref == ReturnPref.minimal); // first wins
    expect(hdr.handling == HandlingPref.strict);
    expect(hdr.wait_sec.? == 30);

    let buf: [128]u8 = [0; 128];
    expect(strEq(hdr.formatApplied(buf[0..]).?, "respond-async, return=minimal, handling=strict, wait=30"));
});

test("PreferIter handles quoted values, custom params, and invalid wait", () => {
    let it = PreferIter.init("Handling=\"lenient\"; level=2, wait=abc, custom=val");
    const p1 = it.next().?;
    expect(strEq(p1.name, "Handling"));
    expect(strEq(p1.value.?, "lenient"));
    expect(strEq(p1.params, "level=2"));

    const p2 = it.next().?;
    expect(strEq(p2.name, "wait"));
    expect(strEq(p2.value.?, "abc"));

    const p3 = it.next().?;
    expect(strEq(p3.name, "custom"));
    expect(it.next() == null);

    const parsed = PreferHeader.parse("Handling=\"lenient\", wait=abc");
    expect(parsed.handling == HandlingPref.lenient);
    expect(parsed.wait_sec == null);
});
