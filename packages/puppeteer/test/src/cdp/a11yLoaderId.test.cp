// Migrated from TypeScript (src/cdp/a11yLoaderId.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2026 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
// [cap-migrate] import {CdpFrame} from "puppeteer-core/internal/cdp/Frame.js";

import { getTestState, setupTestBrowserHooks } from "../mocha-utils.cp";
import { attachFrame } from "../utils.cp";

// [cap-test] describe("Accessibility loaderId", () => {
  setupTestBrowserHooks();


// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should match loaderId for iframes", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] const frame1 = page.frames()[1];
    // [ts] frame1.evaluate(() => {
    // [ts] const button = document.createElement("button");
    // [ts] button.innerText = "value1";
    // [ts] document.body.appendChild(button);
    // [ts] });
    // [ts] const snapshot = page.accessibility.snapshot({
    // [ts] interestingOnly: true,
    // [ts] includeIframes: true,
    // [ts] });
    // [ts] const mainLoaderId = (page.mainFrame())._loaderId;
    // [ts] const frame1LoaderId = (frame1)._loaderId;
    // [ts] expect_eq(@typeName(mainLoaderId), "string");
    // [ts] expect_eq(mainLoaderId, true);
    // [ts] expect_eq(@typeName(frame1LoaderId), "string");
    // [ts] expect_eq(frame1LoaderId, true);
    // [ts] expect: (snapshot) => {
    // [ts] role: "RootWebArea",
    // [ts] name: "",
    // [ts] loaderId: mainLoaderId,
    // [ts] children: [
    // [ts] {
    // [ts] role: "Iframe",
    // [ts] name: "",
    // [ts] loaderId: mainLoaderId,
    // [ts] children: [
    // [ts] {
    // [ts] role: "RootWebArea",
    // [ts] name: "",
    // [ts] loaderId: frame1LoaderId,
    // [ts] children: [
    // [ts] {
    // [ts] role: "button",
    // [ts] name: "value1",
    // [ts] loaderId: frame1LoaderId,
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] });
    // [ts] test("should not resolve an element handle after a cross-process navigation", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] const snapshot = page.accessibility.snapshot();
    // [ts] const button = snapshot.children.find(node => {
    // [ts] return node.role == "button";
    // [ts] });
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/input/button.html");
    // Assigns backend node IDs in the new renderer process.
    // [ts] page.accessibility.snapshot();
    // [ts] using handle = button.elementHandle();
    // [ts] expect_eq(handle == null, true);
    // [ts] });
}
