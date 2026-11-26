// cap/test/testCapJsonflat.cp
// Comprehensive unit tests for @cap/jsonflat

import { test, expect_eq } from "cap:test";
import {
    flattenJson,
    flattenJsonWith,
    getByPath,
    hasPath,
    formatFlatJsonObject,
} from "../../packages/@cap/jsonflat/src/index.cp";

fn main() {
    test("1. @cap/jsonflat — Nested objects & arrays flattening", () => {
        const src = "{\"user\":{\"name\":\"alice\",\"roles\":[\"admin\",\"ops\"]},\"active\":true,\"score\":99}";
        const flat = flattenJson(src);
        expect_eq(flat.ok, true);
        expect_eq(flat.count, 5 as usize);
        expect_eq(flat.get("user.name") ?? "", "alice");
        expect_eq(flat.get("user.roles.0") ?? "", "admin");
        expect_eq(flat.get("user.roles.1") ?? "", "ops");
        expect_eq(flat.get("active") ?? "", "true");
        expect_eq(flat.get("score") ?? "", "99");
    });

    test("2. @cap/jsonflat — Custom delimiter & empty container preservation", () => {
        const src = "{\"a\":{\"b\":1,\"emptyObj\":{},\"emptyArr\":[]}}";
        const flat = flattenJsonWith(src, '/');
        expect_eq(flat.ok, true);
        expect_eq(flat.get("a/b") ?? "", "1");
        expect_eq(flat.get("a/emptyObj") ?? "", "{}");
        expect_eq(flat.get("a/emptyArr") ?? "", "[]");
    });

    test("3. @cap/jsonflat — dot-prop getByPath & hasPath", () => {
        const src = "{\"cluster\":{\"nodes\":[{\"id\":\"sin-1\",\"cpu\":4},{\"id\":\"iad-1\",\"cpu\":8}]}}";
        let buf: [64]u8 = undefined;
        expect_eq(getByPath(src, "cluster.nodes.0.id", buf[0..64]) ?? "", "sin-1");
        expect_eq(getByPath(src, "cluster.nodes.1.cpu", buf[0..64]) ?? "", "8");
        expect_eq(hasPath(src, "cluster.nodes.1.id"), true);
        expect_eq(hasPath(src, "cluster.nodes.9.id"), false);
    });

    test("4. @cap/jsonflat — Flat JSON serialization & malformed JSON rejection", () => {
        const flat = flattenJson("{\"a\":{\"x\":1},\"b\":2}");
        let out: [256]u8 = undefined;
        expect_eq(formatFlatJsonObject(flat, out[0..256]), "{\"a.x\":\"1\",\"b\":\"2\"}");

        expect_eq(flattenJson("{bad json").ok, false);
        expect_eq(flattenJson("{\"a\":1} trailing").ok, false);
    });
}
