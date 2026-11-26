import { test, expect_eq } from "cap:test";
import {
    RouteParam,
    RouteMatch,
    matchPath,
    compilePath,
    countRouteParams,
} from "../../packages/@cap/path-to-regexp/src/index.cp";

fn main() {
    test("1. @cap/path-to-regexp — Static segments, case sensitivity & trailing slash tolerance", () => {
        expect_eq(matchPath("/", "/", true).matched, true);
        expect_eq(matchPath("/users/profile", "/users/profile", true).matched, true);
        expect_eq(matchPath("/users/profile", "/users/profile/", true).matched, true);
        expect_eq(matchPath("/users/profile/", "/users/profile", true).matched, true);
        expect_eq(matchPath("/users/profile", "/Users/Profile", true).matched, false);
        expect_eq(matchPath("/users/profile", "/Users/Profile", false).matched, true);
        expect_eq(matchPath("/users/profile", "/users/profile/extra", false).matched, false);
    });

    test("2. @cap/path-to-regexp — Named parameters extraction", () => {
        const m = matchPath("/users/:id/posts/:postId", "/users/42/posts/hello-world", true);
        expect_eq(m.matched, true);
        expect_eq(m.param_count, 2);
        expect_eq(m.getParam("id") ?? "", "42");
        expect_eq(m.getParam("postId") ?? "", "hello-world");
        expect_eq(m.getParam("missing") == null, true);

        expect_eq(matchPath("/users/:id/posts/:postId", "/users/42/posts", true).matched, false);
    });

    test("3. @cap/path-to-regexp — Optional parameters (:name?)", () => {
        const m1 = matchPath("/files/:name?", "/files", true);
        expect_eq(m1.matched, true);
        expect_eq(m1.param_count, 0);
        expect_eq(m1.getParam("name") == null, true);

        const m2 = matchPath("/files/:name?", "/files/", true);
        expect_eq(m2.matched, true);
        expect_eq(m2.param_count, 0);

        const m3 = matchPath("/files/:name?", "/files/report.pdf", true);
        expect_eq(m3.matched, true);
        expect_eq(m3.param_count, 1);
        expect_eq(m3.getParam("name") ?? "", "report.pdf");
    });

    test("4. @cap/path-to-regexp — Wildcard rest segments (*filepath)", () => {
        const m1 = matchPath("/assets/*filepath", "/assets/img/icons/logo.svg", true);
        expect_eq(m1.matched, true);
        expect_eq(m1.param_count, 1);
        expect_eq(m1.getParam("filepath") ?? "", "img/icons/logo.svg");

        // Required wildcard does not match empty remainder
        expect_eq(matchPath("/assets/*filepath", "/assets", true).matched, false);

        // Optional wildcard matches empty remainder
        const m2 = matchPath("/assets/*filepath?", "/assets", true);
        expect_eq(m2.matched, true);
        expect_eq(m2.param_count, 0);
    });

    test("5. @cap/path-to-regexp — Reverse compilation via compilePath()", () => {
        const params: [3]RouteParam = [
            RouteParam.new("id", "99"),
            RouteParam.new("postId", "intro"),
            RouteParam.new("filepath", "js/bundle.min.js"),
        ];
        let out: [128]u8 = undefined;

        const n1 = compilePath("/users/:id/posts/:postId", params[0..3], out[0..128]);
        expect_eq(out[0..n1], "/users/99/posts/intro");

        const n2 = compilePath("/assets/*filepath", params[0..3], out[0..128]);
        expect_eq(out[0..n2], "/assets/js/bundle.min.js");

        // Optional param omitted -> cleanly dropped
        const n3 = compilePath("/files/:name?", params[0..0], out[0..128]);
        expect_eq(out[0..n3], "/files");

        // Missing required param -> returns 0
        const n4 = compilePath("/users/:missing", params[0..3], out[0..128]);
        expect_eq(n4, 0);
    });

    test("6. @cap/path-to-regexp — countRouteParams()", () => {
        expect_eq(countRouteParams("/"), 0);
        expect_eq(countRouteParams("/users/profile"), 0);
        expect_eq(countRouteParams("/users/:id/posts/:postId"), 2);
        expect_eq(countRouteParams("/files/:name?"), 1);
        expect_eq(countRouteParams("/api/:ver/files/:name?/*rest"), 3);
    });
}
