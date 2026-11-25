// Regression Test Suite for `@web/env`
import { App, AppMethod, Context, Request, Response } from "../../../../lib/web/app.cp";
import { EnvKind, EnvVarSpec, EnvKV, registerEnv } from "../src/index.cp";

fn strEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn hInfo(ctx: *Context): Response {
    const mode = ctx.stateVal("APP_ENV") ?? "";
    const port = ctx.stateVal("PORT") ?? "";
    if (strEql(mode, "prod") && strEql(port, "8080")) {
        return Response.json(200, "{\"ok\":true}");
    }
    return Response.json(500, "{\"ok\":false}");
}

fn main() {
    const specs: [4]EnvVarSpec = [
        EnvVarSpec{ name: "APP_ENV", kind: EnvKind.Enum, required: true, enum_values: "dev,staging,prod" },
        EnvVarSpec{ name: "PORT", kind: EnvKind.Int, default_val: "8080", min_val: 1, max_val: 65535, has_bounds: true },
        EnvVarSpec{ name: "DEBUG", kind: EnvKind.Bool, default_val: "false" },
        EnvVarSpec{ name: "API_SECRET", kind: EnvKind.Str, required: true },
    ];

    // 1. Valid config with defaults injected into App state
    let app = App.init();
    const ok_entries: [2]EnvKV = [
        EnvKV{ key: "APP_ENV", value: "prod" },
        EnvKV{ key: "API_SECRET", value: "s3cr3t" },
    ];
    const r1 = registerEnv(*app, specs[0..], ok_entries[0..]);
    if (!r1.ok || r1.loaded_count != 4) @panic("Test 1a failed: valid env rejected");
    * = app.get("/info", hInfo);
    if (app.dispatch(Request.init(AppMethod.GET, "/info")).status != 200) {
        @panic("Test 1b failed: stateVal not injected");
    }

    // 2. Missing required var fails
    let app2 = App.init();
    const miss_entries: [1]EnvKV = [EnvKV{ key: "APP_ENV", value: "dev" }];
    const r2 = registerEnv(*app2, specs[0..], miss_entries[0..]);
    if (r2.ok || r2.missing_count != 1 || !strEql(r2.first_error_key, "API_SECRET")) {
        @panic("Test 2 failed: missing required var not caught");
    }

    // 3. Invalid enum / out-of-range port fails
    let app3 = App.init();
    const bad_entries: [3]EnvKV = [
        EnvKV{ key: "APP_ENV", value: "unknown" },
        EnvKV{ key: "PORT", value: "99999" },
        EnvKV{ key: "API_SECRET", value: "ok" },
    ];
    const r3 = registerEnv(*app3, specs[0..], bad_entries[0..]);
    if (r3.ok || r3.invalid_count != 2) {
        @panic("Test 3 failed: invalid enum/port not caught");
    }

    @print("PASS @web/env (3/3 tests)");
}
