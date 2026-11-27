// Migrated from TypeScript (mcp/cli-devtools.spec.ts) via cap from -ts

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
import { spawnSync } from "cap:process";
import { registry } from "../../packages/playwright-core/lib/coreBundle.cp";
import { test, expect } from "./cli-fixtures.cp";















fn escapeRegExp(text: string): string {
  return text.replace("[.*+?^${}()|[\\]\\\\]", "\\$&");
}












fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("console", () => {
    // [ts] cli("open", server.PREFIX);
    // [ts] cli("eval", "console.log(\"Hello, world!\")");
    // [ts] const { output } = cli("console");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Total messages: 1 (Errors: 0, Warnings: 0)") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Hello, world!") != null, true);
    // [ts] });
    // [ts] test("console error", () => {
    // [ts] cli("open", server.PREFIX);
    // [ts] cli("eval", "console.log(\"log-level\")");
    // [ts] cli("eval", "console.error(\"error-level\")");
    // [ts] const { output } = cli("console", "error");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Total messages: 2 (Errors: 1, Warnings: 0)") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Returning 1 messages for level \"error\"") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "log-level") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "error-level") != null, true);
    // [ts] });
    // [ts] test("console --clear", () => {
    // [ts] cli("open", server.PREFIX);
    // [ts] cli("eval", "console.log(\"log-level\")");
    // [ts] cli("console", "--clear");
    // [ts] const { output } = cli("console");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "log-level") == null, true);
    // [ts] });
    // [ts] test("console --all", () => {
    // [ts] cli("open", server.PREFIX);
    // [ts] cli("eval", "console.log(\"before-navigation\")");
    // [ts] cli("goto", server.PREFIX);
    // [ts] const { output: defaultOutput } = cli("console");
    // [ts] expect_eq(std.mem.indexOf(u8, defaultOutput, "before-navigation") == null, true);
    // [ts] const { output } = cli("console", "--all");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "before-navigation") != null, true);
    // [ts] });
    // [ts] test("requests", () => {
    // [ts] cli("open", server.PREFIX);
    // [ts] cli("eval", "() => fetch(\"/hello-world\")");
    // [ts] const { output } = cli("requests");
    // [ts] expect_eq(std.mem.indexOf(u8, output, `[GET] ${`${server.PREFIX}/`} => [200] OK`) == null, true);
    // [ts] expect(output).toMatch(RegExp.init(String.raw`^\d+\. \[GET\] ${escapeRegExp(`${server.PREFIX}/hello-world`)} => \[200\] OK$`, "m"));
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Note: 1 static request not shown, run with --static option to see it.") != null, true);
    // [ts] });
    // [ts] test("requests --static", () => {
    // [ts] cli("open", server.PREFIX);
    // [ts] const { output } = cli("requests", "--static");
    // [ts] expect(output).toMatch(RegExp.init(String.raw`^\d+\. \[GET\] ${escapeRegExp(`${server.PREFIX}/`)} => \[200\] OK$`, "m"));
    // [ts] expect_eq(std.mem.indexOf(u8, output, "not shown") == null, true);
    // [ts] });
    // [ts] test("requests --filter", () => {
    // [ts] server.setContent("/", `<script>
    // [ts] Promise.all([fetch('/api/users'), fetch('/api/orders'), fetch('/static/image.png')]);
    // [ts] </script>`, 'text/html');
    // [ts] cli("open", server.PREFIX);
    // [ts] const { output } = cli("requests", "--filter=\"api\"", "--static");
    // [ts] expect_eq(std.mem.indexOf(u8, output, `${server.PREFIX}/api/users`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, `${server.PREFIX}/api/orders`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, `${server.PREFIX}/static/image.png`) == null, true);
    // [ts] });
    // [ts] test("requests --clear", () => {
    // [ts] cli("open", server.PREFIX);
    // [ts] cli("eval", "() => fetch(\"/hello-world\")");
    // [ts] cli("requests", "--clear");
    // [ts] const { output } = cli("requests");
    // [ts] expect_eq(std.mem.indexOf(u8, output, `${server.PREFIX}/hello-world`) == null, true);
    // [ts] });
    // [ts] test("requests --all", () => {
    // [ts] cli("open", server.PREFIX);
    // [ts] cli("eval", "() => fetch(\"/hello-world\")");
    // [ts] cli("goto", server.PREFIX);
    // [ts] const { output: defaultOutput } = cli("requests");
    // [ts] expect_eq(std.mem.indexOf(u8, defaultOutput, `${server.PREFIX}/hello-world`) == null, true);
    // [ts] const { output } = cli("requests", "--all");
    // [ts] expect(output).toMatch(RegExp.init(String.raw`^\d+\. \[GET\] ${escapeRegExp(`${server.PREFIX}/hello-world`)} => \[200\] OK$`, "m"));
    // [ts] });
    // [ts] test("request shows full request and response details", () => {
    // [ts] server.setContent("/", `
    // [ts] <button onclick="fetch('/api', { method: 'POST', headers: { 'X-Custom-Header': 'test-value' }, body: JSON.stringify({ key: 'value' }) })">Click me</button>
    // [ts] `, 'text/html');
    // [ts] server.setRoute("/api", (_req, res) => {
    // [ts] res.setHeader("X-Custom-Response", "response-value");
    // [ts] res.setHeader("Content-Type", "application/json");
    // [ts] res.end(JSON.stringify({ name: "John Doe" }));
    // [ts] });
    // [ts] cli("open", server.PREFIX);
    // [ts] cli("click", "e2");
    // [ts] const { output: list } = cli("requests");
    // [ts] const match_val = list.match("^(\\d+)\\. \\[POST\\] [^ ]+\\/api =>");
    // [ts] expect_eq(match_val != null, true);
    // [ts] const { output } = cli("request", match_val[1]);
    // [ts] expect_eq(std.mem.indexOf(u8, output, `#${match_val[1]} [POST] ${server.PREFIX}/api`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "General") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "status:    [200] OK") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Request headers") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "x-custom-header: test-value") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Response headers") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "x-custom-response: response-value") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, `Run \`request-body ${match_val[1]}\` to read the request body.`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, `Run \`response-body ${match_val[1]}\` to read the response body.`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Request body") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Response body") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "{\"key\":\"value\"}") == null, true);
    // [ts] });
    // [ts] test("per-part commands extract individual parts", () => {
    // [ts] server.setContent("/", `
    // [ts] <button onclick="fetch('/api', { method: 'POST', headers: { 'X-Custom-Header': 'test-value' }, body: JSON.stringify({ key: 'value' }) })">Click me</button>
    // [ts] `, 'text/html');
    // [ts] server.setRoute("/api", (_req, res) => {
    // [ts] res.setHeader("X-Custom-Response", "response-value");
    // [ts] res.setHeader("Content-Type", "application/json");
    // [ts] res.end(JSON.stringify({ name: "John Doe" }));
    // [ts] });
    // [ts] cli("open", server.PREFIX);
    // [ts] cli("click", "e2");
    // [ts] const { output: list } = cli("requests");
    // [ts] const match_val = list.match("^(\\d+)\\. \\[POST\\] [^ ]+\\/api =>");
    // [ts] expect_eq(match_val != null, true);
    // [ts] const num = match_val[1];
    // [ts] expect_eq(std.mem.indexOf(u8, (cli("request-headers", num)).output, "x-custom-header: test-value") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, (cli("request-body", num)).output, "{\"key\":\"value\"}") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, (cli("response-headers", num)).output, "x-custom-response: response-value") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, (cli("response-body", num)).output, "{\"name\":\"John Doe\"}") != null, true);
    // [ts] });
    // [ts] test("request* and response* commands support --filename", () => {
    // [ts] server.setContent("/", `
    // [ts] <button onclick="fetch('/api', { method: 'POST', body: 'hello' })">Click me</button>
    // [ts] `, 'text/html');
    // [ts] server.setRoute("/api", (_req, res) => {
    // [ts] res.setHeader("X-Custom-Response", "response-value");
    // [ts] res.setHeader("Content-Type", "application/json");
    // [ts] res.end(JSON.stringify({ name: "John Doe" }));
    // [ts] });
    // [ts] cli("open", server.PREFIX);
    // [ts] cli("click", "e2");
    // [ts] const { output: list } = cli("requests");
    // [ts] const match_val = list.match("^(\\d+)\\. \\[POST\\] [^ ]+\\/api =>");
    // [ts] expect_eq(match_val != null, true);
    // [ts] const num = match_val[1];
    // [ts] const read = (file) => fs.readFileSync(testInfo.outputPath(file), "utf-8");
    // [ts] expect_eq(std.mem.indexOf(u8, (cli("request", num, "--filename=req.log")).output, "[Request](./req.log)") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, read("req.log"), `[POST] ${server.PREFIX}/api`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, (cli("request-headers", num, "--filename=req-h.txt")).output, "[Request headers](./req-h.txt)") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, read("req-h.txt"), "content-type: text/plain;charset=UTF-8") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, (cli("request-body", num, "--filename=req-b.txt")).output, "[Request body](./req-b.txt)") != null, true);
    // [ts] expect_eq(read("req-b.txt"), "hello");
    // [ts] expect_eq(std.mem.indexOf(u8, (cli("response-headers", num, "--filename=res-h.txt")).output, "[Response headers](./res-h.txt)") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, read("res-h.txt"), "x-custom-response: response-value") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, (cli("response-body", num, "--filename=res-b.json")).output, "[Response body](./res-b.json)") != null, true);
    // [ts] expect_eq(read("res-b.json"), "{\"name\":\"John Doe\"}");
    // [ts] });
    // [ts] test("response-body returns just the body", () => {
    // [ts] server.setContent("/", `
    // [ts] <button onclick="fetch('/api')">Click me</button>
    // [ts] `, 'text/html');
    // [ts] server.setContent("/api", JSON.stringify({ name: "John Doe" }), "application/json");
    // [ts] cli("open", server.PREFIX);
    // [ts] cli("click", "e2");
    // [ts] const { output: list } = cli("requests");
    // [ts] const match_val = list.match("^(\\d+)\\. \\[GET\\] [^ ]+\\/api =>");
    // [ts] expect_eq(match_val != null, true);
    // [ts] const { output } = cli("response-body", match_val[1]);
    // [ts] expect_eq(output.trim(), "{\"name\":\"John Doe\"}");
    // [ts] });
    // [ts] test("request with out-of-range index", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright-cli/issues/515" } }, (_p) => {
    // [ts] cli("open", server.PREFIX);
    // [ts] const { output } = cli("request", "999");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Request #999 not found. Run requests to see available numbers.") != null, true);
    // [ts] const { output: invalid, exitCode } = cli("request", "0");
    // [ts] expect_eq(invalid, `### Error\nInvalid arguments for command "request":\n'index' argument: Too small: expected number to be >=1`);
    // [ts] expect_eq(exitCode, 1);
    // [ts] });
    // [ts] test("recording-start-stop", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright-cli/issues/515" } }, (_p) => {
    // [ts] server.setContent("/", `<title>Title</title><button>Submit</button>`, "text/html");
    // [ts] const { snapshot } = cli("open", server.PREFIX);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, `- button "Submit" [ref=e2]`) != null, true);
    // [ts] const { output } = cli("recording-start");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Recording started. Run recording-stop to retrieve the recorded actions.") != null, true);
    // [ts] cli("click", "e2");
    // [ts] const { output: stopOutput } = cli("recording-stop");
    // [ts] expect(stopOutput).toContain(`Recording stopped. Recorded actions:
    // [ts] \`\`\`js
    // [ts] page.getByRole('button', { name: 'Submit' }).click();
    // [ts] \`\`\``);
    // [ts] expect_eq(std.mem.indexOf(u8, (cli("recording-stop")).output, "No recording in progress, use recording-start to start one.") != null, true);
    // [ts] });
    // [ts] test("tracing-start-stop", () => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { output } = cli("tracing-start");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Trace recording started") != null, true);
    // [ts] cli("eval", "() => fetch(\"/hello-world\")");
    // [ts] const { output: tracingStopOutput } = cli("tracing-stop");
    // [ts] expect_eq(std.mem.indexOf(u8, tracingStopOutput, "Trace recording stopped") != null, true);
    // [ts] const [, timestamp] = tracingStopOutput.match("trace-(\\d+)\\.trace");
    // [ts] expect_eq(std.mem.indexOf(u8, tracingStopOutput, `- [Trace](.playwright-cli${path.sep}traces${path.sep}trace-${timestamp}.trace)`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, tracingStopOutput, `- [Network log](.playwright-cli${path.sep}traces${path.sep}trace-${timestamp}.network)`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, tracingStopOutput, `- [Resources](.playwright-cli${path.sep}traces${path.sep}resources)`) != null, true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath(".playwright-cli", "traces", "resources")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath(".playwright-cli", "traces", `trace-${timestamp}.trace`)), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath(".playwright-cli", "traces", `trace-${timestamp}.network`)), true);
    // [ts] });
    // [ts] test("video-start-stop", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright-cli/issues/540" } }, (_p, testInfo) => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { output: videoStartOutput } = cli("video-start", "recordings/video.webm", "--size=400x300");
    // [ts] expect_eq(std.mem.indexOf(u8, videoStartOutput, "Video recording started.") != null, true);
    // [ts] const { output: tabNewOutput } = cli("tab-new");
    // [ts] expect_eq(std.mem.indexOf(u8, tabNewOutput, "1: (current) [](about:blank)") != null, true);
    // [ts] cli("goto", server.EMPTY_PAGE);
    // [ts] cli("tab-select", "0");
    // [ts] const { output: tabCloseOutput } = cli("tab-close");
    // [ts] expect_eq(std.mem.indexOf(u8, tabCloseOutput, `0: (current) [](${server.EMPTY_PAGE})`) != null, true);
    // [ts] const { output: videoStopOutput } = cli("video-stop");
    // [ts] expect_eq(std.mem.indexOf(u8, videoStopOutput, `### Result\n- [Video](recordings${path.sep}video.webm)\n- [Video](recordings${path.sep}video-1.webm)`) != null, true);
    // [ts] cli("close");
    // [ts] const emptyBrowsersPath = testInfo.outputPath("empty-browsers");
    // [ts] fs.promises.mkdir(emptyBrowsersPath, { recursive: true });
    // [ts] cli("open", server.HELLO_WORLD, {
    // [ts] env: {
    // [ts] PLAYWRIGHT_BROWSERS_PATH: emptyBrowsersPath,
    // [ts] PLAYWRIGHT_MCP_EXECUTABLE_PATH: registry.registry.findExecutable(mcpBrowser).executablePath(),
    // [ts] },
    // [ts] });
    // [ts] const { output: missingFfmpegOutput } = cli("video-start", "video.webm");
    // [ts] expect_eq(std.mem.indexOf(u8, missingFfmpegOutput, "FFmpeg is not installed") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, missingFfmpegOutput, `expected executable at ${emptyBrowsersPath}`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, missingFfmpegOutput, "playwright-cli install-browser ffmpeg") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, missingFfmpegOutput, "npx playwright install ffmpeg") == null, true);
    // [ts] });
    // [ts] test("video-start with fps", () => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { output } = cli("video-start", "video.webm", "--fps=60");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Video recording started.") != null, true);
    // [ts] cli("video-stop");
    // [ts] const ffmpeg = registry.registry.findExecutable("ffmpeg").executablePath();
    // [ts] const { stderr } = spawnSync(ffmpeg, ["-i", testInfo.outputPath("video.webm")]);
    // [ts] expect_eq(std.mem.indexOf(u8, stderr.toString(), ", 60 fps,") != null, true);
    // [ts] });
    // [ts] test("video-start --cursor", () => {
    // [ts] const page = boundBrowser.newPage();
    // [ts] page.setContent(`<button>Submit</button>`);
    // [ts] cli("attach", "default");
    // [ts] cli("snapshot");
    // [ts] cli("video-start", "video.webm", "--cursor");
    // [ts] cli("click", "e2");
    // The cursor travels to the action point, the action title stays out of the video.
    // [ts] expect(page.locator("x-pw-action-cursor")).toBeVisible();
    // [ts] expect(page.locator("x-pw-title")).toBeHidden();
    // [ts] cli("video-stop");
    // [ts] });
    // [ts] test("video-chapter", () => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] cli("video-start", "video.webm");
    // [ts] const { output } = cli("video-chapter", "Introduction", "--description=Welcome to the demo", "--duration=100");
    // [ts] expect_eq(std.mem.indexOf(u8, output, `Chapter 'Introduction' added.`) != null, true);
    // [ts] cli("video-stop");
    // [ts] });
    // [ts] test("video-show-actions and video-hide-actions", () => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { output: showOutput } = cli("video-show-actions", "--duration=200", "--position=bottom-right");
    // [ts] expect_eq(std.mem.indexOf(u8, showOutput, "Action annotations enabled.") != null, true);
    // [ts] const { output: hideOutput } = cli("video-hide-actions");
    // [ts] expect_eq(std.mem.indexOf(u8, hideOutput, "Action annotations disabled.") != null, true);
    // [ts] });
    // [ts] test("video-show-actions rejects invalid position", () => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { error, exitCode } = cli("video-show-actions", "--position=middle");
    // [ts] expect_eq(exitCode != 0, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error, "position") != null, true);
    // [ts] });
    // [ts] test("generate-locator", () => {
    // [ts] server.setContent("/", `<button>Submit</button>`, "text/html");
    // [ts] cli("open", server.PREFIX);
    // [ts] cli("snapshot");
    // [ts] const { output } = cli("generate-locator", "e2", "--raw");
    // [ts] expect_eq(std.mem.indexOf(u8, output, `getByRole('button', { name: 'Submit' })`) != null, true);
    // [ts] });
    // [ts] test("highlight", () => {
    // [ts] const page = boundBrowser.newPage();
    // [ts] page.setContent(`<button>Submit</button>`);
    // [ts] cli("attach", "default");
    // [ts] cli("snapshot");
    // [ts] const { output } = cli("highlight", "e2");
    // [ts] expect_eq(std.mem.indexOf(u8, output, `Highlighted locator('aria-ref=e2')`) != null, true);
    // [ts] const highlight = page.locator("x-pw-highlight");
    // [ts] const tooltip = page.locator("x-pw-tooltip-line");
    // [ts] expect(highlight).toBeVisible();
    // [ts] expect(tooltip).toHaveText(`locator('aria-ref=e2')`);
    // [ts] expect_eq(highlight.boundingBox(), page.getByRole("button", { name: "Submit" }).boundingBox());
    // [ts] });
    // [ts] test("highlight --hide", () => {
    // [ts] const page = boundBrowser.newPage();
    // [ts] page.setContent(`<button>Submit</button>`);
    // [ts] cli("attach", "default");
    // [ts] cli("snapshot");
    // [ts] cli("highlight", "e2");
    // [ts] expect(page.locator("x-pw-highlight")).toBeVisible();
    // [ts] const { output } = cli("highlight", "e2", "--hide");
    // [ts] expect_eq(std.mem.indexOf(u8, output, `Hid highlight for locator('aria-ref=e2')`) != null, true);
    // [ts] expect(page.locator("x-pw-highlight")).toHaveCount(0);
    // [ts] });
    // [ts] test("highlight --hide all", () => {
    // [ts] const page = boundBrowser.newPage();
    // [ts] page.setContent(`<button>Submit</button><a href="#">Go</a>`);
    // [ts] cli("attach", "default");
    // [ts] cli("snapshot");
    // [ts] cli("highlight", "e2");
    // [ts] cli("highlight", "e3");
    // [ts] expect(page.locator("x-pw-highlight")).toHaveCount(2);
    // [ts] const { output } = cli("highlight", "--hide");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Hid page highlight") != null, true);
    // [ts] expect(page.locator("x-pw-highlight")).toHaveCount(0);
    // [ts] });
    // [ts] test("highlight --style", () => {
    // [ts] const page = boundBrowser.newPage();
    // [ts] page.setContent(`<button>Submit</button>`);
    // [ts] cli("attach", "default");
    // [ts] cli("snapshot");
    // [ts] cli("highlight", "e2", "--style=outline: 3px solid rgb(255, 0, 0); background-color: rgba(0, 255, 0, 0.25)");
    // [ts] const highlight = page.locator("x-pw-highlight");
    // [ts] expect(highlight).toBeVisible();
    // [ts] expect(highlight.evaluate((el) => ({
    // [ts] outlineWidth: el.style.outlineWidth,
    // [ts] outlineStyle: el.style.outlineStyle,
    // [ts] outlineColor: el.style.outlineColor,
    // [ts] backgroundColor: el.style.backgroundColor,
    // [ts] }))).toEqual({
    // [ts] outlineWidth: "3px",
    // [ts] outlineStyle: "solid",
    // [ts] outlineColor: "rgb(255, 0, 0)",
    // [ts] backgroundColor: "rgba(0, 255, 0, 0.25)",
    // [ts] });
    // [ts] });
}
