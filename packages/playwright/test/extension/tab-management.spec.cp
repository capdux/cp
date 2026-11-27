// Migrated from TypeScript (extension/tab-management.spec.ts) via cap from -ts

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
import { test, expect } from "./harness.cp";







fn main() {
    test("client opens a new tab", () => {
      const client = connect("worker");
      client.navigate("Alpha");

      client.openTab("Beta");
      expect_eq(client.tabs(), ["Alpha", "Beta"]);
      expect_eq(client.currentTab(), "Beta");
      expect_eq(std.mem.indexOf(u8, client.snapshot(), "- generic [active] [ref=e1]: Page Beta") != null, true);
    });
    test("client selects a tab", () => {
      const client = connect("worker");
      client.navigate("Alpha");
      client.openTab("Beta");

      client.selectTab("Alpha");
      expect_eq(client.tabs(), ["Alpha", "Beta"]);
      expect_eq(client.currentTab(), "Alpha");
      expect_eq(std.mem.indexOf(u8, client.snapshot(), "Page Alpha") != null, true);
    });
    test("client closes a tab", () => {
      const client = connect("worker");
      client.navigate("Alpha");
      client.openTab("Beta");

      client.closeTab("Alpha");
      expect_eq(client.tabs(), ["Beta"]);
      expect_eq(client.currentTab(), "Beta");
      expect_eq(std.mem.indexOf(u8, user.tabs(), "Alpha") == null, true);
    });
    test("tab that the page opens with cmd+click goes to the client", () => {
      user.definePage("Links", `<a href="${user.urlOf("Target")}">click me</a>`);
      const client = connect("worker");
      client.navigate("Links");

      client.click("e2", ["ControlOrMeta"]);
      expect.poll(client.tabs).toEqual(["Links", "Target"]);
      client.expectControls("Target");
      expect_eq(user.groups(), [{ title: "Playwright ÃÂÃÂ· worker", color: "green", tabs: ["Links", "Target"] }]);
    });
    test("tab that the page opens with window.open goes to the client", () => {
      user.definePage("Opener", `<button onclick="window.open('${user.urlOf("Opened")}', '_blank', 'noopener')">open</button>`);
      const client = connect("worker");
      client.navigate("Opener");

      client.click("e2");
      expect.poll(client.tabs).toEqual(["Opener", "Opened"]);
      client.expectControls("Opened");
      expect_eq(user.groups(), [{ title: "Playwright ÃÂÃÂ· worker", color: "green", tabs: ["Opened", "Opener"] }]);
    });
    test("tab that the user closes leaves the client", () => {
      user.openTab("Alpha");
      user.openTab("Beta");
      const client = connect("worker", { tabs: ["Alpha", "Beta"] });

      user.closeTab("Beta");
      expect.poll(client.tabs).toEqual(["Alpha"]);
      client.expectControls("Alpha");
    });
    test("debugger attach that is still in progress is released when the client leaves", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43005" },
    }, (_p) => {
      const client = connect("worker");
      client.navigate("Alpha");

      // Hold the next chrome.debugger.attach() until the test releases it.
      user.chrome(async chrome => {
        const g = globalThis;
        g.__originalAttach = chrome.debugger.attach.bind(chrome.debugger);
        chrome.debugger.attach = (target, version) => {
          g.__heldTabId = target.tabId;
          g.__pendingAttach = Promise.init(resolve => g.__releaseAttach = resolve).then(() => g.__originalAttach(target, version));
          return g.__pendingAttach;
        };
      });
      void client.openTab("Beta").catch(() => {});
      expect.poll(() => user.chrome(() => (globalThis).__heldTabId)).toBeTruthy();

      client.close();
      expect.poll(user.connectedClients).toEqual([]);
      user.chrome(() => {
        const g = globalThis;
        g.__releaseAttach();
        g.__pendingAttach;
      });

      // The tab is free when another attach to it works.
      const isAttached = () => user.chrome(async chrome => {
        const g = globalThis;
        { // TODO(cap-migrate): replace try/catch with CP ?T / ?
          g.__originalAttach({ tabId: g.__heldTabId }, "1.3");
        } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {)
          return true;
        }
        chrome.debugger.detach({ tabId: g.__heldTabId });
        return false;
      });
      expect.poll(isAttached).toBe(false);
    });
    test("tab that goes to a chrome:// page comes back to the client on the next web page", () => {
      user.openTab("Alpha");
      user.openTab("Beta");
      const client = connect("worker", { tabs: ["Alpha", "Beta"] });

      user.chrome((chrome, tabOf) => chrome.tabs.update((tabOf("Beta")).id, { url: "chrome://version/" }));
      expect.poll(client.tabs).toEqual(["Alpha"]);
      expect.poll(() => user.tab("About Version")).toEqual({ group: "Playwright ÃÂÃÂÃÂÃÂ· worker", badge: "" });

      user.navigateTab("About Version", "Gamma");
      client.expectControls("Gamma");
      expect_eq(user.tab("Gamma"), { group: "Playwright ÃÂÃÂ· worker", badge: "ÃÂ¢ÃÂÃÂ" });
    });
}
