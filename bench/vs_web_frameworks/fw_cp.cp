// Real Web Framework Benchmark: CP (`cap:web` App + WebServer)
// 24 routes (12 static + 8 dynamic :param + 2 wildcard + 2 POST JSON) + derive auth/cookie hook
// Env vars: PORT (default 18301), WORKERS (default 1)

import { App, Context, Response } from "../../lib/web/app.cp";
import { WebServer, ServerConfig } from "../../lib/web/server.cp";

extern fn getenv(name: [*:0]const u8): [*:0]const u8?;
extern fn atoi(s: [*:0]const u8): i32;

fn envInt(name: [*:0]const u8, default_val: i32): i32 {
    if (v := getenv(name)) {
        const n = atoi(v);
        if (n > 0) return n;
    }
    return default_val;
}

// ============================================================================
// Zero-Alloc Response JSON Buffer Helpers (writes into Response.body_buf[128])
// ============================================================================

fn bufWriteStr(buf: []u8, pos: *usize, s: string): bool {
    if (s.len > buf.len || pos.* > buf.len - s.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        buf[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn bufWriteI64(buf: []u8, pos: *usize, val: i64): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v: u64 = 0;
    if (val < 0) {
        if (pos.* >= buf.len) return false;
        buf[pos.*] = '-';
        pos.* += 1;
        v = (-val) as u64;
    } else {
        v = val as u64;
    }
    if (v == 0) {
        if (pos.* >= buf.len) return false;
        buf[pos.*] = '0';
        pos.* += 1;
        return true;
    }
    while (v > 0) {
        tmp[n] = (48 + ((v % 10) as u8)) as u8;
        n += 1;
        v = v / 10;
    }
    if (pos.* + n > buf.len) return false;
    while (n > 0) {
        n -= 1;
        buf[pos.*] = tmp[n];
        pos.* += 1;
    }
    return true;
}

fn newJsonResponse(status_code: u16): Response {
    return Response{
        status: status_code,
        content_type: "application/json; charset=utf-8",
        body: "",
        body_buf: undefined,
        body_buf_len: 0,
        headers: undefined,
        header_len: 0,
    };
}

// ============================================================================
// Zero-Alloc Flat JSON Object Parser for POST Bodies
// ============================================================================

fn strEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn skipWs(s: string, p: usize): usize {
    let i = p;
    while (i < s.len && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) : (i += 1) {}
    return i;
}

struct ParsedOrder {
    item: string = "";
    qty: i64 = 0;
    price: i64 = 0;
    has_item: bool = false;
    has_qty: bool = false;
    has_price: bool = false;
    valid: bool = false;
}

fn parseOrderJson(body: string): ParsedOrder {
    let out = ParsedOrder{};
    let i = skipWs(body, 0);
    if (i >= body.len || body[i] != '{') return out;
    i += 1;

    while (i < body.len) {
        i = skipWs(body, i);
        if (i >= body.len) return out;
        if (body[i] == '}') {
            out.valid = true;
            return out;
        }
        if (body[i] != '"') return out;
        i += 1;
        const k_start = i;
        while (i < body.len && body[i] != '"') : (i += 1) {}
        if (i >= body.len) return out;
        const key = body[k_start..i];
        i += 1;

        i = skipWs(body, i);
        if (i >= body.len || body[i] != ':') return out;
        i += 1;
        i = skipWs(body, i);
        if (i >= body.len) return out;

        if (body[i] == '"') {
            i += 1;
            const v_start = i;
            while (i < body.len && body[i] != '"') : (i += 1) {
                if (body[i] == '\\' && i + 1 < body.len) {
                    i += 1;
                }
            }
            if (i >= body.len) return out;
            const val = body[v_start..i];
            i += 1;
            if (strEql(key, "item")) {
                out.item = val;
                out.has_item = true;
            }
        } else {
            let neg = false;
            if (body[i] == '-') {
                neg = true;
                i += 1;
            }
            const num_start = i;
            let acc: i64 = 0;
            while (i < body.len && body[i] >= '0' && body[i] <= '9') : (i += 1) {
                acc = acc * 10 + ((body[i] - '0') as i64);
            }
            if (i == num_start) return out;
            if (neg) acc = -acc;
            if (strEql(key, "qty")) {
                out.qty = acc;
                out.has_qty = true;
            } else if (strEql(key, "price")) {
                out.price = acc;
                out.has_price = true;
            }
        }

        i = skipWs(body, i);
        if (i < body.len && body[i] == ',') {
            i += 1;
        } else if (i < body.len && body[i] == '}') {
            out.valid = true;
            return out;
        } else {
            return out;
        }
    }
    return out;
}

// ============================================================================
// Derive Middleware: Extract Cookie `sid` and `Authorization: Bearer <token>`
// ============================================================================

fn authDeriveHook(ctx: *Context): bool {
    if (sid := ctx.cookie("sid")) {
        * = ctx.setDerived("sid", sid);
    }
    if (tok := ctx.bearerToken()) {
        * = ctx.setDerived("token", tok);
    }
    return true;
}

// ============================================================================
// 12 Static Route Handlers
// ============================================================================

fn hRoot(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"service\":\"cap-web\",\"version\":\"1.0\"}");
}

fn hHealth(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"ok\":true,\"service\":\"cap-web\"}");
}

