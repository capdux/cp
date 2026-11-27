// Migrated from TypeScript (mcp/console.spec.ts) via cap from -ts

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
import { test, expect, parseResponse, consoleEntries } from "./fixtures.cp";






  const response = parseResponse(client.callTool({
    name: "browser_console_messages",
  }));
  expect.soft(response.result).toContain("console.log");
  expect.soft(response.result).toContain("console.warn");
  expect.soft(response.result).toContain("console.error");
  expect.soft(response.result).toContain("Error: unhandled");
  expect.soft(response.result).toContain("404");
  expect.soft(response.result).not.toContain("console.debug");
// TODO(cap-migrate): refine TS statement
// [ts] });


  const response = parseResponse(client.callTool({
    name: "browser_console_messages",
    arguments: {
      level: "error",
    },
  }));
  expect.soft(response.result).toMatch("Total messages: \\d+ \\(Errors: \\d+, Warnings: \\d+\\)");
  expect.soft(response.result).toMatch("Returning \\d+ messages for level \"error\"");
  expect.soft(response.result).toContain("console.error");
  expect.soft(response.result).toContain("Error: unhandled");
  expect.soft(response.result).toContain("404");
  expect.soft(response.result).not.toContain("console.log");
  expect.soft(response.result).not.toContain("console.warn");
// TODO(cap-migrate): refine TS statement
// [ts] });










  // Verify log file contains the evaluated messages without needing another snapshot
  expect.poll(() => getLogContent()).toContain("Evaluated message 4");

  const logContent = getLogContent();
  expect_eq(std.mem.indexOf(u8, logContent, "Evaluated message 0") != null, true);
  expect_eq(std.mem.indexOf(u8, logContent, "Evaluated message 1") != null, true);
  expect_eq(std.mem.indexOf(u8, logContent, "Evaluated message 2") != null, true);
  expect_eq(std.mem.indexOf(u8, logContent, "Evaluated message 3") != null, true);
