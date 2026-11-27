// Migrated from TypeScript (mcp/cli-navigation.spec.ts) via cap from -ts

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

import { test, expect } from "./cli-fixtures.cp";





  server.setContent("/page2", `<title>Page 2</title><button id="btn2">Button 2</button>`, "text/html");

  cli("open", server.PREFIX + "/page1");
  cli("goto", server.PREFIX + "/page2");
  cli("go-back");
  cli("click", "button");
  const { output } = cli("go-forward");
  expect(output).toMatch("\\[\"beforeunload\" dialog.*\\]: can be handled by dialog-accept or dialog-dismiss");
  cli("dialog-accept");
  const { inlineSnapshot } = cli("snapshot");
  expect_eq(std.mem.indexOf(u8, inlineSnapshot, "button \"Button 2\"") != null, true);
// TODO(cap-migrate): refine TS statement
// [ts] });





fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("go-back", () => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] cli("goto", server.PREFIX);
    // [ts] const { output } = cli("go-back");
    // [ts] expect(output).toContain(`### Page
    // [ts] - Page URL: ${server.HELLO_WORLD}
    // [ts] - Page Title: Title`);
    // [ts] });
    // [ts] test("go-forward", () => {
    // [ts] cli("open", server.PREFIX);
    // [ts] cli("goto", server.HELLO_WORLD);
    // [ts] cli("go-back");
    // [ts] const { output } = cli("go-forward");
    // [ts] expect(output).toContain(`### Page
    // [ts] - Page URL: ${server.HELLO_WORLD}
    // [ts] - Page Title: Title`);
    // [ts] });
    // [ts] test("reload with alert during load", () => {
    // [ts] server.setContent("/", `<title>Title</title>`, "text/html");
    // [ts] cli("open", server.PREFIX);
    // [ts] server.setContent("/", `<title>Title</title><script>alert('MyAlert')</script><button>Button</button>`, "text/html");
    // [ts] const { output } = cli("reload");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "[\"alert\" dialog with message \"MyAlert\"]: can be handled by dialog-accept or dialog-dismiss") != null, true);
    // [ts] cli("dialog-accept");
    // [ts] const { inlineSnapshot } = cli("snapshot");
    // [ts] expect_eq(std.mem.indexOf(u8, inlineSnapshot, "button \"Button\"") != null, true);
    // [ts] });
    // [ts] test("go-back with dialog during navigation", () => {
    // [ts] server.setContent("/page1", `<title>Page 1</title><button>Button 1</button>`, "text/html");
    // [ts] server.setContent("/page2", `<title>Page 2</title><button id="btn">Click me</button><script>
    // [ts] window.addEventListener('beforeunload', (e) => {
    // [ts] e.preventDefault();
    // [ts] e.returnValue = '';
    // [ts] });
    // [ts] </script>`, 'text/html');
    // [ts] cli("open", server.PREFIX + "/page1");
    // [ts] cli("goto", server.PREFIX + "/page2");
    // [ts] cli("click", "e2");
    // [ts] const { output } = cli("go-back");
    // [ts] expect(output).toMatch("\\[\"beforeunload\" dialog.*\\]: can be handled by dialog-accept or dialog-dismiss");
    // [ts] cli("dialog-accept");
    // [ts] const { inlineSnapshot } = cli("snapshot");
    // [ts] expect_eq(std.mem.indexOf(u8, inlineSnapshot, "button \"Button 1\"") != null, true);
    // [ts] });
    // [ts] test("go-forward with dialog during navigation", () => {
    // [ts] server.setContent("/page1", `<title>Page 1</title><button id="btn1" onclick="window.addEventListener('beforeunload', (e) => {
    // [ts] e.preventDefault();
    // [ts] e.returnValue = '';
    // [ts] })">Button 1</button>`, 'text/html');
    // [ts] test("open without url opens about:blank", () => {
    // [ts] const { output } = cli("open");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "- Page URL: about:blank") != null, true);
    // [ts] });
    // [ts] test("open with bare 127.0.0.1 and port", () => {
    // [ts] const { output } = cli("open", "127.0.0.1:" + server.PORT);
    // [ts] expect_eq(std.mem.indexOf(u8, output, `- Page URL: http://127.0.0.1:${server.PORT}/`) != null, true);
    // [ts] });
    // [ts] test("open with bare localhost and port", () => {
    // [ts] const { output } = cli("open", "localhost:" + server.PORT);
    // [ts] expect_eq(std.mem.indexOf(u8, output, `- Page URL: http://localhost:${server.PORT}/`) != null, true);
    // [ts] });
    // [ts] test("tab-new with url", () => {
    // [ts] cli("open");
    // [ts] const { output } = cli("tab-new", server.HELLO_WORLD);
    // [ts] expect_eq(std.mem.indexOf(u8, output, `- 0: [](about:blank)`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, `- 1: (current) [Title](${server.HELLO_WORLD})`) != null, true);
    // [ts] });
    // [ts] test("tab-new with isolated context", () => {
    // [ts] cli("open");
    // [ts] const { output } = cli("tab-new", server.HELLO_WORLD, "--isolated-context=alice");
    // [ts] expect_eq(std.mem.indexOf(u8, output, `- 1: (current) [Title](${server.HELLO_WORLD}) [isolatedContext: alice]`) != null, true);
    // [ts] });
    // [ts] test("run-code", () => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { output } = cli("run-code", "() => page.title()");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "\"Title\"") != null, true);
    // [ts] });
}
