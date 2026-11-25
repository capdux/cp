// Self-contained unit & integration tests for @web/swagger

const std = @import("std");
import { App, AppMethod, Context, Request, Response } from "../../../../lib/web/app.cp";
import {
    SwaggerConfig,
    renderDocsHtml,
    refreshSpec,
    cachedSpec,
    cachedDocsHtml,
    registerSwagger,
} from "../src/index.cp";

fn assertTrue(cond: bool, msg: string) {
    if (!cond) {
        std.debug.print("FAIL: {s}\n", .{msg});
        std.process.exit(1);
    }
}

fn contains(haystack: string, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i <= haystack.len - needle.len) : (i += 1) {
        let eq: bool = true;
        let j: usize = 0;
        while (j < needle.len) : (j += 1) {
            if (haystack[i + j] != needle[j]) {
                eq = false;
                break;
            }
        }
        if (eq) return true;
    }
    return false;
}

fn healthHandler(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"ok\":true}");
}

fn userHandler(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"id\":1}");
}

fn createOrderHandler(ctx: *Context): Response {
    * = ctx;
    return Response.json(201, "{\"created\":true}");
}

fn testScalarDefaultAndRefresh() {
    let app = App.init();
    * = app.routeDoc(AppMethod.GET, "/health", "Health check", &healthHandler);
    * = app.routeDoc(AppMethod.GET, "/users/:id", "Get user by ID", &userHandler);

    const cfg = SwaggerConfig{
        title: "CP Cloud API",
        version: "2.1.0",
        spec_path: "/openapi.json",
        docs_path: "/docs",
        ui_theme: "scalar",
    };

    * = registerSwagger(*app, cfg);

    // 1. GET /openapi.json
    const resp_spec = app.dispatch(Request.init(AppMethod.GET, "/openapi.json"));
    assertTrue(resp_spec.status == 200, "openapi.json status 200");
    assertTrue(contains(resp_spec.content_type, "application/json"), "openapi.json content-type");
    assertTrue(contains(resp_spec.body, "\"openapi\":\"3.1.0\""), "openapi version 3.1.0");
    assertTrue(contains(resp_spec.body, "\"title\":\"CP Cloud API\""), "openapi title");
    assertTrue(contains(resp_spec.body, "\"version\":\"2.1.0\""), "openapi version");
    assertTrue(contains(resp_spec.body, "\"/health\""), "openapi contains /health");
    assertTrue(contains(resp_spec.body, "\"/users/{id}\""), "openapi contains /users/{id}");
    assertTrue(contains(resp_spec.body, "\"summary\":\"Get user by ID\""), "openapi summary");

    // 2. GET /docs (Scalar UI HTML)
    const resp_docs = app.dispatch(Request.init(AppMethod.GET, "/docs"));
    assertTrue(resp_docs.status == 200, "docs status 200");
    assertTrue(contains(resp_docs.content_type, "text/html"), "docs content-type html");
    assertTrue(contains(resp_docs.body, "CP Cloud API - API Docs"), "docs title");
    assertTrue(contains(resp_docs.body, "data-url=\"/openapi.json\""), "docs scalar data-url");
    assertTrue(contains(resp_docs.body, "@scalar/api-reference"), "docs scalar script");

    // 3. Dynamic route registration + refreshSpec(app)
    * = app.routeDoc(AppMethod.POST, "/orders", "Create order", &createOrderHandler);
    const refreshed_len = refreshSpec(*app);
    assertTrue(refreshed_len != null && refreshed_len.? > 0, "refreshSpec succeeds");

    const resp_spec2 = app.dispatch(Request.init(AppMethod.GET, "/openapi.json"));
    assertTrue(resp_spec2.status == 200, "refreshed openapi.json 200");
    assertTrue(contains(resp_spec2.body, "\"/orders\""), "refreshed spec contains /orders");
    assertTrue(contains(resp_spec2.body, "\"summary\":\"Create order\""), "refreshed spec summary");
    assertTrue(contains(cachedSpec(), "\"/orders\""), "cachedSpec updated");
    assertTrue(contains(cachedDocsHtml(), "CP Cloud API"), "cachedDocsHtml available");
}

fn testSwaggerUiThemeAndEscaping() {
    let html_buf: [4096]u8 = [0; 4096];
    const cfg = SwaggerConfig{
        title: "Acme <Beta> & Co",
        version: "1.0.0",
        spec_path: "/api/spec.json",
        docs_path: "/api/docs",
        ui_theme: "swagger",
    };

    const n = renderDocsHtml(cfg, html_buf[0..]);
    assertTrue(n != null && n.? > 0, "render swagger-ui html");
    const html = html_buf[0..n.?];
    assertTrue(contains(html, "Acme &lt;Beta&gt; &amp; Co"), "html title escaped");
    assertTrue(contains(html, "SwaggerUIBundle"), "swagger-ui bundle script");
    assertTrue(contains(html, "/api/spec.json"), "custom spec_path embedded");
}

pub fn main() {
    testScalarDefaultAndRefresh();
    testSwaggerUiThemeAndEscaping();
    std.debug.print("ALL @web/swagger TESTS PASSED!\n", .{});
}
