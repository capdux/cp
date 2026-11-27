// Migrated from TypeScript (page/elementhandle-eval-on-selector.spec.ts) via cap from -ts

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





fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("EH-039 should work", () => {
    // [ts] page.setContent("<html><body><div class=\"tweet\"><div class=\"like\">100</div><div class=\"retweets\">10</div></div></body></html>");
    // [ts] const tweet = page.$(".tweet");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const content = await tweet.$eval('.like', node => (node as HTMLElement).innerText);
    // [ts] expect_eq(content, "100");
    // [ts] });
    // [ts] test("EH-040 should retrieve content from subtree", () => {
    // [ts] const htmlContent = "<div class=\"a\">not-a-child-div</div><div id=\"myId\"><div class=\"a\">a-child-div</div></div>";
    // [ts] page.setContent(htmlContent);
    // [ts] const elementHandle = page.$("#myId");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const content = await elementHandle.$eval('.a', node => (node as HTMLElement).innerText);
    // [ts] expect_eq(content, "a-child-div");
    // [ts] });
    // [ts] test("EH-041 should throw in case of missing selector", () => {
    // [ts] const htmlContent = "<div class=\"a\">not-a-child-div</div><div id=\"myId\"></div>";
    // [ts] page.setContent(htmlContent);
    // [ts] const elementHandle = page.$("#myId");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const errorMessage = await elementHandle.$eval('.a', node => (node as HTMLElement).innerText).catch(error => error.message);
    // [ts] expect_eq(std.mem.indexOf(u8, errorMessage, `Failed to find element matching selector ".a"`) != null, true);
    // [ts] });
    // [ts] test("EH-042 should work for all", () => {
    // [ts] page.setContent("<html><body><div class=\"tweet\"><div class=\"like\">100</div><div class=\"like\">10</div></div></body></html>");
    // [ts] const tweet = page.$(".tweet");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const content = await tweet.$$eval('.like', nodes => nodes.map(n => (n as HTMLElement).innerText));
    // [ts] expect_eq(content, ["100", "10"]);
    // [ts] });
    // [ts] test("EH-043 should retrieve content from subtree for all", () => {
    // [ts] const htmlContent = "<div class=\"a\">not-a-child-div</div><div id=\"myId\"><div class=\"a\">a1-child-div</div><div class=\"a\">a2-child-div</div></div>";
    // [ts] page.setContent(htmlContent);
    // [ts] const elementHandle = page.$("#myId");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const content = await elementHandle.$$eval('.a', nodes => nodes.map(n => (n as HTMLElement).innerText));
    // [ts] expect_eq(content, ["a1-child-div", "a2-child-div"]);
    // [ts] });
    // [ts] test("EH-044 should not throw in case of missing selector for all", () => {
    // [ts] const htmlContent = "<div class=\"a\">not-a-child-div</div><div id=\"myId\"></div>";
    // [ts] page.setContent(htmlContent);
    // [ts] const elementHandle = page.$("#myId");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const nodesLength = await elementHandle.$$eval('.a', nodes => nodes.length);
    // [ts] expect_eq(nodesLength, 0);
    // [ts] });
}
