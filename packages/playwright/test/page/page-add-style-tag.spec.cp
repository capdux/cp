// Migrated from TypeScript (page/page-add-style-tag.spec.ts) via cap from -ts

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

import { test as it, expect } from "./pageTest.cp";
import path from "cap:path";







fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("AT-013 should throw an error if no options are provided", () => {
    // [ts] let error = null;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // @ts-ignore
    // [ts] page.addStyleTag("/injectedstyle.css");
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] error = e;
    // [ts] }
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Provide an object with a `url`, `path` or `content` property") != null, true);
    // [ts] });
    // [ts] test("AT-014 should work with a url @smoke", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const styleHandle = page.addStyleTag({ url: "/injectedstyle.css" });
    // [ts] expect_eq(styleHandle.asElement() != null, true);
    // [ts] expect_eq(page.evaluate(`window.getComputedStyle(document.querySelector('body')).getPropertyValue('background-color')`), "rgb(255, 0, 0)");
    // [ts] });
    // [ts] test("AT-015 should throw an error if loading from url fail", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let error = null;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.addStyleTag({ url: "/nonexistfile.js" });
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] error = e;
    // [ts] }
    // [ts] expect_eq(error != null, true);
    // [ts] });
    // [ts] test("AT-016 should work with a path", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const styleHandle = page.addStyleTag({ path: asset("injectedstyle.css") });
    // [ts] expect_eq(styleHandle.asElement() != null, true);
    // [ts] expect_eq(page.evaluate(`window.getComputedStyle(document.querySelector('body')).getPropertyValue('background-color')`), "rgb(255, 0, 0)");
    // [ts] });
    // [ts] test("AT-017 should include sourceURL when path is provided", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.addStyleTag({ path: asset("injectedstyle.css") });
    // [ts] const styleHandle = page.$("style");
    // [ts] const styleContent = page.evaluate(style => style.innerHTML, styleHandle);
    // [ts] expect_eq(std.mem.indexOf(u8, styleContent, path.join("assets", "injectedstyle.css")) != null, true);
    // [ts] });
    // [ts] test("AT-018 should work with content", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const styleHandle = page.addStyleTag({ content: "body { background-color: green; }" });
    // [ts] expect_eq(styleHandle.asElement() != null, true);
    // [ts] expect_eq(page.evaluate(`window.getComputedStyle(document.querySelector('body')).getPropertyValue('background-color')`), "rgb(0, 128, 0)");
    // [ts] });
    // [ts] test("AT-019 should throw when added with content to the CSP page", () => {
    // [ts] page.goto(server.PREFIX + "/csp.html");
    // [ts] let error = null;
    // [ts] page.addStyleTag({ content: "body { background-color: green; }" }).catch(e => error = e);
    // [ts] expect_eq(error, true);
    // [ts] });
    // [ts] test("AT-020 should throw when added with URL to the CSP page", () => {
    // [ts] test(isAndroid, "No cross-process on Android");
    // [ts] page.goto(server.PREFIX + "/csp.html");
    // [ts] let error = null;
    // [ts] page.addStyleTag({ url: server.CROSS_PROCESS_PREFIX + "/injectedstyle.css" }).catch(e => error = e);
    // [ts] expect_eq(error, true);
    // [ts] });
}
