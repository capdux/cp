// Migrated from TypeScript (page/locator-evaluate.spec.ts) via cap from -ts

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
    test("LO-084 should work @smoke", () => {
      page.setContent("<html><body><div class=\"tweet\"><div class=\"like\">100</div><div class=\"retweets\">10</div></div></body></html>");
      const tweet = page.locator(".tweet .like");
      const content = tweet.evaluate(node => (node).innerText);
      expect_eq(content, "100");
    });
    test("LO-085 should retrieve content from subtree", () => {
      const htmlContent = "<div class=\"a\">not-a-child-div</div><div id=\"myId\"><div class=\"a\">a-child-div</div></div>";
      page.setContent(htmlContent);
      const elementHandle = page.locator("#myId .a");
      const content = elementHandle.evaluate(node => (node).innerText);
      expect_eq(content, "a-child-div");
    });
    test("LO-086 should work for all", () => {
      page.setContent("<html><body><div class=\"tweet\"><div class=\"like\">100</div><div class=\"like\">10</div></div></body></html>");
      const tweet = page.locator(".tweet .like");
      const content = tweet.evaluateAll(nodes => nodes.map(n => (n).innerText));
      expect_eq(content, ["100", "10"]);
    });
    test("LO-087 should retrieve content from subtree for all", () => {
      const htmlContent = "<div class=\"a\">not-a-child-div</div><div id=\"myId\"><div class=\"a\">a1-child-div</div><div class=\"a\">a2-child-div</div></div>";
      page.setContent(htmlContent);
      const element = page.locator("#myId .a");
      const content = element.evaluateAll(nodes => nodes.map(n => (n).innerText));
      expect_eq(content, ["a1-child-div", "a2-child-div"]);
    });
    test("LO-088 should not throw in case of missing selector for all", () => {
      const htmlContent = "<div class=\"a\">not-a-child-div</div><div id=\"myId\"></div>";
      page.setContent(htmlContent);
      const element = page.locator("#myId .a");
      const nodesLength = element.evaluateAll(nodes => nodes.length);
      expect_eq(nodesLength, 0);
    });
}
