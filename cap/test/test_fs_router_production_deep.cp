// Source: cp/cap/test/test_fs_router_production_deep.cp (Production deep tests for cap:fs_router)

import { test, expect_eq } from "cap:test";
import {
    RouteKind,
    SegmentKind,
    RouteParam,
    FsRouteMatch as RouteMatch,
    RoutePattern,
    FsRouter,
} from "cap:web";

test("fs_router: static routes, root path, trailing slash and query normalization") {
    let router = FsRouter.init();
    const root_idx = router.addRoute("/").?;
    const about_idx = router.addRoute("/about/").?;
    const api_idx = router.addRoute("/api/v1/health").?;

    const m_root = router.matchPath("/").?;
    expect_eq(m_root.route_idx, root_idx);
    expect_eq(m_root.param_len, 0);

    const m_about = router.matchPath("/about///?lang=zh#top").?;
    expect_eq(m_about.route_idx, about_idx);

    const m_api = router.matchPath("/api/v1/health/").?;
    expect_eq(m_api.route_idx, api_idx);

    expect_eq(router.matchPath("/not-found") == null, true);
}

test("fs_router: single and multi dynamic params extraction") {
    let router = FsRouter.init();
    const idx = router.addRoute("/users/[id]/posts/[postId]").?;

    const m = router.matchPath("/users/alice/posts/9876?ref=feed").?;
    expect_eq(m.route_idx, idx);
    expect_eq(m.param_len, 2);
    expect_eq(std.mem.eql(u8, m.getParam("id").?, "alice"), true);
    expect_eq(std.mem.eql(u8, m.getParam("postId").?, "9876"), true);
    expect_eq(m.getParam("missing") == null, true);

    // 缺少段或多余段不匹配
    expect_eq(router.matchPath("/users/alice/posts") == null, true);
    expect_eq(router.matchPath("/users/alice/posts/9876/extra") == null, true);
}

test("fs_router: catch-all and optional-catch-all matching") {
    let router = FsRouter.init();
    const docs_idx = router.addRoute("/docs/[...slug]").?;
    const shop_idx = router.addRoute("/shop/[[...cats]]").?;

    // 必选 catch-all: 不匹配 /docs，匹配 /docs/a 和 /docs/a/b/c
    expect_eq(router.matchPath("/docs") == null, true);

    const m_docs1 = router.matchPath("/docs/getting-started").?;
    expect_eq(m_docs1.route_idx, docs_idx);
    expect_eq(std.mem.eql(u8, m_docs1.getParam("slug").?, "getting-started"), true);

    const m_docs2 = router.matchPath("/docs/api/v2/router/").?;
    expect_eq(m_docs2.route_idx, docs_idx);
    expect_eq(std.mem.eql(u8, m_docs2.getParam("slug").?, "api/v2/router"), true);

    // 可选 catch-all: 同时匹配 /shop 与 /shop/a/b
    const m_shop0 = router.matchPath("/shop").?;
    expect_eq(m_shop0.route_idx, shop_idx);
    expect_eq(std.mem.eql(u8, m_shop0.getParam("cats").?, ""), true);

    const m_shop2 = router.matchPath("/shop/books/rust").?;
    expect_eq(m_shop2.route_idx, shop_idx);
    expect_eq(std.mem.eql(u8, m_shop2.getParam("cats").?, "books/rust"), true);
}

test("fs_router: priority competition static > dynamic > catch-all > optional-catch-all") {
    let router = FsRouter.init();
    // 故意按低优先级 -> 高优先级逆序注册
    const opt_idx = router.addRoute("/blog/[[...rest]]").?;
    const catch_idx = router.addRoute("/blog/[...slug]").?;
    const dyn_idx = router.addRoute("/blog/[id]").?;
    const static_idx = router.addRoute("/blog/featured").?;

    // 1. /blog/featured -> 静态必胜
    const m1 = router.matchPath("/blog/featured").?;
    expect_eq(m1.route_idx, static_idx);

    // 2. /blog/hello -> 动态胜于 catch-all 与 optional-catch-all
    const m2 = router.matchPath("/blog/hello").?;
    expect_eq(m2.route_idx, dyn_idx);
    expect_eq(std.mem.eql(u8, m2.getParam("id").?, "hello"), true);

    // 3. /blog/2026/05 -> 必选 catch-all 胜于 optional-catch-all
    const m3 = router.matchPath("/blog/2026/05").?;
    expect_eq(m3.route_idx, catch_idx);
    expect_eq(std.mem.eql(u8, m3.getParam("slug").?, "2026/05"), true);

    // 4. /blog -> 仅 optional-catch-all 命中
    const m4 = router.matchPath("/blog").?;
    expect_eq(m4.route_idx, opt_idx);
    expect_eq(std.mem.eql(u8, m4.getParam("rest").?, ""), true);

    // 验证 comparePriority 严格序
    const p_static = RoutePattern.parse("/blog/featured").?;
    const p_dyn = RoutePattern.parse("/blog/[id]").?;
    const p_catch = RoutePattern.parse("/blog/[...slug]").?;
    const p_opt = RoutePattern.parse("/blog/[[...rest]]").?;
    expect_eq(p_static.kind == .static_route, true);
    expect_eq(p_dyn.kind == .dynamic, true);
    expect_eq(p_catch.kind == .catch_all, true);
    expect_eq(p_opt.kind == .optional_catch_all, true);
    expect_eq(p_static.comparePriority(p_dyn) < 0, true);
    expect_eq(p_dyn.comparePriority(p_catch) < 0, true);
    expect_eq(p_catch.comparePriority(p_opt) < 0, true);
}

test("fs_router: malformed patterns rejection and invalid path guards") {
    // 非末段 catch-all / optional-catch-all 必须拒绝
    expect_eq(RoutePattern.parse("/[...a]/b") == null, true);
    expect_eq(RoutePattern.parse("/[[...a]]/b") == null, true);

    // 空参数名、缺失三点、未闭合括号、空段与非前导斜杠必须拒绝
    expect_eq(RoutePattern.parse("/users/[]") == null, true);
    expect_eq(RoutePattern.parse("/docs/[...]") == null, true);
    expect_eq(RoutePattern.parse("/shop/[[...]]") == null, true);
    expect_eq(RoutePattern.parse("/shop/[[cats]]") == null, true);
    expect_eq(RoutePattern.parse("/users/[id") == null, true);
    expect_eq(RoutePattern.parse("/users/id]") == null, true);
    expect_eq(RoutePattern.parse("/users//profile") == null, true);
    expect_eq(RoutePattern.parse("users/profile") == null, true);
    expect_eq(RoutePattern.parse("") == null, true);

    let router = FsRouter.init();
    expect_eq(router.addRoute("/[...bad]/tail") == null, true);
    expect_eq(router.len, 0);
    expect_eq(router.matchPath("") == null, true);
    expect_eq(router.matchPath("no-slash") == null, true);
}
