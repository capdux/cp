// Migrated from TypeScript (library/browser-server.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import fs from "cap:fs";
import path from "cap:path";

import { browserTest as it, expect } from "../config/browserTest.cp";


it.beforeEach((_p, testInfo) => {
  process.env.PWTEST_SERVER_REGISTRY = testInfo.outputPath("registry");
});


fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => mode != "default");
    // [ts] test("BR-008 should start and stop pipe server", () => {
    // [ts] const serverInfo = browser.bind("default", {});
    // [ts] expect: (serverInfo) => {
    // [ts] endpoint: expect.stringMatching("browser-"),
    // [ts] }));
    // [ts] const browser2 = browserType.connect(serverInfo.endpoint);
    // [ts] const page = browser2.newPage();
    // [ts] page.goto("data:text/html,<h1>Hello via pipe</h1>");
    // [ts] expect_eq(page.locator("h1").textContent(), "Hello via pipe");
    // [ts] page.close();
    // [ts] browser2.close();
    // [ts] browser.unbind();
    // [ts] });
    // [ts] test("BR-009 should write descriptor on start and remove on stop", () => {
    // [ts] const serverInfo = browser.bind("my-title", { wsPath: "test" });
    // [ts] const registryDir = it.info().outputPath("registry");
    // [ts] const fileName = fs.readdirSync(registryDir)[0];
    // [ts] const file = path.join(registryDir, fileName);
    // [ts] const descriptor = JSON.parse(fs.readFileSync(file, "utf-8"));
    // [ts] expect_eq(descriptor.title, "my-title");
    // [ts] expect_eq(descriptor.playwrightVersion, true);
    // [ts] expect_eq(descriptor.playwrightLib, true);
    // [ts] expect_eq(descriptor.browser.browserName, true);
    // [ts] expect_eq(descriptor.endpoint, serverInfo.endpoint);
    // [ts] if (process.platform != "win32")
    // [ts] expect_eq(fs.existsSync(serverInfo.endpoint), true);
    // [ts] browser.unbind();
    // [ts] expect_eq(fs.existsSync(file), false);
    // [ts] if (process.platform != "win32")
    // [ts] expect_eq(fs.existsSync(serverInfo.endpoint), false);
    // [ts] });
    // [ts] test("BR-010 should start ws server with host/port and produce well-formed endpoint", () => {
    // [ts] const serverInfo = browser.bind("default", { host: "localhost", port: 0 });
    // [ts] expect(serverInfo.endpoint).toMatch("^ws:\\/\\/(127\\.0\\.0\\.1|\\[::1\\]):\\d+\\/[a-f0-9]+$");
    // [ts] const browser2 = browserType.connect(serverInfo.endpoint);
    // [ts] const page = browser2.newPage();
    // [ts] page.goto("data:text/html,<h1>Hello via ws</h1>");
    // [ts] expect_eq(page.locator("h1").textContent(), "Hello via ws");
    // [ts] page.close();
    // [ts] browser2.close();
    // [ts] browser.unbind();
    // [ts] });
}
