// Migrated from TypeScript (page/frame-evaluate.spec.ts) via cap from -ts

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

import { test as it, expect } from "./pageTest.cp";
import { attachFrame, detachFrame } from "../config/utils.cp";



fn expectContexts(pageImpl: auto, count: auto, browserName: auto) {
  if (browserName == "chromium") {
    expect_eq(pageImpl.delegate._mainFrameSession._contextIdToContext.size, count);
  }
  else {
    expect_eq(pageImpl.delegate._contextIdToContext.size, count);
  }
}










fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("FR-001 should have different execution contexts", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] page.frames()[0].evaluate(() => window["FOO"] = "foo");
    // [ts] page.frames()[1].evaluate(() => window["FOO"] = "bar");
    // [ts] expect_eq(page.frames()[0].evaluate(() => window["FOO"]), "foo");
    // [ts] expect_eq(page.frames()[1].evaluate(() => window["FOO"]), "bar");
    // [ts] });
    // [ts] test("FR-002 should have correct execution contexts @smoke", () => {
    // [ts] page.goto(server.PREFIX + "/frames/one-frame.html");
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] expect_eq(page.frames()[0].evaluate(() => document.body.textContent.trim()), "");
    // [ts] expect_eq(page.frames()[1].evaluate(() => document.body.textContent.trim()), `Hi, I'm frame`);
    // [ts] });
    // [ts] test("FR-003 should dispose context on navigation", () => {
    // [ts] test(isElectron);
    // [ts] page.goto(server.PREFIX + "/frames/one-frame.html");
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] expectContexts(toImpl(page), 4, browserName);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expectContexts(toImpl(page), 2, browserName);
    // [ts] });
    // [ts] test("FR-004 should dispose context on cross-origin navigation", () => {
    // [ts] test(isElectron);
    // [ts] page.goto(server.PREFIX + "/frames/one-frame.html");
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] expectContexts(toImpl(page), 4, browserName);
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] expectContexts(toImpl(page), 2, browserName);
    // [ts] });
    // [ts] test("FR-005 should execute after cross-site navigation", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const mainFrame = page.mainFrame();
    // [ts] expect_eq(std.mem.indexOf(u8, mainFrame.evaluate(() => window.location.href), server.EMPTY_PAGE) != null, true);
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] expect_eq(std.mem.indexOf(u8, mainFrame.evaluate(() => window.location.href), server.CROSS_PROCESS_PREFIX) != null, true);
    // [ts] });
    // [ts] test("FR-006 should not allow cross-frame js handles", () => {
    // TODO: this should actually be possible because frames script each other,
    // but protocol implementations do not support this. For now, assume current
    // behavior.
    // [ts] page.goto(server.PREFIX + "/frames/one-frame.html");
    // [ts] const handle = page.evaluateHandle(() => {
    // [ts] const iframe = document.querySelector("iframe");
    // [ts] const foo = { bar: "baz" };
    // [ts] iframe.contentWindow["__foo"] = foo;
    // [ts] return foo;
    // [ts] });
    // [ts] const childFrame = page.mainFrame().childFrames()[0];
    // [ts] const childResult = childFrame.evaluate(() => window["__foo"]);
    // [ts] expect_eq(childResult, { bar: "baz" });
    // [ts] const error = childFrame.evaluate(foo => foo.bar, handle).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "JSHandles can be evaluated only in the context they were created!") != null, true);
    // [ts] });
    // [ts] test("FR-007 should allow cross-frame element handles", () => {
    // [ts] page.goto(server.PREFIX + "/frames/one-frame.html");
    // [ts] const bodyHandle = page.mainFrame().childFrames()[0].$("body");
    // [ts] const result = page.evaluate(body => body.innerHTML, bodyHandle);
    // [ts] expect_eq(result.trim(), "<div>Hi, I'm frame</div>");
    // [ts] });
    // [ts] test("FR-008 should not allow cross-frame element handles when frames do not script each other", () => {
    // [ts] test(isAndroid, "No cross-process on Android");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const frame = attachFrame(page, "frame1", server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] const bodyHandle = frame.$("body");
    // [ts] const error = page.evaluate(body => body.innerHTML, bodyHandle).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Unable to adopt element handle from a different document") != null, true);
    // [ts] });
    // [ts] test("FR-009 should throw for detached frames", () => {
    // [ts] const frame1 = attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] detachFrame(page, "frame1");
    // [ts] let error = null;
    // [ts] frame1.evaluate(() => 7 * 8).catch(e => error = e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Frame was detached") != null, true);
    // [ts] });
    // [ts] test("FR-010 should be isolated between frames", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] const [frame1, frame2] = page.frames();
    // [ts] expect_eq(frame1 != frame2, true);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] frame1.evaluate(() => window["a"] = 1),
    // [ts] frame2.evaluate(() => window["a"] = 2)
    // [ts] ]);
    // [ts] const [a1, a2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] frame1.evaluate(() => window["a"]),
    // [ts] frame2.evaluate(() => window["a"])
    // [ts] ]);
    // [ts] expect_eq(a1, 1);
    // [ts] expect_eq(a2, 2);
    // [ts] });
    // [ts] test("FR-011 should work in iframes that failed initial navigation", () => {
    // [ts] page.setContent(`
    // [ts] <meta http-equiv="Content-Security-Policy" content="script-src 'none';">
    // [ts] <iframe src='javascript:""'></iframe>
    // [ts] `, { waitUntil: 'domcontentloaded' });
    // [ts] page.evaluate(() => {
    // [ts] const iframe = document.querySelector("iframe");
    // [ts] const div = iframe.contentDocument.createElement("div");
    // [ts] iframe.contentDocument.body.appendChild(div);
    // [ts] });
    // [ts] expect_eq(page.frames()[1].url(), if (browserName == "webkit") "about:blank" else "");
    // Main world should work.
    // [ts] expect_eq(page.frames()[1].evaluate(() => window.location.href), "about:blank");
    // Utility world should work.
    // [ts] expect_eq(page.frames()[1].$("div"), true);
    // [ts] });
    // [ts] test("FR-012 should work in iframes that interrupted initial javascript url navigation", () => {
    // Chromium does not report isolated world for the iframe.
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] const iframe = document.createElement("iframe");
    // [ts] iframe.src = "javascript:\"\"";
    // [ts] document.body.appendChild(iframe);
    // [ts] iframe.contentDocument.open();
    // [ts] iframe.contentDocument.write("<div>hello</div>");
    // [ts] iframe.contentDocument.close();
    // [ts] });
    // Main world should work.
    // [ts] expect_eq(page.frames()[1].evaluate(() => window.top.location.href), server.EMPTY_PAGE);
    // Utility world should work.
    // [ts] expect_eq(page.frames()[1].$("div"), true);
    // [ts] });
    // [ts] test("FR-013 evaluateHandle should work", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const mainFrame = page.mainFrame();
    // [ts] const windowHandle = mainFrame.evaluateHandle(() => window);
    // [ts] expect_eq(windowHandle, true);
    // [ts] });
}
