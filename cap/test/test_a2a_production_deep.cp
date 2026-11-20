// Source: cp/cap/test/test_a2a_production_deep.cp (Production deep tests for cap:a2a / cap.a2a)
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    AgentSkill,
    AgentCapabilities,
    AgentCard,
    TaskState,
    MessageRole,
    Part,
    Message,
    Artifact,
    SseTaskEvent,
    A2aServer,
    encodeAgentCardJson,
    parseAgentCardSummary,
    encodeTaskEventSse,
    parseSseTaskEvent,
} from "cap:a2a";

test("a2a AgentCard JSON encoding with escaping and summary parsing") {
    const alloc = std.testing.allocator;
    const tags: [2][]const u8 = [ "code", "security" ];
    const skills: [2]AgentSkill = [
        AgentSkill{
            .id = "audit",
            .name = "Security \"Audit\"\nTool",
            .description = "Checks\tpaths\\rules",
            .tags = &tags,
        },
        AgentSkill{
            .id = "patch",
            .name = "Auto Patch",
            .description = "Generates fixes",
        },
    ];
    const card = AgentCard{
        .name = "Sentinel \"Alpha\"",
        .description = "Line1\nLine2\\end",
        .url = "https://agents.example.com/a2a",
        .version = "2.1.0",
        .capabilities = AgentCapabilities{ .streaming = true, .push_notifications = true },
        .skills = &skills,
    };

    const json = try encodeAgentCardJson(alloc, card);
    defer alloc.free(json);

    let summary = try parseAgentCardSummary(alloc, json);
    defer summary.deinit(alloc);

    expect_eq(std.mem.eql(u8, summary.name, "Sentinel \"Alpha\""), true);
    expect_eq(std.mem.eql(u8, summary.url, "https://agents.example.com/a2a"), true);
    expect_eq(std.mem.eql(u8, summary.version, "2.1.0"), true);
    expect_eq(summary.streaming, true);
    expect_eq(summary.push_notifications, true);
    expect_eq(summary.skill_count, @as(usize, 2));
}

test("a2a A2aServer full task lifecycle tasks/send -> InputRequired -> follow-up -> Completed") {
    const alloc = std.testing.allocator;
    const card = AgentCard{ .name = "Planner", .url = "https://planner.local/a2a" };
    let srv = A2aServer.init(alloc, card, .{});
    defer srv.deinit();

    const req1 = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tasks/send\",\"params\":{\"id\":\"task-101\",\"contextId\":\"ctx-9\",\"message\":{\"text\":\"Deploy service\"}}}";
    const resp1 = try srv.handleJsonRpc(req1);
    defer alloc.free(resp1);
    expect_eq(std.mem.indexOf(u8, resp1, "\"state\":\"working\"") != null, true);

    let ask_parts: [1]Part = [Part.textPart("Which region?")];
    try srv.transitionTask("task-101", .InputRequired, Message{
        .role = .Agent,
        .parts = &ask_parts,
    });
    const t1 = srv.getTask("task-101").?;
    expect_eq(t1.state == .InputRequired, true);
    expect_eq(t1.historyLen(), @as(usize, 2));

    const req2 = "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tasks/send\",\"params\":{\"id\":\"task-101\",\"message\":{\"text\":\"us-west-2\"}}}";
    const resp2 = try srv.handleJsonRpc(req2);
    defer alloc.free(resp2);
    expect_eq(std.mem.indexOf(u8, resp2, "\"state\":\"working\"") != null, true);

    let done_parts: [1]Part = [Part.textPart("Deployed to us-west-2")];
    try srv.transitionTask("task-101", .Completed, Message{
        .role = .Agent,
        .parts = &done_parts,
    });

    const req_get = "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"tasks/get\",\"params\":{\"id\":\"task-101\",\"historyLength\":2}}";
    const resp_get = try srv.handleJsonRpc(req_get);
    defer alloc.free(resp_get);
    expect_eq(std.mem.indexOf(u8, resp_get, "\"state\":\"completed\"") != null, true);
    expect_eq(std.mem.indexOf(u8, resp_get, "us-west-2") != null, true);
    expect_eq(std.mem.indexOf(u8, resp_get, "Deploy service") == null, true);
}

