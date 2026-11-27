import { test, expect_eq } from "cap:test";
import {
    stripContentTypeParams,
    isCompressible,
} from "../../packages/compressible/src/index.cp";

fn main() {
    test("1. @cap/compressible — stripContentTypeParams removes parameters and trims", () => {
        expect_eq(stripContentTypeParams("text/html; charset=utf-8"), "text/html");
        expect_eq(stripContentTypeParams("  Application/JSON ; charset=UTF-8  "), "Application/JSON");
        expect_eq(stripContentTypeParams("image/png"), "image/png");
        expect_eq(stripContentTypeParams("  ; charset=utf-8"), "");
    });

    test("2. @cap/compressible — matches text/* and structured +json/+xml/+text suffixes", () => {
        expect_eq(isCompressible("text/plain"), true);
        expect_eq(isCompressible("TEXT/HTML; charset=utf-8"), true);
        expect_eq(isCompressible("text/css"), true);
        expect_eq(isCompressible("application/ld+json"), true);
        expect_eq(isCompressible("application/problem+JSON; charset=utf-8"), true);
        expect_eq(isCompressible("image/svg+xml"), true);
        expect_eq(isCompressible("application/atom+XML"), true);
        expect_eq(isCompressible("application/custom+text"), true);
    });

    test("3. @cap/compressible — matches compressible application/* MIME types", () => {
        expect_eq(isCompressible("application/json"), true);
        expect_eq(isCompressible("application/javascript; charset=utf-8"), true);
        expect_eq(isCompressible("application/ecmascript"), true);
        expect_eq(isCompressible("application/xml"), true);
        expect_eq(isCompressible("application/wasm"), true);
        expect_eq(isCompressible("application/graphql"), true);
        expect_eq(isCompressible("application/x-www-form-urlencoded"), true);
    });

    test("4. @cap/compressible — matches uncompressed fonts and rejects woff/woff2", () => {
        expect_eq(isCompressible("font/ttf"), true);
        expect_eq(isCompressible("FONT/OTF"), true);
        expect_eq(isCompressible("font/woff"), false);
        expect_eq(isCompressible("font/woff2"), false);
    });

    test("5. @cap/compressible — rejects already-compressed media and archive types", () => {
        expect_eq(isCompressible("image/png"), false);
        expect_eq(isCompressible("image/jpeg"), false);
        expect_eq(isCompressible("image/webp"), false);
        expect_eq(isCompressible("video/mp4"), false);
        expect_eq(isCompressible("audio/mpeg"), false);
        expect_eq(isCompressible("application/gzip"), false);
        expect_eq(isCompressible("application/zip"), false);
        expect_eq(isCompressible("application/octet-stream"), false);
    });

    test("6. @cap/compressible — rejects empty and malformed Content-Type values", () => {
        expect_eq(isCompressible(""), false);
        expect_eq(isCompressible("   "), false);
        expect_eq(isCompressible("text/"), false);
        expect_eq(isCompressible("/plain"), false);
        expect_eq(isCompressible("not-a-mime-type"), false);
    });
}
