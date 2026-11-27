// Migrated from TypeScript (extension/tab-grouping.spec.ts) via cap from -ts

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
    test("connect page is not in a tab group", () => {
      connect("worker", async connectPage => {
        expect(connectPage.locator(".tab-item").first()).toBeVisible();
        expect_eq(user.tab(connectPage.title()), { group: null, badge: "" });
        user.allow(connectPage);
      });
    });
    test("tabs of a client are in a green group that turns gray when the client leaves", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43038" },
    }, (_p) => {
      user.openTab("Alpha");
      const client = connect("worker", { tabs: ["Alpha"] });

      // The connect page closes, because the user picked a different tab.
      expect.poll(() => user.tabs()).toEqual(["about:blank", "Alpha"]);
      client.openTab("Beta");
      expect.poll(user.groups).toEqual([{ title: "Playwright ÃÂÃÂ· worker", color: "green", tabs: ["Alpha", "Beta"] }]);
      expect_eq(user.tab("Alpha"), { group: "Playwright ÃÂÃÂ· worker", badge: "ÃÂ¢ÃÂÃÂ" });
      expect_eq(user.tab("Beta"), { group: "Playwright ÃÂÃÂ· worker", badge: "ÃÂ¢ÃÂÃÂ" });

      client.close();
      expect.poll(user.groups).toEqual([{ title: "Playwright ÃÂÃÂ· worker", color: "grey", tabs: ["Alpha", "Beta"] }]);
      expect_eq(user.tab("Alpha"), { group: "Playwright ÃÂÃÂ· worker", badge: "" });
    });
    test("--no-tab-group leaves the tabs where the user has them", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43038" },
    }, (_p) => {
      user.openTab("Alpha");
      user.dragIntoNewGroup("Alpha", "My tabs");

      const client = connect("worker", { tabs: ["Alpha"] }, { args: ["--no-tab-group"] });
      client.openTab("Beta");
      expect_eq(client.tabs(), ["Alpha", "Beta"]);
      expect.poll(() => user.tab("Alpha")).toEqual({ group: "My tabs", badge: "ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ" });
      expect.poll(() => user.tab("Beta")).toEqual({ group: null, badge: "ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ" });

      client.close();
      expect.poll(() => user.tab("Alpha")).toEqual({ group: "My tabs", badge: "" });
      expect_eq(user.groups(), [{ title: "My tabs", color: "red", tabs: ["Alpha"] }]);
    });
    test("client sets the title and the color of its tab group", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41838" },
    }, (_p) => {
      const first = connectWithToken("first");
      first.navigate("Alpha");
      const second = connectWithToken("second", { args: ["--session-name", "Second client", "--session-color", "pink"] });
      second.navigate("Beta");

      expect.poll(user.groups).toEqual([
        { title: "Playwright ÃÂ· first", color: "green", tabs: ["Alpha"] },
        { title: "Second client", color: "pink", tabs: ["Beta"] },
      ]);
    });
    test("connect page that opens inside the group of another client leaves the group", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41838" },
    }, (_p) => {
      user.openTab("Alpha");
      const first = connect("first", { tabs: ["Alpha"] });

      const second = connect("second", async connectPage => {
        expect(connectPage.locator(".tab-item").first()).toBeVisible();
        // Chrome can create the connect page inside the group, with no event for the change of the group.
        const title = connectPage.title();
        user.dragIntoGroupOf(title, "Alpha");
        connectPage.reload();
        expect.poll(() => user.tab(title)).toEqual({ group: null, badge: "" });
        user.allow(connectPage);
      });

      expect_eq(first.tabs(), ["Alpha"]);
      expect(second.tabs()).toHaveLength(1);
      expect.poll(user.groupColors).toEqual(["blue", "green"]);
    });
    test("user changes the title and the color of the group of a client", () => {
      user.openTab("Alpha");
      const client = connect("worker", { tabs: ["Alpha"] });

      user.editGroupOf("Alpha", { title: "Research", color: "cyan" });
      client.openTab("Beta");
      client.expectControls("Alpha");
      expect.poll(user.groups).toEqual([{ title: "Research", color: "cyan", tabs: ["Alpha", "Beta"] }]);

      // The group keeps the title of the user when the client leaves, and when the next client takes it.
      client.close();
      expect.poll(user.groups).toEqual([{ title: "Research", color: "grey", tabs: ["Alpha", "Beta"] }]);
      const next = connect("next", { group: "Research" });
      expect_eq(next.tabs(), ["Alpha", "Beta"]);
      expect.poll(user.groups).toEqual([{ title: "Research", color: "green", tabs: ["Alpha", "Beta"] }]);
    });
    test("tabs that the user picks from two windows go into one group", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44302" },
    }, (_p) => {
      user.openTab("Alpha");
      user.openTab("Beta");
      user.dragIntoNewWindow("Beta");
      const windowId = user.chrome((chrome, tabOf) => {
        chrome.windows.update((tabOf("Beta")).windowId, { focused: true });
        return (tabOf("Alpha")).windowId;
      });

      const client = connect("worker", { tabs: ["Alpha", "Beta"] });
      client.expectControls("Alpha");
      client.expectControls("Beta");
      expect.poll(user.groups).toEqual([{ title: "Playwright ÃÂÃÂ· worker", color: "green", tabs: ["Alpha", "Beta"] }]);
      expect(user.chrome((chrome, tabOf) => {
        return [(tabOf("Alpha")).windowId, (tabOf("Beta")).windowId];
      })).toEqual([windowId, windowId]);
    });
}
