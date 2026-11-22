// Deep production regression tests for cap:router routeTypes.cp
// Covers: 32-bit packed RouteTinyPtr, Next.js bracket syntax validation (static, [id], [...slug], [[...opt]]),
// exact Bun diagnostic errors (CatchAllMustBeAtTheEnd, InvalidCatchAllRoute, InvalidOptionalCatchAllRoute,
// MissingParamName, PatternMissingClosingBracket, NonAsciiRoute), and zero-allocation parameter matching.

import { test, expect, expectEq } from "cap:test";
import {
    RoutePatternError,
    RoutePatternTag,
    RouteTinyPtr,
    RouteCapturedParams,
    RoutePatternEngine,
} from "../../lib/router/routeTypes";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("routeTypes: RouteTinyPtr 32-bit pack/unpack and backing slice extraction", fn () {
    const p = RouteTinyPtr{ offset: 6, len: 4 };
    const packed = p.packU32();
    const unpacked = RouteTinyPtr.unpackU32(packed);
    expect(unpacked.eql(p));
    expect(strEq(unpacked.sliceFrom("/blog/[slug]"), "[slu"));
    expect(strEq(RouteTinyPtr{ offset: 100, len: 5 }.sliceFrom("short"), ""));
});

test("routeTypes: validates static, dynamic, catch-all, and optional catch-all patterns", fn () {
    const v_static = RoutePatternEngine.validate("/settings/profile") catch unreachable;
    expectEq(v_static.kind, RoutePatternTag.Static);
    expectEq(v_static.param_count, @as(u16, 0));

    const v_dyn = RoutePatternEngine.validate("/users/[id]/posts/[postId]") catch unreachable;
    expectEq(v_dyn.kind, RoutePatternTag.Dynamic);
    expectEq(v_dyn.param_count, @as(u16, 2));

    const v_ca = RoutePatternEngine.validate("/docs/[...slug]") catch unreachable;
    expectEq(v_ca.kind, RoutePatternTag.CatchAll);
    expectEq(v_ca.param_count, @as(u16, 1));

    const v_opt = RoutePatternEngine.validate("/shop/[cat]/[[...filters]]") catch unreachable;
    expectEq(v_opt.kind, RoutePatternTag.OptionalCatchAll);
    expectEq(v_opt.param_count, @as(u16, 2));
});

test("routeTypes: matches static, dynamic, catch-all, and optional catch-all paths with params", fn () {
    let params = RouteCapturedParams.init();

    // Exact static match
    expect(RoutePatternEngine.match("/settings", "/settings", true, *params) catch unreachable);
    expectEq(params.slice().len, @as(usize, 0));
    expect(!(RoutePatternEngine.match("/settings", "/settings/extra", true, *params) catch unreachable));

    // Dynamic single segment
    expect(RoutePatternEngine.match("/blog/[slug]", "/blog/my-cool-post", true, *params) catch unreachable);
    expect(strEq(params.get("slug").?, "my-cool-post"));

    // Catch-all requires at least 1 segment
    expect(!(RoutePatternEngine.match("/docs/[...slug]", "/docs", true, *params) catch unreachable));
    expect(RoutePatternEngine.match("/docs/[...slug]", "/docs/api/v2/router", true, *params) catch unreachable);
    expect(strEq(params.get("slug").?, "api/v2/router"));

    // Optional catch-all matches both 0 segments and multi segments
    expect(RoutePatternEngine.match("/blog/[slug]/[[...rest]]", "/blog/intro", true, *params) catch unreachable);
    expect(strEq(params.get("slug").?, "intro"));
    expect(params.get("rest") == null);

    expect(RoutePatternEngine.match("/blog/[slug]/[[...rest]]", "/blog/intro/part1/part2", true, *params) catch unreachable);
    expect(strEq(params.get("slug").?, "intro"));
    expect(strEq(params.get("rest").?, "part1/part2"));

    // Disabling allow_optional_catch_all rejects optional catch-all
    expect(!(RoutePatternEngine.match("/[[...all]]", "/a/b", false, *params) catch unreachable));
});

test("routeTypes: rejects catch-all in middle, malformed brackets, empty param names, and non-ASCII", fn () {
    let err_mid = false;
    * = RoutePatternEngine.validate("/docs/[...slug]/edit") catch {
        err_mid = true;
    };
    expect(err_mid);

    let err_ca = false;
    * = RoutePatternEngine.validate("/docs/[..slug]") catch {
        err_ca = true;
    };
    expect(err_ca);

    let err_opt = false;
    * = RoutePatternEngine.validate("/docs/[[slug]]") catch {
        err_opt = true;
    };
    expect(err_opt);

    let err_missing = false;
    * = RoutePatternEngine.validate("/users/[]") catch {
        err_missing = true;
    };
    expect(err_missing);

    let err_unclosed = false;
    * = RoutePatternEngine.validate("/users/[id") catch {
        err_unclosed = true;
    };
    expect(err_unclosed);

    const non_ascii: [5]u8 = ['/', 0xe4, 0xb8, 0xad, 'a'];
    let err_ascii = false;
    * = RoutePatternEngine.validate(non_ascii[0..]) catch {
        err_ascii = true;
    };
    expect(err_ascii);
});

test("routeTypes: diagnosticMessage provides exact human-readable Bun diagnostics", fn () {
    expect(strEq(
        RoutePatternEngine.diagnosticMessage(RoutePatternError.CatchAllMustBeAtTheEnd),
        "Catch-all route must be at the end of the path",
    ));
    expect(strEq(
        RoutePatternEngine.diagnosticMessage(RoutePatternError.InvalidOptionalCatchAllRoute),
        "Invalid optional catch-all route, e.g. should be [[...param]]",
    ));
    expect(strEq(
        RoutePatternEngine.diagnosticMessage(RoutePatternError.NonAsciiRoute),
        "Route name must be plaintext",
    ));
});
