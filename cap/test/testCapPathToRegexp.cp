import { test, expect_eq } from "cap:test";
import { RoutePattern, buildPath } from "../../packages/@cap/path-to-regexp/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/path-to-regexp — Static literal path exact match & mismatch", () => {
        const route = RoutePattern.compile("/api/v1/health");
        expect_eq(route.segmentCount(), 3 as usize);
        expect_eq(route.paramCount(), 0 as usize);

        const ok1 = route.matchPath("/api/v1/health");
        expect_eq(ok1.matched, true);
        expect_eq(ok1.param_count, 0 as usize);

        const bad1 = route.matchPath("/api/v1/status");
        expect_eq(bad1.matched, false);

        const bad2 = route.matchPath("/api/v1/health/extra");
        expect_eq(bad2.matched, false);

        const bad3 = route.matchPath("/api/v1");
        expect_eq(bad3.matched, false);
    });

    test("2. @cap/path-to-regexp — Multiple named params /users/:uid/repos/:repo", () => {
        const pat = "/users/:uid/repos/:repo";
        const path = "/users/alice42/repos/cp-lang";
        const route = RoutePattern.compile(pat);
        expect_eq(route.segmentCount(), 4 as usize);
        expect_eq(route.paramCount(), 2 as usize);

        const res = route.matchPath(path);
        expect_eq(res.matched, true);
        expect_eq(res.param_count, 2 as usize);

        const p0 = res.params[0];
        expect_eq(sliceEq(pat[p0.name_start..p0.name_start + p0.name_len], "uid"), true);
        expect_eq(sliceEq(path[p0.val_start..p0.val_start + p0.val_len], "alice42"), true);

        const p1 = res.params[1];
        expect_eq(sliceEq(pat[p1.name_start..p1.name_start + p1.name_len], "repo"), true);
        expect_eq(sliceEq(path[p1.val_start..p1.val_start + p1.val_len], "cp-lang"), true);
    });

    test("3. @cap/path-to-regexp — Optional trailing param /items/:id?", () => {
        const pat = "/items/:id?";
        const route = RoutePattern.compile(pat);

        const with_id = route.matchPath("/items/987");
        expect_eq(with_id.matched, true);
        expect_eq(with_id.param_count, 1 as usize);
        const p0 = with_id.params[0];
        expect_eq(sliceEq(pat[p0.name_start..p0.name_start + p0.name_len], "id"), true);
        expect_eq(sliceEq("/items/987"[p0.val_start..p0.val_start + p0.val_len], "987"), true);

        const without_id = route.matchPath("/items");
        expect_eq(without_id.matched, true);
        expect_eq(without_id.param_count, 0 as usize);

        const too_deep = route.matchPath("/items/987/details");
        expect_eq(too_deep.matched, false);
    });

    test("4. @cap/path-to-regexp — Wildcard /static/*filepath captures multi-segment subpath", () => {
        const pat = "/static/*filepath";
        const path = "/static/assets/img/logo.png";
        const route = RoutePattern.compile(pat);

        const res = route.matchPath(path);
        expect_eq(res.matched, true);
        expect_eq(res.param_count, 1 as usize);
        const p0 = res.params[0];
        expect_eq(sliceEq(pat[p0.name_start..p0.name_start + p0.name_len], "filepath"), true);
        expect_eq(sliceEq(path[p0.val_start..p0.val_start + p0.val_len], "assets/img/logo.png"), true);
    });

    test("5. @cap/path-to-regexp — buildPath reverse URL assembly & error handling", () => {
        let buf: [64]u8 = undefined;
        const vals: [2][]const u8 = ["alice", "cp-core"];
        const len = buildPath("/users/:uid/repos/:repo", vals[0..2], buf[0..64]);
        expect_eq(len, 26 as i64);
        expect_eq(sliceEq(buf[0..26], "/users/alice/repos/cp-core"), true);

        // Optional param omitted
        const len_opt = buildPath("/items/:id?", vals[0..0], buf[0..64]);
        expect_eq(len_opt, 6 as i64);
        expect_eq(sliceEq(buf[0..6], "/items"), true);

        // Missing required param -> -1
        const one_val: [1][]const u8 = ["alice"];
        expect_eq(buildPath("/users/:uid/repos/:repo", one_val[0..1], buf[0..64]), -1 as i64);

        // Undersized buffer -> -1
        let tiny: [8]u8 = undefined;
        expect_eq(buildPath("/users/:uid/repos/:repo", vals[0..2], tiny[0..8]), -1 as i64);
    });

    test("6. @cap/path-to-regexp — Root path '/' & trailing slash normalization", () => {
        const root = RoutePattern.compile("/");
        expect_eq(root.segmentCount(), 0 as usize);
        expect_eq(root.matchPath("/").matched, true);
        expect_eq(root.matchPath("").matched, true);
        expect_eq(root.matchPath("/about").matched, false);

        const users = RoutePattern.compile("/users/:id");
        const m_trail = users.matchPath("/users/55/");
        expect_eq(m_trail.matched, true);
        expect_eq(m_trail.param_count, 1 as usize);
        expect_eq(sliceEq("/users/55/"[m_trail.params[0].val_start..m_trail.params[0].val_start + m_trail.params[0].val_len], "55"), true);

        let buf: [8]u8 = undefined;
        const dummy: [1][]const u8 = [""];
        const rlen = buildPath("/", dummy[0..0], buf[0..8]);
        expect_eq(rlen, 1 as i64);
        expect_eq(buf[0], 47 as u8);
    });
}
