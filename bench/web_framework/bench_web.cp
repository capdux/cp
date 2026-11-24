// ReleaseFast Performance Ceiling Benchmark for `cap:web` (RadixRouter + App.dispatch + Lifecycle + Extractors)
import {
    App,
    AppMethod,
    Context,
    Request,
    Response,
    CookieOptions,
    CookieSameSite,
    formatSetCookie,
} from "../../lib/web/app.cp";
import { RadixRouter } from "../../lib/web/radixRouter.cp";

@c struct Timespec {
    tv_sec: i64,
    tv_nsec: i64,
}
extern fn clock_gettime(clk_id: i32, tp: *Timespec): i32;

fn nowNs(): u64 {
    let ts = Timespec{ tv_sec: 0, tv_nsec: 0 };
    * = clock_gettime(1, *ts);
    return (ts.tv_sec as u64) * 1000000000 + (ts.tv_nsec as u64);
}

fn hOk(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"ok\":true}");
}

fn hUserPost(ctx: *Context): Response {
    const uid = ctx.paramU64("uid") ?? 0;
    const pid = ctx.paramU64("pid") ?? 0;
    if (uid > 0 && pid > 0) {
        return Response.json(200, "{\"matched\":true}");
    }
    return Response.json(400, "{\"matched\":false}");
}

fn mwTrace(ctx: *Context): bool {
    * = ctx.setHeader("X-Trace", "cp-1.0");
    return true;
}

fn runBench() {
    let app = App.init();
    * = app
        .onRequest(mwTrace)
        .get("/health", hOk)
        .get("/api/v1/status", hOk)
        .get("/api/v1/users", hOk)
        .post("/api/v1/users", hOk)
        .get("/api/v1/users/:uid", hOk)
        .get("/api/v1/users/:uid/posts/:pid", hUserPost)
        .get("/api/v1/orgs/{org}/repos/{repo}", hOk)
        .get("/static/*filepath", hOk);

    // 1. Raw RadixRouter O(1) static lookup benchmark (5,000,000 iterations)
    const n_radix: usize = 5000000;
    let acc1: usize = 0;
    const t0 = nowNs();
    let i: usize = 0;
    while (i < n_radix) : (i += 1) {
        let m1 = app.radix.lookup(0, "/api/v1/status");
        if (m1.matched) acc1 += 1;
    }
    const dt1_ns = nowNs() - t0;

    // 2. Raw RadixRouter dynamic `:uid` + `:pid` lookup benchmark (2,000,000 iterations)
    const n_dyn_rx: usize = 2000000;
    let acc2: usize = 0;
    const t1 = nowNs();
    i = 0;
    while (i < n_dyn_rx) : (i += 1) {
        let m2 = app.radix.lookup(0, "/api/v1/users/12345/posts/99");
        if (m2.matched) acc2 += m2.param_len;
    }
    const dt2_ns = nowNs() - t1;

    // 3. Full App.dispatch static route + onRequest hook (1,000,000 iterations)
    const n_disp_static: usize = 1000000;
    let acc3: usize = 0;
    const req_static = Request.init(AppMethod.GET, "/health");
    const t2 = nowNs();
    i = 0;
    while (i < n_disp_static) : (i += 1) {
        let resp1 = app.dispatch(req_static);
        acc3 += resp1.status as usize;
    }
    const dt3_ns = nowNs() - t2;

    // 4. Full App.dispatch dynamic `:uid/:pid` + query + onRequest hook (500,000 iterations)
    const n_disp_dyn: usize = 500000;
    let acc4: usize = 0;
    const req_dyn = Request.init(AppMethod.GET, "/api/v1/users/12345/posts/99?active=true");
    const t3 = nowNs();
    i = 0;
    while (i < n_disp_dyn) : (i += 1) {
        let resp2 = app.dispatch(req_dyn);
        acc4 += resp2.status as usize;
    }
    const dt4_ns = nowNs() - t3;

    // 5. RFC 6265 Cookie formatSetCookie benchmark (2,000,000 iterations)
    const n_cookie: usize = 2000000;
    let acc5: usize = 0;
    let cbuf: [256]u8 = [0; 256];
    const opts = CookieOptions{
        path: "/",
        domain: "cp.dev",
        max_age: 86400,
        http_only: true,
        secure: true,
        same_site: CookieSameSite.Lax,
    };
    const t4 = nowNs();
    i = 0;
    while (i < n_cookie) : (i += 1) {
        if (n := formatSetCookie("session_id", "tok_0123456789abcdef", opts, cbuf[0..])) {
            acc5 += n;
        }
    }
    const dt5_ns = nowNs() - t4;

    @print("radix_static_5M_ns={} ({} ops/s) acc={}", dt1_ns, (n_radix as u64 * 1000000000) / (if (dt1_ns == 0) 1 else dt1_ns), acc1);
    @print("radix_dynamic_2M_ns={} ({} ops/s) acc={}", dt2_ns, (n_dyn_rx as u64 * 1000000000) / (if (dt2_ns == 0) 1 else dt2_ns), acc2);
    @print("dispatch_static_1M_ns={} ({} req/s) acc={}", dt3_ns, (n_disp_static as u64 * 1000000000) / (if (dt3_ns == 0) 1 else dt3_ns), acc3);
    @print("dispatch_dynamic_500K_ns={} ({} req/s) acc={}", dt4_ns, (n_disp_dyn as u64 * 1000000000) / (if (dt4_ns == 0) 1 else dt4_ns), acc4);
    @print("cookie_format_2M_ns={} ({} ops/s) acc={}", dt5_ns, (n_cookie as u64 * 1000000000) / (if (dt5_ns == 0) 1 else dt5_ns), acc5);
}

fn main() {
    runBench();
}