fn hReady(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"ready\":true}");
}

fn hVersion(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"version\":\"1.0.0\"}");
}

fn hMetrics(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"uptime\":1,\"requests\":0}");
}

fn hMe(ctx: *Context): Response {
    const sid = ctx.derived("sid") ?? "";
    const tok = ctx.derived("token") ?? "";
    let r = newJsonResponse(200);
    let pos: usize = 0;
    * = bufWriteStr(r.body_buf[0..], *pos, "{\"sid\":\"");
    * = bufWriteStr(r.body_buf[0..], *pos, sid);
    * = bufWriteStr(r.body_buf[0..], *pos, "\",\"token\":\"");
    * = bufWriteStr(r.body_buf[0..], *pos, tok);
    * = bufWriteStr(r.body_buf[0..], *pos, "\"}");
    r.body_buf_len = pos;
    return r;
}

fn hStatus(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"status\":\"operational\"}");
}

fn hConfig(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"env\":\"prod\",\"region\":\"us-east-1\"}");
}

fn hUsersList(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"items\":[],\"total\":0}");
}

fn hProductsList(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"items\":[],\"total\":0}");
}

fn hOrdersList(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"items\":[],\"total\":0}");
}

fn hCategories(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"categories\":[\"gpu\",\"cpu\",\"net\"]}");
}

// ============================================================================
// 8 Dynamic :param Route Handlers
// ============================================================================

fn hUserById(ctx: *Context): Response {
    const id = ctx.param("id") ?? "";
    let r = newJsonResponse(200);
    let pos: usize = 0;
    * = bufWriteStr(r.body_buf[0..], *pos, "{\"userId\":\"");
    * = bufWriteStr(r.body_buf[0..], *pos, id);
    * = bufWriteStr(r.body_buf[0..], *pos, "\"}");
    r.body_buf_len = pos;
    return r;
}

fn hUserProfile(ctx: *Context): Response {
    const id = ctx.param("id") ?? "";
    let r = newJsonResponse(200);
    let pos: usize = 0;
    * = bufWriteStr(r.body_buf[0..], *pos, "{\"userId\":\"");
    * = bufWriteStr(r.body_buf[0..], *pos, id);
    * = bufWriteStr(r.body_buf[0..], *pos, "\",\"role\":\"member\"}");
    r.body_buf_len = pos;
    return r;
}

