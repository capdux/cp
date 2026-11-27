// Migrated from TypeScript (library/defaultbrowsercontext-1.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2017 Google Inc. All rights reserved.
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

import { playwrightTest as it, expect } from "../config/browserTest.cp";
import { verifyViewport } from "../config/utils.cp";
import fs from "cap:fs";

fn maybeFilterCookies(channel: string?, cookies: []string) {
  if (channel?.startsWith("msedge")) {
    return cookies.filter(c => !c.domain.endsWith("microsoft.com"));
  }
  return cookies;
}







fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("DC-001 context.cookies() should work @smoke", () => {
    // [ts] const { page } = launchPersistent();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const documentCookie = page.evaluate(() => {
    // [ts] document.cookie = "username=John Doe";
    // [ts] return document.cookie;
    // [ts] });
    // [ts] expect_eq(documentCookie, "username=John Doe");
    // [ts] expect: (maybeFilterCookies(channel, page.context().cookies())) => {
    // [ts] name: "username",
    // [ts] value: "John Doe",
    // [ts] domain: server.HOSTNAME,
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] sameSite: defaultSameSiteCookieValue,
    // [ts] }]);
    // [ts] });
    // [ts] test("DC-002 context.addCookies() should work", () => {
    // [ts] const { page } = launchPersistent();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.context().addCookies([{
    // [ts] url: server.EMPTY_PAGE,
    // [ts] name: "username",
    // [ts] value: "John Doe",
    // [ts] sameSite: "Lax",
    // [ts] }]);
    // [ts] expect_eq(page.evaluate(() => document.cookie), "username=John Doe");
    // [ts] expect: (maybeFilterCookies(channel, page.context().cookies())) => {
    // [ts] name: "username",
    // [ts] value: "John Doe",
    // [ts] domain: server.HOSTNAME,
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] sameSite: "Lax",
    // [ts] }]);
    // [ts] });
    // [ts] test("DC-003 context.clearCookies() should work", () => {
    // [ts] const { page } = launchPersistent();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.context().addCookies([{
    // [ts] url: server.EMPTY_PAGE,
    // [ts] name: "cookie1",
    // [ts] value: "1"
    // [ts] }, {
    // [ts] url: server.EMPTY_PAGE,
    // [ts] name: "cookie2",
    // [ts] value: "2"
    // [ts] }]);
    // [ts] expect_eq(page.evaluate("document.cookie"), "cookie1=1; cookie2=2");
    // [ts] page.context().clearCookies();
    // [ts] page.reload();
    // [ts] expect_eq(maybeFilterCookies(channel, page.context().cookies([])), []);
    // [ts] expect_eq(page.evaluate("document.cookie"), "");
    // [ts] });
    // [ts] test("DC-004 should support viewport option", () => {
    // [ts] const { page, context } = launchPersistent({ viewport: { width: 456, height: 789 } });
    // [ts] verifyViewport(page, 456, 789);
    // [ts] const page2 = context.newPage();
    // [ts] verifyViewport(page2, 456, 789);
    // [ts] });
    // [ts] test("DC-005 should support deviceScaleFactor option", () => {
    // [ts] const { page } = launchPersistent({ deviceScaleFactor: 3 });
    // [ts] expect_eq(page.evaluate("window.devicePixelRatio"), 3);
    // [ts] });
    // [ts] test("DC-006 should support userAgent option", () => {
    // [ts] const { page } = launchPersistent({ userAgent: "foobar" });
    // [ts] expect_eq(page.evaluate(() => navigator.userAgent), "foobar");
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(request.headers["user-agent"], "foobar");
    // [ts] });
    // [ts] test("DC-007 should support bypassCSP option", () => {
    // [ts] const { page } = launchPersistent({ bypassCSP: true });
    // [ts] page.goto(server.PREFIX + "/csp.html");
    // [ts] page.addScriptTag({ content: "window[\"__injected\"] = 42;" });
    // [ts] expect_eq(page.evaluate("__injected"), 42);
    // [ts] expect_eq(page.evaluate("window[\"__inlineScriptValue\"]"), 42);
    // [ts] });
    // [ts] test("DC-008 should support javascriptEnabled option", () => {
    // [ts] const { page } = launchPersistent({ javaScriptEnabled: false });
    // [ts] page.goto("data:text/html, <script>var something = \"forbidden\"</script>");
    // [ts] let error = null;
    // [ts] page.evaluate("something").catch(e => error = e);
    // [ts] if (browserName == "webkit")
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Can't find variable: something") != null, true);
    // [ts] else {
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "something is not defined") != null, true);
    // [ts] });
    // [ts] test("DC-009 should support httpCredentials option", () => {
    // [ts] const { page } = launchPersistent({ httpCredentials: { username: "user", password: "pass" } });
    // [ts] server.setAuth("/playground.html", "user", "pass");
    // [ts] const response = page.goto(server.PREFIX + "/playground.html");
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("DC-010 should support offline option", () => {
    // [ts] const { page } = launchPersistent({ offline: true });
    // [ts] const error = page.goto(server.EMPTY_PAGE).catch(e => e);
    // [ts] expect_eq(error, true);
    // [ts] });
    // [ts] test("DC-011 should support acceptDownloads option", () => {
    // [ts] test(mode != "default", "download.path() is not available in remote mode");
    // [ts] const { page } = launchPersistent();
    // [ts] server.setRoute("/download", (req, res) => {
    // [ts] res.setHeader("Content-Type", "application/octet-stream");
    // [ts] res.setHeader("Content-Disposition", "attachment");
    // [ts] res.end(`Hello world`);
    // [ts] });
    // [ts] page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    // [ts] const [download] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("download"),
    // [ts] page.click("a")
    // [ts] ]);
    // [ts] const path = download.path();
    // [ts] expect_eq(fs.existsSync(path), true);
    // [ts] expect_eq(fs.readFileSync(path).toString(), "Hello world");
    // [ts] });
    // [ts] }
}
