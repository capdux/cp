// Source: cp/cap/test/test_har_production_deep.cp (Production deep tests for cap:har)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    HarHeader,
    HarTimings,
    HarEntry,
    HarStats,
    HarEntryIter,
    isHar,
    parseHarHeader,
    summarizeHar,
} from "cap:binfmt";

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const diff = if (a > b) (a - b) else (b - a);
    return diff < eps;
}

const SAMPLE_HAR_12: []const u8 =
    "{\n" ++
    "  \"log\": {\n" ++
    "    \"version\": \"1.2\",\n" ++
    "    \"creator\": { \"name\": \"WebInspector\", \"version\": \"537.36\" },\n" ++
    "    \"browser\": { \"name\": \"Chrome\", \"version\": \"124.0.0.0\" },\n" ++
    "    \"entries\": [\n" ++
    "      {\n" ++
    "        \"startedDateTime\": \"2026-11-21T10:00:00.000Z\",\n" ++
    "        \"time\": 120.5,\n" ++
    "        \"serverIPAddress\": \"93.184.216.34\",\n" ++
    "        \"request\": {\n" ++
    "          \"method\": \"GET\",\n" ++
    "          \"url\": \"https://example.com/api/v1/items\",\n" ++
    "          \"httpVersion\": \"http/2.0\",\n" ++
    "          \"headers\": [{ \"name\": \"User-Agent\", \"value\": \"\\\"Chrome\\\" {fake: \\\"status\\\": 999}\" }],\n" ++
    "          \"bodySize\": 0\n" ++
    "        },\n" ++
    "        \"response\": {\n" ++
    "          \"status\": 200,\n" ++
    "          \"statusText\": \"OK\",\n" ++
    "          \"content\": {\n" ++
    "            \"size\": 4096,\n" ++
    "            \"mimeType\": \"application/json\",\n" ++
    "            \"text\": \"{\\\"status\\\": 500, \\\"method\\\": \\\"POST\\\", \\\"escaped\\\": \\\"\\\\\\\"}\\\"}\"\n" ++
    "          },\n" ++
    "          \"bodySize\": 1024\n" ++
    "        },\n" ++
    "        \"timings\": {\n" ++
    "          \"blocked\": 2.5,\n" ++
    "          \"dns\": 10.0,\n" ++
    "          \"connect\": 15.0,\n" ++
    "          \"ssl\": 12.0,\n" ++
    "          \"send\": 1.0,\n" ++
    "          \"wait\": 60.0,\n" ++
    "          \"receive\": 20.0\n" ++
    "        }\n" ++
    "      },\n" ++
    "      {\n" ++
    "        \"startedDateTime\": \"2026-11-21T10:00:01.000Z\",\n" ++
    "        \"time\": 45.0,\n" ++
    "        \"request\": { \"method\": \"GET\", \"url\": \"https://example.com/old\", \"httpVersion\": \"HTTP/1.1\", \"bodySize\": 0 },\n" ++
    "        \"response\": { \"status\": 301, \"statusText\": \"Moved Permanently\", \"content\": { \"size\": 0, \"mimeType\": \"text/html\" }, \"bodySize\": 128 },\n" ++
    "        \"timings\": { \"blocked\": -1, \"dns\": -1, \"connect\": -1, \"ssl\": -1, \"send\": 0.5, \"wait\": 30.0, \"receive\": 14.5 }\n" ++
    "      },\n" ++
    "      {\n" ++
    "        \"startedDateTime\": \"2026-11-21T10:00:02.000Z\",\n" ++
    "        \"time\": 80.0,\n" ++
    "        \"request\": { \"method\": \"POST\", \"url\": \"https://example.com/missing\", \"httpVersion\": \"http/2.0\", \"bodySize\": 64 },\n" ++
    "        \"response\": { \"status\": 404, \"statusText\": \"Not Found\", \"content\": { \"size\": 256, \"mimeType\": \"text/plain\" }, \"bodySize\": -1 },\n" ++
    "        \"timings\": { \"wait\": 40.0, \"receive\": 5.0 }\n" ++
    "      },\n" ++
    "      {\n" ++
    "        \"startedDateTime\": \"2026-11-21T10:00:03.000Z\",\n" ++
    "        \"time\": 254.5,\n" ++
    "        \"request\": { \"method\": \"DELETE\", \"url\": \"https://example.com/crash\", \"httpVersion\": \"http/2.0\", \"bodySize\": 0 },\n" ++
    "        \"response\": { \"status\": 500, \"statusText\": \"Internal Server Error\", \"content\": { \"size\": 512, \"mimeType\": \"application/json\" }, \"bodySize\": 512 },\n" ++
    "        \"timings\": { \"wait\": 230.0, \"receive\": 10.0 }\n" ++
    "      }\n" ++
    "    ]\n" ++
    "  }\n" ++
    "}\n";

