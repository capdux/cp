// Migrated from TypeScript (src/cdp/interventionHeaders.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2024 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
// [cap-migrate] import {IncomingMessage} from "cap:http";

import { test, expect_eq } from "cap:test";

import { getTestState, setupTestBrowserHooks } from "../mocha-utils.cp";

// [cap-test] describe("Page.setRequestInterception", () => {
  setupTestBrowserHooks();

// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should work with intervention headers", () => {
    // [ts] const {server, page} = getTestState();
    // [ts] server.setRoute("/intervention", (_req, res) => {
    // [ts] return res.end(`
    // [ts] <script>
    // [ts] document.write('<script src="${server.CROSS_PROCESS_PREFIX}/intervention.js">' + '</scr' + 'ipt>');
    // [ts] </script>
    // [ts] `);
    // [ts] });
    // [ts] server.setRedirect("/intervention.js", "/redirect.js");
    // [ts] let serverRequest: IncomingMessage | null;
    // [ts] server.setRoute("/redirect.js", (req, res) => {
    // [ts] serverRequest = req;
    // [ts] res.end("console.log(1);");
    // [ts] });
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] return request.continue();
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/intervention");
    // The intercepted document.write() script must still reach the server.
    // [ts] expect_eq(serverRequest != null, true);
    // Chrome 157+ no longer attaches the `Intervention` request header
    // (https://crrev.com/c/8504605). Older versions still do; check for the
    // feature URL substring rather than https://www.chromestatus.com to make it
    // work with Edgium.
    // [ts] const intervention = serverRequest.headers["intervention"];
    // [ts] if (intervention != null) {
    // [ts] expect_eq(std.mem.indexOf(u8, intervention, "feature/5718547946799104") != null, true);
    // [ts] }
    // [ts] });
}