fn hUserPost(ctx: *Context): Response {
    const id = ctx.param("id") ?? "";
    const post_id = ctx.param("postId") ?? "";
    const lang = ctx.query("lang") ?? "";
    const limit = ctx.query("limit") ?? "";
    let r = newJsonResponse(200);
    let pos: usize = 0;
    * = bufWriteStr(r.body_buf[0..], *pos, "{\"userId\":\"");
    * = bufWriteStr(r.body_buf[0..], *pos, id);
    * = bufWriteStr(r.body_buf[0..], *pos, "\",\"postId\":\"");
    * = bufWriteStr(r.body_buf[0..], *pos, post_id);
    * = bufWriteStr(r.body_buf[0..], *pos, "\",\"lang\":\"");
    * = bufWriteStr(r.body_buf[0..], *pos, lang);
    * = bufWriteStr(r.body_buf[0..], *pos, "\",\"limit\":\"");
    * = bufWriteStr(r.body_buf[0..], *pos, limit);
    * = bufWriteStr(r.body_buf[0..], *pos, "\"}");
    r.body_buf_len = pos;
    return r;
}

fn hProductBySku(ctx: *Context): Response {
    const sku = ctx.param("sku") ?? "";
    let r = newJsonResponse(200);
    let pos: usize = 0;
    * = bufWriteStr(r.body_buf[0..], *pos, "{\"sku\":\"");
    * = bufWriteStr(r.body_buf[0..], *pos, sku);
    * = bufWriteStr(r.body_buf[0..], *pos, "\"}");
    r.body_buf_len = pos;
    return r;
}

fn hProductReview(ctx: *Context): Response {
    const sku = ctx.param("sku") ?? "";
    const review_id = ctx.param("reviewId") ?? "";
    let r = newJsonResponse(200);
    let pos: usize = 0;
    * = bufWriteStr(r.body_buf[0..], *pos, "{\"sku\":\"");
    * = bufWriteStr(r.body_buf[0..], *pos, sku);
    * = bufWriteStr(r.body_buf[0..], *pos, "\",\"reviewId\":\"");
    * = bufWriteStr(r.body_buf[0..], *pos, review_id);
    * = bufWriteStr(r.body_buf[0..], *pos, "\"}");
    r.body_buf_len = pos;
    return r;
}

fn hOrderById(ctx: *Context): Response {
    const oid = ctx.param("orderId") ?? "";
    let r = newJsonResponse(200);
    let pos: usize = 0;
    * = bufWriteStr(r.body_buf[0..], *pos, "{\"orderId\":\"");
    * = bufWriteStr(r.body_buf[0..], *pos, oid);
    * = bufWriteStr(r.body_buf[0..], *pos, "\"}");
    r.body_buf_len = pos;
    return r;
}

fn hTeamMember(ctx: *Context): Response {
    const tid = ctx.param("teamId") ?? "";
    const mid = ctx.param("memberId") ?? "";
    let r = newJsonResponse(200);
    let pos: usize = 0;
    * = bufWriteStr(r.body_buf[0..], *pos, "{\"teamId\":\"");
    * = bufWriteStr(r.body_buf[0..], *pos, tid);
    * = bufWriteStr(r.body_buf[0..], *pos, "\",\"memberId\":\"");
    * = bufWriteStr(r.body_buf[0..], *pos, mid);
    * = bufWriteStr(r.body_buf[0..], *pos, "\"}");
    r.body_buf_len = pos;
    return r;
}

fn hOrgProject(ctx: *Context): Response {
    const oid = ctx.param("orgId") ?? "";
    const pid = ctx.param("projId") ?? "";
    let r = newJsonResponse(200);
    let pos: usize = 0;
    * = bufWriteStr(r.body_buf[0..], *pos, "{\"orgId\":\"");
    * = bufWriteStr(r.body_buf[0..], *pos, oid);
    * = bufWriteStr(r.body_buf[0..], *pos, "\",\"projId\":\"");
    * = bufWriteStr(r.body_buf[0..], *pos, pid);
    * = bufWriteStr(r.body_buf[0..], *pos, "\"}");
    r.body_buf_len = pos;
    return r;
}

// ============================================================================
// 2 Wildcard Route Handlers
// ============================================================================

fn hStaticWildcard(ctx: *Context): Response {
    const asset = ctx.param("*") ?? "";
    let r = newJsonResponse(200);
    let pos: usize = 0;
    * = bufWriteStr(r.body_buf[0..], *pos, "{\"asset\":\"");
    * = bufWriteStr(r.body_buf[0..], *pos, asset);
    * = bufWriteStr(r.body_buf[0..], *pos, "\"}");
    r.body_buf_len = pos;
    return r;
}

