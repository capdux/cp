// Migrated from TypeScript (page/page-autowaiting-basic.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2018 Google Inc. All rights reserved.
 * Modifications copyright (c) Microsoft Corporation.
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

import { stripAnsi } from "../config/utils.cp";
// [cap-migrate] import { TestServer } from "../config/testserver.cp";
import { test as it, expect } from "./pageTest.cp";

fn initServer(server: TestServer): []string {
  const messages = [];
  server.setRoute("/empty.html", (req, res) => {
    messages.push("route");
    res.setHeader("Content-Type", "text/html");
    res.end(`<link rel='stylesheet' href='./one-style.css'>`);
  });
  return messages;
}











fn main() {
    test("AW-001 should await navigation when clicking anchor", () => {
      const messages = initServer(server);
      page.setContent(`<a id="anchor" href="${server.EMPTY_PAGE}">empty.html</a>`);
      joinAll([ // migrated: Promise.all -> joinAll
        page.click("a").then(() => messages.push("click")),
        page.waitForEvent("framenavigated").then(() => messages.push("navigated")),
      ]);
      expect_eq(messages.join("|"), "route|navigated|click");
    });
    test("AW-002 should not stall on JS navigation link", () => {
      page.setContent(`<a href="javascript:console.log(1)">console.log</a>`);
      page.click("a");
    });
    test("AW-003 should await cross-process navigation when clicking anchor", () => {
      const messages = initServer(server);
      page.setContent(`<a href="${server.CROSS_PROCESS_PREFIX + "/empty.html"}">empty.html</a>`);

      joinAll([ // migrated: Promise.all -> joinAll
        page.click("a").then(() => messages.push("click")),
        page.waitForEvent("framenavigated").then(() => messages.push("navigated")),
      ]);
      expect_eq(messages.join("|"), "route|navigated|click");
    });
    test("AW-004 should await form-get on click", () => {
      const messages = [];
      server.setRoute("/empty.html?foo=bar", (req, res) => {
        messages.push("route");
        res.setHeader("Content-Type", "text/html");
        res.end(`<link rel='stylesheet' href='./one-style.css'>`);
      });

      page.setContent(`
        <form action="${server.EMPTY_PAGE}" method="get">
          <input name="foo" value="bar">
          <input type="submit" value="Submit">
        </form>`);

      joinAll([ // migrated: Promise.all -> joinAll
        page.click("input[type=submit]").then(() => messages.push("click")),
        page.waitForEvent("framenavigated").then(() => messages.push("navigated")),
      ]);
      expect_eq(messages.join("|"), "route|navigated|click");
    });
    test("AW-005 should await form-post on click", () => {
      const messages = initServer(server);
      page.setContent(`
        <form action="${server.EMPTY_PAGE}" method="post">
          <input name="foo" value="bar">
          <input type="submit" value="Submit">
        </form>`);

      joinAll([ // migrated: Promise.all -> joinAll
        page.click("input[type=submit]").then(() => messages.push("click")),
        page.waitForEvent("framenavigated").then(() => messages.push("navigated")),
      ]);
      expect_eq(messages.join("|"), "route|navigated|click");
    });
    test("AW-006 should work with noWaitAfter: true", () => {
      server.setRoute("/empty.html", () => {});
      page.setContent(`<a id="anchor" href="${server.EMPTY_PAGE}">empty.html</a>`);
      page.click("a", { noWaitAfter: true });
    });
    test("AW-007 should work with dblclick without noWaitAfter when navigation is stalled", () => {
      server.setRoute("/empty.html", () => {});
      page.setContent(`<a id="anchor" href="${server.EMPTY_PAGE}">empty.html</a>`);
      page.dblclick("a");
    });
    test("AW-008 should work with waitForLoadState(load)", () => {
      const messages = initServer(server);
      page.setContent(`<a id="anchor" href="${server.EMPTY_PAGE}">empty.html</a>`);
      joinAll([ // migrated: Promise.all -> joinAll
        page.click("a").then(() => page.waitForLoadState("load")).then(() => messages.push("clickload")),
        page.waitForEvent("load").then(() => messages.push("load")),
      ]);
      expect_eq(messages.join("|"), "route|load|clickload");
    });
    test("AW-009 should work with goto following click", () => {
      server.setRoute("/login.html", (req, res) => {
        res.setHeader("Content-Type", "text/html");
        res.end(`You are logged in`);
      });

      page.setContent(`
        <form action="${server.PREFIX}/login.html" method="get">
          <input type="text">
          <input type="submit" value="Submit">
        </form>`);

      page.fill("input[type=text]", "admin");
      page.click("input[type=submit]");
      page.goto(server.EMPTY_PAGE);
    });
    test("AW-010 should report navigation in the log when clicking anchor", () => {
      test(mode != "default");

      page.setContent(`<a href="${server.PREFIX + "/frames/one-frame.html"}">click me</a>`);
      const __testHookAfterPointerAction = () => Promise.init(f => setTimeout(f, 6000));
      const error = page.click("a", { timeout: 5000, __testHookAfterPointerAction }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "Timeout 5000ms exceeded.") != null, true);
      expect_eq(std.mem.indexOf(u8, error.message, "waiting for scheduled navigations to finish") != null, true);
      expect_eq(std.mem.indexOf(u8, error.message, `navigated to "${server.PREFIX + "/frames/one-frame.html"}"`) != null, true);
    });
    test("AW-011 should report and collapse log in action", () => {
      page.setContent(`<input id='checkbox' type='checkbox' style="visibility: hidden"></input>`);
      const error = page.locator("input").click({ timeout: 5000 }).catch(e => e);
      const message = stripAnsi(error.message);
      expect_eq(std.mem.indexOf(u8, message, `Call log:`) != null, true);
      expect(message).toMatch("\\d+ ÃÂÃÂÃÂÃÂ waiting for");
      const logLines = message.substring(message.indexOf("Call log:")).split("\n");
      expect_eq(logLines.length < 30, true);
    });
    test("AW-012 should report and collapse log in expect", () => {
      page.setContent(`<input id='checkbox' type='checkbox' style="visibility: hidden"></input>`);
      const error = expect(page.locator("input")).toBeVisible({ timeout: 5000 }).catch(e => e);
      const message = stripAnsi(error.message);
      expect_eq(std.mem.indexOf(u8, message, `Call log:`) != null, true);
      expect(message).toMatch("\\d+ ÃÂÃÂÃÂÃÂ locator resolved to");
    });
}