test("1. Chrome DevTools HAR 1.2 header & multi-status (200/301/404/500) entry parsing", () => {
    expect_eq(isHar(SAMPLE_HAR_12), true);

    const hdr = parseHarHeader(SAMPLE_HAR_12) ?? unreachable;
    expect_eq(std.mem.eql(u8, hdr.version, "1.2"), true);
    expect_eq(std.mem.eql(u8, hdr.creator_name, "WebInspector"), true);
    expect_eq(std.mem.eql(u8, hdr.creator_version, "537.36"), true);
    expect_eq(std.mem.eql(u8, hdr.browser_name, "Chrome"), true);
    expect_eq(std.mem.eql(u8, hdr.browser_version, "124.0.0.0"), true);
    expect_eq(hdr.entries_slice.len > 0, true);

    let it = HarEntryIter.init(hdr.entries_slice);

    const e1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.started_date_time, "2026-11-21T10:00:00.000Z"), true);
    expect_eq(approxEq(e1.time_ms, 120.5, 0.001), true);
    expect_eq(std.mem.eql(u8, e1.server_ip, "93.184.216.34"), true);
    expect_eq(std.mem.eql(u8, e1.method, "GET"), true);
    expect_eq(std.mem.eql(u8, e1.url, "https://example.com/api/v1/items"), true);
    expect_eq(std.mem.eql(u8, e1.http_version, "http/2.0"), true);
    expect_eq(e1.req_body_size, 0);
    expect_eq(e1.status, 200);
    expect_eq(std.mem.eql(u8, e1.status_text, "OK"), true);
    expect_eq(std.mem.eql(u8, e1.mime_type, "application/json"), true);
    expect_eq(e1.resp_body_size, 1024);
    expect_eq(e1.content_size, 4096);
    expect_eq(e1.isSuccess(), true);
    expect_eq(e1.isError(), false);

    const e2 = it.next() ?? unreachable;
    expect_eq(e2.status, 301);
    expect_eq(e2.isSuccess(), false);
    expect_eq(e2.isError(), false);

    const e3 = it.next() ?? unreachable;
    expect_eq(e3.status, 404);
    expect_eq(e3.req_body_size, 64);
    expect_eq(e3.isError(), true);

    const e4 = it.next() ?? unreachable;
    expect_eq(e4.status, 500);
    expect_eq(std.mem.eql(u8, e4.method, "DELETE"), true);
    expect_eq(e4.isError(), true);

    expect_eq(it.next() == null, true);
    expect_eq(it.malformed, false);
});

test("2. Nested headers/content.text with escaped quotes and fake keys are safely skipped", () => {
    // 验证第一条记录中 headers 和 content.text 内伪造的 "status": 500 / "method": "POST" 不会干扰真实字段
    let it = HarEntryIter.init(SAMPLE_HAR_12);
    const e1 = it.next() ?? unreachable;
    expect_eq(e1.status, 200);
    expect_eq(std.mem.eql(u8, e1.method, "GET"), true);
    expect_eq(std.mem.eql(u8, e1.mime_type, "application/json"), true);
    expect_eq(e1.resp_body_size, 1024);
});