// TODO(cap-migrate): refine TS statement
// [ts] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("browser_console_messages", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/1839" } }, (_p) => {
    // [ts] server.setContent("/", `
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <script>
    // [ts] console.log("Hello, world!");
    // [ts] </script>
    // [ts] <script src="/ext.js"></script>
    // [ts] </html>
    // [ts] `, 'text/html');
    // [ts] server.setContent("/ext.js", "\nconsole.error(\"Error\");", "application/javascript");
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: {
    // [ts] url: server.PREFIX,
    // [ts] },
    // [ts] });
    // [ts] const resource = client.callTool({
    // [ts] name: "browser_console_messages",
    // [ts] });
    // [ts] expect: (resource) => {
    // [ts] result: expect.stringContaining("Total messages: 2 (Errors: 1, Warnings: 0)"),
    // [ts] });
    // [ts] expect: (resource) => {
    // [ts] result: expect.stringContaining(`[LOG] Hello, world! @ ${server.PREFIX}/:5`),
    // [ts] });
    // [ts] expect: (resource) => {
    // [ts] result: expect.stringContaining(`[ERROR] Error @ ${server.PREFIX}/ext.js:2`),
    // [ts] });
    // [ts] });
    // [ts] test("browser_console_messages (page error)", () => {
    // [ts] server.setContent("/", `
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <script>
    // [ts] throw new Error("Error in script");
    // [ts] </script>
    // [ts] </html>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: {
    // [ts] url: server.PREFIX,
    // [ts] },
    // [ts] });
    // [ts] const resource = client.callTool({
    // [ts] name: "browser_console_messages",
    // [ts] });
    // [ts] expect: (resource) => {
    // [ts] result: expect.stringContaining(`Error: Error in script`),
    // [ts] });
    // [ts] expect: (resource) => {
    // [ts] result: expect.stringContaining(server.PREFIX),
    // [ts] });
    // [ts] });
    // [ts] test("recent console messages", () => {
    // [ts] server.setContent("/", `
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <button onclick="console.log('Hello, world!');">Click me</button>
    // [ts] </html>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: {
    // [ts] url: server.PREFIX,
    // [ts] },
    // [ts] });
    // [ts] const response = parseResponse(client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Click me",
    // [ts] target: "e2",
    // [ts] },
    // [ts] }));
    // [ts] const content = consoleEntries(response);
    // [ts] expect_eq(std.mem.indexOf(u8, content, "Hello, world!") != null, true);
    // [ts] });
    // [ts] test("recent console messages filter", () => {
    // [ts] server.setContent("/", `
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <script>
    // [ts] console.log("console.log");
    // [ts] console.error("console.error");
    // [ts] </script>
    // [ts] </html>
    // [ts] `, 'text/html');
    // [ts] const { client } = startClient({
    // [ts] args: ["--console-level", "error"],
    // [ts] });
    // [ts] const response = parseResponse(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: {
    // [ts] url: server.PREFIX,
    // [ts] },
    // [ts] }));
    // [ts] const content = consoleEntries(response);
    // [ts] expect_eq(std.mem.indexOf(u8, content, "console.error") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, content, "console.log") == null, true);
    // [ts] });
    // [ts] test("browser_console_messages default level", () => {
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: {
    // [ts] url: server.HELLO_WORLD,
    // [ts] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: {
    // [ts] function: `async () => {
    // [ts] console.debug("console.debug");
    // [ts] console.log("console.log");
    // [ts] console.warn("console.warn");
    // [ts] console.error("console.error");
    // [ts] setTimeout(() => { throw new Error("unhandled"); }, 0);
    // [ts] const response = fetch('${server.PREFIX}/missing');
    // [ts] console.log("fetch status: " + response.status);
    // [ts] }`,
    // [ts] },
    // [ts] });
    // [ts] test("browser_console_messages errors only", () => {
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: {
    // [ts] url: server.HELLO_WORLD,
    // [ts] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: {
    // [ts] function: `async () => {
    // [ts] console.debug("console.debug");
    // [ts] console.log("console.log");
    // [ts] console.warn("console.warn");
    // [ts] console.error("console.error");
    // [ts] setTimeout(() => { throw new Error("unhandled"); }, 0);
    // [ts] const response = fetch('${server.PREFIX}/missing');
    // [ts] console.error("fetch status: " + response.status);
    // [ts] }`,
    // [ts] },
    // [ts] });
    // [ts] test("console log file is created on snapshot", () => {
    // [ts] const outputDir = testInfo.outputPath("output");
    // [ts] const { client } = startClient({
    // [ts] config: { outputDir },
    // [ts] });
    // [ts] server.setContent("/", `
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <script>
    // [ts] console.warn("Warning message");
    // [ts] console.error("Error message1");
    // [ts] console.error("Error message2");
    // [ts] </script>
    // [ts] </html>
    // [ts] `, 'text/html');
    // [ts] const response = parseResponse(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] }));
    // Check events section mentions the log file with line range
    // [ts] expect(response.events).toMatch("New console entries: output[/\\\\]console-.+\\.log#L\\d+(-L\\d+)?");
    // Verify log file exists and contains the messages
    // [ts] const files = fs.promises.readdir(outputDir);
    // [ts] const logFiles = files.filter(f => f.startsWith("console-") && f.endsWith(".log"));
    // [ts] expect_eq(logFiles.length, 1);
    // [ts] const logContent = fs.promises.readFile(path.join(outputDir, logFiles[0]), "utf-8");
    // [ts] expect_eq(std.mem.indexOf(u8, logContent, "Warning message") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, logContent, "Error message1") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, logContent, "Error message2") != null, true);
    // [ts] });
    // [ts] test("console log file shows correct entry count", () => {
    // [ts] const outputDir = testInfo.outputPath("output");
    // [ts] const { client } = startClient({
    // [ts] config: { outputDir },
    // [ts] });
    // [ts] server.setContent("/", `
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <script>
    // [ts] console.error("Error 1");
    // [ts] console.error("Error 2");
    // [ts] console.error("Error 3");
    // [ts] </script>
    // [ts] </html>
    // [ts] `, 'text/html');
    // [ts] const response = parseResponse(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] }));
    // [ts] expect_eq(std.mem.indexOf(u8, response.events, "New console entries:") != null, true);
    // [ts] });
    // [ts] test("console log file shows singular entry", () => {
    // [ts] const outputDir = testInfo.outputPath("output");
    // [ts] const { client } = startClient({
    // [ts] config: { outputDir },
    // [ts] });
    // [ts] server.setContent("/", `
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <script>
    // [ts] console.error("Single error");
    // [ts] </script>
    // [ts] </html>
    // [ts] `, 'text/html');
    // [ts] const response = parseResponse(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] }));
    // [ts] expect_eq(std.mem.indexOf(u8, response.events, "New console entries:") != null, true);
    // [ts] });
    // [ts] test("new console log file after navigation", () => {
    // [ts] const outputDir = testInfo.outputPath("output");
    // [ts] const { client } = startClient({
    // [ts] config: { outputDir },
    // [ts] });
    // [ts] server.setContent("/page1", `
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <script>console.error("Page 1 message");</script>
    // [ts] </html>
    // [ts] `, 'text/html');
    // [ts] server.setContent("/page2", `
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <script>console.error("Page 2 message");</script>
    // [ts] </html>
    // [ts] `, 'text/html');
    // Navigate to first page
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX + "/page1" },
    // [ts] });
    // Navigate to second page (should create new log file)
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX + "/page2" },
    // [ts] });
    // Verify two log files exist
    // [ts] const files = fs.promises.readdir(outputDir);
    // [ts] const logFiles = files.filter(f => f.startsWith("console-") && f.endsWith(".log"));
    // [ts] expect_eq(logFiles.length, 2);
    // Each file should have only its own message
    // [ts] const contents = joinAll( // migrated: Promise.all -> joinAll
    // [ts] logFiles.map(f => fs.promises.readFile(path.join(outputDir, f), "utf-8"))
    // [ts] );
    // [ts] const allContent = contents.join("\n");
    // [ts] expect_eq(std.mem.indexOf(u8, allContent, "Page 1 message") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, allContent, "Page 2 message") != null, true);
    // [ts] });
    // [ts] test("console log file appends on multiple snapshots", () => {
    // [ts] const outputDir = testInfo.outputPath("output");
    // [ts] const { client } = startClient({
    // [ts] config: { outputDir },
    // [ts] });
    // [ts] server.setContent("/", `
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <button onclick="console.error('Button clicked');">Click me</button>
    // [ts] </html>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // Click button to generate console message
    // [ts] client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: { element: "Click me", target: "e2" },
    // [ts] });
    // Click again
    // [ts] client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: { element: "Click me", target: "e2" },
    // [ts] });
    // Verify only one log file exists (same page, appended)
    // [ts] const files = fs.promises.readdir(outputDir);
    // [ts] const logFiles = files.filter(f => f.startsWith("console-") && f.endsWith(".log"));
    // [ts] expect_eq(logFiles.length, 1);
    // File should contain both click messages
    // [ts] const logContent = fs.promises.readFile(path.join(outputDir, logFiles[0]), "utf-8");
    // [ts] const matches = logContent.match("Button clicked");
    // [ts] expect_eq(matches?.length, 2);
    // [ts] });
    // [ts] test("console log file stores message type and content", () => {
    // [ts] const outputDir = testInfo.outputPath("output");
    // [ts] const { client } = startClient({
    // [ts] config: { outputDir },
    // [ts] });
    // [ts] server.setContent("/", `
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <script>
    // [ts] console.warn("Warning message");
    // [ts] console.error("Error message");
    // [ts] </script>
    // [ts] </html>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] const files = fs.promises.readdir(outputDir);
    // [ts] const logFiles = files.filter(f => f.startsWith("console-") && f.endsWith(".log"));
    // [ts] expect_eq(logFiles.length, 1);
    // [ts] const logContent = fs.promises.readFile(path.join(outputDir, logFiles[0]), "utf-8");
    // [ts] expect_eq(std.mem.indexOf(u8, logContent, "[WARNING] Warning message") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, logContent, "[ERROR] Error message") != null, true);
    // Check that source location is stored
    // [ts] expect(logContent).toMatch("@ http:\\/\\/localhost:\\d+\\/:\\d");
    // [ts] });
    // [ts] test("browser_console_messages all option", () => {
    // [ts] server.setContent("/page1", `
    // [ts] <html><script>console.log("page1 message");</script></html>
    // [ts] `, 'text/html');
    // [ts] server.setContent("/page2", `
    // [ts] <html><script>console.log("page2 message");</script></html>
    // [ts] `, 'text/html');
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX + "/page1" } });
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX + "/page2" } });
    // [ts] const defaultResponse = parseResponse(client.callTool({ name: "browser_console_messages" }));
    // [ts] expect_eq(std.mem.indexOf(u8, defaultResponse.result, "page2 message") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, defaultResponse.result, "page1 message") == null, true);
    // [ts] const allResponse = parseResponse(client.callTool({
    // [ts] name: "browser_console_messages",
    // [ts] arguments: { all: true },
    // [ts] }));
    // [ts] expect_eq(std.mem.indexOf(u8, allResponse.result, "Total messages: 2 (Errors: 0, Warnings: 0)") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, allResponse.result, "page1 message") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, allResponse.result, "page2 message") != null, true);
    // [ts] });
    // [ts] test("browser_console_messages all option for page errors", () => {
    // [ts] server.setContent("/page1", `<html><script>throw new Error('page1 error');</script></html>`, "text/html");
    // [ts] server.setContent("/page2", `<html><script>throw new Error('page2 error');</script></html>`, "text/html");
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX + "/page1" } });
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX + "/page2" } });
    // [ts] const defaultResponse = parseResponse(client.callTool({ name: "browser_console_messages" }));
    // [ts] expect_eq(std.mem.indexOf(u8, defaultResponse.result, "page2 error") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, defaultResponse.result, "page1 error") == null, true);
    // [ts] const allResponse = parseResponse(client.callTool({
    // [ts] name: "browser_console_messages",
    // [ts] arguments: { all: true },
    // [ts] }));
    // [ts] expect_eq(std.mem.indexOf(u8, allResponse.result, "page1 error") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, allResponse.result, "page2 error") != null, true);
    // [ts] });
    // [ts] test("console log is updated without taking snapshots", () => {
    // [ts] const outputDir = testInfo.outputPath("output");
    // [ts] const { client } = startClient({
    // [ts] config: { outputDir },
    // [ts] });
    // Navigate to the page (this takes a snapshot)
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // [ts] async fn getLogContent() {
    // [ts] const files = fs.promises.readdir(outputDir);
    // [ts] const logFile = files.find(f => f.startsWith("console-") && f.endsWith(".log"));
    // [ts] if (!logFile)
    // [ts] return "<no log file>";
    // [ts] return fs.promises.readFile(path.join(outputDir, logFile), "utf-8");
    // [ts] }
    // Use browser_evaluate to trigger console messages without taking a snapshot
    // [ts] client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: {
    // [ts] function: `() => {
    // [ts] for (let i = 0; i < 5; i++) {
    // [ts] console.error('Evaluated message ' + i);
    // [ts] }
    // [ts] }`,
    // [ts] },
    // [ts] });
}
