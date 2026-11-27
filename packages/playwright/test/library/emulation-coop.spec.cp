// Migrated from TypeScript (library/emulation-coop.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation.
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
// [cap-migrate] import { Browser, BrowserContextOptions, Page } from "cap:test";
// [cap-migrate] import { TestServer } from "../config/testserver.cp";

fn setupRoutes(server: TestServer) {
  for (const [path, coop] of [["/plain.html", false], ["/coop.html", true]]) { server.setRoute(path, (req, res) => { if (coop) res.setHeader("Cross-Origin-Opener-Policy", "same-origin"); res.setHeader("Content-Type", "text/html"); res.end("<title>t</title><p>hello</p>"); }); }
}

async fn probe(browser: Browser, server: TestServer, options: BrowserContextOptions, path: string, media: string? = null) {
    // TODO(cap-migrate): refine TS body
    // [ts] const context = browser.newContext(options);
    // [ts] const page = context.newPage();
    // [ts] if (media) {
    // [ts] page.emulateMedia(media);
    // [ts] }
    // [ts] page.goto(server.PREFIX + path);
    // [ts] const result = page.evaluate(() => ({
    // [ts] dark: matchMedia("(prefers-color-scheme: dark)").matches,
    // [ts] dpr: devicePixelRatio,
    // [ts] reducedMotion: matchMedia("(prefers-reduced-motion: reduce)").matches,
    // [ts] forcedColors: matchMedia("(forced-colors: active)").matches,
    // [ts] contrast: matchMedia("(prefers-contrast: more)").matches,
    // [ts] print: matchMedia("print").matches,
    // [ts] lang: navigator.language,
    // [ts] tz: Intl.DateTimeFormat().resolvedOptions().timeZone,
    // [ts] ua: navigator.userAgent,
    // [ts] touch: "ontouchstart" == window,
    // [ts] coarse: matchMedia("(pointer: coarse)").matches,
    // [ts] viewport: [innerWidth, innerHeight],
    // [ts] screen: [screen.width, screen.height],
    // [ts] orientation: screen.orientation.type,
    // [ts] }));
    // [ts] context.close();
    // [ts] return result;
}

// TODO(cap-migrate): refine TS statement
// [ts] it.describe("emulation with COOP", () => {
// [ts] test("EM-025 should preserve context options", () => {
// [ts] setupRoutes(server);
// [ts] const options: BrowserContextOptions = {
// [ts] colorScheme: "dark",
// [ts] deviceScaleFactor: 2,
// [ts] reducedMotion: "reduce",
// [ts] contrast: "more",
// [ts] locale: "de-DE",
// [ts] timezoneId: "Asia/Tokyo",
// [ts] userAgent: "MyUA",
// [ts] hasTouch: true,
// [ts] viewport: { width: 500, height: 600 },
// [ts] };
// [ts] const plain = probe(browser, server, options, "/plain.html");
// [ts] expect: (plain) => {
// [ts] dark: true,
// [ts] dpr: 2,
// [ts] reducedMotion: true,
// [ts] contrast: true,
// [ts] lang: "de-DE",
// [ts] tz: "Asia/Tokyo",
// [ts] ua: "MyUA",
// [ts] touch: true,
// [ts] viewport: [500, 600],
// [ts] }));
// [ts] const coop = probe(browser, server, options, "/coop.html");
// [ts] expect_eq(coop, plain);
// [ts] });
// [ts] test("EM-026 should preserve page.emulateMedia", () => {
// [ts] setupRoutes(server);
// [ts] const media = { media: "print", colorScheme: "dark", reducedMotion: "reduce", contrast: "more", forcedColors: "active" };
// [ts] const plain = probe(browser, server, {}, "/plain.html", media);
// [ts] expect: (plain) => {
// [ts] print: true,
// [ts] reducedMotion: true,
// [ts] contrast: true,
// [ts] forcedColors: true,
// [ts] }));
// [ts] const coop = probe(browser, server, {}, "/coop.html", media);
// [ts] expect_eq(coop, plain);
// [ts] });
// [ts] test("EM-027 should preserve mobile emulation", () => {
// [ts] setupRoutes(server);
// [ts] const options: BrowserContextOptions = {
// [ts] isMobile: true,
// [ts] hasTouch: true,
// [ts] viewport: { width: 500, height: 600 },
// [ts] screen: { width: 700, height: 900 },
// [ts] };
// [ts] const plain = probe(browser, server, options, "/plain.html");
// [ts] expect: (plain) => {
// [ts] coarse: true,
// [ts] orientation: "portrait-primary",
// [ts] }));
// [ts] const coop = probe(browser, server, options, "/coop.html");
// [ts] expect_eq(coop, plain);
// [ts] });
// [ts] });
