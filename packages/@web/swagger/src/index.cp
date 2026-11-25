// Official @web/swagger Plugin — OpenAPI 3.1.0 Spec & Interactive Docs UI for cap:web
// Pure CP: zero zig{}, zero var, zero anyopaque, zero heap GC.

import { App, AppMethod, Context, Request, Response } from "../../../../lib/web/app.cp";

pub const SWAGGER_SPEC_BUF_CAP: usize = 65536;
pub const SWAGGER_HTML_BUF_CAP: usize = 8192;

pub struct SwaggerConfig {
    title: string = "CP API",
    version: string = "1.0.0",
    spec_path: string = "/openapi.json",
    docs_path: string = "/docs",
    ui_theme: string = "scalar",
}

let g_swagger_cfg: SwaggerConfig = SwaggerConfig{};
let g_spec_buf: [SWAGGER_SPEC_BUF_CAP]u8 = [0; SWAGGER_SPEC_BUF_CAP];
let g_spec_len: usize = 0;
let g_html_buf: [SWAGGER_HTML_BUF_CAP]u8 = [0; SWAGGER_HTML_BUF_CAP];
let g_html_len: usize = 0;

fn swAsciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn swAsciiEqlIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (swAsciiLower(a[i]) != swAsciiLower(b[i])) return false;
    }
    return true;
}

fn swWriteByte(out_buf: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out_buf.len) return false;
    out_buf[pos.*] = b;
    pos.* += 1;
    return true;
}

fn swWriteRaw(out_buf: []u8, pos: *usize, s: string): bool {
    if (s.len > out_buf.len || pos.* > out_buf.len - s.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn swWriteHtmlEscaped(out_buf: []u8, pos: *usize, s: string): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '&') {
            if (!swWriteRaw(out_buf, pos, "&amp;")) return false;
        } else if (c == '<') {
            if (!swWriteRaw(out_buf, pos, "&lt;")) return false;
        } else if (c == '>') {
            if (!swWriteRaw(out_buf, pos, "&gt;")) return false;
        } else if (c == '"') {
            if (!swWriteRaw(out_buf, pos, "&quot;")) return false;
        } else if (c == '\'') {
            if (!swWriteRaw(out_buf, pos, "&#39;")) return false;
        } else {
            if (!swWriteByte(out_buf, pos, c)) return false;
        }
    }
    return true;
}

/// Render the interactive API documentation HTML page (Scalar or Swagger UI) into `out_buf`.
pub fn renderDocsHtml(cfg: SwaggerConfig, out_buf: []u8): usize? {
    let pos: usize = 0;
    const is_swagger = swAsciiEqlIgnoreCase(cfg.ui_theme, "swagger") || swAsciiEqlIgnoreCase(cfg.ui_theme, "swagger-ui");

    if (!swWriteRaw(out_buf, *pos, "<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\"/><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"/><title>")) return null;
    if (!swWriteHtmlEscaped(out_buf, *pos, cfg.title)) return null;
    if (!swWriteRaw(out_buf, *pos, " - API Docs</title>")) return null;

    if (is_swagger) {
        if (!swWriteRaw(out_buf, *pos, "<link rel=\"stylesheet\" href=\"https://unpkg.com/swagger-ui-dist@5/swagger-ui.css\"/></head><body><div id=\"swagger-ui\"></div><script src=\"https://unpkg.com/swagger-ui-dist@5/swagger-ui-bundle.js\"></script><script>window.onload=function(){SwaggerUIBundle({url:\"")) return null;
        if (!swWriteHtmlEscaped(out_buf, *pos, cfg.spec_path)) return null;
        if (!swWriteRaw(out_buf, *pos, "\",dom_id:\"#swagger-ui\"});};</script></body></html>")) return null;
    } else {
        // Default: Scalar API Reference UI
        if (!swWriteRaw(out_buf, *pos, "<style>body{margin:0;padding:0;}</style></head><body><script id=\"api-reference\" data-url=\"")) return null;
        if (!swWriteHtmlEscaped(out_buf, *pos, cfg.spec_path)) return null;
        if (!swWriteRaw(out_buf, *pos, "\"></script><script src=\"https://cdn.jsdelivr.net/npm/@scalar/api-reference\"></script></body></html>")) return null;
    }

    return pos;
}

/// Dynamically regenerate and cache the OpenAPI 3.1.0 JSON specification from `app`.
pub fn refreshSpec(app: *App): usize? {
    if (n := app.toOpenApiJson(g_swagger_cfg.title, g_swagger_cfg.version, g_spec_buf[0..])) {
        g_spec_len = n;
        return n;
    }
    return null;
}

/// Return the currently cached OpenAPI 3.1.0 JSON specification slice.
pub fn cachedSpec(): string {
    return g_spec_buf[0..g_spec_len];
}

/// Return the currently cached interactive docs HTML slice.
pub fn cachedDocsHtml(): string {
    return g_html_buf[0..g_html_len];
}

fn swOpenApiSpecHandler(ctx: *Context): Response {
    * = ctx;
    if (g_spec_len == 0) {
        return Response.json(200, "{\"openapi\":\"3.1.0\",\"info\":{\"title\":\"CP API\",\"version\":\"1.0.0\"},\"paths\":{}}");
    }
    return Response.json(200, g_spec_buf[0..g_spec_len]);
}

fn swDocsUiHandler(ctx: *Context): Response {
    * = ctx;
    return Response.html(200, g_html_buf[0..g_html_len]);
}

/// Register OpenAPI 3.1.0 JSON endpoint (`GET cfg.spec_path`) and interactive Docs UI (`GET cfg.docs_path`).
pub fn registerSwagger(app: *App, cfg: SwaggerConfig): *App {
    g_swagger_cfg = cfg;

    if (html_len := renderDocsHtml(cfg, g_html_buf[0..])) {
        g_html_len = html_len;
    } else {
        g_html_len = 0;
    }

    // Generate initial OpenAPI 3.1.0 spec from routes already registered on `app`.
    * = refreshSpec(app);

    // Mount spec JSON & interactive HTML docs routes.
    * = app.get(cfg.spec_path, &swOpenApiSpecHandler);
    * = app.get(cfg.docs_path, &swDocsUiHandler);
    return app;
}
