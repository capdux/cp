// Official @web/html Plugin — Zero-Heap HTML SSR, XSS Escaping & HTMX Helpers for cap:web
// Pure CP: zero zig{}, zero var, zero anyopaque, zero heap GC.

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";

pub struct HtmlConfig {
    default_charset: string = "utf-8",
}

let htm_cfg: HtmlConfig = HtmlConfig{};

fn htmAsciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn htmAsciiEqlIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (htmAsciiLower(a[i]) != htmAsciiLower(b[i])) return false;
    }
    return true;
}

fn htmWriteByte(out_buf: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out_buf.len) return false;
    out_buf[pos.*] = b;
    pos.* += 1;
    return true;
}

fn htmWriteRaw(out_buf: []u8, pos: *usize, s: string): bool {
    if (s.len > out_buf.len || pos.* > out_buf.len - s.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn htmEscapeInto(out_buf: []u8, pos: *usize, input: string): bool {
    let i: usize = 0;
    while (i < input.len) : (i += 1) {
        const c = input[i];
        if (c == '&') {
            if (!htmWriteRaw(out_buf, pos, "&amp;")) return false;
        } else if (c == '<') {
            if (!htmWriteRaw(out_buf, pos, "&lt;")) return false;
        } else if (c == '>') {
            if (!htmWriteRaw(out_buf, pos, "&gt;")) return false;
        } else if (c == '"') {
            if (!htmWriteRaw(out_buf, pos, "&quot;")) return false;
        } else if (c == '\'') {
            if (!htmWriteRaw(out_buf, pos, "&#39;")) return false;
        } else {
            if (!htmWriteByte(out_buf, pos, c)) return false;
        }
    }
    return true;
}

/// Zero-heap XSS-safe HTML escaping (`&`, `<`, `>`, `"`, `'`).
/// Returns the number of bytes written to `out_buf`, or `null` if `out_buf` is too small.
pub fn htmlEscape(input: string, out_buf: []u8): usize? {
    let pos: usize = 0;
    if (!htmEscapeInto(out_buf, *pos, input)) return null;
    return pos;
}

/// Stack-backed zero-heap HTML document builder.
pub struct HtmlBuilder {
    buf: []u8,
    pos: usize = 0,
    overflow: bool = false,

    pub static fn init(buf: []u8): HtmlBuilder {
        return HtmlBuilder{
            buf: buf,
            pos: 0,
            overflow: false,
        };
    }

    pub fn raw(*_, s: string): *HtmlBuilder {
        if (_.overflow) return _;
        if (!htmWriteRaw(_.buf, *_.pos, s)) {
            _.overflow = true;
        }
        return _;
    }

    pub fn text(*_, s: string): *HtmlBuilder {
        if (_.overflow) return _;
        if (!htmEscapeInto(_.buf, *_.pos, s)) {
            _.overflow = true;
        }
        return _;
    }

    /// Append `<name>escaped_content</name>`.
    pub fn tag(*_, name: string, content: string): *HtmlBuilder {
        if (_.overflow) return _;
        if (!htmWriteByte(_.buf, *_.pos, '<')) {
            _.overflow = true;
            return _;
        }
        if (!htmWriteRaw(_.buf, *_.pos, name)) {
            _.overflow = true;
            return _;
        }
        if (!htmWriteByte(_.buf, *_.pos, '>')) {
            _.overflow = true;
            return _;
        }
        if (!htmEscapeInto(_.buf, *_.pos, content)) {
            _.overflow = true;
            return _;
        }
        if (!htmWriteRaw(_.buf, *_.pos, "</")) {
            _.overflow = true;
            return _;
        }
        if (!htmWriteRaw(_.buf, *_.pos, name)) {
            _.overflow = true;
            return _;
        }
        if (!htmWriteByte(_.buf, *_.pos, '>')) {
            _.overflow = true;
            return _;
        }
        return _;
    }

    /// Append `<name>inner_html</name>` without escaping `inner_html`.
    pub fn tagRaw(*_, name: string, inner_html: string): *HtmlBuilder {
        if (_.overflow) return _;
        if (!htmWriteByte(_.buf, *_.pos, '<')) {
            _.overflow = true;
            return _;
        }
        if (!htmWriteRaw(_.buf, *_.pos, name)) {
            _.overflow = true;
            return _;
        }
        if (!htmWriteByte(_.buf, *_.pos, '>')) {
            _.overflow = true;
            return _;
        }
        if (!htmWriteRaw(_.buf, *_.pos, inner_html)) {
            _.overflow = true;
            return _;
        }
        if (!htmWriteRaw(_.buf, *_.pos, "</")) {
            _.overflow = true;
            return _;
        }
        if (!htmWriteRaw(_.buf, *_.pos, name)) {
            _.overflow = true;
            return _;
        }
        if (!htmWriteByte(_.buf, *_.pos, '>')) {
            _.overflow = true;
            return _;
        }
        return _;
    }

    pub fn slice(): string {
        return _.buf[0.._.pos];
    }

    pub fn len(): usize {
        return _.pos;
    }

    pub fn isOverflow(): bool {
        return _.overflow;
    }

    pub fn reset(*_) {
        _.pos = 0;
        _.overflow = false;
    }
}

/// Detect whether the incoming request is an HTMX request (`HX-Request: true`).
pub fn htmlIsHtmx(ctx: *const Context): bool {
    if (v := ctx.header("HX-Request")) {
        return htmAsciiEqlIgnoreCase(v, "true");
    }
    return false;
}

/// Return a `200 OK` response with `HX-Redirect` header for HTMX client navigation.
pub fn htmlHxRedirect(url: string): Response {
    let resp = Response.html(200, "");
    * = resp.setHeader("HX-Redirect", url);
    return resp;
}

/// Return a `200 OK` `text/html` response with `HX-Trigger` header for HTMX client events.
pub fn htmlHxTrigger(body_html: string, event_name: string): Response {
    let resp = Response.html(200, body_html);
    * = resp.setHeader("HX-Trigger", event_name);
    return resp;
}

fn htmAfterHook(ctx: *Context, resp: *Response) {
    if (htmlIsHtmx(ctx)) {
        if (resp.header("Vary") == null) {
            * = resp.setHeader("Vary", "HX-Request");
        }
    }
}

/// Register the `@web/html` SSR & HTMX plugin on `app`.
pub fn registerHtml(app: *App, cfg: HtmlConfig): *App {
    htm_cfg = cfg;
    let p = Plugin.init("@web/html");
    * = p.withScope(PluginScope.Global).onAfterHandle(htmAfterHook);
    return app.usePlugin(&p);
}
