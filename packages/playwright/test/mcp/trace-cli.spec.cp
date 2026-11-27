// Migrated from TypeScript (mcp/trace-cli.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import fs from "cap:fs";
import path from "cap:path";

import { test, expect } from "./trace-cli-fixtures.cp";
























fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => mcpBrowser != "chrome", "Chrome-only");
    // [ts] test("trace open shows metadata", () => {
    // [ts] const { stdout, exitCode } = runTraceCli(["open", traceFile]);
    // [ts] expect_eq(exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "Browser:") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "chromium") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "Viewport:") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "800x600") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "Actions:") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "Pages:") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "Network:") != null, true);
    // [ts] });
    // [ts] test("trace actions shows actions with ordinals", () => {
    // [ts] const { stdout, exitCode } = runTraceCli(["actions"]);
    // [ts] expect_eq(exitCode, 0);
    // Should have ordinal numbers
    // [ts] expect(stdout).toMatch("^\\s+\\d+\\.\\s");
    // Should have formatted action titles
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "Navigate") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "Click") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "Fill") != null, true);
    // [ts] });
    // [ts] test("trace actions --grep filters actions", () => {
    // [ts] const { stdout, exitCode } = runTraceCli(["actions", "--grep", "Click"]);
    // [ts] expect_eq(exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "Click") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "Navigate") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "Fill") == null, true);
    // [ts] });
    // [ts] test("trace action displays action details", () => {
    // First get an action ordinal from list
    // [ts] const { stdout: listOutput } = runTraceCli(["actions", "--grep", "Navigate"]);
    // [ts] const match_val = listOutput.match("^\\s+(\\d+)\\.\\s");
    // [ts] expect_eq(match_val, true);
    // [ts] const { stdout, exitCode } = runTraceCli(["action", match_val[1]]);
    // [ts] expect_eq(exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "Navigate") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "Time") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "start:") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "duration:") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "Parameters") != null, true);
    // [ts] });
    // [ts] test("trace action reports available snapshots", () => {
    // [ts] const { stdout: listOutput } = runTraceCli(["actions", "--grep", "Click"]);
    // [ts] const match_val = listOutput.match("^\\s+(\\d+)\\.\\s");
    // [ts] expect_eq(match_val, true);
    // [ts] const { stdout, exitCode } = runTraceCli(["action", match_val[1]]);
    // [ts] expect_eq(exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "Snapshots") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "available:") != null, true);
    // [ts] });
    // [ts] test("trace action with invalid action ID", () => {
    // [ts] const { stderr, exitCode } = runTraceCli(["action", "999999"]);
    // [ts] expect_eq(exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, stderr, "not found") != null, true);
    // [ts] });
    // [ts] test("trace requests shows requests with ordinals", () => {
    // [ts] const { stdout, exitCode } = runTraceCli(["requests"]);
    // [ts] expect_eq(exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "Method") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "Status") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "GET") != null, true);
    // [ts] expect(stdout).toMatch("\\d+\\.");
    // [ts] });
    // [ts] test("trace requests shows start times and aborted request durations", () => {
    // [ts] const { stdout, exitCode } = runTraceCli(["requests"]);
    // [ts] expect_eq(exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "Start") != null, true);
    // Every request row carries a start timestamp in the `trace actions` Time format.
    // [ts] expect(stdout).toMatch("\\d+\\.\\s+\\d+:\\d{2}\\.\\d{3}\\s");
    // The aborted request has a recorded duration rather than '-'.
    // [ts] const abortedRow = stdout.split("\n").find(line => line.includes("aborted"));
    // [ts] expect_eq(std.mem.indexOf(u8, abortedRow, "blocked") != null, true);
    // [ts] expect(abortedRow).toMatch("\\s\\d+(\\.\\d+)?m?s\\s");
    // [ts] });
    // [ts] test("trace requests --method filters", () => {
    // [ts] const { stdout, exitCode } = runTraceCli(["requests", "--method", "GET"]);
    // [ts] expect_eq(exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "GET") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "POST") == null, true);
    // [ts] });
    // [ts] test("trace request shows details", () => {
    // [ts] const { stdout, exitCode } = runTraceCli(["request", "1"]);
    // [ts] expect_eq(exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "General") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "status:") != null, true);
    // [ts] expect(stdout).toMatch("start:\\s+\\d+:\\d{2}\\.\\d{3}");
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "Request headers") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "Response headers") != null, true);
    // [ts] });
    // [ts] test("trace request shows request and response body", () => {
    // [ts] const { stdout, exitCode } = runTraceCli(["requests", "--grep", "feedback"]);
    // [ts] expect_eq(exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "feedback") != null, true);
    // [ts] const ordinal = stdout.match("^\\s+(\\d+)\\.\\s")[1];
    // [ts] const { stdout: requestOutput, exitCode: requestExitCode } = runTraceCli(["request", ordinal]);
    // [ts] expect_eq(requestExitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, requestOutput, "Request body") != null, true);
    // [ts] const requestBodyPath = path.join(".playwright-cli", "trace", "resources", "60e4c013c41de11280ed8ba99dd94144124a0c35.txt");
    // [ts] expect_eq(std.mem.indexOf(u8, requestOutput, " " + requestBodyPath) != null, true);
    // [ts] expect_eq(fs.readFileSync(path.join(traceCwd, requestBodyPath), "utf-8"), "What a great product!");
    // [ts] expect_eq(std.mem.indexOf(u8, requestOutput, "Response body") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, requestOutput, "application/json") != null, true);
    // [ts] const responseBodyPath = path.join(".playwright-cli", "trace", "resources", "030b61c2fb7042fb4fc0ef4287a4ae3c0d98ed68.json");
    // [ts] expect_eq(std.mem.indexOf(u8, requestOutput, " " + responseBodyPath) != null, true);
    // [ts] expect_eq(fs.readFileSync(path.join(traceCwd, responseBodyPath), "utf-8"), JSON.stringify({ received: true }));
    // [ts] });
    // [ts] test("trace request with invalid ID", () => {
    // [ts] const { stderr, exitCode } = runTraceCli(["request", "999999"]);
    // [ts] expect_eq(exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, stderr, "not found") != null, true);
    // [ts] });
    // [ts] test("trace console shows messages", () => {
    // [ts] const { stdout, exitCode } = runTraceCli(["console"]);
    // [ts] expect_eq(exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "info message") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "warning message") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "error message") != null, true);
    // [ts] });
    // [ts] test("trace console --errors-only", () => {
    // [ts] const { stdout, exitCode } = runTraceCli(["console", "--errors-only"]);
    // [ts] expect_eq(exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "error message") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "info message") == null, true);
    // [ts] });
    // [ts] test("trace console --grep filters by message text", () => {
    // [ts] const { stdout, exitCode } = runTraceCli(["console", "--grep", "warning"]);
    // [ts] expect_eq(exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "warning message") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "info message") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "error message") == null, true);
    // [ts] });
    // [ts] test("trace errors", () => {
    // [ts] const { stdout, exitCode } = runTraceCli(["errors"]);
    // [ts] expect_eq(exitCode, 0);
    // Our test trace may or may not have errors, just verify it doesn't crash
    // [ts] expect_eq(stdout, true);
    // [ts] });
    // [ts] test("trace snapshot runs command on snapshot", () => {
    // [ts] const { stdout: listOutput } = runTraceCli(["actions", "--grep", "Navigate"]);
    // [ts] const match_val = listOutput.match("^\\s+(\\d+)\\.\\s");
    // [ts] expect_eq(match_val, true);
    // [ts] const { stdout, exitCode } = runTraceCli(["snapshot", match_val[1]]);
    // [ts] expect_eq(exitCode, 0);
    // [ts] expect_eq(stdout, true);
    // [ts] });
    // [ts] test("trace snapshot --phase before", () => {
    // [ts] const { stdout: listOutput } = runTraceCli(["actions", "--grep", "Click"]);
    // [ts] const match_val = listOutput.match("^\\s+(\\d+)\\.\\s");
    // [ts] expect_eq(match_val, true);
    // [ts] const { stdout, exitCode } = runTraceCli(["snapshot", "--phase", "before", match_val[1]]);
    // [ts] expect_eq(exitCode, 0);
    // [ts] expect_eq(stdout, true);
    // [ts] });
    // [ts] test("trace snapshot resolves inner frames", () => {
    // [ts] const { stdout: listOutput } = runTraceCli(["actions", "--grep", "Click"]);
    // [ts] const ordinals = [...listOutput.matchAll("^\\s+(\\d+)\\.\\s")].map(m => m[1]);
    // [ts] expect_eq(ordinals.length >= 2, true);
    // [ts] const anchorClickOrdinal = ordinals[ordinals.length - 1];
    // [ts] const { stdout } = runTraceCli(["snapshot", "--phase", "after", anchorClickOrdinal]);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "Innermost") != null, true);
    // [ts] });
    // [ts] test("trace snapshot replays sub-resource stylesheets from the archive", () => {
    // [ts] const { stdout: listOutput } = runTraceCli(["actions", "--grep", "Click"]);
    // [ts] const match_val = listOutput.match("^\\s+(\\d+)\\.\\s");
    // [ts] expect_eq(match_val, true);
    // [ts] const ordinal = match_val[1];
    // [ts] const { stdout, exitCode } = runTraceCli([
    // [ts] "snapshot", "--phase", "before", ordinal,
    // [ts] "--", "eval", "el => getComputedStyle(el).color", "#styled",
    // [ts] ]);
    // [ts] expect_eq(exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "rgb(255, 0, 0)") != null, true);
    // [ts] });
    // [ts] test("trace screenshot saves image file", () => {
    // [ts] const { stdout: listOutput } = runTraceCli(["actions", "--grep", "Navigate"]);
    // [ts] const match_val = listOutput.match("^\\s+(\\d+)\\.\\s");
    // [ts] expect_eq(match_val, true);
    // [ts] const outPath = testInfo.outputPath("test-screenshot.png");
    // [ts] const { stdout, exitCode } = runTraceCli(["screenshot", match_val[1], "-o", outPath]);
    // Screenshot may or may not be available depending on timing
    // [ts] if (exitCode == 0) {
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "Screenshot saved to") != null, true);
    // [ts] expect_eq(fs.existsSync(outPath), true);
    // [ts] }
    // [ts] });
    // [ts] test("trace close removes extracted trace", () => {
    // [ts] const { exitCode } = runTraceCli(["close"]);
    // [ts] expect_eq(exitCode, 0);
    // Subsequent commands should fail because trace is closed.
    // [ts] const { stderr, exitCode: exitCode2 } = runTraceCli(["actions"]);
    // [ts] expect_eq(exitCode2, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, stderr, "No trace opened") != null, true);
    // Re-open for remaining tests.
    // [ts] const { exitCode: exitCode3 } = runTraceCli(["open", traceFile]);
    // [ts] expect_eq(exitCode3, 0);
    // [ts] });
    // [ts] test("trace open with .trace file creates link", () => {
    // Extract the zip using the CLI to get raw trace files.
    // [ts] const extractCwd = path.join(path.dirname(traceFile), "extracted");
    // [ts] fs.mkdirSync(extractCwd, { recursive: true });
    // [ts] const cliPath = path.resolve(__dirname, "../../packages/playwright-core/cli.js");
    // [ts] const { execFileSync } = require("child_process");
    // [ts] execFileSync(process.execPath, [cliPath, "trace", "open", traceFile], { cwd: extractCwd });
    // [ts] const extractDir = path.join(extractCwd, ".playwright-cli", "trace");
    // [ts] const traceEntries = fs.readdirSync(extractDir).filter((f) => f.endsWith(".trace"));
    // [ts] expect_eq(traceEntries.length > 0, true);
    // [ts] const dotTraceFile = path.join(extractDir, traceEntries[0]);
    // [ts] const { exitCode } = runTraceCli(["open", dotTraceFile]);
    // [ts] expect_eq(exitCode, 0);
    // [ts] const linkFile = path.join(traceCwd, ".playwright-cli", "trace", ".link");
    // [ts] expect_eq(fs.existsSync(linkFile), true);
    // [ts] expect_eq(fs.readFileSync(linkFile, "utf-8"), dotTraceFile);
    // [ts] const { stdout: actionsOutput, exitCode: actionsExitCode } = runTraceCli(["actions"]);
    // [ts] expect_eq(actionsExitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, actionsOutput, "Navigate") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, actionsOutput, "Click") != null, true);
    // [ts] });
    // [ts] test("trace attachments lists and extracts attachments", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44174" } }, (_p, testInfo) => {
    // [ts] const inlineTrace = testInfo.outputPath("inline.trace");
    // [ts] fs.promises.writeFile(inlineTrace, [
    // [ts] { type: "context-options", version: 8, origin: "testRunner", browserName: "", options: {}, platform: process.platform, wallTime: 0, monotonicTime: 0 },
    // [ts] { type: "before", callId: "call@1", startTime: 0, class: "Test", method: "step", params: {} },
    // [ts] { type: "after", callId: "call@1", endTime: 1, attachments: [{ name: "empty.txt", contentType: "text/plain", base64: "" }] },
    // [ts] ].map(event => JSON.stringify(event)).join("\n"));
    // [ts] expect_eq((runTraceCli(["open", inlineTrace])).exitCode, 0);
    // [ts] const { stdout: listOutput, exitCode: listExitCode } = runTraceCli(["attachments"]);
    // [ts] expect_eq(listExitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, listOutput, "empty.txt") != null, true);
    // [ts] const { stdout, exitCode } = runTraceCli(["attachment", "1"]);
    // [ts] expect_eq(exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, stdout, "Attachment saved to") != null, true);
    // [ts] expect_eq(fs.promises.readFile(path.join(traceCwd, ".playwright-cli", "empty.txt")), Buffer.alloc(0));
    // [ts] expect_eq((runTraceCli(["open", traceFile])).exitCode, 0);
    // [ts] });
}
