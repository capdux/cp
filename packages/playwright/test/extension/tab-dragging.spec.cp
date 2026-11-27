// Migrated from TypeScript (extension/tab-dragging.spec.ts) via cap from -ts

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

// The user drags tabs. For the extension, a drag is a change of the tab group of a tab.










fn main() {
    test("free tab dragged into the group of a client, and out of it", () => {
      user.openTab("Alpha");
      user.openTab("Beta");
      const client = connect("worker", { tabs: ["Alpha"] });
      expect_eq(user.tab("Beta"), { group: null, badge: "" });

      user.dragIntoGroupOf("Beta", "Alpha");
      client.expectControls("Beta");
      expect_eq(user.tab("Beta"), { group: "Playwright ÃÂÃÂ· worker", badge: "ÃÂ¢ÃÂÃÂ" });

      user.dragOutOfGroup("Beta");
      expect.poll(client.tabs).toEqual(["Alpha"]);
      expect.poll(() => user.tab("Beta")).toEqual({ group: null, badge: "" });
    });
    test("chrome:// tab dragged into the group of a client waits for a page that can be controlled", () => {
      user.openTab("Alpha");
      const client = connect("worker", { tabs: ["Alpha"] });
      user.openUrl("chrome://version/", "About Version");

      user.dragIntoGroupOf("About Version", "Alpha");
      expect.poll(() => user.tab("About Version")).toEqual({ group: "Playwright ÃÂÃÂÃÂÃÂ· worker", badge: "" });
      expect_eq(client.tabs(), ["Alpha"]);

      user.navigateTab("About Version", "Beta");
      client.expectControls("Beta");
      expect_eq(user.tab("Beta"), { group: "Playwright ÃÂÃÂ· worker", badge: "ÃÂ¢ÃÂÃÂ" });
    });
    test("tab dragged from a group of the user into the group of a client", () => {
      user.openTab("Alpha");
      user.openTab("Beta");
      user.openTab("Gamma");
      user.dragIntoNewGroup(["Beta", "Gamma"], "Work");
      const client = connect("worker", { tabs: ["Alpha"] });

      user.dragIntoGroupOf("Beta", "Alpha");
      client.expectControls("Beta");
      expect_eq(client.tabs(), ["Alpha", "Beta"]);
      expect(user.groups()).toEqual([
        { title: "Playwright ÃÂ· worker", color: "green", tabs: ["Alpha", "Beta"] },
        { title: "Work", color: "red", tabs: ["Gamma"] },
      ]);
    });
    test("tab dragged from the group of a client into a group of the user", () => {
      user.openTab("Alpha");
      user.openTab("Beta");
      user.openTab("Gamma");
      user.dragIntoNewGroup("Gamma", "Work");
      const first = connect("first", { tabs: ["Alpha", "Beta"] });

      user.dragIntoGroupOf("Beta", "Gamma");
      expect.poll(first.tabs).toEqual(["Alpha"]);
      expect(user.groups()).toEqual([
        { title: "Playwright ÃÂ· first", color: "green", tabs: ["Alpha"] },
        { title: "Work", color: "red", tabs: ["Beta", "Gamma"] },
      ]);

      // The group of the user is free, so the next client can take it with the tab in it.
      const second = connect("second", {
        offered: ["Work", "about:blank", "Beta", "Gamma"],
        group: "Work",
      });
      second.expectControls("Beta");
      second.expectControls("Gamma");
      expect.poll(user.groups).toEqual([
        { title: "Playwright ÃÂ· first", color: "green", tabs: ["Alpha"] },
        { title: "Work", color: "blue", tabs: ["Beta", "Gamma"] },
      ]);
    });
    test("tab dragged from the group of a client into a new group", () => {
      user.openTab("Alpha");
      user.openTab("Beta");
      const client = connect("worker", { tabs: ["Alpha", "Beta"] });

      user.dragIntoNewGroup("Beta", "Mine");
      expect.poll(client.tabs).toEqual(["Alpha"]);
      expect_eq(user.tab("Beta"), { group: "Mine", badge: "" });
      expect(user.groups()).toEqual([
        { title: "Playwright ÃÂ· worker", color: "green", tabs: ["Alpha"] },
        { title: "Mine", color: "red", tabs: ["Beta"] },
      ]);
    });
    test("tabs dragged into a group that a client left go to the client that takes the group", () => {
      user.openTab("Alpha");
      user.openTab("Beta");
      user.openTab("Delta");
      const first = connect("first", { tabs: ["Alpha", "Beta"] });
      const gone = connectWithToken("gone");
      gone.navigate("Gamma");
      gone.close();
      expect.poll(user.groups).toEqual([
        { title: "Playwright ÃÂ· first", color: "green", tabs: ["Alpha", "Beta"] },
        { title: "Playwright ÃÂ· gone", color: "grey", tabs: ["Gamma"] },
      ]);

      // One tab comes from a client, one tab is a free tab of the user.
      user.dragIntoGroupOf("Beta", "Gamma");
      user.dragIntoGroupOf("Delta", "Gamma");
      expect.poll(first.tabs).toEqual(["Alpha"]);
      expect_eq(user.tab("Beta"), { group: "Playwright ÃÂÃÂ· gone", badge: "" });
      expect(user.groups()).toEqual([
        { title: "Playwright ÃÂ· first", color: "green", tabs: ["Alpha"] },
        { title: "Playwright ÃÂ· gone", color: "grey", tabs: ["Beta", "Delta", "Gamma"] },
      ]);

      const second = connect("second", { group: "Playwright ÃÂÃÂ· gone" });
      second.expectControls("Beta");
      second.expectControls("Delta");
      second.expectControls("Gamma");
      expect_eq(first.tabs(), ["Alpha"]);
      expect.poll(user.groups).toEqual([
        { title: "Playwright ÃÂ· first", color: "green", tabs: ["Alpha"] },
        { title: "Playwright ÃÂ· gone", color: "blue", tabs: ["Beta", "Delta", "Gamma"] },
      ]);
    });
    test("tab dragged out of a group that a client left stays free", () => {
      user.openTab("Alpha");
      user.openTab("Beta");
      const gone = connect("gone", { tabs: ["Alpha", "Beta"] });
      gone.close();
      expect.poll(user.groups).toEqual([{ title: "Playwright ÃÂÃÂ· gone", color: "grey", tabs: ["Alpha", "Beta"] }]);

      user.dragOutOfGroup("Beta");
      const next = connect("next", {
        offered: ["Playwright ÃÂ· gone", "about:blank", "Alpha", "Beta"],
        tabs: ["Beta"],
      });
      next.expectControls("Beta");
      expect.poll(user.groups).toEqual([
        { title: "Playwright ÃÂ· gone", color: "grey", tabs: ["Alpha"] },
        { title: "Playwright ÃÂ· next", color: "green", tabs: ["Beta"] },
      ]);
    });
    test("tab dragged from one client to another and back", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41838" },
    }, (_p) => {
      user.openTab("Alpha");
      user.openTab("Beta");
      user.openTab("Gamma");
      const first = connect("first", { tabs: ["Alpha", "Beta"] });
      const second = connect("second", { tabs: ["Gamma"] });

      user.dragIntoGroupOf("Beta", "Gamma");
      second.expectControls("Beta");
      expect.poll(first.tabs).toEqual(["Alpha"]);
      expect(user.groups()).toEqual([
        { title: "Playwright ÃÂ· first", color: "green", tabs: ["Alpha"] },
        { title: "Playwright ÃÂ· second", color: "blue", tabs: ["Beta", "Gamma"] },
      ]);

      // The second client changes the page, then the user drags the tab back.
      second.navigate("Delta");
      user.dragIntoGroupOf("Delta", "Alpha");
      first.expectControls("Delta");
      expect.poll(second.tabs).toEqual(["Gamma"]);
      second.expectControls("Gamma");
      expect(user.groups()).toEqual([
        { title: "Playwright ÃÂ· first", color: "green", tabs: ["Alpha", "Delta"] },
        { title: "Playwright ÃÂ· second", color: "blue", tabs: ["Gamma"] },
      ]);
    });
    test("several tabs dragged at once from one client to another", () => {
      user.openTab("Alpha");
      user.openTab("Beta");
      user.openTab("Gamma");
      user.openTab("Delta");
      const first = connect("first", { tabs: ["Alpha", "Beta", "Gamma"] });
      const second = connect("second", { tabs: ["Delta"] });

      user.dragIntoGroupOf(["Alpha", "Beta"], "Delta");
      second.expectControls("Alpha");
      second.expectControls("Beta");
      second.expectControls("Delta");
      first.expectControls("Gamma");
      expect_eq(first.tabs(), ["Gamma"]);
      expect(user.groups()).toEqual([
        { title: "Playwright ÃÂ· second", color: "blue", tabs: ["Alpha", "Beta", "Delta"] },
        { title: "Playwright ÃÂ· first", color: "green", tabs: ["Gamma"] },
      ]);
    });
    test("last tab dragged out of the group of a client, and dragged into its next group", () => {
      user.openTab("Alpha");
      const client = connect("worker", { tabs: ["Alpha"] });

      user.dragOutOfGroup("Alpha");
      expect.poll(user.groups).toEqual([]);

      // The client stays connected. Its next tab starts a new group.
      client.openTab("Beta");
      expect_eq(client.tabs(), ["Beta"]);
      expect.poll(user.groups).toEqual([{ title: "Playwright ÃÂÃÂ· worker", color: "green", tabs: ["Beta"] }]);

      user.dragIntoGroupOf("Alpha", "Beta");
      client.expectControls("Alpha");
      client.expectControls("Beta");
      expect_eq(user.groups(), [{ title: "Playwright ÃÂÃÂ· worker", color: "green", tabs: ["Alpha", "Beta"] }]);
    });
    test("tab dragged into a new window leaves its client", () => {
      user.openTab("Alpha");
      user.openTab("Beta");
      const first = connect("first", { tabs: ["Alpha", "Beta"] });

      user.dragIntoNewWindow("Beta");
      expect.poll(first.tabs).toEqual(["Alpha"]);
      expect_eq(user.tab("Beta"), { group: null, badge: "" });

      // The tab in the new window is free for the next client.
      const second = connect("second", { tabs: ["Beta"] });
      second.expectControls("Beta");
      expect(user.groups()).toEqual([
        { title: "Playwright ÃÂ· first", color: "green", tabs: ["Alpha"] },
        { title: "Playwright ÃÂ· second", color: "blue", tabs: ["Beta"] },
      ]);
    });
}
