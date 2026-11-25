// Self-contained unit & integration tests for @web/html

const std = @import("std");
import { App, AppMethod, Context, Request, Response } from "../../../../lib/web/app.cp";
import {
    HtmlConfig,
    HtmlBuilder,
    htmlEscape,
    htmlIsHtmx,
    htmlHxRedirect,
    htmlHxTrigger,
    registerHtml,
} from "../src/index.cp";

fn assertTrue(cond: bool, msg: string) {
    if (!cond) {
        std.debug.print("FAIL: {s}\n", .{msg});
        std.process.exit(1);
    }
}

fn strEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn testHtmlEscape() {
    let buf: [256]u8 = [0; 256];
    if (n := htmlEscape("<script>alert(\"x\" & 'y')</script>", buf[0..])) {
        const out = buf[0..n];
        assertTrue(
            strEql(out, "&lt;script&gt;alert(&quot;x&quot; &amp; &#39;y&#39;)&lt;/script&gt;"),
            "htmlEscape escapes all 5 XSS chars",
        );
    } else {
        assertTrue(false, "htmlEscape should succeed");
    }

    let tiny: [4]u8 = [0; 4];
    const overflow = htmlEscape("<tag>", tiny[0..]);
    assertTrue(overflow == null, "htmlEscape returns null on small buffer");
}

fn testHtmlBuilder() {
    let buf: [512]u8 = [0; 512];
    let b = HtmlBuilder.init(buf[0..]);
    * = b.raw("<!doctype html><html><body>");
    * = b.tag("h1", "Hello <Admin> & \"Team\"");
    * = b.raw("<p>");
    * = b.text("1 < 2 && 'ok'");
    * = b.raw("</p></body></html>");
    assertTrue(!b.isOverflow(), "HtmlBuilder no overflow");
    assertTrue(
        strEql(
            b.slice(),
            "<!doctype html><html><body><h1>Hello &lt;Admin&gt; &amp; &quot;Team&quot;</h1><p>1 &lt; 2 &amp;&amp; &#39;ok&#39;</p></body></html>",
        ),
        "HtmlBuilder produces expected XSS-safe HTML",
    );

    let small: [8]u8 = [0; 8];
    let b2 = HtmlBuilder.init(small[0..]);
    * = b2.tag("div", "overflow-content");
    assertTrue(b2.isOverflow(), "HtmlBuilder marks overflow on small buffer");
}

fn pageHandler(ctx: *Context): Response {
    if (htmlIsHtmx(ctx)) {
        return htmlHxTrigger("<div>partial</div>", "itemUpdated");
    }
    return Response.html(200, "<html><body>full</body></html>");
}

fn redirectHandler(ctx: *Context): Response {
    * = ctx;
    return htmlHxRedirect("/dashboard");
}

fn testHtmxIntegration() {
    let app = App.init();
    * = registerHtml(*app, HtmlConfig{ default_charset: "utf-8" });
    * = app.get("/page", &pageHandler);
    * = app.post("/login", &redirectHandler);

    // Non-HTMX request
    const req_full = Request.init(AppMethod.GET, "/page");
    const resp_full = app.dispatch(req_full);
    assertTrue(resp_full.status == 200, "full page status 200");
    assertTrue(strEql(resp_full.body, "<html><body>full</body></html>"), "full page html body");
    assertTrue(resp_full.header("HX-Trigger") == null, "non-htmx has no HX-Trigger");

    // HTMX request
    let req_hx = Request.init(AppMethod.GET, "/page");
    * = req_hx.withHeader("HX-Request", "true");
    const resp_hx = app.dispatch(req_hx);
    assertTrue(resp_hx.status == 200, "htmx partial status 200");
    assertTrue(strEql(resp_hx.body, "<div>partial</div>"), "htmx partial body");
    assertTrue(strEql(resp_hx.content_type, "text/html; charset=utf-8"), "htmx content-type text/html");
    if (trig := resp_hx.header("HX-Trigger")) {
        assertTrue(strEql(trig, "itemUpdated"), "HX-Trigger header matches");
    } else {
        assertTrue(false, "HX-Trigger header missing");
    }
    if (vary := resp_hx.header("Vary")) {
        assertTrue(strEql(vary, "HX-Request"), "Vary header set for HTMX");
    } else {
        assertTrue(false, "Vary header missing for HTMX");
    }

    // HX-Redirect
    const req_redir = Request.init(AppMethod.POST, "/login");
    const resp_redir = app.dispatch(req_redir);
    assertTrue(resp_redir.status == 200, "HX-Redirect status 200");
    if (loc := resp_redir.header("HX-Redirect")) {
        assertTrue(strEql(loc, "/dashboard"), "HX-Redirect target matches");
    } else {
        assertTrue(false, "HX-Redirect header missing");
    }
}

pub fn main() {
    testHtmlEscape();
    testHtmlBuilder();
    testHtmxIntegration();
    std.debug.print("ALL @web/html TESTS PASSED!\n", .{});
}
