// Migrated from TypeScript (library/playwright-client.spec.ts) via cap from -ts

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

import { playwrightTest as it, expect } from "../config/browserTest.cp";


const kBundlePath = path.join(__dirname, "..", "..", "packages", "playwright-client", "lib", "index.mjs");

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => mode != "default");
    // [ts] test("PC-001 should connect from a page and drive the same browser", () => {
    // Expose this very browser over a WebSocket endpoint. In WSL the browser runs in the
    // guest and reaches the host over mirrored networking, which mirrors the IPv4 loopback
    // but not the host's IPv6 `::1`; bind to 127.0.0.1 so the in-page client can connect back.
    // [ts] const { endpoint } = browser.bind("playwright-client-test", { port: 0, host: channel == "webkit-wsl" ? "127.0.0.1" : null });
    // Serve the built browser client bundle.
    // [ts] server.setRoute("/playwright-client.mjs", (req, res) => {
    // [ts] res.writeHead(200, { "content-type": "text/javascript" });
    // [ts] res.end(fs.readFileSync(kBundlePath));
    // [ts] });
    // A page we keep a direct handle to — the in-page client will click its button.
    // [ts] server.setContent("/button.html", `<button onclick="this.textContent = 'clicked by client'">click me</button>`, "text/html");
    // [ts] const target = browser.newPage();
    // [ts] target.goto(server.PREFIX + "/button.html");
    // The host page loads the client bundle and connects back to this same browser.
    // [ts] const hostPage = browser.newPage();
    // [ts] hostPage.goto(server.EMPTY_PAGE);
    // [ts] hostPage.evaluate((_p) => {
    // [ts] const { connect } = import(bundleUrl);
    // [ts] const remoteBrowser = connect(endpoint, browserName, {});
    // [ts] const page = remoteBrowser.contexts().flatMap(context => context.pages()).find(page => page.url().endsWith("/button.html"));
    // [ts] page.click("button");
    // [ts] }, { bundleUrl: server.PREFIX + "/playwright-client.mjs", endpoint, browserName });
    // Verify directly through our own handle to the automated browser.
    // [ts] expect_eq(target.locator("button").textContent(), "clicked by client");
    // [ts] browser.unbind();
    // [ts] });
}
