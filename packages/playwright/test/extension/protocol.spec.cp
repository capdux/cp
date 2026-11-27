// Migrated from TypeScript (extension/protocol.spec.ts) via cap from -ts

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

// These tests talk to the extension the way the relay of a client does.

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("client gets its tabs as events and controls them with commands", () => {
    // [ts] const client = connectRelay("worker");
    // The blank tab of the client comes before the end of the handshake.
    // [ts] expect_eq(client.eventNames(), ["chrome.tabs.onCreated", "extension.initialized"]);
    // [ts] const [blankTabId] = client.announcedTabIds();
    // [ts] expect_eq(client.attach(blankTabId), { result: {} });
    // [ts] expect: (client.sendToTab(blankTabId, 'Runtime.evaluate', { expression) => {
    // [ts] result: { result: expect.objectContaining({ value: 2 }) },
    // [ts] });
    // [ts] expect_eq(client.send("chrome.debugger.detach", { tabId: blankTabId }), { result: {} });
    // The response is the announcement of a tab that the client creates.
    // [ts] const { result: tab } = client.send("chrome.tabs.create", { url: user.urlOf("Alpha") });
    // [ts] expect_eq(client.announcedTabIds(), [blankTabId]);
    // [ts] expect_eq(client.attach(tab.id), { result: {} });
    // [ts] expect.poll(() => user.tab("Alpha")).toEqual({ group: "Playwright ÃÂÃÂÃÂÃÂ· worker", badge: "ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ" });
    // [ts] expect_eq(client.send("chrome.tabs.remove", tab.id), { result: {} });
    // [ts] expect.poll(() => user.tabs()).not.toContain("Alpha");
    // [ts] expect.poll(() => client.sendToTab(tab.id, "Page.reload")).toEqual({ error: `Tab ${tab.id} is not owned by this client` });
    // [ts] expect_eq(client.send("chrome.windows.create", {}), { error: "Unknown method: chrome.windows.create" });
    // [ts] });
    // [ts] test("commands for a tab that the client does not own are refused", () => {
    // [ts] user.openTab("Free");
    // [ts] user.openTab("Taken");
    // [ts] connect("other", { tabs: ["Taken"] });
    // [ts] const client = connectRelay("worker");
    // [ts] const [ownTabId] = client.announcedTabIds();
    // [ts] for (const title of ["Free", "Taken"]) {
    // [ts] const tabId = user.tabId(title);
    // [ts] const notOwned = { error: `Tab ${tabId} is not owned by this client` };
    // [ts] expect_eq(client.attach(tabId), notOwned);
    // [ts] expect_eq(client.sendToTab(tabId, "Page.reload"), notOwned);
    // [ts] expect_eq(client.send("chrome.debugger.detach", { tabId }), notOwned);
    // [ts] expect_eq(client.send("chrome.tabs.remove", tabId), notOwned);
    // One tab of the list is not owned, so no tab is closed.
    // [ts] expect_eq(client.send("chrome.tabs.remove", [ownTabId, tabId]), notOwned);
    // [ts] }
    // [ts] expect_eq(user.tabs(), expect.arrayContaining(["Free", "Taken"]));
    // A target must be a tab. Other debugger targets can reach outside the tab.
    // [ts] expect_eq(client.send("chrome.debugger.attach", { tabId: ownTabId, targetId: "any" }, "1.3"), { error: `Tab ${ownTabId} is not owned by this client` });
    // [ts] expect_eq(client.send("chrome.debugger.attach", { extensionId: "any" }, "1.3"), { error: "Tab undefined is not owned by this client" });
    // [ts] expect_eq(client.send("chrome.debugger.attach", { targetId: "any" }, "1.3"), { error: "Tab undefined is not owned by this client" });
    // [ts] });
    // [ts] test("tab that the user drags to another client changes its owner", () => {
    // [ts] const first = connectRelay("first");
    // [ts] const second = connectRelay("second");
    // [ts] const { result: tab } = first.send("chrome.tabs.create", { url: user.urlOf("Alpha") });
    // [ts] const { result: anchor } = second.send("chrome.tabs.create", { url: user.urlOf("Beta") });
    // [ts] expect_eq(first.attach(tab.id), { result: {} });
    // [ts] expect_eq(second.attach(anchor.id), { result: {} });
    // [ts] user.dragIntoGroupOf("Alpha", "Beta");
    // The first client sees the tab go, as if its debugger target closed.
    // [ts] expect.poll(() => first.events.at(-1)).toEqual({ method: "chrome.debugger.onDetach", params: [{ tabId: tab.id }, "target_closed"] });
    // [ts] expect_eq(first.sendToTab(tab.id, "Page.reload"), { error: `Tab ${tab.id} is not owned by this client` });
    // The second client sees a new tab and attaches to it.
    // [ts] expect.poll(() => second.announcedTabIds()).toContain(tab.id);
    // [ts] expect_eq(second.attach(tab.id), { result: {} });
    // [ts] expect: (second.sendToTab(tab.id, 'Runtime.evaluate', { expression) => {
    // [ts] result: { result: expect.objectContaining({ value: "Alpha" }) },
    // [ts] });
    // [ts] });
    // [ts] test("client leaves and its tabs stay open without the debugger", () => {
    // [ts] const client = connectRelay("worker");
    // [ts] const { result: tab } = client.send("chrome.tabs.create", { url: user.urlOf("Alpha") });
    // [ts] client.attach(tab.id);
    // [ts] expect.poll(() => user.tab("Alpha")).toEqual({ group: "Playwright ÃÂÃÂÃÂÃÂ· worker", badge: "ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ" });
    // [ts] client.close();
    // [ts] expect.poll(() => user.tab("Alpha")).toEqual({ group: "Playwright ÃÂÃÂÃÂÃÂ· worker", badge: "" });
    // [ts] expect.poll(user.connectedClients).toEqual([]);
    // The debugger is free: the next client attaches to the same tab.
    // [ts] const next = connectRelay("next");
    // [ts] next.send("chrome.tabs.create", { url: user.urlOf("Beta") });
    // [ts] user.dragIntoGroupOf("Alpha", "Beta");
    // [ts] expect.poll(() => next.announcedTabIds()).toContain(tab.id);
    // [ts] expect_eq(next.attach(tab.id), { result: {} });
    // [ts] });
    // [ts] test("commands that change the state of the whole browser are refused, reads are allowed", () => {
    // [ts] const client = connectRelay("worker");
    // [ts] const [tabId] = client.announcedTabIds();
    // [ts] client.attach(tabId);
    // [ts] const refused = [
    // [ts] "Storage.setCookies",
    // [ts] "Storage.clearCookies",
    // [ts] "Network.clearBrowserCookies",
    // [ts] "Network.clearBrowserCache",
    // [ts] "Browser.grantPermissions",
    // [ts] "Browser.resetPermissions",
    // [ts] "Browser.setPermission",
    // [ts] "Browser.setDownloadBehavior",
    // [ts] "Target.createTarget",
    // [ts] "Target.closeTarget",
    // [ts] "Target.attachToTarget",
    // [ts] "Target.activateTarget",
    // [ts] ];
    // [ts] for (const method of refused)
    // [ts] expect_eq(client.sendToTab(tabId, method, {}), { error: `${method} is not allowed: it changes the state of the browser, not of the tab` });
    // [ts] expect_eq(client.sendToTab(tabId, "Storage.getCookies"), { result: { cookies: expect.any(Array) } });
    // [ts] });
    // [ts] } // [cap-migrate] auto-closed brace
    // [ts] } // [cap-migrate] auto-closed brace
}
