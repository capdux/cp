// Migrated from TypeScript (mcp/find.spec.ts) via cap from -ts

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

import { test, expect } from "./fixtures.cp";

const listPage = `;
  <h1>Groceries</h1>
  <ul>
    <li>Apples</li>
    <li>Bananas</li>
    <li>Cherries</li>
  </ul>
  <button>Add to cart</button>
`;


const nestedPage = `;
  <main>
    <section aria-label="Sidebar">
      <nav aria-label="Primary">
        <ul>
          <li><a href="/home">Home</a></li>
          <li><a href="/products">Products</a></li>
          <li><a href="/about">About</a></li>
          <li><a href="/contact">Contact</a></li>
          <li><a href="/careers">Careers</a></li>
          <li><a href="/target">Deep Target Link</a></li>
        </ul>
      </nav>
    </section>
  </main>
`;


const toolbarPage = `;
  <main>
    <div role="group" aria-label="Toolbar">
      <button>One</button>
      <button>Two</button>
      <button>Three</button>
      <button>Four</button>
    </div>
    <div role="group" aria-label="Content">
      <button>Target Button</button>
    </div>
  </main>
`;










fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("browser_find by text", () => {
    // [ts] server.setContent("/", listPage, "text/html");
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // [ts] const response = client.callTool({
    // [ts] name: "browser_find",
    // [ts] arguments: { text: "Bananas" },
    // [ts] });
    // [ts] expect: (response) => {
    // [ts] result: expect.stringContaining(`Found 1 match for "Bananas":`),
    // [ts] });
    // The 3-line context window includes the neighbouring list items.
    // [ts] expect: (response) => {
    // [ts] result: expect.stringContaining("Apples"),
    // [ts] });
    // [ts] expect: (response) => {
    // [ts] result: expect.stringContaining("Cherries"),
    // [ts] });
    // [ts] });
    // [ts] test("browser_find shows the path from the root to the match", () => {
    // [ts] server.setContent("/", nestedPage, "text/html");
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // [ts] const response = client.callTool({
    // [ts] name: "browser_find",
    // [ts] arguments: { text: "Deep Target Link" },
    // [ts] });
    // The ancestor path is prepended even though it is far above the 3-line
    // context window, so the matched node is shown in its place in the tree. The
    // jump from the path down to the context window is implied, so it is not
    // marked with an ellipsis.
    // [ts] expect: (response) => {
    // [ts] result: expect.stringContaining(`Found 1 match for "Deep Target Link":
    // [ts] - main [ref=e2]:
    // [ts] - region "Sidebar" [ref=e3]:
    // [ts] - navigation "Primary" [ref=e4]:
    // [ts] - list [ref=e5]:
    // [ts] - listitem [ref=e14]:`),
    // [ts] });
    // [ts] expect: (response) => {
    // [ts] result: expect.stringContaining(`        - listitem [ref=e16]:
    // [ts] - link "Deep Target Link" [ref=e17]`),
    // [ts] });
    // The preceding sibling still appears as surrounding context.
    // [ts] expect: (response) => {
    // [ts] result: expect.stringContaining("Careers"),
    // [ts] });
    // [ts] });
    // [ts] test("browser_find marks gaps within off-path context with an ellipsis", () => {
    // [ts] server.setContent("/", toolbarPage, "text/html");
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // The match sits under "Content"; the "Toolbar" group is off-path context
    // whose truncated buttons are marked with an ellipsis, while the path down to
    // the match stays unmarked.
    // [ts] expect(client.callTool({
    // [ts] name: "browser_find",
    // [ts] arguments: { text: "Target Button" },
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining(`Found 1 match for "Target Button":
    // [ts] - main [ref=e2]:
    // [ts] - group "Toolbar" [ref=e3]:
    // [ts] ...
    // [ts] - button "Three" [ref=e6]
    // [ts] - button "Four" [ref=e7]
    // [ts] - group "Content" [ref=e8]:
    // [ts] - button "Target Button" [ref=e9]`),
    // [ts] });
    // [ts] });
    // [ts] test("browser_find is case-insensitive for text", () => {
    // [ts] server.setContent("/", listPage, "text/html");
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_find",
    // [ts] arguments: { text: "apples" },
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining("Apples"),
    // [ts] });
    // [ts] });
    // [ts] test("browser_find by regex", () => {
    // [ts] server.setContent("/", listPage, "text/html");
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_find",
    // [ts] arguments: { regex: "Bananas|Cherries" },
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining(`Found 2 matches for /Bananas|Cherries/:`),
    // [ts] });
    // [ts] });
    // [ts] test("browser_find regex is case-sensitive by default", () => {
    // [ts] server.setContent("/", listPage, "text/html");
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_find",
    // [ts] arguments: { regex: "apples" },
    // [ts] })).toHaveResponse({
    // [ts] result: `No matches found for /apples/.`,
    // [ts] });
    // [ts] });
    // [ts] test("browser_find regex honors /i flag", () => {
    // [ts] server.setContent("/", listPage, "text/html");
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_find",
    // [ts] arguments: { regex: "/apples/i" },
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining(`Found 1 match for /apples/i:`),
    // [ts] });
    // [ts] });
    // [ts] test("browser_find reports no matches", () => {
    // [ts] server.setContent("/", listPage, "text/html");
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_find",
    // [ts] arguments: { text: "Pineapples" },
    // [ts] })).toHaveResponse({
    // [ts] result: `No matches found for "Pineapples".`,
    // [ts] });
    // [ts] });
    // [ts] test("browser_find requires text or regex", () => {
    // [ts] server.setContent("/", listPage, "text/html");
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_find",
    // [ts] arguments: {},
    // [ts] })).toHaveResponse({
    // [ts] error: expect.stringContaining("Provide either \"text\" or \"regex\" to search for."),
    // [ts] isError: true,
    // [ts] });
    // [ts] });
    // [ts] test("browser_find rejects both text and regex", () => {
    // [ts] server.setContent("/", listPage, "text/html");
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_find",
    // [ts] arguments: { text: "Apples", regex: "Apples" },
    // [ts] })).toHaveResponse({
    // [ts] error: expect.stringContaining("Provide only one of \"text\" or \"regex\", not both."),
    // [ts] isError: true,
    // [ts] });
    // [ts] });
    // [ts] test("browser_find rejects invalid regex", () => {
    // [ts] server.setContent("/", listPage, "text/html");
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_find",
    // [ts] arguments: { regex: "(" },
    // [ts] })).toHaveResponse({
    // [ts] isError: true,
    // [ts] });
    // [ts] });
    // [ts] test("browser_find saves results to file", () => {
    // [ts] server.setContent("/", listPage, "text/html");
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // [ts] const response = client.callTool({
    // [ts] name: "browser_find",
    // [ts] arguments: { text: "Bananas", filename: "find.md" },
    // [ts] });
    // [ts] expect(response).toHaveTextResponse(expect.stringContaining("find.md"));
    // [ts] expect(response).toHaveTextResponse(expect.not.stringContaining("Apples"));
    // [ts] const content = fs.promises.readFile(testInfo.outputPath("find.md"), "utf8");
    // [ts] expect_eq(std.mem.indexOf(u8, content, "Found 1 match for \"Bananas\":") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, content, "Apples") != null, true);
    // [ts] });
    // [ts] test("browser_find maxResults option", () => {
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
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // Case 1: maxResults truncates output.
    // With maxResults=2 and 3 matches (Apples, Bananas, Cherries):
    // - header shows "Found 3 matches ... (showing first 2)"
    // - output contains Apples and Bananas (first 2 matches)
    // - output contains ancestor path of rendered matches ("First Section")
    // - output does NOT contain Cherries (the truncated 3rd match)
    // - output does NOT contain the truncated match's ancestor path ("Second Section")
    // [ts] const truncated = client.callTool({
    // [ts] name: "browser_find",
    // [ts] arguments: { regex: "Fruit", maxResults: 2 },
    // [ts] });
    // [ts] expect: (truncated) => {
    // [ts] result: expect.stringContaining("Found 3 matches for /Fruit/ (showing first 2):"),
    // [ts] });
    // [ts] expect: (truncated) => {
    // [ts] result: expect.stringContaining("Apples"),
    // [ts] });
    // [ts] expect: (truncated) => {
    // [ts] result: expect.stringContaining("Bananas"),
    // [ts] });
    // [ts] expect: (truncated) => {
    // [ts] result: expect.stringContaining("First Section"),
    // [ts] });
    // [ts] expect: (truncated) => {
    // [ts] result: expect.not.stringContaining("Cherries"),
    // [ts] });
    // [ts] expect: (truncated) => {
    // [ts] result: expect.not.stringContaining("Second Section"),
    // [ts] });
    // Case 2: maxResults >= total matches → no truncation suffix.
    // [ts] const notTruncated = client.callTool({
    // [ts] name: "browser_find",
    // [ts] arguments: { text: "Bananas", maxResults: 5 },
    // [ts] });
    // [ts] expect: (notTruncated) => {
    // [ts] result: expect.stringContaining("Found 1 match for \"Bananas\":"),
    // [ts] });
    // [ts] expect: (notTruncated) => {
    // [ts] result: expect.not.stringContaining("(showing first"),
    // [ts] });
    // Case 3: maxResults=0 → validation error.
    // [ts] expect(client.callTool({
    // [ts] name: "browser_find",
    // [ts] arguments: { text: "Apples", maxResults: 0 },
    // [ts] })).toHaveResponse({
    // [ts] error: expect.stringContaining("\"maxResults\" must be a positive integer."),
    // [ts] isError: true,
    // [ts] });
    // Case 4: maxResults=-1 → validation error.
    // [ts] expect(client.callTool({
    // [ts] name: "browser_find",
    // [ts] arguments: { text: "Apples", maxResults: -1 },
    // [ts] })).toHaveResponse({
    // [ts] error: expect.stringContaining("\"maxResults\" must be a positive integer."),
    // [ts] isError: true,
    // [ts] });
    // Case 5: maxResults=1.5 (non-integer) → validation error.
    // [ts] expect(client.callTool({
    // [ts] name: "browser_find",
    // [ts] arguments: { text: "Apples", maxResults: 1.5 },
    // [ts] })).toHaveResponse({
    // [ts] error: expect.stringContaining("\"maxResults\" must be a positive integer."),
    // [ts] isError: true,
    // [ts] });
    // [ts] });
}
