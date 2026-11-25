// Regression Test Suite for `@web/pressure`
import { App, AppMethod, Context, Request, Response } from "../../../../lib/web/app.cp";
import {
    PressureConfig,
    registerPressure,
    pressureSetReady,
    pressureSetInflightForTest,
    pressureShedCount,
    pressureResetStats,
} from "../src/index.cp";

fn hEcho(ctx: *Context): Response {
    return Response.text(200, ctx.body);
}

fn main() {
    pressureResetStats();
    let app = App.init();
    * = registerPressure(*app, PressureConfig{
        max_body_bytes: 16,
        max_inflight: 2,
        retry_after_secs: "10",
    });
    * = app.post("/upload", hEcho);

    // 1. K8s probes return 200
    if (app.dispatch(Request.init(AppMethod.GET, "/healthz")).status != 200) {
        @panic("Test 1a failed: /healthz");
    }
    if (app.dispatch(Request.init(AppMethod.GET, "/readyz")).status != 200) {
        @panic("Test 1b failed: /readyz");
    }

    // 2. Draining readiness probe returns 503
    pressureSetReady(false);
    const dr = app.dispatch(Request.init(AppMethod.GET, "/readyz"));
    if (dr.status != 503 || (dr.header("Retry-After") ?? "").len == 0) {
        @panic("Test 2 failed: draining /readyz");
    }
    pressureSetReady(true);

    // 3. Body exceeding max_body_bytes returns 413
    let big_req = Request.init(AppMethod.POST, "/upload");
    * = big_req.withBody("0123456789abcdef_overflow");
    if (app.dispatch(big_req).status != 413) {
        @panic("Test 3 failed: expected 413 Payload Too Large");
    }

    // 4. Concurrency shedding returns 503 + Retry-After
    pressureSetInflightForTest(2);
    let ok_req = Request.init(AppMethod.POST, "/upload");
    * = ok_req.withBody("hello");
    const shed = app.dispatch(ok_req);
    if (shed.status != 503 || pressureShedCount() != 1) {
        @panic("Test 4 failed: expected 503 load shed");
    }

    // 5. Normal request succeeds when under limits
    pressureSetInflightForTest(0);
    if (app.dispatch(ok_req).status != 200) {
        @panic("Test 5 failed: normal POST failed");
    }

    @print("PASS @web/pressure (5/5 tests)");
}
