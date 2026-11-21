import { test, expect_eq } from "cap:test";
import {
    ENC_MASK_IDENTITY,
    ENC_MASK_GZIP,
    ENC_MASK_BR,
    ENC_MASK_ZSTD,
    ENC_MASK_ALL,
    ContentEncoding,
    negotiateEncodingMask as negotiateEncoding,
    MetaFetchCacheMode as FetchCacheMode,
    MetaFetchRedirect as FetchRedirect,
    MetaFetchRequestMode as FetchRequestMode,
    UrlPath,
} from "cap:http";

test("1. Accept-Encoding q-factor negotiation (zstd/br/gzip/q=0 disable/* fallback)", () => {
    // Highest q wins: br (1.0) > zstd (0.9) > gzip (0.5)
    expect_eq(
        negotiateEncoding("zstd;q=0.9, br;q=1.0, gzip;q=0.5", ENC_MASK_ALL) == ContentEncoding.br,
        true,
    );

    // Explicit br;q=0 disables brotli even when '*' is present
    expect_eq(
        negotiateEncoding("br;q=0, *;q=0.8", ENC_MASK_BR | ENC_MASK_GZIP | ENC_MASK_IDENTITY) == ContentEncoding.gzip,
        true,
    );

    // Server does not support zstd/br -> falls back to gzip
    expect_eq(
        negotiateEncoding("zstd, br, gzip;q=0.7", ENC_MASK_GZIP | ENC_MASK_IDENTITY) == ContentEncoding.gzip,
        true,
    );

    // Wildcard '*' selects highest server preference (zstd)
    expect_eq(negotiateEncoding("*", ENC_MASK_ALL) == ContentEncoding.zstd, true);

    // Empty header -> identity
    expect_eq(negotiateEncoding("", ENC_MASK_ALL) == ContentEncoding.identity, true);
    expect_eq(ContentEncoding.gzip.canUseLibDeflate(), true);
    expect_eq(ContentEncoding.zstd.isCompressed(), true);
    expect_eq(ContentEncoding.identity.isCompressed(), false);
});

test("2. FetchCacheMode bidirectional parse/serialize & cache bypass predicates", () => {
    const names: [6][]const u8 = ["default", "no-store", "reload", "no-cache", "force-cache", "only-if-cached"];
    for (n of names) {
        const m = FetchCacheMode.parse(n).?;
        expect_eq(std.mem.eql(u8, m.toStr(), n), true);
    }
    expect_eq(FetchCacheMode.no_store.bypassesCacheRead(), true);
    expect_eq(FetchCacheMode.no_store.skipsCacheWrite(), true);
    expect_eq(FetchCacheMode.reload.bypassesCacheRead(), true);
    expect_eq(FetchCacheMode.reload.skipsCacheWrite(), false);
    expect_eq(FetchCacheMode.parse("invalid") == null, true);
});

test("3. FetchRedirect & FetchRequestMode bidirectional parse/serialize", () => {
    expect_eq(FetchRedirect.parse("follow").? == FetchRedirect.follow, true);
    expect_eq(FetchRedirect.parse("ERROR").? == FetchRedirect.error_mode, true);
    expect_eq(std.mem.eql(u8, FetchRedirect.manual.toStr(), "manual"), true);
    expect_eq(FetchRedirect.parse("auto") == null, true);

    expect_eq(FetchRequestMode.parse("navigate").? == FetchRequestMode.navigate, true);
    expect_eq(FetchRequestMode.parse("same-origin").?.allowsCrossOrigin(), false);
    expect_eq(FetchRequestMode.parse("cors").?.allowsCrossOrigin(), true);
    expect_eq(std.mem.eql(u8, FetchRequestMode.no_cors.toStr(), "no-cors"), true);
    expect_eq(FetchRequestMode.parse("other") == null, true);
});

test("4. UrlPath zero-copy splitting (path/query/fragment/extname/basename)", () => {
    const u1 = UrlPath.parse("/api/v1/archive.tar.gz?download=1&raw=true#section");
    expect_eq(std.mem.eql(u8, u1.path, "/api/v1/archive.tar.gz"), true);
    expect_eq(std.mem.eql(u8, u1.query.?, "download=1&raw=true"), true);
    expect_eq(std.mem.eql(u8, u1.fragment.?, "section"), true);
    expect_eq(std.mem.eql(u8, u1.basename, "archive.tar.gz"), true);
    expect_eq(std.mem.eql(u8, u1.extname, ".gz"), true);
    expect_eq(u1.isNormalized(), true);

    // Dotfile has empty extname
    const u2 = UrlPath.parse("/project/.env");
    expect_eq(std.mem.eql(u8, u2.basename, ".env"), true);
    expect_eq(std.mem.eql(u8, u2.extname, ""), true);
    expect_eq(u2.query == null, true);
    expect_eq(u2.fragment == null, true);
});

test("5. UrlPath normalization detection (.. / // / .) & edge cases", () => {
    expect_eq(UrlPath.parse("/a/b/c").isNormalized(), true);
    expect_eq(UrlPath.parse("/").isNormalized(), true);
    expect_eq(UrlPath.parse("").isNormalized(), false);
    expect_eq(UrlPath.parse("/a//b").isNormalized(), false);
    expect_eq(UrlPath.parse("/a/../b").isNormalized(), false);
    expect_eq(UrlPath.parse("/a/./b").isNormalized(), false);

    // Malformed q-value in Accept-Encoding safely ignored
    expect_eq(
        negotiateEncoding("zstd;q=2.5, gzip;q=0.5", ENC_MASK_ALL) == ContentEncoding.gzip,
        true,
    );
});
