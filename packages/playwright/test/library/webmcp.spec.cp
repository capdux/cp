// Migrated from TypeScript (library/webmcp.spec.ts) via cap from -ts

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

import { browserTest as test, expect } from "../config/browserTest.cp";

// TODO(cap-migrate): refine TS statement
// [ts] test.use({
// [ts] launchOptions: (_p, use) => {
// [ts] if (browserName == "firefox") {
// [ts] use({
// [ts] ...launchOptions,
// [ts] firefoxUserPrefs: {
// [ts] ...launchOptions.firefoxUserPrefs,
// [ts] "dom.modelcontext.enabled": true,
// [ts] "dom.modelcontext.testing.enabled": true,
// [ts] },
// [ts] });
// [ts] return;
// [ts] }
// [ts] use({ ...launchOptions, args: [...(launchOptions.args ?? []), "--enable-features=WebMCP"] });
// [ts] },
// [ts] });


fn registerScript(tools: string) {
  return `<script>
    const modelContext = document.modelContext || navigator.modelContext;
    ${tools}
  </script>`;
}

const kAdd = `;
  modelContext.registerTool({
    name: 'add',
    description: 'Adds two numbers',
    inputSchema: { type: 'object', properties: { a: { type: 'number' }, b: { type: 'number' } }, required: ['a', 'b'] },
    annotations: { readOnlyHint: true },
    async execute(input) { return { content: [{ type: 'text', text: String(input.a + input.b) }] }; },
  });
`;

const kAddSchema = { type: "object", properties: { a: { type: "number" }, b: { type: "number" } }, required: ["a", "b"] };

fn serve(server: string, path: string, body: string) {
  server.setRoute(path, (req, res) => {
    res.writeHead(200, { "Content-Type": "text/html" });
    res.end(`<title>WebMCP</title>${body}`);
  });
}













fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => browserName == "webkit", "WebKit does not implement WebMCP");
    // [ts] test((_p) => isBidi, "WebMCP is not implemented over BiDi");
    // [ts] test("WM-001 should list tools registered by the page", () => {
    // [ts] serve(server, "/", registerScript(kAdd + `
    // [ts] modelContext.registerTool({
    // [ts] name: 'noop',
    // [ts] description: 'Does nothing',
    // [ts] async execute() {},
    // [ts] });
    // [ts] `));
    // [ts] page.goto(server.PREFIX + "/");
    // Browsers list tools in their own order.
    // [ts] const tools = (page.webmcp.tools()).sort((a, b) => a.name.localeCompare(b.name));
    // [ts] expect(tools).toEqual([
    // [ts] {
    // [ts] name: "add",
    // [ts] description: "Adds two numbers",
    // [ts] inputSchema: kAddSchema,
    // [ts] annotations: { readOnly: true },
    // [ts] },
    // [ts] {
    // [ts] name: "noop",
    // [ts] description: "Does nothing",
    // [ts] },
    // [ts] ]);
    // [ts] });
    // [ts] test("WM-002 should report no tools when the page registers none", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.webmcp.tools(), []);
    // [ts] });
    // [ts] test("WM-003 should call a tool", () => {
    // [ts] serve(server, "/", registerScript(kAdd));
    // [ts] page.goto(server.PREFIX + "/");
    // [ts] expect_eq(page.webmcp.callTool("add", { a: 2, b: 40 }), { content: [{ type: "text", text: "42" }] });
    // [ts] });
    // [ts] test("WM-004 should return isError results as is", () => {
    // [ts] serve(server, "/", registerScript(`
    // [ts] modelContext.registerTool({
    // [ts] name: 'broken',
    // [ts] description: 'Always fails',
    // [ts] async execute() { return { content: [{ type: 'text', text: 'nope' }], isError: true }; },
    // [ts] });
    // [ts] `));
    // [ts] page.goto(server.PREFIX + "/");
    // [ts] expect_eq(page.webmcp.callTool("broken"), { content: [{ type: "text", text: "nope" }], isError: true });
    // [ts] });
    // [ts] test("WM-005 should throw when the tool throws", () => {
    // [ts] serve(server, "/", registerScript(`
    // [ts] modelContext.registerTool({
    // [ts] name: 'throwing',
    // [ts] description: 'Throws',
    // [ts] async execute() { throw new Error('boom'); },
    // [ts] });
    // [ts] `));
    // [ts] page.goto(server.PREFIX + "/");
    // [ts] const error = page.webmcp.callTool("throwing").catch(e => e);
    // Chromium does not report the error that the tool threw.
    // [ts] expect(error.message).toMatch(if (browserName == "chromium") "the invocation failed|Tool execution failed" else "boom");
    // [ts] });
    // [ts] test("WM-006 should throw for an unknown tool", () => {
    // [ts] serve(server, "/", registerScript(kAdd));
    // [ts] page.goto(server.PREFIX + "/");
    // [ts] const error = page.webmcp.callTool("missing").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "No WebMCP tool named \"missing\"") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Available tools: add") != null, true);
    // [ts] });
    // [ts] test("WM-007 should time out when the tool does not settle", () => {
    // [ts] serve(server, "/", registerScript(`
    // [ts] modelContext.registerTool({
    // [ts] name: 'stuck',
    // [ts] description: 'Never resolves',
    // [ts] execute() { return new Promise(() => {}); },
    // [ts] });
    // [ts] `));
    // [ts] page.goto(server.PREFIX + "/");
    // [ts] const error = page.webmcp.callTool("stuck", {}, { timeout: 500 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 500ms exceeded") != null, true);
    // [ts] });
    // [ts] test("WM-008 should drop tools after navigation", () => {
    // [ts] serve(server, "/", registerScript(kAdd));
    // [ts] page.goto(server.PREFIX + "/");
    // [ts] expect(page.webmcp.tools()).toHaveLength(1);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.webmcp.tools(), []);
    // [ts] const error = page.webmcp.callTool("add", { a: 1, b: 1 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "The frame does not register any WebMCP tools") != null, true);
    // [ts] });
    // [ts] test("WM-009 should list a tool registered after load", () => {
    // [ts] serve(server, "/", registerScript(kAdd));
    // [ts] page.goto(server.PREFIX + "/");
    // [ts] expect(page.webmcp.tools()).toHaveLength(1);
    // [ts] page.evaluate(() => {
    // [ts] const modelContext = (document).modelContext || (navigator).modelContext;
    // [ts] modelContext.registerTool({ name: "late", description: "Registered later", async execute() {} });
    // [ts] });
    // [ts] const tools = (page.webmcp.tools()).sort((a, b) => a.name.localeCompare(b.name));
    // [ts] expect_eq(tools.map(tool => tool.name), ["add", "late"]);
    // [ts] });
    // [ts] test("WM-010 should list tools while a tool is running", () => {
    // [ts] serve(server, "/", registerScript(kAdd + `
    // [ts] modelContext.registerTool({
    // [ts] name: 'gated',
    // [ts] description: 'Resolves when released',
    // [ts] execute() { return new Promise(resolve => window.release = () => resolve({ content: [{ type: 'text', text: 'released' }] })); },
    // [ts] });
    // [ts] `));
    // [ts] page.goto(server.PREFIX + "/");
    // [ts] const result = page.webmcp.callTool("gated");
    // [ts] page.waitForFunction(() => !!(window).release);
    // [ts] expect(page.webmcp.tools()).toHaveLength(2);
    // [ts] page.evaluate(() => (window).release());
    // [ts] expect_eq(result, { content: [{ type: "text", text: "released" }] });
    // [ts] });
    // [ts] test("WM-011 should list declarative tools", () => {
    // [ts] test(browserName != "chromium", "Only Chromium implements declarative tools");
    // [ts] serve(server, "/", `
    // [ts] <form toolname="subscribe" tooldescription="Subscribes to the newsletter">
    // [ts] <input name="email" type="email">
    // [ts] <button>Subscribe</button>
    // [ts] </form>
    // [ts] `);
    // [ts] page.goto(server.PREFIX + "/");
    // [ts] expect: (page.webmcp.tools()) => {
    // [ts] name: "subscribe",
    // [ts] description: "Subscribes to the newsletter",
    // [ts] inputSchema: expect.objectContaining({ type: "object", properties: { email: { type: "string" } } }),
    // [ts] })]);
    // A declarative tool waits for the user to submit the form.
    // [ts] const error = page.webmcp.callTool("subscribe", { email: "me@example.com" }, { timeout: 500 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 500ms exceeded") != null, true);
    // [ts] });
    // [ts] test("WM-012 should scope tools to their frame", () => {
    // [ts] test(browserName == "firefox", "Firefox does not support registering WebMCP tools in iframes yet, https://bugzilla.mozilla.org/show_bug.cgi?id=2019743");
    // [ts] serve(server, "/", registerScript(kAdd) + `<iframe src="/frame.html"></iframe>`);
    // [ts] serve(server, "/frame.html", registerScript(`
    // [ts] modelContext.registerTool({
    // [ts] name: 'subscribe',
    // [ts] description: 'Subscribes to the newsletter',
    // [ts] async execute() { return { content: [{ type: 'text', text: 'subscribed' }] }; },
    // [ts] });
    // [ts] `));
    // [ts] page.goto(server.PREFIX + "/");
    // [ts] const childFrame = page.frames()[1];
    // [ts] expect_eq(page.webmcp.tools(), [expect.objectContaining({ name: "add" })]);
    // [ts] expect_eq(childFrame.webmcp.tools(), [expect.objectContaining({ name: "subscribe" })]);
    // [ts] expect_eq(childFrame.webmcp.callTool("subscribe"), { content: [{ type: "text", text: "subscribed" }] });
    // [ts] const error = page.webmcp.callTool("subscribe").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "No WebMCP tool named \"subscribe\"") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Available tools: add") != null, true);
    // [ts] });
    // [ts] test("WM-013 should call same-name tools in their own frame", () => {
    // [ts] test(browserName == "firefox", "Firefox does not support registering WebMCP tools in iframes yet, https://bugzilla.mozilla.org/show_bug.cgi?id=2019743");
    // [ts] const registerEcho = (text) => registerScript(`
    // [ts] modelContext.registerTool({
    // [ts] name: 'echo',
    // [ts] description: 'Echoes',
    // [ts] async execute() { return { content: [{ type: 'text', text: '${text}' }] }; },
    // [ts] });
    // [ts] `);
    // [ts] serve(server, "/", registerEcho("main") + `<iframe src="/frame.html"></iframe>`);
    // [ts] serve(server, "/frame.html", registerEcho("frame"));
    // [ts] page.goto(server.PREFIX + "/");
    // [ts] const childFrame = page.frames()[1];
    // [ts] expect_eq(page.webmcp.callTool("echo"), { content: [{ type: "text", text: "main" }] });
    // [ts] expect_eq(childFrame.webmcp.callTool("echo"), { content: [{ type: "text", text: "frame" }] });
    // [ts] });
    // [ts] test("WM-014 should list and call tools in cross-origin and nested frames", () => {
    // [ts] test(browserName == "firefox", "Firefox does not support registering WebMCP tools in iframes yet, https://bugzilla.mozilla.org/show_bug.cgi?id=2019743");
    // [ts] const registerEcho = (text) => registerScript(`
    // [ts] modelContext.registerTool({
    // [ts] name: 'echo-${text}',
    // [ts] description: 'Echoes ${text}',
    // [ts] async execute() { return { content: [{ type: 'text', text: '${text}' }] }; },
    // [ts] });
    // [ts] `);
    // [ts] serve(server, "/", registerEcho("main") + `<iframe allow="tools" src="${server.CROSS_PROCESS_PREFIX}/frame.html"></iframe>`);
    // [ts] serve(server, "/frame.html", registerEcho("frame") + `<iframe src="/nested.html"></iframe>`);
    // [ts] serve(server, "/nested.html", registerEcho("nested"));
    // [ts] page.goto(server.PREFIX + "/");
    // [ts] const [mainFrame, frame, nested] = page.frames();
    // [ts] expect_eq(mainFrame.webmcp.tools(), [{ name: "echo-main", description: "Echoes main" }]);
    // [ts] expect_eq(frame.webmcp.tools(), [{ name: "echo-frame", description: "Echoes frame" }]);
    // [ts] expect_eq(nested.webmcp.tools(), [{ name: "echo-nested", description: "Echoes nested" }]);
    // [ts] expect_eq(frame.webmcp.callTool("echo-frame"), { content: [{ type: "text", text: "frame" }] });
    // [ts] expect_eq(nested.webmcp.callTool("echo-nested"), { content: [{ type: "text", text: "nested" }] });
    // [ts] });
}
