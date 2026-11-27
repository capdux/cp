// Migrated from TypeScript (mcp/cli-find.spec.ts) via cap from -ts

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

import { test, expect } from "./cli-fixtures.cp";

const listPage = `;
  <h1>Groceries</h1>
  <ul>
    <li>Apples</li>
    <li>Bananas</li>
    <li>Cherries</li>
  </ul>
`;







fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("find by text", () => {
    // [ts] server.setContent("/", listPage, "text/html");
    // [ts] cli("open", server.PREFIX);
    // [ts] const { output } = cli("find", "Bananas");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Found 1 match for \"Bananas\":") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Apples") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Cherries") != null, true);
    // [ts] });
    // [ts] test("find by regex", () => {
    // [ts] server.setContent("/", listPage, "text/html");
    // [ts] cli("open", server.PREFIX);
    // [ts] const { output } = cli("find", "--regex=Bananas|Cherries");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Found 2 matches for /Bananas|Cherries/:") != null, true);
    // [ts] });
    // [ts] test("find by regex with /i flag", () => {
    // [ts] server.setContent("/", listPage, "text/html");
    // [ts] cli("open", server.PREFIX);
    // [ts] const { output } = cli("find", "--regex=\"apples\"");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Found 1 match for /apples/i:") != null, true);
    // [ts] });
    // [ts] test("find reports no matches", () => {
    // [ts] server.setContent("/", listPage, "text/html");
    // [ts] cli("open", server.PREFIX);
    // [ts] const { output } = cli("find", "Pineapples");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "No matches found for \"Pineapples\".") != null, true);
    // [ts] });
    // [ts] test("find --filename", () => {
    // [ts] server.setContent("/", listPage, "text/html");
    // [ts] cli("open", server.PREFIX);
    // [ts] const { output } = cli("find", "Bananas", "--filename=find.md");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "[Find results](./find.md)") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Apples") == null, true);
    // [ts] const content = fs.readFileSync(testInfo.outputPath("find.md"), "utf-8");
    // [ts] expect_eq(std.mem.indexOf(u8, content, "Found 1 match for \"Bananas\":") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, content, "Apples") != null, true);
    // [ts] });
    // [ts] test("find --max-results truncates output", () => {
    // [ts] server.setContent("/", `
    // [ts] <section aria-label="First Section">
    // [ts] <ul>
    // [ts] <li>Fruit: Apples</li>
    // [ts] <li>Fruit: Bananas</li>
    // [ts] <li>Spacer 1</li>
    // [ts] <li>Spacer 2</li>
    // [ts] <li>Spacer 3</li>
    // [ts] <li>Spacer 4</li>
    // [ts] </ul>
    // [ts] </section>
    // [ts] <section aria-label="Second Section">
    // [ts] <ul>
    // [ts] <li>Fruit: Cherries</li>
    // [ts] </ul>
    // [ts] </section>
    // [ts] `, 'text/html');
    // [ts] cli("open", server.PREFIX);
    // With 3 matches and --max-results=2: header shows truncation notice,
    // output contains first 2 matches and their ancestor, and omits the 3rd match and its ancestor.
    // [ts] const { output } = cli("find", "--regex=Fruit", "--max-results=2");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Found 3 matches for /Fruit/ (showing first 2):") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Apples") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Bananas") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "First Section") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Cherries") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Second Section") == null, true);
    // [ts] });
    // [ts] test("find --max-results exceeding match count shows all results", () => {
    // [ts] server.setContent("/", listPage, "text/html");
    // [ts] cli("open", server.PREFIX);
    // With --max-results exceeding the total match count, no truncation suffix appears.
    // [ts] const { output } = cli("find", "Bananas", "--max-results=10");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Found 1 match for \"Bananas\":") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "(showing first") == null, true);
    // [ts] });
    // [ts] test("find --max-results=0 is rejected", () => {
    // [ts] server.setContent("/", listPage, "text/html");
    // [ts] cli("open", server.PREFIX);
    // [ts] const { output } = cli("find", "Apples", "--max-results=0");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "\"maxResults\" must be a positive integer.") != null, true);
    // [ts] });
}
