// Source: cp/cap/test/test_header_value_iter_production_deep.cp (Production deep tests for cap:header_value_iter)

import { test, expect_eq } from "cap:test";
import {
    HeaderValueIterator,
    HeaderParam,
    HeaderParamIterator,
    CacheControl,
} from "cap:header_value_iter";

test("header_value_iter: comma splitting with OWS trimming, empty skipping and quoted comma/escape protection") {
    const raw = " , \t gzip , , \"quoted, with \\\"escaped\\\", commas\" , br , \t ";
    let iter = HeaderValueIterator.init(raw);

    const v1 = iter.next().?;
    expect_eq(std.mem.eql(u8, v1, "gzip"), true);

    const v2 = iter.next().?;
    expect_eq(std.mem.eql(u8, v2, "\"quoted, with \\\"escaped\\\", commas\""), true);

    const v3 = iter.next().?;
    expect_eq(std.mem.eql(u8, v3, "br"), true);

    expect_eq(iter.next() == null, true);
}

test("header_value_iter: Content-Disposition semicolon parameters with quoted semicolon") {
    const header = "attachment; filename=\"report; 2026.csv\"; size=4096; ; inline-flag";
    let iter = HeaderParamIterator.init(header);

    const p0 = iter.next().?;
    expect_eq(std.mem.eql(u8, p0.key, "attachment"), true);
    expect_eq(std.mem.eql(u8, p0.value, ""), true);
    expect_eq(p0.was_quoted, false);

    const p1 = iter.next().?;
    expect_eq(std.mem.eql(u8, p1.key, "filename"), true);
    expect_eq(std.mem.eql(u8, p1.value, "report; 2026.csv"), true);
    expect_eq(p1.was_quoted, true);

    const p2 = iter.next().?;
    expect_eq(std.mem.eql(u8, p2.key, "size"), true);
    expect_eq(std.mem.eql(u8, p2.value, "4096"), true);
    expect_eq(p2.was_quoted, false);

    const p3 = iter.next().?;
    expect_eq(std.mem.eql(u8, p3.key, "inline-flag"), true);
    expect_eq(std.mem.eql(u8, p3.value, ""), true);

    expect_eq(iter.next() == null, true);
}

test("header_value_iter: Set-Cookie flag parameters HttpOnly, Secure and SameSite") {
    const cookie = "session_id=abc123; Path=/; Max-Age=3600; HttpOnly; Secure; SameSite=Lax";
    let iter = HeaderParamIterator.init(cookie);

    const p0 = iter.next().?;
    expect_eq(std.mem.eql(u8, p0.key, "session_id"), true);
    expect_eq(std.mem.eql(u8, p0.value, "abc123"), true);

    const p1 = iter.next().?;
    expect_eq(std.mem.eql(u8, p1.key, "Path"), true);
    expect_eq(std.mem.eql(u8, p1.value, "/"), true);

    const p2 = iter.next().?;
    expect_eq(std.mem.eql(u8, p2.key, "Max-Age"), true);
    expect_eq(std.mem.eql(u8, p2.value, "3600"), true);

    const p3 = iter.next().?;
    expect_eq(std.mem.eql(u8, p3.key, "HttpOnly"), true);
    expect_eq(p3.value.len, 0);

    const p4 = iter.next().?;
    expect_eq(std.mem.eql(u8, p4.key, "Secure"), true);
    expect_eq(p4.value.len, 0);

    const p5 = iter.next().?;
    expect_eq(std.mem.eql(u8, p5.key, "SameSite"), true);
    expect_eq(std.mem.eql(u8, p5.value, "Lax"), true);

    expect_eq(iter.next() == null, true);
}

test("header_value_iter: Cache-Control full directives parsing") {
    const cc = CacheControl.parse(
        "PUBLIC, max-age=3600, s-maxage=\"86400\", stale-while-revalidate=60, immutable, no-transform"
    );
    expect_eq(cc.is_public, true);
    expect_eq(cc.is_private, false);
    expect_eq(cc.immutable, true);
    expect_eq(cc.no_transform, true);
    expect_eq(cc.no_cache, false);
    expect_eq(cc.no_store, false);
    expect_eq(cc.must_revalidate, false);
    expect_eq(cc.max_age.?, 3600);
    expect_eq(cc.s_maxage.?, 86400);
    expect_eq(cc.stale_while_revalidate.?, 60);

    const cc2 = CacheControl.parse("private, no-cache=\"Set-Cookie, Authorization\", no-store, must-revalidate");
    expect_eq(cc2.is_private, true);
    expect_eq(cc2.no_cache, true);
    expect_eq(cc2.no_store, true);
    expect_eq(cc2.must_revalidate, true);
    expect_eq(cc2.max_age == null, true);
}

test("header_value_iter: malformed inputs, empty headers and invalid Cache-Control numbers") {
    // 空与纯分隔符头部
    let empty_v = HeaderValueIterator.init("  , , \t , ");
    expect_eq(empty_v.next() == null, true);

    let empty_p = HeaderParamIterator.init(" ; ; =onlyval ; ");
    expect_eq(empty_p.next() == null, true);

    // 未闭合引号不越界崩溃
    let unclosed = HeaderParamIterator.init("name=\"unterminated");
    const pu = unclosed.next().?;
    expect_eq(std.mem.eql(u8, pu.key, "name"), true);
    expect_eq(std.mem.eql(u8, pu.value, "unterminated"), true);
    expect_eq(pu.was_quoted, true);
    expect_eq(unclosed.next() == null, true);

    // 非法 max-age 负值/非数字应安全忽略
    const bad_cc = CacheControl.parse("max-age=-10, s-maxage=abc, stale-while-revalidate=, public");
    expect_eq(bad_cc.is_public, true);
    expect_eq(bad_cc.max_age == null, true);
    expect_eq(bad_cc.s_maxage == null, true);
    expect_eq(bad_cc.stale_while_revalidate == null, true);
}
