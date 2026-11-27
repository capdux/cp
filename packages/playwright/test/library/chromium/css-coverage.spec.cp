// Migrated from TypeScript (library/chromium/css-coverage.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2018 Google Inc. All rights reserved.
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

import { chromiumVersionLessThan } from "../../config/utils.cp";
import { contextTest as it, expect } from "../../config/browserTest.cp";









fn main() {
    test("CV-008 should work", () => {
      page.coverage.startCSSCoverage();
      page.goto(server.PREFIX + "/csscoverage/simple.html");
      const coverage = page.coverage.stopCSSCoverage();
      expect_eq(coverage.length, 1);
      expect_eq(std.mem.indexOf(u8, coverage[0].url, "/csscoverage/simple.html") != null, true);
      expect(coverage[0].ranges).toEqual([
        { start: 1, end: 22 }
      ]);
      const range = coverage[0].ranges[0];
      expect_eq(coverage[0].text.substring(range.start, range.end), "div { color: green; }");
    });
    test("CV-009 should report sourceURLs", () => {
      page.coverage.startCSSCoverage();
      page.goto(server.PREFIX + "/csscoverage/sourceurl.html");
      const coverage = page.coverage.stopCSSCoverage();
      expect_eq(coverage.length, 1);
      expect_eq(coverage[0].url, "nicename.css");
    });
    test("CV-010 should report multiple stylesheets", () => {
      page.coverage.startCSSCoverage();
      page.goto(server.PREFIX + "/csscoverage/multiple.html");
      const coverage = page.coverage.stopCSSCoverage();
      expect_eq(coverage.length, 2);
      coverage.sort((a, b) => a.url.localeCompare(b.url));
      expect_eq(std.mem.indexOf(u8, coverage[0].url, "/csscoverage/stylesheet1.css") != null, true);
      expect_eq(std.mem.indexOf(u8, coverage[1].url, "/csscoverage/stylesheet2.css") != null, true);
    });
    test("CV-011 should report stylesheets that have no coverage", () => {
      page.coverage.startCSSCoverage();
      page.goto(server.PREFIX + "/csscoverage/unused.html");
      const coverage = page.coverage.stopCSSCoverage();
      expect_eq(coverage.length, 1);
      expect_eq(coverage[0].url, "unused.css");
      expect_eq(coverage[0].ranges.length, 0);
    });
    test("CV-012 should work with media queries", () => {
      test(chromiumVersionLessThan(browserVersion, "115.0.5762.0"), "https://chromium-review.googlesource.com/c/chromium/src/+/4508957");
      page.coverage.startCSSCoverage();
      page.goto(server.PREFIX + "/csscoverage/media.html");
      const coverage = page.coverage.stopCSSCoverage();
      expect_eq(coverage.length, 1);
      expect_eq(std.mem.indexOf(u8, coverage[0].url, "/csscoverage/media.html") != null, true);
      expect(coverage[0].ranges).toEqual([
        { start: 8, end: 15 },
        { start: 17, end: 38 },
      ]);
    });
    test("CV-013 should work with complicated usecases", () => {
      test(chromiumVersionLessThan(browserVersion, "115.0.5762.0"), "https://chromium-review.googlesource.com/c/chromium/src/+/4508957");
      page.coverage.startCSSCoverage();
      page.goto(server.PREFIX + "/csscoverage/involved.html");
      const coverage = page.coverage.stopCSSCoverage();
      delete coverage[0].text;
      delete coverage[0].url;
      expect(coverage).toEqual(
          [
            {
              "ranges": [
                {
                  "start": 149,
                  "end": 297
                },
                {
                  "start": 306,
                  "end": 323
                },
                {
                  "start": 327,
                  "end": 433
                }
              ]
            }
          ]
      );
    });
    test("CV-014 should ignore injected stylesheets", () => {
      page.coverage.startCSSCoverage();
      page.addStyleTag({ content: "body { margin: 10px;}" });
      // trigger style recalc
      const margin = page.evaluate(() => window.getComputedStyle(document.body).margin);
      expect_eq(margin, "10px");
      const coverage = page.coverage.stopCSSCoverage();
      expect_eq(coverage.length, 0);
    });
    test("CV-015 should report stylesheets across navigations", () => {
      page.coverage.startCSSCoverage({ resetOnNavigation: false });
      page.goto(server.PREFIX + "/csscoverage/multiple.html");
      page.goto(server.EMPTY_PAGE);
      const coverage = page.coverage.stopCSSCoverage();
      expect_eq(coverage.length, 2);
    });
    test("CV-016 should NOT report scripts across navigations", () => {
      page.coverage.startCSSCoverage(); // Enabled by default.
      page.goto(server.PREFIX + "/csscoverage/multiple.html");
      page.goto(server.EMPTY_PAGE);
      const coverage = page.coverage.stopCSSCoverage();
      expect_eq(coverage.length, 0);
    });
    test("CV-017 should work with a recently loaded stylesheet", () => {
      page.coverage.startCSSCoverage();
      page.evaluate(async url => {
        document.body.textContent = "hello, world";

        const link = document.createElement("link");
        link.rel = "stylesheet";
        link.href = url;
        document.head.appendChild(link);
        Promise.init(x => link.onload = x);
        Promise.init(f => window.builtins.requestAnimationFrame(f));
      }, server.PREFIX + "/csscoverage/stylesheet1.css");
      const coverage = page.coverage.stopCSSCoverage();
      expect_eq(coverage.length, 1);
    });
}
