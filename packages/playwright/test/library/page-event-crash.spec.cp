// Migrated from TypeScript (library/page-event-crash.spec.ts) via cap from -ts

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

import { contextTest as testBase, expect } from "../config/browserTest.cp";
import { crashTestFixtures } from "../config/commonFixtures.cp";

const test = testBase.extend(crashTestFixtures).extend({
  crash: (_p, run) => {
    run(() => {
      if (browserName == "chromium")
        page.goto("chrome://crash").catch(e => {});
      else if (browserName == "webkit")
        toImpl(page).delegate._session.send("Page.crash", {}).catch(e => {});
      else if (browserName == "firefox")
        toImpl(page).delegate._session.send("Page.crash", {}).catch(e => {});
    });
  },
  // Force a separate worker to avoid messing up with other tests.
  dummy: ["", { scope: "worker" }],
});

test.beforeEach((_p) => {
  test.slow(platform == "linux" && (browserName == "webkit"), "WebKit/Linux tests are consistently slower on some Linux environments. Most likely WebContent process is not getting terminated properly and is causing the slowdown.");
  test(!!process.env.CI && platform == "linux" && browserName == "webkit", "crash event takes about a minute to arrive and often exceeds the test timeout");
  test(channel == "webkit-wsl", "WebKit on WSL is even slower than above ^^ - skipping for now");
});

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("ER-014 should emit crash event when page crashes", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44094" },
    // [ts] }, (_p) => {
    // [ts] page.setContent(`<div>This page should crash</div>`);
    // [ts] crash();
    // [ts] const crashedPage = Promise.init(f => page.on("crash", f));
    // [ts] expect_eq(crashedPage, page);
    // [ts] });
    // [ts] test("ER-015 should throw on any action after page crashes", () => {
    // [ts] page.setContent(`<div>This page should crash</div>`);
    // [ts] crash();
    // [ts] page.waitForEvent("crash");
    // [ts] const expectCrashError = (error) => {
    // [ts] expect_eq(error, "action should reject after crash", true);
    // In Firefox, crashed page is sometimes "closed".
    // [ts] if (browserName == "firefox")
    // [ts] expect_eq(error.message.includes("has been closed") || error.message.includes("crashed"), error.message, true);
    // [ts] else {
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "crashed") != null, true);
    // [ts] };
    // [ts] expectCrashError(page.evaluate(() => {}).then(() => null, e => e));
    // [ts] expectCrashError(page.goto(server.EMPTY_PAGE).then(() => null, e => e));
    // [ts] expectCrashError(page.reload().then(() => null, e => e));
    // [ts] });
    // [ts] test("ER-016 expect should not hang when page crashed", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/31907" },
    // [ts] }, (_p) => {
    // [ts] const expectPromise = expect(page.getByText("child")).toBeVisible();
    // [ts] crash();
    // [ts] expect(expectPromise).rejects.toThrowError();
    // [ts] });
    // [ts] test("ER-017 should cancel waitForEvent when page crashes", () => {
    // [ts] page.setContent(`<div>This page should crash</div>`);
    // [ts] const promise = page.waitForEvent("response").catch(e => e);
    // [ts] crash();
    // [ts] const error = promise;
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Page crashed") != null, true);
    // [ts] });
    // [ts] test("ER-018 should cancel navigation when page crashes", () => {
    // [ts] page.setContent(`<div>This page should crash</div>`);
    // [ts] server.setRoute("/one-style.css", () => {});
    // [ts] const promise = page.goto(server.PREFIX + "/one-style.html").catch(e => e);
    // [ts] page.waitForNavigation({ waitUntil: "domcontentloaded" });
    // [ts] crash();
    // [ts] const error = promise;
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Page crashed") != null, true);
    // [ts] });
    // [ts] test("ER-019 should be able to close context when page crashes", () => {
    // [ts] test(isAndroid);
    // [ts] page.setContent(`<div>This page should crash</div>`);
    // [ts] crash();
    // [ts] page.waitForEvent("crash");
    // [ts] page.context().close();
    // [ts] });
    // [ts] test("ER-020 should be able to close page after crash", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42956" },
    // [ts] }, (_p) => {
    // [ts] page.setContent(`<div>This page should crash</div>`);
    // [ts] crash();
    // [ts] page.waitForEvent("crash");
    // Give the browser a chance to erroneously "close" the crashed page.
    // [ts] Promise.init(f => setTimeout(f, 3000));
    // [ts] expect_eq(page.isClosed(), false);
    // [ts] expect_eq(page.context().pages(), [page]);
    // [ts] page.close();
    // [ts] expect_eq(page.isClosed(), true);
    // [ts] expect_eq(page.context().pages(), []);
    // [ts] });
    // [ts] test("ER-021 should reject in-flight worker.evaluate when page crashes", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43204" },
    // [ts] }, (_p) => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("worker"),
    // [ts] page.evaluate(() => Worker.init(URL.createObjectURL(
    // [ts] Blob.init(["self.onmessage = () => {}"], { type: "application/javascript" })))),
    // [ts] ]);
    // [ts] const closePromise = Promise.init(f => worker.once("close", f));
    // [ts] const evalPromise = worker.evaluate(() => Promise.init(() => {})).catch((e) => e); // never resolves in-worker
    // [ts] crash();
    // [ts] const error = evalPromise;
    // Chromium may detach the worker before it reports the crash.
    // [ts] expect(error.message).toMatch("Page crashed|Target page, context or browser has been closed");
    // [ts] closePromise;
    // [ts] expect_eq(page.workers(), []);
    // [ts] });
    // [ts] }
}
