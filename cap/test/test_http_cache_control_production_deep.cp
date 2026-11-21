import std from "std";
import { test, expect_eq } from "cap:test";
import { CacheControlIter, ParsedCacheControl as CacheControl } from "cap:http";

test("1. CacheControlIter handles quoted values with commas, escapes, and empty tokens", () => {
    const hdr = " , no-cache=\"Set-Cookie, Authorization\", max-age=3600 , x-custom=\"a\\\"b\" , ";
    let it = CacheControlIter.init(hdr);

    const d0 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, d0.name, "no-cache"), true);
    expect_eq(d0.was_quoted, true);
    expect_eq(std.mem.eql(u8, d0.value ?? unreachable, "Set-Cookie, Authorization"), true);

    const d1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, d1.name, "max-age"), true);
    expect_eq(d1.was_quoted, false);
    expect_eq(std.mem.eql(u8, d1.value ?? unreachable, "3600"), true);

    const d2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, d2.name, "x-custom"), true);
    expect_eq(d2.was_quoted, true);
    expect_eq(std.mem.eql(u8, d2.value ?? unreachable, "a\\\"b"), true);

    expect_eq(it.next() == null, true);
});

test("2. CacheControl.parse handles case-insensitivity and all standard RFC 9111 directives", () => {
    const cc = CacheControl.parse(
        "PUBLIC, Max-Age=60, S-MaxAge=600, stale-while-revalidate=30, stale-if-error=86400, min-fresh=10, max-stale=120, no-transform, must-revalidate, proxy-revalidate, IMMUTABLE, only-if-cached"
    );

    expect_eq(cc.is_public, true);
    expect_eq(cc.is_private, false);
    expect_eq(cc.max_age ?? unreachable, u64(60));
    expect_eq(cc.s_maxage ?? unreachable, u64(600));
    expect_eq(cc.stale_while_revalidate ?? unreachable, u64(30));
    expect_eq(cc.stale_if_error ?? unreachable, u64(86400));
    expect_eq(cc.min_fresh ?? unreachable, u64(10));
    expect_eq(cc.max_stale ?? unreachable, u64(120));
    expect_eq(cc.no_transform, true);
    expect_eq(cc.must_revalidate, true);
    expect_eq(cc.proxy_revalidate, true);
    expect_eq(cc.immutable, true);
    expect_eq(cc.only_if_cached, true);
    expect_eq(cc.isStorable(), true);
    expect_eq(cc.isStorableInSharedCache(), true);
});

test("3. effectiveMaxAge shared vs private cache precedence and no-store blocking", () => {
    const cdn = CacheControl.parse("public, max-age=60, s-maxage=3600");
    expect_eq(cdn.effectiveMaxAge(false) ?? unreachable, u64(60));
    expect_eq(cdn.effectiveMaxAge(true) ?? unreachable, u64(3600));

    const priv_cc = CacheControl.parse("private, max-age=300, s-maxage=900");
    expect_eq(priv_cc.isStorable(), true);
    expect_eq(priv_cc.isStorableInSharedCache(), false);
    expect_eq(priv_cc.effectiveMaxAge(false) ?? unreachable, u64(300));
    expect_eq(priv_cc.effectiveMaxAge(true) == null, true);

    const nostore = CacheControl.parse("no-store, max-age=300");
    expect_eq(nostore.isStorable(), false);
    expect_eq(nostore.effectiveMaxAge(false) == null, true);
    expect_eq(nostore.effectiveMaxAge(true) == null, true);
});

test("4. delta-seconds validation rejects negative, non-numeric, and u64 overflow", () => {
    const bad = CacheControl.parse("max-age=-10, s-maxage=abc, min-fresh=99999999999999999999999, stale-if-error=12s");
    expect_eq(bad.max_age == null, true);
    expect_eq(bad.s_maxage == null, true);
    expect_eq(bad.min_fresh == null, true);
    expect_eq(bad.stale_if_error == null, true);

    // max-stale 无值表示允许任意陈旧（U64_MAX）
    const ms = CacheControl.parse("max-stale");
    expect_eq(ms.max_stale ?? unreachable, u64(0xFFFF_FFFF_FFFF_FFFF));
});

test("5. CacheControl.format canonical serialization, round-trip, and buffer guard", () => {
    const src = CacheControl{
        is_public: true,
        immutable: true,
        max_age: 31536000,
        s_maxage: 604800,
        stale_while_revalidate: 60,
    };
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    const n = src.format(&buf) ?? unreachable;
    const formatted = buf[0..n];
    expect_eq(std.mem.eql(u8, formatted, "public, immutable, max-age=31536000, s-maxage=604800, stale-while-revalidate=60"), true);

    // 往返解析一致
    const reparsed = CacheControl.parse(formatted);
    expect_eq(reparsed.is_public, true);
    expect_eq(reparsed.immutable, true);
    expect_eq(reparsed.max_age ?? unreachable, u64(31536000));
    expect_eq(reparsed.s_maxage ?? unreachable, u64(604800));
    expect_eq(reparsed.stale_while_revalidate ?? unreachable, u64(60));

    // 空结构输出 0 字节
    const empty = CacheControl{};
    expect_eq(empty.format(&buf) ?? unreachable, usize(0));

    // 缓冲区不足安全返回 null
    let tiny: [10]u8 = std.mem.zeroes([10]u8);
    expect_eq(src.format(&tiny) == null, true);
});
