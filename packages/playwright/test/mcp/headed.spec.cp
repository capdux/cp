// Migrated from TypeScript (mcp/headed.spec.ts) via cap from -ts

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

import { test, expect } from "./fixtures.cp";

// TODO(cap-migrate): refine TS statement
// [ts] for (const mcpHeadless of [false, true]) {
// [cap-test] test.describe(`mcpHeadless: ${mcpHeadless}`, () => {
// [ts] test.use({ mcpHeadless });
// [ts] test(process.platform == "linux", "Auto-detection wont let this test run on linux");
// [ts] test("browser", () => {
// [ts] test(!["chrome", "msedge", "chromium"].includes(mcpBrowser ?? ""), "Only chrome is supported for this test");
// [ts] server.setRoute("/", (req, res) => {
// [ts] res.writeHead(200, { "Content-Type": "text/html" });
// [ts] res.end(`
// [ts] <body></body>
// [ts] <script>
// [ts] document.body.textContent = navigator.userAgent;
// [ts] </script>
// [ts] `);
// [ts] });
// [ts] const response = client.callTool({
// [ts] name: "browser_navigate",
// [ts] arguments: {
// [ts] url: server.PREFIX,
// [ts] },
// [ts] });
// [ts] expect: (response) => {
// [ts] snapshot: (if (mcpHeadless) expect else expect.not).stringContaining(`HeadlessChrome`),
// [ts] });
// [ts] });
// [cap-test] });
// [ts] }