fn hFilesWildcard(ctx: *Context): Response {
    const file = ctx.param("*") ?? "";
    let r = newJsonResponse(200);
    let pos: usize = 0;
    * = bufWriteStr(r.body_buf[0..], *pos, "{\"file\":\"");
    * = bufWriteStr(r.body_buf[0..], *pos, file);
    * = bufWriteStr(r.body_buf[0..], *pos, "\"}");
    r.body_buf_len = pos;
    return r;
}

// ============================================================================
// 2 POST JSON Route Handlers
// ============================================================================

fn hCreateOrder(ctx: *Context): Response {
    const parsed = parseOrderJson(ctx.body);
    if (!parsed.valid || !parsed.has_item || parsed.item.len == 0 || !parsed.has_qty || parsed.qty <= 0 || !parsed.has_price || parsed.price <= 0) {
        return Response.json(400, "{\"accepted\":false,\"error\":\"invalid_order\"}");
    }
    let r = newJsonResponse(200);
    let pos: usize = 0;
    * = bufWriteStr(r.body_buf[0..], *pos, "{\"accepted\":true,\"item\":\"");
    * = bufWriteStr(r.body_buf[0..], *pos, parsed.item);
    * = bufWriteStr(r.body_buf[0..], *pos, "\",\"qty\":");
    * = bufWriteI64(r.body_buf[0..], *pos, parsed.qty);
    * = bufWriteStr(r.body_buf[0..], *pos, "}");
    r.body_buf_len = pos;
    return r;
}

fn hCreateEvent(ctx: *Context): Response {
    if (ctx.body.len < 2 || ctx.body[0] != '{') {
        return Response.json(400, "{\"recorded\":false,\"error\":\"invalid_event\"}");
    }
    return Response.json(200, "{\"recorded\":true}");
}

// ============================================================================
// Main Entrypoint
// ============================================================================

fn main() {
    const port: u16 = envInt("PORT", 18301) as u16;
    const workers: usize = envInt("WORKERS", 1) as usize;

    let app = App.init();
    * = app
        .derive(authDeriveHook)
        // 12 Static Routes
        .get("/", hRoot)
        .get("/health", hHealth)
        .get("/ready", hReady)
        .get("/version", hVersion)
        .get("/metrics", hMetrics)
        .get("/api/v1/me", hMe)
        .get("/api/v1/status", hStatus)
        .get("/api/v1/config", hConfig)
        .get("/api/v1/users", hUsersList)
        .get("/api/v1/products", hProductsList)
        .get("/api/v1/orders", hOrdersList)
        .get("/api/v1/catalog/categories", hCategories)
        // 8 Dynamic :param Routes
        .get("/api/v1/users/:id", hUserById)
        .get("/api/v1/users/:id/profile", hUserProfile)
        .get("/api/v1/users/:id/posts/:postId", hUserPost)
        .get("/api/v1/products/:sku", hProductBySku)
        .get("/api/v1/products/:sku/reviews/:reviewId", hProductReview)
        .get("/api/v1/orders/:orderId", hOrderById)
        .get("/api/v1/teams/:teamId/members/:memberId", hTeamMember)
        .get("/api/v1/orgs/:orgId/projects/:projId", hOrgProject)
        // 2 Wildcard Routes
        .get("/static/*", hStaticWildcard)
        .get("/files/*", hFilesWildcard)
        // 2 POST JSON Routes
        .post("/api/v1/orders", hCreateOrder)
        .post("/api/v1/events", hCreateEvent);

    let srv = WebServer.init(ServerConfig{
        port: port,
        workers: workers,
        backlog: 4096,
        pin_cpu: true,
    });
    if (!srv.start(*app)) {
        @panic("fw_cp failed to bind port");
    }
    @print("CP_FW_SERVER_READY port={} workers={} routes={}", port, workers, app.routeCount());
    srv.join();
}
