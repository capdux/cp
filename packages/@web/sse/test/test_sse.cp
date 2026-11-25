// Regression Test Suite for `@web/sse`
import { App, AppMethod, Request } from "../../../../lib/web/app.cp";
import { SseConfig, registerSse, ssePublish, sseResetBroker } from "../src/index.cp";

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
    sseResetBroker();
    let app = App.init();
    * = registerSse(*app, SseConfig{ endpoint_path: "/events", retry_ms: 2500 });

    // 1. Empty broker emits heartbeat `: ping`
    const r1 = app.dispatch(Request.init(AppMethod.GET, "/events"));
    if (r1.status != 200 || !strContains(r1.content_type, "text/event-stream") || !strContains(r1.body, ": ping")) {
        @panic("Test 1 failed: expected heartbeat SSE stream");
    }

    // 2. Publish 2 events and receive both
    const id1 = ssePublish("token", "{\"delta\":\"Hello\"}");
    const id2 = ssePublish("token", "{\"delta\":\" CP!\"}");
    if (id1 != 1 || id2 != 2) @panic("Test 2a failed: monotonic IDs");

    const r2 = app.dispatch(Request.init(AppMethod.GET, "/events"));
    if (!strContains(r2.body, "id: 1\nevent: token\ndata: {\"delta\":\"Hello\"}\n\n")
        || !strContains(r2.body, "id: 2\nevent: token\ndata: {\"delta\":\" CP!\"}\n\n"))
    {
        @panic("Test 2b failed: full SSE stream mismatch");
    }

    // 3. Last-Event-ID: 1 replays ONLY event 2
    let req3 = Request.init(AppMethod.GET, "/events");
    * = req3.withHeader("Last-Event-ID", "1");
    const r3 = app.dispatch(req3);
    if (strContains(r3.body, "id: 1\n") || !strContains(r3.body, "id: 2\n")) {
        @panic("Test 3 failed: Last-Event-ID replay mismatch");
    }

    @print("PASS @web/sse (3/3 tests)");
}
