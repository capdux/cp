// Migrated from TypeScript (mcp/snapshot-mode.spec.ts) via cap from -ts

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
import { test, expect } from "./fixtures.cp";


  expect(client.callTool({
    name: "browser_snapshot",
  })).toHaveResponse({
    inlineSnapshot: expect.stringContaining(`- button "Button 1" [ref=e2]
  - button "Button 2" [ref=e3]`),
  });
// TODO(cap-migrate): refine TS statement
// [ts] });



fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should respect --snapshot-mode=full", () => {
    // [ts] server.setContent("/", `<button>Button 1</button>`, "text/html");
    // [ts] const { client } = startClient({
    // [ts] args: ["--snapshot-mode=full"],
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: {
    // [ts] url: server.PREFIX,
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`- button "Button 1" [ref=e2]`),
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: {
    // [ts] function: `async () => {
    // [ts] const button2 = document.createElement('button');
    // [ts] button2.textContent = 'Button 2';
    // [ts] document.body.appendChild(button2);
    // [ts] }`,
    // [ts] },
    // [ts] });
    // [ts] test("should respect --snapshot-mode=none", () => {
    // [ts] server.setContent("/", `<button>Button 1</button>`, "text/html");
    // [ts] const { client } = startClient({
    // [ts] args: ["--snapshot-mode=none"],
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: {
    // [ts] url: server.PREFIX,
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] page: `- Page URL: ${server.PREFIX}/`,
    // [ts] });
    // [ts] });
    // [ts] test("should not inline console messages with --snapshot-mode=none", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Tab one</title>
    // [ts] <body>
    // [ts] <button>Click me</button>
    // [ts] <script>
    // [ts] console.log('info message');
    // [ts] console.error('error message');
    // [ts] </script>
    // [ts] </body>
    // [ts] `, 'text/html');
    // [ts] const { client } = startClient({
    // [ts] args: ["--snapshot-mode=none"],
    // [ts] });
    // [ts] const response = client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect: (response) => {
    // [ts] events: expect.stringContaining("error message"),
    // [ts] });
    // [ts] });
    // [ts] test("should respect --snapshot-boxes", () => {
    // [ts] server.setContent("/", `
    // [ts] <style>body { margin: 0; }</style>
    // [ts] <button style="position:absolute;left:100px;top:50px;width:80px;height:40px;margin:0;padding:0;border:0;">click</button>
    // [ts] `, 'text/html');
    // [ts] const { client } = startClient({
    // [ts] args: ["--snapshot-boxes"],
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: {
    // [ts] url: server.PREFIX,
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`- button "click" [ref=e1] [box=100,50,80,40]`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] })).toHaveResponse({
    // [ts] inlineSnapshot: expect.stringContaining(`- button "click" [ref=e1] [box=100,50,80,40]`),
    // [ts] });
    // [ts] });
    // [ts] test("should respect snapshot[filename]", () => {
    // [ts] server.setContent("/", `<button>Button 1</button>`, "text/html");
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: {
    // [ts] url: server.PREFIX,
    // [ts] },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] arguments: {
    // [ts] filename: "snapshot1.yml",
    // [ts] },
    // [ts] })).toHaveTextResponse(expect.stringContaining("snapshot1.yml"));
    // [ts] expect_eq(std.mem.indexOf(u8, fs.promises.readFile(path.join(testInfo.outputPath(), "snapshot1.yml"), "utf8"), `- button "Button 1" [ref=e2]`) != null, true);
    // [ts] });
}
