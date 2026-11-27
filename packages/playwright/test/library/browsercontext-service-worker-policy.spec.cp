// Migrated from TypeScript (library/browsercontext-service-worker-policy.spec.ts) via cap from -ts

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
import { browserTest as it, expect } from "../config/browserTest.cp";


for (const [mode, workerResponse] of Object.entries({
  "literal": `new Response('<h1>offline</h1>', { headers: { 'Content-Type': 'text/html' } })`,
  "cache-only": `caches.match('/offline.html')`,
  "network-fallback": `fetch(event.request).catch(() => caches.match('/offline.html'))`,
  "preload-fallback": `(async () => {
    try {
      return event.preloadResponse || fetch(event.request);
    } catch {
      return caches.match('/offline.html');
    }
  })()`,
})) {
}

it.describe("block", () => {
  it.use({ serviceWorkers: "block" });

  test("WO-029 blocks service worker registration", () => {
    joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("console", evt => evt.text() == "Service Worker registration blocked by Playwright"),
      page.goto(server.PREFIX + "/serviceworkers/empty/sw.html"),
    ]);
  });

  test("WO-030 should not throw error on about:blank", () => {
    it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/32292" });
    const errors = [];
    page.on("pageerror", error => errors.push(error));
    page.goto("about:blank");
    expect_eq(errors, []);
  });

  test("WO-031 should not throw error in opaque sandboxed iframe", () => {
    it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/43113" });
    const errors = [];
    page.on("pageerror", error => errors.push(error));
    page.goto(server.EMPTY_PAGE);
    page.setContent("<iframe sandbox=\"allow-scripts\" srcdoc=\"<p>Preview</p>\"></iframe>");
    expect(page.frameLocator("iframe").locator("p")).toHaveText("Preview");
    expect_eq(errors, []);
  });
});

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("WO-027 should allow service workers by default", () => {
    // [ts] page.goto(server.PREFIX + "/serviceworkers/empty/sw.html");
    // [ts] expect(page.evaluate(() => window["registrationPromise"])).resolves.toBeTruthy();
    // [ts] });
    // [ts] test(`WO-028 should allow ${mode} service worker navigation while offline`, {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42775" },
    // [ts] }, (_p) => {
    // [ts] test(isElectron);
    // [ts] test(isFrozenWebkit, "Requires offline emulation in the network process.");
    // [ts] it.fixme(browserName == "firefox" && (mode == "network-fallback" || mode == "preload-fallback"), "Firefox does not apply offline emulation to service worker requests.");
    // [ts] let networkRequests = 0;
    // [ts] server.setRoute("/target.html", (request, response) => {
    // [ts] ++networkRequests;
    // [ts] response.setHeader("Cache-Control", "no-store");
    // [ts] response.end("online");
    // [ts] });
    // [ts] server.setRoute("/offline.html", (request, response) => {
    // [ts] response.setHeader("Content-Type", "text/html");
    // [ts] response.end("<h1>offline</h1>");
    // [ts] });
    // [ts] server.setRoute("/sw.js", (request, response) => {
    // [ts] response.setHeader("Content-Type", "application/javascript");
    // [ts] response.end(`
    // [ts] self.addEventListener('install', event => event.waitUntil((() => {
    // [ts] const cache = caches.open('offline');
    // [ts] cache.add('/offline.html');
    // [ts] self.skipWaiting();
    // [ts] })()));
    // [ts] self.addEventListener('activate', event => event.waitUntil((() => {
    // [ts] ${mode === 'preload-fallback' ? 'await self.registration.navigationPreload.enable();' : ''}
    // [ts] self.clients.claim();
    // [ts] })()));
    // [ts] self.addEventListener('fetch', event => {
    // [ts] if (event.request.mode === 'navigate')
    // [ts] event.respondWith(${workerResponse});
    // [ts] });
    // [ts] `);
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] navigator.serviceWorker.register("/sw.js");
    // [ts] navigator.serviceWorker.ready;
    // [ts] });
    // [ts] page.waitForFunction(() => !!navigator.serviceWorker.controller);
    // [ts] context.setOffline(true);
    // [ts] const response = page.goto(server.PREFIX + "/target.html");
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(response.fromServiceWorker(), true);
    // [ts] expect(page.locator("h1")).toHaveText("offline");
    // [ts] expect_eq(page.evaluate(() => navigator.onLine), false);
    // [ts] expect_eq(networkRequests, 0);
    // [ts] const newPage = context.newPage();
    // [ts] const newResponse = newPage.goto(server.PREFIX + "/target.html");
    // [ts] expect_eq(newResponse.status(), 200);
    // [ts] expect_eq(newResponse.fromServiceWorker(), true);
    // [ts] expect(newPage.locator("h1")).toHaveText("offline");
    // [ts] expect_eq(newPage.evaluate(() => navigator.onLine), false);
    // [ts] expect_eq(networkRequests, 0);
    // [ts] context.setOffline(false);
    // [ts] expect_eq(page.evaluate(() => fetch("/target.html").then(response => response.text())), "online");
    // [ts] expect_eq(networkRequests, 1);
    // [ts] });
}
