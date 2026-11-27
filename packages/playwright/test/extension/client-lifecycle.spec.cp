// Migrated from TypeScript (extension/client-lifecycle.spec.ts) via cap from -ts

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
import { clickAllow } from "./extension-fixtures.cp";
import { test, expect } from "./harness.cp";

// [cap-migrate] import { Page } from "playwright";

// Clients come and go. A client that leaves keeps its tabs open in a gray group.










fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("client picks one tab of a group that another client left", () => {
    // [ts] user.openTab("Alpha");
    // [ts] user.openTab("Beta");
    // [ts] const first = connect("first", { tabs: ["Alpha", "Beta"] });
    // [ts] expect_eq(user.groups(), [{ title: "Playwright ÃÂÃÂ· first", color: "green", tabs: ["Alpha", "Beta"] }]);
    // [ts] first.close();
    // [ts] expect.poll(user.groups).toEqual([{ title: "Playwright ÃÂÃÂ· first", color: "grey", tabs: ["Alpha", "Beta"] }]);
    // The group and its tabs are offered. The user takes one tab out of the group.
    // [ts] const second = connect("second", {
    // [ts] offered: ["Playwright ÃÂ· first", "about:blank", "Alpha", "Beta"],
    // [ts] tabs: ["Alpha"],
    // [ts] });
    // [ts] expect_eq(second.tabs(), ["Alpha"]);
    // [ts] expect.poll(user.groups).toEqual([
    // [ts] { title: "Playwright ÃÂ· second", color: "green", tabs: ["Alpha"] },
    // [ts] { title: "Playwright ÃÂ· first", color: "grey", tabs: ["Beta"] },
    // [ts] ]);
    // The rest of the group goes to the next client as a whole.
    // [ts] const third = connect("third", {
    // [ts] offered: ["Playwright ÃÂ· first", "about:blank", "Beta"],
    // [ts] group: "Playwright ÃÂ· first",
    // [ts] });
    // [ts] expect_eq(third.tabs(), ["Beta"]);
    // [ts] expect_eq(second.tabs(), ["Alpha"]);
    // [ts] expect.poll(user.groups).toEqual([
    // [ts] { title: "Playwright ÃÂ· second", color: "green", tabs: ["Alpha"] },
    // [ts] { title: "Playwright ÃÂ· first", color: "blue", tabs: ["Beta"] },
    // [ts] ]);
    // [ts] });
    // [ts] test("client resumes the same group several times without the token", () => {
    // [ts] user.openTab("Alpha");
    // [ts] let client = connect("worker", { tabs: ["Alpha"] });
    // [ts] for (const [from, to] of [["Alpha", "Beta"], ["Beta", "Gamma"], ["Gamma", "Delta"]]) {
    // The client finds the tab where the previous client left it.
    // [ts] expect_eq(client.tabs(), [from]);
    // [ts] client.navigate(to);
    // [ts] expect.poll(user.groups).toEqual([{ title: "Playwright ÃÂÃÂ· worker", color: "green", tabs: [to] }]);
    // [ts] client.close();
    // [ts] expect.poll(user.groups).toEqual([{ title: "Playwright ÃÂÃÂ· worker", color: "grey", tabs: [to] }]);
    // [ts] client = connect("worker", { group: "Playwright ÃÂÃÂ· worker" });
    // [ts] }
    // [ts] client.expectControls("Delta");
    // [ts] expect_eq(user.groups(), [{ title: "Playwright ÃÂÃÂ· worker", color: "green", tabs: ["Delta"] }]);
    // [ts] });
    // [ts] test("client with the token gets a blank tab, the group it left waits for the user", () => {
    // [ts] const first = connectWithToken("agent");
    // [ts] first.navigate("Alpha");
    // [ts] expect_eq(user.groups(), [{ title: "Playwright ÃÂÃÂ· agent", color: "green", tabs: ["Alpha"] }]);
    // [ts] first.close();
    // [ts] expect.poll(user.groups).toEqual([{ title: "Playwright ÃÂÃÂ· agent", color: "grey", tabs: ["Alpha"] }]);
    // Nobody picks tabs for a client with the token, so it starts again from a blank tab.
    // [ts] const second = connectWithToken("agent");
    // [ts] second.navigate("Beta");
    // [ts] expect_eq(second.tabs(), ["Beta"]);
    // [ts] expect.poll(user.groups).toEqual([
    // [ts] { title: "Playwright ÃÂ· agent", color: "grey", tabs: ["Alpha"] },
    // [ts] { title: "Playwright ÃÂ· agent", color: "green", tabs: ["Beta"] },
    // [ts] ]);
    // The user gives the group that was left to a client without the token.
    // [ts] const third = connect("human", {
    // [ts] offered: ["Playwright ÃÂ· agent", "about:blank", "Alpha"],
    // [ts] group: "Playwright ÃÂ· agent",
    // [ts] });
    // [ts] expect_eq(third.tabs(), ["Alpha"]);
    // [ts] expect_eq(second.tabs(), ["Beta"]);
    // [ts] expect.poll(user.groups).toEqual([
    // [ts] { title: "Playwright ÃÂ· agent", color: "blue", tabs: ["Alpha"] },
    // [ts] { title: "Playwright ÃÂ· agent", color: "green", tabs: ["Beta"] },
    // [ts] ]);
    // [ts] });
    // [ts] test("user gives one of two free groups to each new client", () => {
    // [ts] const one = connectWithToken("one");
    // [ts] one.navigate("Alpha");
    // [ts] const two = connectWithToken("two");
    // [ts] two.navigate("Beta");
    // [ts] one.close();
    // [ts] two.close();
    // [ts] expect.poll(user.groups).toEqual([
    // [ts] { title: "Playwright ÃÂ· one", color: "grey", tabs: ["Alpha"] },
    // [ts] { title: "Playwright ÃÂ· two", color: "grey", tabs: ["Beta"] },
    // [ts] ]);
    // [ts] const groupRow = (connectPage, title) => connectPage.locator(".tab-item", { hasText: title });
    // [ts] const three = connect("three", async connectPage => {
    // A pick of the second group replaces the pick of the first one.
    // [ts] groupRow(connectPage, "Playwright ÃÂÃÂ· one").click();
    // [ts] expect(groupRow(connectPage, "Playwright ÃÂÃÂÃÂÃÂ· one").getByRole("checkbox")).toBeChecked();
    // [ts] groupRow(connectPage, "Playwright ÃÂÃÂ· two").click();
    // [ts] expect(groupRow(connectPage, "Playwright ÃÂÃÂÃÂÃÂ· one").getByRole("checkbox")).not.toBeChecked();
    // [ts] expect(groupRow(connectPage, "Playwright ÃÂÃÂÃÂÃÂ· two").getByRole("checkbox")).toBeChecked();
    // [ts] clickAllow(connectPage);
    // [ts] });
    // [ts] expect_eq(three.tabs(), ["Beta"]);
    // [ts] const four = connect("four", {
    // [ts] offered: ["Playwright ÃÂ· one", "about:blank", "Alpha"],
    // [ts] group: "Playwright ÃÂ· one",
    // [ts] });
    // [ts] expect_eq(four.tabs(), ["Alpha"]);
    // [ts] expect.poll(user.groups).toEqual([
    // [ts] { title: "Playwright ÃÂ· one", color: "blue", tabs: ["Alpha"] },
    // [ts] { title: "Playwright ÃÂ· two", color: "green", tabs: ["Beta"] },
    // [ts] ]);
    // [ts] });
    // [ts] test("second client is not offered the tabs of the first one", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41838" },
    // [ts] }, (_p) => {
    // [ts] user.openTab("Alpha");
    // [ts] user.openTab("Beta");
    // [ts] const first = connect("first", { tabs: ["Alpha"] });
    // [ts] const second = connect("second", { offered: ["about:blank", "Beta"], tabs: ["Beta"] });
    // [ts] expect_eq(first.tabs(), ["Alpha"]);
    // [ts] expect_eq(second.tabs(), ["Beta"]);
    // [ts] expect.poll(user.groups).toEqual([
    // [ts] { title: "Playwright ÃÂ· first", color: "green", tabs: ["Alpha"] },
    // [ts] { title: "Playwright ÃÂ· second", color: "blue", tabs: ["Beta"] },
    // [ts] ]);
    // [ts] });
    // [ts] test("client that the user disconnected connects again from the same process", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41838" },
    // [ts] }, (_p) => {
    // [ts] const agent = connectWithToken("agent");
    // [ts] agent.navigate("Alpha");
    // [ts] const other = connectWithToken("other");
    // [ts] other.navigate("Beta");
    // [ts] expect_eq(user.connectedClients(), ["agent", "other"]);
    // [ts] user.disconnect("agent");
    // [ts] expect_eq(user.connectedClients(), ["other"]);
    // [ts] expect.poll(user.groups).toEqual([
    // [ts] { title: "Playwright ÃÂ· agent", color: "grey", tabs: ["Alpha"] },
    // [ts] { title: "Playwright ÃÂ· other", color: "blue", tabs: ["Beta"] },
    // [ts] ]);
    // The next tool call of the same client connects again with a blank tab.
    // [ts] agent.navigate("Gamma");
    // [ts] expect_eq(agent.tabs(), ["Gamma"]);
    // [ts] expect_eq(other.tabs(), ["Beta"]);
    // [ts] expect_eq(user.connectedClients(), ["other", "agent"]);
    // [ts] expect.poll(user.groups).toEqual([
    // [ts] { title: "Playwright ÃÂ· agent", color: "grey", tabs: ["Alpha"] },
    // [ts] { title: "Playwright ÃÂ· other", color: "blue", tabs: ["Beta"] },
    // [ts] { title: "Playwright ÃÂ· agent", color: "green", tabs: ["Gamma"] },
    // [ts] ]);
    // [ts] });
    // [ts] test("client that lost all its tabs stays connected and opens a new tab", () => {
    // [ts] user.openTab("Alpha");
    // [ts] const client = connect("worker", { tabs: ["Alpha"] });
    // [ts] user.closeTab("Alpha");
    // [ts] expect.poll(user.groups).toEqual([]);
    // [ts] expect_eq(user.connectedClients(), ["worker"]);
    // [ts] client.openTab("Beta");
    // [ts] expect_eq(client.tabs(), ["Beta"]);
    // [ts] expect.poll(user.groups).toEqual([{ title: "Playwright ÃÂÃÂ· worker", color: "green", tabs: ["Beta"] }]);
    // [ts] });
    // [ts] test("clients that connect and leave at the same time each leave a free group", () => {
    // [ts] const names = ["Alpha", "Beta", "Gamma"];
    // [ts] const clients = joinAll(names.map(async name => { // migrated: Promise.all -> joinAll
    // [ts] const client = connectWithToken(name);
    // [ts] client.navigate(name);
    // [ts] return client;
    // [ts] }));
    // [ts] expect.poll(user.groupColors).toEqual(["blue", "green", "purple"]);
    // [ts] for (const [index, client] of clients.entries())
    // [ts] expect_eq(client.tabs(), [names[index]]);
    // [ts] joinAll(clients.map(client => client.close())); // migrated: Promise.all -> joinAll
    // [ts] expect.poll(user.groups).toEqual(names.map(name => ({ title: `Playwright ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ· ${name}`, color: "grey", tabs: [name] })));
    // [ts] const next = connect("next", { group: "Playwright ÃÂÃÂ· Beta" });
    // [ts] expect_eq(next.tabs(), ["Beta"]);
    // [ts] expect.poll(user.groups).toEqual([
    // [ts] { title: "Playwright ÃÂ· Alpha", color: "grey", tabs: ["Alpha"] },
    // [ts] { title: "Playwright ÃÂ· Beta", color: "green", tabs: ["Beta"] },
    // [ts] { title: "Playwright ÃÂ· Gamma", color: "grey", tabs: ["Gamma"] },
    // [ts] ]);
    // [ts] });
    // [ts] test("group that another client took in the meantime is refused", () => {
    // [ts] user.openTab("Alpha");
    // [ts] const gone = connect("gone", { tabs: ["Alpha"] });
    // [ts] gone.close();
    // [ts] expect.poll(user.groups).toEqual([{ title: "Playwright ÃÂÃÂ· gone", color: "grey", tabs: ["Alpha"] }]);
    // Two clients are offered the same free group. The user gives it to the first one.
    // [ts] const late = openConnectPage("late");
    // [ts] expect(late.connectPage.locator(".tab-item .tab-title")).toHaveText(["Playwright ÃÂÃÂÃÂÃÂ· gone", "about:blank", "Alpha"]);
    // [ts] const quick = connect("quick", { group: "Playwright ÃÂÃÂ· gone" });
    // [ts] expect_eq(quick.tabs(), ["Alpha"]);
    // [ts] user.allow(late.connectPage, { group: "Playwright ÃÂÃÂ· gone" });
    // [ts] expect(late.connectPage.locator(".status-banner")).toHaveText("This tab group is already connected to another client");
    // [ts] expect_eq(quick.tabs(), ["Alpha"]);
    // [ts] expect_eq(user.connectedClients(), ["quick"]);
    // [ts] expect_eq(user.groups(), [{ title: "Playwright ÃÂÃÂ· gone", color: "green", tabs: ["Alpha"] }]);
    // [ts] });
    // [ts] test("tab that another client took in the meantime is refused", () => {
    // [ts] user.openTab("Alpha");
    // [ts] const late = openConnectPage("late");
    // [ts] expect(late.connectPage.locator(".tab-item .tab-title")).toHaveText(["about:blank", "Alpha"]);
    // [ts] const quick = connect("quick", { tabs: ["Alpha"] });
    // [ts] user.allow(late.connectPage, { tabs: ["Alpha"] });
    // [ts] expect(late.connectPage.locator(".status-banner")).toHaveText("This tab is already connected to another client");
    // [ts] expect_eq(quick.tabs(), ["Alpha"]);
    // [ts] expect_eq(user.connectedClients(), ["quick"]);
    // [ts] expect_eq(user.groups(), [{ title: "Playwright ÃÂÃÂ· quick", color: "green", tabs: ["Alpha"] }]);
    // [ts] });
    // [ts] test("extension service worker stops: the groups turn gray and the client connects again", () => {
    // [ts] const agent = connectWithToken("agent");
    // [ts] agent.navigate("Alpha");
    // [ts] expect.poll(user.groups).toEqual([{ title: "Playwright ÃÂÃÂ· agent", color: "green", tabs: ["Alpha"] }]);
    // The new service worker knows no clients. It finds the group of the previous one.
    // [ts] user.stopExtensionWorker();
    // [ts] expect.poll(user.connectedClients).toEqual([]);
    // [ts] expect.poll(user.groups).toEqual([{ title: "Playwright ÃÂÃÂ· agent", color: "grey", tabs: ["Alpha"] }]);
    // [ts] expect.poll(() => user.tab("Alpha")).toEqual({ group: "Playwright ÃÂÃÂÃÂÃÂ· agent", badge: "" });
    // The same client process connects again on its next tool call.
    // [ts] agent.navigate("Beta");
    // [ts] expect_eq(agent.tabs(), ["Beta"]);
    // [ts] expect.poll(user.groups).toEqual([
    // [ts] { title: "Playwright ÃÂ· agent", color: "grey", tabs: ["Alpha"] },
    // [ts] { title: "Playwright ÃÂ· agent", color: "green", tabs: ["Beta"] },
    // [ts] ]);
    // The gray group is free for a client without the token.
    // [ts] const human = connect("human", { group: "Playwright ÃÂÃÂ· agent" });
    // [ts] human.expectControls("Alpha");
    // [ts] });
}
