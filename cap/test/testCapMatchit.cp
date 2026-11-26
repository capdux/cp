// cap/test/testCapMatchit.cp
// Comprehensive unit tests for @cap/matchit

import { test, expect_eq } from "cap:test";
import {
    Router,
} from "../../packages/@cap/matchit/src/index.cp";

fn main() {
    test("1. @cap/matchit — Static route takes priority over param & catch-all regardless of insert order", () => {
        let r = Router.init();
        // Deliberately insert lowest priority first
        expect_eq(r.insert("/*rest", 100 as u32), true);
        expect_eq(r.insert("/users/:id", 20 as u32), true);
        expect_eq(r.insert("/users/me", 10 as u32), true);
        expect_eq(r.insert("/", 1 as u32), true);

        const mRoot = r.at("/");
        expect_eq(mRoot.matched, true);
        expect_eq(mRoot.handler_id, 1 as u32);

        const mMe = r.at("/users/me");
        expect_eq(mMe.matched, true);
        expect_eq(mMe.handler_id, 10 as u32);
        expect_eq(mMe.param_count, 0 as usize);

        const mId = r.at("/users/42");
        expect_eq(mId.matched, true);
        expect_eq(mId.handler_id, 20 as u32);
        expect_eq(mId.param("id") ?? "", "42");
    });

    test("2. @cap/matchit — Both :name and {name} named parameter syntaxes", () => {
        let r = Router.init();
        expect_eq(r.insert("/orgs/{org}/repos/{repo}", 5 as u32), true);
        expect_eq(r.insert("/api/:ver/health", 6 as u32), true);

        const m1 = r.at("/orgs/cp-lang/repos/cap");
        expect_eq(m1.matched, true);
        expect_eq(m1.handler_id, 5 as u32);
        expect_eq(m1.param("org") ?? "", "cp-lang");
        expect_eq(m1.param("repo") ?? "", "cap");
        expect_eq(m1.param("missing") == null, true);

        const m2 = r.at("/api/v2/health");
        expect_eq(m2.matched, true);
        expect_eq(m2.handler_id, 6 as u32);
        expect_eq(m2.param("ver") ?? "", "v2");
    });

    test("3. @cap/matchit — Multi-parameter nested routes", () => {
        let r = Router.init();
        expect_eq(r.insert("/users/:id/posts/:post_id/comments/:cid", 77 as u32), true);

        const m = r.at("/users/42/posts/99/comments/7");
        expect_eq(m.matched, true);
        expect_eq(m.handler_id, 77 as u32);
        expect_eq(m.param_count, 3 as usize);
        expect_eq(m.param("id") ?? "", "42");
        expect_eq(m.param("post_id") ?? "", "99");
        expect_eq(m.param("cid") ?? "", "7");
    });

    test("4. @cap/matchit — Catch-all *filepath and {*path} trailing wildcards", () => {
        let r = Router.init();
        expect_eq(r.insert("/files/*filepath", 30 as u32), true);
        expect_eq(r.insert("/static/{*asset}", 31 as u32), true);

        const m1 = r.at("/files/a/b/c.png");
        expect_eq(m1.matched, true);
        expect_eq(m1.handler_id, 30 as u32);
        expect_eq(m1.param("filepath") ?? "", "a/b/c.png");

        const m2 = r.at("/static/js/app.min.js");
        expect_eq(m2.matched, true);
        expect_eq(m2.handler_id, 31 as u32);
        expect_eq(m2.param("asset") ?? "", "js/app.min.js");
    });

    test("5. @cap/matchit — Unmatched paths return matched == false", () => {
        let r = Router.init();
        expect_eq(r.insert("/users/:id", 2 as u32), true);

        expect_eq(r.at("/").matched, false);
        expect_eq(r.at("/users").matched, false);
        expect_eq(r.at("/users/42/extra").matched, false);
        expect_eq(r.at("/other/42").matched, false);
        expect_eq(r.at("no-leading-slash").matched, false);
    });

    test("6. @cap/matchit — Conflict / duplicate route detection & malformed pattern rejection", () => {
        let r = Router.init();
        expect_eq(r.insert("/users/:id", 1 as u32), true);
        // Structurally equivalent param route must be rejected
        expect_eq(r.insert("/users/:uid", 2 as u32), false);
        expect_eq(r.insert("/users/{other}", 3 as u32), false);
        // Duplicate static route must be rejected
        expect_eq(r.insert("/health", 4 as u32), true);
        expect_eq(r.insert("/health", 5 as u32), false);
        // Malformed patterns rejected
        expect_eq(r.insert("no-slash", 6 as u32), false);
        expect_eq(r.insert("/bad/*mid/tail", 7 as u32), false);
        expect_eq(r.insert("/empty/:", 8 as u32), false);
        expect_eq(r.routeCount(), 2 as usize);
    });
}
