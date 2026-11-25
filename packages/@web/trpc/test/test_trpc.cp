// Self-contained unit & integration tests for @web/trpc

const std = @import("std");
import { App, AppMethod, Context, Request } from "../../../../lib/web/app.cp";
import {
    TrpcConfig,
    TrpcProcKind,
    trpcQuery,
    trpcMutation,
    trpcReset,
    registerTrpc,
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

fn queryGetUser(ctx: *Context, input: string): string {
    * = ctx;
    if (contains(input, "42")) {
        return "{\"id\":42,\"name\":\"Ada\"}";
    }
    return "{\"id\":0,\"name\":\"Guest\"}";
}

fn mutationCreateUser(ctx: *Context, body: string): string {
    * = ctx;
    if (contains(body, "Grace")) {
        return "{\"created\":true,\"name\":\"Grace\"}";
    }
    return "{\"created\":false}";
}

fn testTrpcProcedures() {
    trpcReset();
    const kq = TrpcProcKind.Query;
    const km = TrpcProcKind.Mutation;
    assertTrue(kq != km, "TrpcProcKind enum distinct");
    assertTrue(trpcQuery("user.get", &queryGetUser), "register query user.get");
    assertTrue(trpcMutation("user.create", &mutationCreateUser), "register mutation user.create");

    let app = App.init();
    * = registerTrpc(*app, TrpcConfig{ base_path: "/trpc" });

    // 1. GET query with ?input=...
    const req_q = Request.init(AppMethod.GET, "/trpc/user.get?input={\"id\":42}");
    const resp_q = app.dispatch(req_q);
    assertTrue(resp_q.status == 200, "GET query status 200");
    assertTrue(
        contains(resp_q.body, "{\"result\":{\"data\":{\"id\":42,\"name\":\"Ada\"}}}"),
        "GET query wraps result.data",
    );

    // 2. POST mutation with JSON body
    let req_m = Request.init(AppMethod.POST, "/trpc/user.create");
    * = req_m.withBody("{\"name\":\"Grace\"}");
    const resp_m = app.dispatch(req_m);
    assertTrue(resp_m.status == 200, "POST mutation status 200");
    assertTrue(
        contains(resp_m.body, "{\"result\":{\"data\":{\"created\":true,\"name\":\"Grace\"}}}"),
        "POST mutation wraps result.data",
    );

    // 3. GET calling a Mutation -> 405
    const req_wrong_get = Request.init(AppMethod.GET, "/trpc/user.create");
    const resp_wrong_get = app.dispatch(req_wrong_get);
    assertTrue(resp_wrong_get.status == 405, "GET on mutation returns 405");

    // 4. POST calling a Query -> 405
    let req_wrong_post = Request.init(AppMethod.POST, "/trpc/user.get");
    * = req_wrong_post.withBody("{}");
    const resp_wrong_post = app.dispatch(req_wrong_post);
    assertTrue(resp_wrong_post.status == 405, "POST on query returns 405");

    // 5. Unknown procedure -> 404 {"error":{"code":"NOT_FOUND"}}
    const req_unk = Request.init(AppMethod.GET, "/trpc/does.not.exist");
    const resp_unk = app.dispatch(req_unk);
    assertTrue(resp_unk.status == 404, "unknown procedure returns 404");
    assertTrue(
        contains(resp_unk.body, "{\"error\":{\"code\":\"NOT_FOUND\"}}"),
        "unknown procedure returns NOT_FOUND json",
    );
}

pub fn main() {
    testTrpcProcedures();
    std.debug.print("ALL @web/trpc TESTS PASSED!\n", .{});
}
