// Regression Test Suite for `@web/sensible`
import { App, AppMethod, Context, Request, Response } from "../../../../lib/web/app.cp";
import {
    SensibleConfig,
    registerSensible,
    httpBadRequest,
    httpNotFound,
    httpConflict,
    sensibleAssert,
} from "../src/index.cp";

fn strContains(hay: string, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > hay.len) return false;
    let i: usize = 0;
    while (i + needle.len <= hay.len) : (i += 1) {
        let ok = true;
        let j: usize = 0;
        while (j < needle.len) : (j += 1) {
            if (hay[i + j] != needle[j]) {
                ok = false;
                break;
            }
        }
        if (ok) return true;
    }
    return false;
}

fn hCheck(ctx: *Context): Response {
    const id = ctx.query("id") ?? "";
    if (!sensibleAssert(ctx, id.len > 0, 400, "Bad Request", "missing id")) {
        return httpBadRequest("missing id");
    }
    if (id.len == 1 && id[0] == '0') {
        return httpNotFound("item not found");
    }
    return Response.json(200, "{\"ok\":true}");
}

fn main() {
    let app = App.init();
    * = registerSensible(*app, SensibleConfig{});
    * = app.get("/check", hCheck);

    const r1 = app.dispatch(Request.init(AppMethod.GET, "/check"));
    const b1 = r1.body_buf[0..r1.body_buf_len];
    if (r1.status != 400 || !strContains(b1, "\"status\":400") || !strContains(b1, "missing id")) {
        @panic("Test 1 failed: sensibleAssert 400");
    }

    const r2 = app.dispatch(Request.init(AppMethod.GET, "/check?id=0"));
    const b2 = r2.body_buf[0..r2.body_buf_len];
    if (r2.status != 404 || !strContains(b2, "Not Found") || !strContains(r2.header("X-Error-Format") ?? "", "rfc9457")) {
        @panic("Test 2 failed: httpNotFound + X-Error-Format");
    }

    const r3 = httpConflict("duplicate email");
    if (r3.status != 409 || !strContains(r3.body_buf[0..r3.body_buf_len], "Conflict")) {
        @panic("Test 3 failed: httpConflict");
    }

    const r4 = app.dispatch(Request.init(AppMethod.GET, "/check?id=42"));
    if (r4.status != 200) @panic("Test 4 failed: valid request");

    @print("PASS @web/sensible (4/4 tests)");
}
