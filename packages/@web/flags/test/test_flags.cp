// Regression Test Suite for `@web/flags`
import { App, AppMethod, Request } from "../../../../lib/web/app.cp";
import { FlagsConfig, registerFlags, flagDefine, flagEval, flagBucket, flagReset } from "../src/index.cp";

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

fn main() {
    flagReset();
    * = flagDefine("new_checkout", true, 0, "vip_alice,vip_bob");
    * = flagDefine("ai_search", true, 100, "");
    * = flagDefine("disabled_exp", false, 100, "vip_alice");

    // 1. Allowlist & enabled checks
    if (!flagEval("new_checkout", "vip_alice") || flagEval("new_checkout", "normal_user")) {
        @panic("Test 1a failed: allowlist evaluation");
    }
    if (!flagEval("ai_search", "anyone") || flagEval("disabled_exp", "vip_alice")) {
        @panic("Test 1b failed: 100% / disabled evaluation");
    }

    // 2. Deterministic bucketing
    const b1 = flagBucket("beta_ui", "user_42");
    const b2 = flagBucket("beta_ui", "user_42");
    if (b1 != b2 || b1 >= 100) @panic("Test 2 failed: non-deterministic bucket");

    // 3. GET /_flags endpoint reflects per-user evaluation
    let app = App.init();
    * = registerFlags(*app, FlagsConfig{});
    let req = Request.init(AppMethod.GET, "/_flags");
    * = req.withHeader("X-User-Id", "vip_alice");
    const resp = app.dispatch(req);
    if (resp.status != 200 || !strContains(resp.body, "\"name\":\"new_checkout\",\"rollout\":0,\"active\":true")) {
        @panic("Test 3 failed: /_flags response mismatch");
    }

    @print("PASS @web/flags (3/3 tests)");
}
