// Source: cp/cap/test/test_http_router_production_deep.cp

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    HttpRouter,
    RouterMethod as HttpMethod,
    RouterError
} from "cap:http";

test("static, multi-param, catch-all and priority Static > Param > CatchAll") {
    let router = try HttpRouter.init(std.testing.allocator);
    defer router.deinit();

    try router.get("/", 10);
    try router.get("/users/me", 20);
    try router.get("/users/:id/posts/:pid", 30);
    try router.get("/users/:id", 25);
    try router.get("/static/*filepath", 40);
    try router.get("/*rest", 50);

    // Root route
    if (m := router.match(.GET, "/").matchInfo()) {
        expect_eq(m.handler_id, 10);
        expect_eq(m.param_count, 0);
    } else {
        expect_eq(true, false);
    }

    // Static > Param priority: /users/me matches static 20 instead of :id 25
    if (m := router.match(.GET, "/users/me").matchInfo()) {
        expect_eq(m.handler_id, 20);
        expect_eq(m.param_count, 0);
    } else {
        expect_eq(true, false);
    }

    // Multi-level params :id and :pid
    if (m := router.match(.GET, "/users/42/posts/99").matchInfo()) {
        expect_eq(m.handler_id, 30);
        expect_eq(m.param_count, 2);
        expect_eq(std.mem.eql(u8, m.getParam("id") orelse "", "42"), true);
        expect_eq(std.mem.eql(u8, m.getParam("pid") orelse "", "99"), true);
    } else {
        expect_eq(true, false);
    }

    // Catch-all *filepath
    if (m := router.match(.GET, "/static/css/app.min.css").matchInfo()) {
        expect_eq(m.handler_id, 40);
        expect_eq(m.param_count, 1);
        expect_eq(std.mem.eql(u8, m.getParam("filepath") orelse "", "css/app.min.css"), true);
    } else {
        expect_eq(true, false);
    }

    // Param > CatchAll priority
    if (m := router.match(.GET, "/users/alice").matchInfo()) {
        expect_eq(m.handler_id, 25);
        expect_eq(std.mem.eql(u8, m.getParam("id") orelse "", "alice"), true);
    } else {
        expect_eq(true, false);
    }
}

test("405 MethodNotAllowed Allow header generation and 404 NotFound") {
    let router = try HttpRouter.init(std.testing.allocator);
    defer router.deinit();

    try router.get("/items/:id", 101);
    try router.post("/items/:id", 102);
    try router.delete("/items/:id", 103);

    // Matching method
    if (m := router.match(.POST, "/items/77").matchInfo()) {
        expect_eq(m.handler_id, 102);
    } else {
        expect_eq(true, false);
    }

    // 405 Method Not Allowed with Allow header
    const res405 = router.match(.PUT, "/items/77");
    if (allow := res405.allowInfo()) {
        expect_eq(std.mem.eql(u8, allow.allowHeader(), "GET, POST, DELETE"), true);
    } else {
        expect_eq(true, false);
    }

    // 404 Not Found
    const res404 = router.match(.GET, "/nonexistent/path");
    expect_eq(res404.isNotFound(), true);
}

test("route conflict detection and non-terminal catch-all rejection") {
    let router = try HttpRouter.init(std.testing.allocator);
    defer router.deinit();

    try router.get("/orders/:a", 1);

    // Duplicate method on same path -> RouteConflict
    let dup_err = false;
    router.get("/orders/:a", 2) catch (e) {
        if (e == RouterError.RouteConflict) {
            dup_err = true;
        }
    };
    expect_eq(dup_err, true);

    // Different param name at same position (:a vs :b) -> RouteConflict
    let param_conflict = false;
    router.post("/orders/:b", 3) catch (e) {
        if (e == RouterError.RouteConflict) {
            param_conflict = true;
        }
    };
    expect_eq(param_conflict, true);

    // Non-terminal catch-all -> InvalidCatchAll
    let wild_err = false;
    router.get("/files/*wild/extra", 4) catch (e) {
        if (e == RouterError.InvalidCatchAll) {
            wild_err = true;
        }
    };
    expect_eq(wild_err, true);
}

test("group prefix concatenation, slash normalization and scale test") {
    let router = try HttpRouter.init(std.testing.allocator);
    defer router.deinit();

    const api = try router.group("//api///v1/");
    const users = try api.group("/users/");
    try users.get("/:id//profile/", 500);

    // Query string & extra slashes should normalize cleanly
    if (m := router.match(.GET, "/api//v1/users/999/profile/?tab=info").matchInfo()) {
        expect_eq(m.handler_id, 500);
        expect_eq(std.mem.eql(u8, m.getParam("id") orelse "", "999"), true);
    } else {
        expect_eq(true, false);
    }

    // Scale test: register 100 distinct routes and verify lookup
    let buf: [10]u8 = undefined;
    const prefix_str = "/scale/r";
    let k: usize = 0;
    while (k < 8) {
        buf[k] = prefix_str[k];
        k += 1;
    }
    let i: usize = 0;
    while (i < 100) {
        buf[8] = @as(u8, @intCast('0' + (i / 10)));
        buf[9] = @as(u8, @intCast('0' + (i % 10)));
        try router.get(buf[0..10], 1000 + i);
        i += 1;
    }

    if (m := router.match(.GET, "/scale/r73").matchInfo()) {
        expect_eq(m.handler_id, 1073);
    } else {
        expect_eq(true, false);
    }
}

test("cap.http_router dynamic mount and std.testing.allocator zero-leak") {
    let router = try cap.http_router.HttpRouter.init(std.testing.allocator);
    defer router.deinit();

    const m_opt = cap.http_router.HttpMethod.fromString("PATCH");
    expect_eq(m_opt != null, true);

    if (method := m_opt) {
        try router.addRoute(method, "/cap/mount/:name", 888);
    }

    if (m := router.match(.PATCH, "/cap/mount/radix").matchInfo()) {
        expect_eq(m.handler_id, 888);
        expect_eq(std.mem.eql(u8, m.getParam("name") orelse "", "radix"), true);
    } else {
        expect_eq(true, false);
    }
}