test("a2a illegal state transitions, terminal cancel rejection, and unknown task handling") {
    const alloc = std.testing.allocator;
    const card = AgentCard{ .name = "Worker", .url = "https://worker.local/a2a" };
    let srv = A2aServer.init(alloc, card, .{});
    defer srv.deinit();

    let p: [1]Part = [Part.textPart("run job")];
    const st = try srv.sendTask("task-term", "ctx-1", Message{ .role = .User, .parts = &p });
    expect_eq(st == .Working, true);
    try srv.transitionTask("task-term", .Completed, null);

    let err_caught = false;
    * = srv.transitionTask("task-term", .Working, null) catch |e| {
        if (e == error.InvalidStateTransition) {
            err_caught = true;
        }
    };
    expect_eq(err_caught, true);

    const cancel_term_resp = try srv.handleJsonRpc("{\"jsonrpc\":\"2.0\",\"id\":10,\"method\":\"tasks/cancel\",\"params\":{\"id\":\"task-term\"}}");
    defer alloc.free(cancel_term_resp);
    expect_eq(std.mem.indexOf(u8, cancel_term_resp, "-32002") != null, true);

    const unknown_resp = try srv.handleJsonRpc("{\"jsonrpc\":\"2.0\",\"id\":11,\"method\":\"tasks/get\",\"params\":{\"id\":\"missing-task\"}}");
    defer alloc.free(unknown_resp);
    expect_eq(std.mem.indexOf(u8, unknown_resp, "-32001") != null, true);
}

test("a2a SSE TaskStatusUpdateEvent and TaskArtifactUpdateEvent round-trip") {
    const alloc = std.testing.allocator;

    let status_parts: [1]Part = [Part.textPart("Step 1 \"done\"\nok")];
    const status_ev = SseTaskEvent{
        .kind = .StatusUpdate,
        .status = .{
            .task_id = "sse-t1",
            .context_id = "sse-ctx",
            .state = .Working,
            .message = Message{ .role = .Agent, .parts = &status_parts },
            .final = false,
        },
    };
    const status_sse = try encodeTaskEventSse(alloc, status_ev);
    defer alloc.free(status_sse);

    let parsed_status = try parseSseTaskEvent(alloc, status_sse);
    defer parsed_status.deinit(alloc);
    expect_eq(std.mem.eql(u8, parsed_status.task_id, "sse-t1"), true);
    expect_eq(parsed_status.state.? == .Working, true);
    expect_eq(parsed_status.final, false);
    expect_eq(std.mem.eql(u8, parsed_status.message_text.?, "Step 1 \"done\"\nok"), true);

    let art_parts: [1]Part = [Part.textPart("diff --git a/x b/x")];
    const art_ev = SseTaskEvent{
        .kind = .ArtifactUpdate,
        .artifact = .{
            .task_id = "sse-t1",
            .context_id = "sse-ctx",
            .artifact = Artifact{
                .artifact_id = "art-42",
                .name = "patch.diff",
                .parts = &art_parts,
                .index = 3,
                .append = true,
                .last_chunk = false,
            },
        },
    };
    const art_sse = try encodeTaskEventSse(alloc, art_ev);
    defer alloc.free(art_sse);

    let parsed_art = try parseSseTaskEvent(alloc, art_sse);
    defer parsed_art.deinit(alloc);
    expect_eq(std.mem.eql(u8, parsed_art.artifact_id.?, "art-42"), true);
    expect_eq(std.mem.eql(u8, parsed_art.artifact_name.?, "patch.diff"), true);
    expect_eq(std.mem.eql(u8, parsed_art.artifact_text.?, "diff --git a/x b/x"), true);
    expect_eq(parsed_art.index, @as(usize, 3));
    expect_eq(parsed_art.append, true);
    expect_eq(parsed_art.last_chunk, false);
}

test("a2a cap.a2a dynamic namespace mount and zero-leak allocator verification") {
    const alloc = std.testing.allocator;
    const card = cap.a2a.AgentCard{
        .name = "DynamicAgent",
        .url = "https://dyn.local/a2a",
    };
    let srv = cap.a2a.A2aServer.init(alloc, card, .{ .max_tasks = 2 });
    defer srv.deinit();

    let p: [1]cap.a2a.Part = [cap.a2a.Part.textPart("ping")];
    const st1 = try srv.sendTask("t-1", "", cap.a2a.Message{ .role = .User, .parts = &p });
    expect_eq(st1 == .Working, true);
    const canceled = try srv.cancelTask("t-1");
    expect_eq(canceled == .Canceled, true);
    expect_eq(srv.taskCount(), @as(usize, 1));
}
