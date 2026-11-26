// cap/test/testCapPathToRegexp.cp
// Comprehensive unit tests for @cap/path-to-regexp

import { test, expect_eq } from "cap:test";
import {
    ParamPair,
    matchRoute,
    isRouteMatch,
    buildRoute,
} from "../../packages/@cap/path-to-regexp/src/index.cp";

fn main() {
    test("1. @cap/path-to-regexp — Static & named parameter extraction", () => {
        const m = matchRoute("/v1/packages/:scope/:name", "/v1/packages/@cap/lru?raw=1");
        expect_eq(m.matched, true);
        expect_eq(m.param_count, 2 as usize);
        expect_eq(m.get("scope") ?? "", "@cap");
        expect_eq(m.get("name") ?? "", "lru");
        expect_eq(m.has("missing"), false);

        expect_eq(isRouteMatch("/v1/packages/:scope/:name", "/v1/packages/@cap"), false);
        expect_eq(isRouteMatch("/v1/packages/:scope/:name", "/v1/other/@cap/lru"), false);
    });

    test("2. @cap/path-to-regexp — Optional parameters (:param?)", () => {
        const m1 = matchRoute("/archive/:year/:month?", "/archive/2026");
        expect_eq(m1.matched, true);
        expect_eq(m1.get("year") ?? "", "2026");
        expect_eq(m1.has("month"), false);

        const m2 = matchRoute("/archive/:year/:month?", "/archive/2026/11/");
        expect_eq(m2.matched, true);
        expect_eq(m2.get("year") ?? "", "2026");
        expect_eq(m2.get("month") ?? "", "11");
    });

    test("3. @cap/path-to-regexp — Wildcard catch-all (*filepath)", () => {
        const m = matchRoute("/dl/*filepath", "/dl/pkg/@cap/lru/1.0.0/lru-1.0.0.cap");
        expect_eq(m.matched, true);
        expect_eq(m.get("filepath") ?? "", "pkg/@cap/lru/1.0.0/lru-1.0.0.cap");
    });

    test("4. @cap/path-to-regexp — Reverse URL buildRoute & missing param guard", () => {
        const params: [2]ParamPair = [
            ParamPair{ name: "scope", value: "@cap" },
            ParamPair{ name: "name", value: "validator" },
        ];
        let out: [128]u8 = undefined;
        const url = buildRoute("/packages/:scope/:name/:ver?", params[0..2], out[0..128]);
        expect_eq(url ?? "", "/packages/@cap/validator");

        const missing = buildRoute("/packages/:scope/:name/:required", params[0..2], out[0..128]);
        expect_eq(missing == null, true);
    });
}
