// Self-contained unit & integration tests for @web/a2a

const std = @import("std");
import { App, AppMethod, Context, Request } from "../../../../lib/web/app.cp";
import {
    A2aConfig,
    A2aTaskState,
    a2aSetTaskHandler,
    a2aResetTasks,
    registerA2a,
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

fn demoAgentHandler(ctx: *Context, payload: string): string {
    * = ctx;
    if (contains(payload, "summarize")) {
        return "Summary: CP Agent completed task.";
    }
    return "Ack";
}

fn testA2aProtocolLifecycle() {
    a2aResetTasks();
    a2aSetTaskHandler(&demoAgentHandler);

    const st = A2aTaskState.Completed;
    assertTrue(contains(st.asString(), "completed"), "A2aTaskState.asString works");

    let app = App.init();
    const cfg = A2aConfig{
        agent_name: "cp-research-agent",
        agent_description: "Ultra-fast CP A2A Agent",
        agent_url: "https://agent.cp-lang.dev/a2a",
        card_path: "/.well-known/agent.json",
        rpc_path: "/a2a",
        version: "1.2.0",
    };
    * = registerA2a(*app, cfg);

    // 1. GET /.well-known/agent.json -> Agent Card
    const req_card = Request.init(AppMethod.GET, "/.well-known/agent.json");
    const resp_card = app.dispatch(req_card);
    assertTrue(resp_card.status == 200, "agent card status 200");
    assertTrue(contains(resp_card.body, "\"name\":\"cp-research-agent\""), "card has agent_name");
    assertTrue(contains(resp_card.body, "\"description\":\"Ultra-fast CP A2A Agent\""), "card has description");
    assertTrue(contains(resp_card.body, "\"url\":\"https://agent.cp-lang.dev/a2a\""), "card has url");
    assertTrue(contains(resp_card.body, "\"version\":\"1.2.0\""), "card has version");
    assertTrue(contains(resp_card.body, "\"capabilities\":{"), "card has capabilities");

    // 2. POST /a2a tasks/send
    let req_send = Request.init(AppMethod.POST, "/a2a");
    * = req_send.withBody("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tasks/send\",\"params\":{\"id\":\"t-101\",\"message\":{\"role\":\"user\",\"parts\":[{\"type\":\"text\",\"text\":\"summarize\"}]}}}");
    const resp_send = app.dispatch(req_send);
    assertTrue(resp_send.status == 200, "tasks/send status 200");
    assertTrue(contains(resp_send.body, "\"id\":1"), "tasks/send preserves jsonrpc id");
    assertTrue(contains(resp_send.body, "\"id\":\"t-101\""), "tasks/send returns task id");
    assertTrue(contains(resp_send.body, "\"state\":\"completed\""), "tasks/send sets state completed");
    assertTrue(contains(resp_send.body, "Summary: CP Agent completed task."), "tasks/send returns handler output");

    // 3. POST /a2a tasks/get
    let req_get = Request.init(AppMethod.POST, "/a2a");
    * = req_get.withBody("{\"jsonrpc\":\"2.0\",\"id\":\"g-2\",\"method\":\"tasks/get\",\"params\":{\"id\":\"t-101\"}}");
    const resp_get = app.dispatch(req_get);
    assertTrue(resp_get.status == 200, "tasks/get status 200");
    assertTrue(contains(resp_get.body, "\"id\":\"g-2\""), "tasks/get preserves string id");
    assertTrue(contains(resp_get.body, "\"state\":\"completed\""), "tasks/get returns completed state");
    assertTrue(contains(resp_get.body, "Summary: CP Agent completed task."), "tasks/get returns stored result");

    // 4. POST /a2a tasks/cancel
    let req_cancel = Request.init(AppMethod.POST, "/a2a");
    * = req_cancel.withBody("{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"tasks/cancel\",\"params\":{\"id\":\"t-101\"}}");
    const resp_cancel = app.dispatch(req_cancel);
    assertTrue(resp_cancel.status == 200, "tasks/cancel status 200");
    assertTrue(contains(resp_cancel.body, "\"state\":\"canceled\""), "tasks/cancel transitions to canceled");

    // Verify tasks/get reflects canceled state
    let req_get2 = Request.init(AppMethod.POST, "/a2a");
    * = req_get2.withBody("{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"tasks/get\",\"params\":{\"id\":\"t-101\"}}");
    const resp_get2 = app.dispatch(req_get2);
    assertTrue(contains(resp_get2.body, "\"state\":\"canceled\""), "tasks/get reflects canceled state");

    // 5. Unknown method -> -32601
    let req_unk = Request.init(AppMethod.POST, "/a2a");
    * = req_unk.withBody("{\"jsonrpc\":\"2.0\",\"id\":5,\"method\":\"tasks/unknown\",\"params\":{}}");
    const resp_unk = app.dispatch(req_unk);
    assertTrue(resp_unk.status == 200, "unknown method returns 200 jsonrpc");
    assertTrue(contains(resp_unk.body, "\"code\":-32601"), "unknown method returns -32601");
}

pub fn main() {
    testA2aProtocolLifecycle();
    std.debug.print("ALL @web/a2a TESTS PASSED!\n", .{});
}
