// Self-contained unit & integration tests for @web/graphql

const std = @import("std");
import { App, AppMethod, Context, Request } from "../../../../lib/web/app.cp";
import {
    GraphQLConfig,
    gqlCheckQueryDepth,
    gqlRegisterResolver,
    gqlResetResolvers,
    registerGraphQL,
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

fn resolveHello(ctx: *Context, args: string): string {
    * = ctx;
    if (args.len > 0) {
        return "\"hello-with-args\"";
    }
    return "\"world\"";
}

fn resolveVersion(ctx: *Context, args: string): string {
    * = ctx;
    * = args;
    return "42";
}

fn resolveUser(ctx: *Context, args: string): string {
    * = ctx;
    * = args;
    return "{\"id\":1,\"name\":\"Alice\"}";
}

fn testQueryDepthGuard() {
    assertTrue(gqlCheckQueryDepth("{ hello }", 2), "depth 1 <= 2");
    assertTrue(gqlCheckQueryDepth("{ user { profile { name } } }", 3), "depth 3 <= 3");
    assertTrue(!gqlCheckQueryDepth("{ a { b { c { d } } } }", 3), "depth 4 > 3 rejected");
    // Braces inside string literals & comments must be ignored
    assertTrue(
        gqlCheckQueryDepth("{ echo(msg: \"{{{{{{{{}}\") }", 2),
        "braces in string literal ignored",
    );
    assertTrue(
        gqlCheckQueryDepth("# {{{{{{{{\n{ hello }", 2),
        "braces in comment ignored",
    );
    assertTrue(
        gqlCheckQueryDepth("{ echo(doc: \"\"\"{{{{{{}}\"\"\") }", 2),
        "braces in block string ignored",
    );
}

fn testGraphQLEndpoint() {
    gqlResetResolvers();
    assertTrue(gqlRegisterResolver("hello", &resolveHello), "register hello");
    assertTrue(gqlRegisterResolver("version", &resolveVersion), "register version");
    assertTrue(gqlRegisterResolver("user", &resolveUser), "register user");

    let app = App.init();
    const cfg = GraphQLConfig{
        endpoint_path: "/graphql",
        enable_playground: true,
        max_depth: 3,
    };
    * = registerGraphQL(*app, cfg);

    // 1. GET /graphql -> GraphiQL Playground HTML
    const resp_pg = app.dispatch(Request.init(AppMethod.GET, "/graphql"));
    assertTrue(resp_pg.status == 200, "playground status 200");
    assertTrue(contains(resp_pg.content_type, "text/html"), "playground content-type html");
    assertTrue(contains(resp_pg.body, "GraphiQL"), "playground contains GraphiQL");

    // 2. POST /graphql -> resolve registered fields
    let req1 = Request.init(AppMethod.POST, "/graphql");
    * = req1.withBody("{\"query\":\"{ hello version user { id name } }\"}");
    const resp1 = app.dispatch(req1);
    assertTrue(resp1.status == 200, "query status 200");
    assertTrue(contains(resp1.content_type, "application/json"), "query content-type json");
    assertTrue(contains(resp1.body, "\"hello\":\"world\""), "resolved hello");
    assertTrue(contains(resp1.body, "\"version\":42"), "resolved version");
    assertTrue(contains(resp1.body, "\"user\":{\"id\":1,\"name\":\"Alice\"}"), "resolved user");

    // 3. POST /graphql with arguments
    let req_args = Request.init(AppMethod.POST, "/graphql");
    * = req_args.withBody("{\"query\":\"query { greeting: hello(name: \\\"cp\\\") }\"}");
    const resp_args = app.dispatch(req_args);
    assertTrue(resp_args.status == 200, "aliased query with args 200");
    assertTrue(contains(resp_args.body, "\"greeting\":\"hello-with-args\""), "alias and args passed");

    // 4. POST /graphql exceeding max_depth -> 400 max_depth_exceeded
    let req_deep = Request.init(AppMethod.POST, "/graphql");
    * = req_deep.withBody("{\"query\":\"{ a { b { c { d } } } }\"}");
    const resp_deep = app.dispatch(req_deep);
    assertTrue(resp_deep.status == 400, "deep query returns 400");
    assertTrue(contains(resp_deep.body, "max_depth_exceeded"), "deep query returns max_depth_exceeded");

    // 5. POST /graphql missing query -> 400
    let req_bad = Request.init(AppMethod.POST, "/graphql");
    * = req_bad.withBody("{}");
    const resp_bad = app.dispatch(req_bad);
    assertTrue(resp_bad.status == 400, "missing query returns 400");
}

fn testPlaygroundDisabled() {
    let app = App.init();
    const cfg = GraphQLConfig{
        endpoint_path: "/gql",
        enable_playground: false,
        max_depth: 5,
    };
    * = registerGraphQL(*app, cfg);
    const resp = app.dispatch(Request.init(AppMethod.GET, "/gql"));
    assertTrue(resp.status == 404, "disabled playground returns 404");
}

pub fn main() {
    testQueryDepthGuard();
    testGraphQLEndpoint();
    testPlaygroundDisabled();
    std.debug.print("ALL @web/graphql TESTS PASSED!\n", .{});
}
