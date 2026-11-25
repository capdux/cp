// Unit & integration tests for @web/audit
const std = @import("std");
import { App, AppMethod, Request, Context, Response } from "../../../../lib/web/app.cp";
import {
    AuditConfig,
    AuditRecord,
    auditMaskHeaderValue,
    auditCount,
    auditTotalSeq,
    auditRecordAt,
    auditReset,
    registerAudit,
} from "../src/index.cp";

fn assertTrue(cond: bool, msg: string) {
    if (!cond) {
        std.debug.print("FAIL: {s}\n", .{msg});
        std.process.exit(1);
    }
}

fn testStrEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn testContains(haystack: string, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) : (i += 1) {
        if (testStrEql(haystack[i .. i + needle.len], needle)) return true;
    }
    return false;
}

fn deriveAuth(ctx: *Context): bool {
    if (sub := ctx.header("X-Sim-Jwt-Sub")) {
        * = ctx.setDerived("jwt_sub", sub);
    }
    if (usr := ctx.header("X-Sim-Auth-User")) {
        * = ctx.setDerived("auth_user", usr);
    }
    return true;
}

fn getHandler(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"items\":[]}");
}

fn createHandler(ctx: *Context): Response {
    * = ctx;
    return Response.json(201, "{\"created\":true}");
}

fn updateHandler(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"updated\":true}");
}

fn deleteHandler(ctx: *Context): Response {
    * = ctx;
    return Response.noContent();
}

pub fn main() {
    auditReset();

    // 1. Sensitive header masking
    assertTrue(testStrEql(auditMaskHeaderValue("Authorization", "Bearer secret"), "[REDACTED]"), "mask Authorization");
    assertTrue(testStrEql(auditMaskHeaderValue("cookie", "sid=abc123"), "[REDACTED]"), "mask Cookie case-insensitive");
    assertTrue(testStrEql(auditMaskHeaderValue("X-API-Key", "sk_live_999"), "[REDACTED]"), "mask X-API-Key");
    assertTrue(testStrEql(auditMaskHeaderValue("Content-Type", "application/json"), "application/json"), "do not mask Content-Type");

    // 2. App integration
    let app = App.init();
    * = app.derive(deriveAuth);
    * = registerAudit(*app, AuditConfig{
        status_path: "/_audit",
        actor_header: "X-Actor",
        mask_sensitive: true,
    });
    * = app.get("/items", getHandler);
    * = app.post("/items", createHandler);
    * = app.put("/items/1", updateHandler);
    * = app.patch("/items/1", updateHandler);
    * = app.delete("/items/1", deleteHandler);

    // GET is not a write operation -> not audited
    assertTrue(app.dispatch(Request.init(AppMethod.GET, "/items")).status == 200, "GET /items -> 200");
    assertTrue(auditCount() == 0, "GET not audited");

    // POST with jwt_sub + X-Forwarded-For
    let req_post = Request.init(AppMethod.POST, "/items");
    * = req_post
        .withHeader("X-Sim-Jwt-Sub", "user-jwt-42")
        .withHeader("X-Forwarded-For", "203.0.113.10, 10.0.0.1");
    assertTrue(app.dispatch(req_post).status == 201, "POST /items -> 201");

    // PUT with auth_user + X-Real-IP
    let req_put = Request.init(AppMethod.PUT, "/items/1");
    * = req_put
        .withHeader("X-Sim-Auth-User", "bob-basic")
        .withHeader("X-Real-IP", "198.51.100.7");
    assertTrue(app.dispatch(req_put).status == 200, "PUT /items/1 -> 200");

    // PATCH with X-Actor header
    let req_patch = Request.init(AppMethod.PATCH, "/items/1");
    * = req_patch.withHeader("X-Actor", "ops-bot");
    assertTrue(app.dispatch(req_patch).status == 200, "PATCH /items/1 -> 200");

    // DELETE anonymous
    assertTrue(app.dispatch(Request.init(AppMethod.DELETE, "/items/1")).status == 204, "DELETE /items/1 -> 204");

    assertTrue(auditCount() == 4, "4 write operations audited");

    const rec0 = auditRecordAt(0) orelse AuditRecord{};
    assertTrue(rec0.seq == 1 && testStrEql(rec0.method, "POST") && rec0.status == 201, "rec0 metadata");
    assertTrue(testStrEql(rec0.actor, "user-jwt-42") && testStrEql(rec0.client_ip, "203.0.113.10"), "rec0 actor & ip");

    const rec1 = auditRecordAt(1) orelse AuditRecord{};
    assertTrue(rec1.seq == 2 && testStrEql(rec1.actor, "bob-basic") && testStrEql(rec1.client_ip, "198.51.100.7"), "rec1 actor & ip");

    const rec2 = auditRecordAt(2) orelse AuditRecord{};
    assertTrue(rec2.seq == 3 && testStrEql(rec2.actor, "ops-bot") && testStrEql(rec2.client_ip, "unknown"), "rec2 actor & ip");

    const rec3 = auditRecordAt(3) orelse AuditRecord{};
    assertTrue(rec3.seq == 4 && testStrEql(rec3.method, "DELETE") && rec3.status == 204 && testStrEql(rec3.actor, "anonymous"), "rec3 anonymous delete");

    // 3. GET /_audit JSON endpoint
    const r_status = app.dispatch(Request.init(AppMethod.GET, "/_audit"));
    assertTrue(r_status.status == 200, "GET /_audit -> 200");
    assertTrue(testContains(r_status.body, "\"count\":4"), "audit json count=4");
    assertTrue(testContains(r_status.body, "\"actor\":\"user-jwt-42\""), "audit json contains jwt actor");
    assertTrue(testContains(r_status.body, "\"method\":\"DELETE\""), "audit json contains DELETE");

    // 4. 32-slot ring buffer wrap-around (push 32 more -> total 36, oldest seq=5)
    let k: usize = 0;
    while (k < 32) : (k += 1) {
        * = app.dispatch(Request.init(AppMethod.POST, "/items"));
    }
    assertTrue(auditCount() == 32, "ring buffer caps at 32");
    assertTrue(auditTotalSeq() == 36, "total seq is 36");
    const oldest = auditRecordAt(0) orelse AuditRecord{};
    const newest = auditRecordAt(31) orelse AuditRecord{};
    assertTrue(oldest.seq == 5, "oldest retained record has seq=5");
    assertTrue(newest.seq == 36, "newest retained record has seq=36");

    auditReset();
    assertTrue(auditCount() == 0 && auditTotalSeq() == 0, "auditReset clears ring buffer");

    std.debug.print("PASS: @web/audit all tests passed\n", .{});
}
