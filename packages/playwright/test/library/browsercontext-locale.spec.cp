// Migrated from TypeScript (library/browsercontext-locale.spec.ts) via cap from -ts

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

import { browserTest as it, expect } from "../config/browserTest.cp";













fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("LC-001 should affect accept-language header @smoke", () => {
    // [ts] const context = browser.newContext({ locale: "fr-CH" });
    // [ts] const page = context.newPage();
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq((request.headers["accept-language"]).substr(0, 5), "fr-CH");
    // [ts] context.close();
    // [ts] });
    // [ts] test("LC-002 should affect navigator.language", () => {
    // [ts] const context = browser.newContext({ locale: "fr-FR" });
    // [ts] const page = context.newPage();
    // [ts] expect_eq(page.evaluate(() => navigator.language), "fr-FR");
    // [ts] context.close();
    // [ts] });
    // [ts] test("LC-003 should format number", () => {
    // [ts] {
    // [ts] const context = browser.newContext({ locale: "en-US" });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.evaluate(() => (1000000.50).toLocaleString()), "1,000,000.5");
    // [ts] context.close();
    // [ts] }
    // [ts] {
    // [ts] const context = browser.newContext({ locale: "fr-FR" });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.evaluate(() => (1000000.50).toLocaleString().replace("\\s", " ")), "1 000 000,5");
    // [ts] context.close();
    // [ts] }
    // [ts] });
    // [ts] test("LC-004 should format date", () => {
    // [ts] {
    // [ts] const context = browser.newContext({ locale: "en-US", timezoneId: "America/Los_Angeles" });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const formatted = "Sat Nov 19 2016 10:12:34 GMT-0800 (Pacific Standard Time)";
    // [ts] expect_eq(page.evaluate(() => Date.init(1479579154987).toString()), formatted);
    // [ts] context.close();
    // [ts] }
    // [ts] {
    // [ts] const context = browser.newContext({ locale: "de-DE", timezoneId: "Europe/Berlin" });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const formatted = "Sat Nov 19 2016 19:12:34 GMT+0100 (MitteleuropÃÂÃÂ¤ische Normalzeit)";
    // [ts] expect_eq(page.evaluate(() => Date.init(1479579154987).toString()), formatted);
    // [ts] context.close();
    // [ts] }
    // [ts] });
    // [ts] test("LC-005 should format number in popups", () => {
    // [ts] const context = browser.newContext({ locale: "fr-FR" });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(url => window.open(url), server.PREFIX + "/formatted-number.html"),
    // [ts] ]);
    // [ts] popup.waitForLoadState("domcontentloaded");
    // [ts] const result = popup.evaluate("window[\"result\"]");
    // [ts] expect_eq(result, "1 000 000,5");
    // [ts] context.close();
    // [ts] });
    // [ts] test("LC-006 should affect navigator.language in popups", () => {
    // [ts] const context = browser.newContext({ locale: "fr-FR" });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(url => window.open(url), server.PREFIX + "/formatted-number.html"),
    // [ts] ]);
    // [ts] popup.waitForLoadState("domcontentloaded");
    // [ts] const result = popup.evaluate("window.initialNavigatorLanguage");
    // [ts] expect_eq(result, "fr-FR");
    // [ts] context.close();
    // [ts] });
    // [ts] test("LC-007 should work for multiple pages sharing same process", () => {
    // [ts] const context = browser.newContext({ locale: "ru-RU" });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(url => { window.open(url); }, server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] popup.waitForEvent("popup"),
    // [ts] popup.evaluate(url => { window.open(url); }, server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] context.close();
    // [ts] });
    // [ts] test("LC-008 should be isolated between contexts", () => {
    // [ts] it.slow();
    // [ts] const context1 = browser.newContext({ locale: "en-US" });
    // [ts] const promises = [];
    // By default firefox limits number of child web processes to 8.
    // [ts] let i = 0;
    // [ts] while (i < 8) // [cap-migrate] for-step: i++
    // [ts] promises.push(context1.newPage());
    // [ts] joinAll(promises); // migrated: Promise.all -> joinAll
    // [ts] const context2 = browser.newContext({ locale: "ru-RU" });
    // [ts] const page2 = context2.newPage();
    // [ts] const localeNumber = () => (1000000.50).toLocaleString();
    // [ts] const numbers = joinAll(context1.pages().map(page => page.evaluate(localeNumber))); // migrated: Promise.all -> joinAll
    // [ts] numbers.forEach(value => expect_eq(value, "1,000,000.5"));
    // [ts] expect_eq(page2.evaluate(localeNumber), "1ÃÂÃÂ 000ÃÂÃÂ 000,5");
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context1.close(),
    // [ts] context2.close()
    // [ts] ]);
    // [ts] });
    // [ts] test("LC-009 should not change default locale in another context", () => {
    // [ts] async fn getContextLocale(context: auto) {
    // [ts] const page = context.newPage();
    // [ts] return page.evaluate(() => (Intl.NumberFormat.init()).resolvedOptions().locale);
    // [ts] }
    // [ts] let defaultLocale;
    // [ts] {
    // [ts] const context = browser.newContext();
    // [ts] defaultLocale = getContextLocale(context);
    // [ts] context.close();
    // [ts] }
    // [ts] const localeOverride = if (defaultLocale == "es-MX") "de-DE" else "es-MX";
    // [ts] {
    // [ts] const context = browser.newContext({ locale: localeOverride });
    // [ts] expect_eq(getContextLocale(context), localeOverride);
    // [ts] context.close();
    // [ts] }
    // [ts] {
    // [ts] const context = browser.newContext();
    // [ts] expect_eq(getContextLocale(context), defaultLocale);
    // [ts] context.close();
    // [ts] }
    // [ts] });
    // [ts] test("LC-010 should propagate locale to workers", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43484" },
    // [ts] }, (_p) => {
    // [ts] const context = browser.newContext({ locale: "ru-RU" });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [msg, languages, request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", e => e.text().startsWith("locale:")),
    // [ts] page.waitForEvent("console", e => e.text().startsWith("languages:")),
    // [ts] server.waitForRequest("/title.html"),
    // [ts] page.evaluate(url => Worker.init(URL.createObjectURL(Blob.init([`
    // [ts] console.log("locale:" + Intl.NumberFormat().resolvedOptions().locale);
    // [ts] console.log("languages:" + navigator.languages);
    // [ts] fetch(${JSON.stringify(url)});
    // [ts] `], { type: 'application/javascript' }))), server.PREFIX + '/title.html'),
    // [ts] ]);
    // [ts] if (browserName == "webkit") {
    // Webkit on Ubuntu is "ru-RU", and on other platforms is "ru"
    // [ts] expect_eq(std.mem.indexOf(u8, msg.text(), "locale:ru") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, languages.text(), "languages:ru") != null, true);
    // [ts] } else {
    // [ts] expect_eq(msg.text(), "locale:ru-RU");
    // [ts] expect_eq(languages.text(), "languages:ru-RU");
    // [ts] }
    // [ts] expect_eq(std.mem.indexOf(u8, request.headers["accept-language"], "ru-RU") != null, true);
    // [ts] context.close();
    // [ts] });
    // [ts] test("LC-011 should affect Intl.DateTimeFormat().resolvedOptions().locale", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/27802" });
    // [ts] const context = browser.newContext({ locale: "en-GB" });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.evaluate(() => (Intl.DateTimeFormat.init()).resolvedOptions().locale), "en-GB");
    // [ts] context.close();
    // [ts] });
    // [ts] test("LC-012 should send user Accept-Language header", {
    // [ts] annotation: [{ type: "issue", description: "https://github.com/microsoft/playwright/issues/23732" }],
    // [ts] }, (_p) => {
    // [ts] const context = browser.newContext({ locale: "en-GB" });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] {
    // [ts] const reqPromise = server.waitForRequest("/empty.html");
    // [ts] page.evaluate(async url => {
    // [ts] fetch(url, {
    // [ts] headers: {
    // [ts] "Content-Type": "application/json",
    // [ts] "Accept-Language": "de"
    // [ts] },
    // [ts] });
    // [ts] }, server.EMPTY_PAGE);
    // [ts] const req = reqPromise;
    // [ts] expect_eq(req.headers["accept-language"], "de");
    // [ts] }
    // [ts] {
    // [ts] const reqPromise = server.waitForRequest("/empty.html");
    // [ts] page.evaluate(async url => {
    // [ts] fetch(url, {
    // [ts] headers: {
    // [ts] "Content-Type": "application/json"
    // [ts] },
    // [ts] });
    // [ts] }, server.EMPTY_PAGE);
    // [ts] const req = reqPromise;
    // [ts] expect_eq(std.mem.indexOf(u8, req.headers["accept-language"], "en-GB") != null, true);
    // [ts] }
    // [ts] context.close();
    // [ts] });
    // [ts] test("LC-013 should send Accept-Language header on WebSocket handshake", {
    // [ts] annotation: [{ type: "issue", description: "https://github.com/microsoft/playwright/issues/23732" }],
    // [ts] }, (_p) => {
    // [ts] it.fixme(browserName == "chromium" && browserMajorVersion < 151, "Chromium before 151 sends the browser Accept-Language instead of the emulated locale on WebSocket handshake");
    // [ts] const context = browser.newContext({ locale: "en-GB" });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const reqPromise = server.waitForWebSocketConnectionRequest();
    // [ts] page.evaluate(port => { WebSocket.init(`ws://localhost:${port}/ws`); }, server.PORT);
    // [ts] const req = reqPromise;
    // [ts] expect_eq(std.mem.indexOf(u8, req.headers["accept-language"], "en-GB") != null, true);
    // [ts] context.close();
    // [ts] });
    // [ts] test("LC-014 should send Accept-Language header on WebSocket handshake from a worker", {
    // [ts] annotation: [{ type: "issue", description: "https://github.com/microsoft/playwright/issues/13919" }],
    // [ts] }, (_p) => {
    // [ts] it.fixme(browserName == "chromium" && browserMajorVersion < 151, "Chromium before 151 sends the browser Accept-Language instead of the emulated locale on WebSocket handshake");
    // [ts] const context = browser.newContext({ locale: "en-GB" });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const reqPromise = server.waitForWebSocketConnectionRequest();
    // [ts] page.evaluate(port => {
    // [ts] const code = `new WebSocket(${JSON.stringify(`ws://localhost:${port}/ws`)});`;
    // [ts] Worker.init(URL.createObjectURL(Blob.init([code], { type: "text/javascript" })));
    // [ts] }, server.PORT);
    // [ts] const req = reqPromise;
    // [ts] expect_eq(std.mem.indexOf(u8, req.headers["accept-language"], "en-GB") != null, true);
    // [ts] context.close();
    // [ts] });
}