test("3. HarTimings fields and summarizeHar performance metrics (avg_ttfb_ms, max_time_ms, transfer bytes)", () => {
    let it = HarEntryIter.init(SAMPLE_HAR_12);
    const e1 = it.next() ?? unreachable;
    expect_eq(approxEq(e1.timings.blocked_ms, 2.5, 0.001), true);
    expect_eq(approxEq(e1.timings.dns_ms, 10.0, 0.001), true);
    expect_eq(approxEq(e1.timings.connect_ms, 15.0, 0.001), true);
    expect_eq(approxEq(e1.timings.ssl_ms, 12.0, 0.001), true);
    expect_eq(approxEq(e1.timings.send_ms, 1.0, 0.001), true);
    expect_eq(approxEq(e1.timings.wait_ms, 60.0, 0.001), true);
    expect_eq(approxEq(e1.timings.receive_ms, 20.0, 0.001), true);

    const e2 = it.next() ?? unreachable;
    expect_eq(approxEq(e2.timings.dns_ms, -1.0, 0.001), true);
    expect_eq(approxEq(e2.timings.wait_ms, 30.0, 0.001), true);

    const stats = summarizeHar(SAMPLE_HAR_12);
    expect_eq(stats.entry_count, 4);
    expect_eq(stats.success_count, 1);
    expect_eq(stats.redirect_count, 1);
    expect_eq(stats.client_error_count, 1);
    expect_eq(stats.server_error_count, 1);
    // transfer: 1024 + 128 + 256 (fallback to content_size when bodySize==-1) + 512 = 1920
    expect_eq(stats.total_transfer_bytes, 1920);
    // total_time: 120.5 + 45.0 + 80.0 + 254.5 = 500.0
    expect_eq(approxEq(stats.total_time_ms, 500.0, 0.001), true);
    expect_eq(approxEq(stats.max_time_ms, 254.5, 0.001), true);
    // avg_ttfb: (60 + 30 + 40 + 230) / 4 = 90.0
    expect_eq(approxEq(stats.avg_ttfb_ms, 90.0, 0.001), true);
});

test("4. Optional browser/timings/serverIPAddress missing defaults gracefully", () => {
    const minimal_har =
        "{\"log\":{\"version\":\"1.2\",\"creator\":{\"name\":\"curl\",\"version\":\"8.0\"}," ++
        "\"entries\":[{\"startedDateTime\":\"2026-01-01T00:00:00Z\",\"time\":12.0," ++
        "\"request\":{\"method\":\"HEAD\",\"url\":\"https://cp-lang.org/\"}," ++
        "\"response\":{\"status\":0,\"statusText\":\"\"}}]}}";

    const hdr = parseHarHeader(minimal_har) ?? unreachable;
    expect_eq(hdr.browser_name.len, 0);
    expect_eq(hdr.browser_version.len, 0);

    let it = HarEntryIter.init(hdr.entries_slice);
    const e = it.next() ?? unreachable;
    expect_eq(e.status, 0);
    expect_eq(e.isError(), true);
    expect_eq(e.isSuccess(), false);
    expect_eq(e.server_ip.len, 0);
    expect_eq(approxEq(e.timings.wait_ms, -1.0, 0.001), true);
    expect_eq(approxEq(e.timings.dns_ms, -1.0, 0.001), true);

    const stats = summarizeHar(minimal_har);
    expect_eq(stats.entry_count, 1);
    expect_eq(approxEq(stats.avg_ttfb_ms, 0.0, 0.001), true);
});

test("5. Resilience against non-HAR JSON, empty input, and truncated entries array", () => {
    expect_eq(isHar(""), false);
    expect_eq(isHar("{\"hello\":\"world\"}"), false);
    expect_eq(isHar("{\"log\":{\"foo\":123}}"), false);
    expect_eq(parseHarHeader("not json at all") == null, true);

    const empty_stats = summarizeHar("{\"not\":\"har\"}");
    expect_eq(empty_stats.entry_count, 0);

    // 截断的 entries 数组安全终止并置 malformed = true
    const truncated_entries = "[{\"time\":10,\"request\":{\"method\":\"GET\"";
    let it = HarEntryIter.init(truncated_entries);
    expect_eq(it.next() == null, true);
    expect_eq(it.malformed, true);
});
