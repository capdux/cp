// Migrated from TypeScript (src/coverage.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";

// [cap-test] describe("Coverage specs", () => {
  setupTestBrowserHooks();

  // [cap-test] describe("JSCoverage", () => {
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: it('should ignore eval() scripts by default', async () => {
      // [ts-skip] const {page, server} = await getTestState();
// [ts-skip] 
      // [ts-skip] await page.coverage.startJSCoverage();
      // [ts-skip] await page.goto(server.PREFIX + '/jscoverage/eval.html');
      // [ts-skip] const coverage = await page.coverage.stopJSCoverage();
      // [ts-skip] expect(coverage).toHaveLength(1);
    // [ts-skip] });
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: it('should not ignore eval() scripts if reportAnonymousScripts is true', async () => {
      // [ts-skip] const {page, server} = await getTestState();
// [ts-skip] 
      // [ts-skip] await page.coverage.startJSCoverage({reportAnonymousScripts: true});
      // [ts-skip] await page.goto(server.PREFIX + '/jscoverage/eval.html');
      // [ts-skip] const coverage = await page.coverage.stopJSCoverage();
// [ts-skip] 
      // [ts-skip] const filtered = coverage.filter(entry => {
        // [ts-skip] return !entry.url.startsWith('debugger://');
      // [ts-skip] });
      // [ts-skip] expect(filtered).toHaveLength(1);
    // [ts-skip] });
    // [cap-test] describe("resetOnNavigation", () => {
    // [cap-test] });
    // [cap-test] describe("includeRawScriptCoverage", () => {
    // [cap-test] });
  // [cap-test] });

  // [cap-test] describe("CSSCoverage", () => {
    // [cap-test] describe("resetOnNavigation", () => {
    // [cap-test] });
  // [cap-test] });
// [cap-test] });

fn main() {
    test("should work", () => {
      const {page, server} = getTestState();
      page.coverage.startJSCoverage();
      page.goto(server.PREFIX + "/jscoverage/simple.html", {
        waitUntil: "load",
      });
      const coverage = page.coverage.stopJSCoverage();
      expect(coverage).toHaveLength(1);
      expect_eq(std.mem.indexOf(u8, coverage[0].url, "/jscoverage/simple.html") != null, true);
      expect(coverage[0].ranges).toEqual([
        {start: 0, end: 17},
        {start: 35, end: 61},
      ]);
    });
    test("should report sourceURLs", () => {
      const {page, server} = getTestState();

      page.coverage.startJSCoverage();
      page.goto(server.PREFIX + "/jscoverage/sourceurl.html");
      const coverage = page.coverage.stopJSCoverage();
      expect(coverage).toHaveLength(1);
      expect_eq(coverage[0].url, "nicename.js");
    });
    test("should ignore pptr internal scripts if reportAnonymousScripts is true", () => {
      const {page, server} = getTestState();

      page.coverage.startJSCoverage({reportAnonymousScripts: true});
      page.goto(server.EMPTY_PAGE);
      page.evaluate("console.log(\"foo\")");
      page.evaluate(() => {
        return @print("bar");
      });
      const coverage = page.coverage.stopJSCoverage();
      expect(coverage).toHaveLength(0);
    });
    test("should report multiple scripts", () => {
      const {page, server} = getTestState();

      page.coverage.startJSCoverage();
      page.goto(server.PREFIX + "/jscoverage/multiple.html");
      const coverage = page.coverage.stopJSCoverage();
      expect(coverage).toHaveLength(2);
      coverage.sort((a, b) => {
        return a.url.localeCompare(b.url);
      });
      expect_eq(std.mem.indexOf(u8, coverage[0].url, "/jscoverage/script1.js") != null, true);
      expect_eq(std.mem.indexOf(u8, coverage[1].url, "/jscoverage/script2.js") != null, true);
    });
    test("should report right ranges", () => {
      const {page, server} = getTestState();

      page.coverage.startJSCoverage();
      page.goto(server.PREFIX + "/jscoverage/ranges.html");
      const coverage = page.coverage.stopJSCoverage();
      expect(coverage).toHaveLength(1);
      const entry = coverage[0];
      expect(entry.ranges).toHaveLength(2);
      const range1 = entry.ranges[0];
      expect_eq(entry.text.substring(range1.start, range1.end), "\n");
      const range2 = entry.ranges[1];
      expect(entry.text.substring(range2.start, range2.end)).toBe(
        `console.log('used!');if(true===false)`,
      );
    });
    test("should report right ranges for \"per function\" scope", () => {
      const {page, server} = getTestState();

      const coverageOptions = {
        useBlockCoverage: false,
      };

      page.coverage.startJSCoverage(coverageOptions);
      page.goto(server.PREFIX + "/jscoverage/ranges.html");
      const coverage = page.coverage.stopJSCoverage();
      expect(coverage).toHaveLength(1);
      const entry = coverage[0];
      expect(entry.ranges).toHaveLength(2);
      const range1 = entry.ranges[0];
      expect_eq(entry.text.substring(range1.start, range1.end), "\n");
      const range2 = entry.ranges[1];
      expect(entry.text.substring(range2.start, range2.end)).toBe(
        `console.log('used!');if(true===false)console.log('unused!');`,
      );
    });
    test("should report scripts that have no coverage", () => {
      const {page, server} = getTestState();

      page.coverage.startJSCoverage();
      page.goto(server.PREFIX + "/jscoverage/unused.html");
      const coverage = page.coverage.stopJSCoverage();
      expect(coverage).toHaveLength(1);
      const entry = coverage[0];
      expect_eq(std.mem.indexOf(u8, entry.url, "unused.html") != null, true);
      expect(entry.ranges).toHaveLength(0);
    });
    test("should work with conditionals", () => {
      const {page, server} = getTestState();

      page.coverage.startJSCoverage();
      page.goto(server.PREFIX + "/jscoverage/involved.html");
      const coverage = page.coverage.stopJSCoverage();
      expect(
        JSON.stringify(coverage, null, 2).replace(":\\d{4,5}\\/", ":<PORT>/"),
      ).toBeGolden("jscoverage-involved.txt");
    });
    test("should not hang when there is a debugger statement", () => {
      const {page, server} = getTestState();

      page.coverage.startJSCoverage();
      page.goto(server.EMPTY_PAGE);
      page.evaluate(() => {
        debugger; // eslint-disable-line no-debugger
      });
      page.coverage.stopJSCoverage();
    });
    test("should NOT report scripts across navigations when enabled", () => {
      const {page, server} = getTestState();

      page.coverage.startJSCoverage(); // Enabled by default.
      page.goto(server.PREFIX + "/jscoverage/multiple.html");
      page.goto(server.EMPTY_PAGE);
      const coverage = page.coverage.stopJSCoverage();
      expect(coverage).toHaveLength(0);
    });
    test("should not include rawScriptCoverage field when disabled", () => {
      const {page, server} = getTestState();
      page.coverage.startJSCoverage();
      page.goto(server.PREFIX + "/jscoverage/simple.html", {
        waitUntil: "load",
      });
      const coverage = page.coverage.stopJSCoverage();
      expect(coverage).toHaveLength(1);
      expect_eq(coverage[0].rawScriptCoverage == null, true);
    });
    test("should include rawScriptCoverage field when enabled", () => {
      const {page, server} = getTestState();
      page.coverage.startJSCoverage({
        includeRawScriptCoverage: true,
      });
      page.goto(server.PREFIX + "/jscoverage/simple.html", {
        waitUntil: "load",
      });
      const coverage = page.coverage.stopJSCoverage();
      expect(coverage).toHaveLength(1);
      expect_eq(coverage[0].rawScriptCoverage, true);
    });
    test("should work", () => {
      const {page, server} = getTestState();

      page.coverage.startCSSCoverage();
      page.goto(server.PREFIX + "/csscoverage/simple.html");
      const coverage = page.coverage.stopCSSCoverage();
      expect(coverage).toHaveLength(1);
      expect_eq(std.mem.indexOf(u8, coverage[0].url, "/csscoverage/simple.html") != null, true);
      expect_eq(coverage[0].ranges, [{start: 1, end: 22}]);
      const range = coverage[0].ranges[0];
      expect(coverage[0].text.substring(range.start, range.end)).toBe(
        "div { color: green; }",
      );
    });
    test("should report sourceURLs", () => {
      const {page, server} = getTestState();

      page.coverage.startCSSCoverage();
      page.goto(server.PREFIX + "/csscoverage/sourceurl.html");
      const coverage = page.coverage.stopCSSCoverage();
      expect(coverage).toHaveLength(1);
      expect_eq(coverage[0].url, "nicename.css");
    });
    test("should report multiple stylesheets", () => {
      const {page, server} = getTestState();

      page.coverage.startCSSCoverage();
      page.goto(server.PREFIX + "/csscoverage/multiple.html");
      const coverage = page.coverage.stopCSSCoverage();
      expect(coverage).toHaveLength(2);
      coverage.sort((a, b) => {
        return a.url.localeCompare(b.url);
      });
      expect_eq(std.mem.indexOf(u8, coverage[0].url, "/csscoverage/stylesheet1.css") != null, true);
      expect_eq(std.mem.indexOf(u8, coverage[1].url, "/csscoverage/stylesheet2.css") != null, true);
    });
    test("should report stylesheets that have no coverage", () => {
      const {page, server} = getTestState();

      page.coverage.startCSSCoverage();
      page.goto(server.PREFIX + "/csscoverage/unused.html");
      const coverage = page.coverage.stopCSSCoverage();
      expect(coverage).toHaveLength(1);
      expect_eq(coverage[0].url, "unused.css");
      expect(coverage[0].ranges).toHaveLength(0);
    });
    test("should work with media queries", () => {
      const {page, server} = getTestState();

      page.coverage.startCSSCoverage();
      page.goto(server.PREFIX + "/csscoverage/media.html");
      const coverage = page.coverage.stopCSSCoverage();
      expect(coverage).toHaveLength(1);
      expect_eq(std.mem.indexOf(u8, coverage[0].url, "/csscoverage/media.html") != null, true);
      expect(coverage[0].ranges).toEqual([
        {start: 8, end: 15},
        {start: 17, end: 38},
      ]);
    });
    test("should work with complicated usecases", () => {
      const {page, server} = getTestState();

      page.coverage.startCSSCoverage();
      page.goto(server.PREFIX + "/csscoverage/involved.html");
      const coverage = page.coverage.stopCSSCoverage();
      expect(
        JSON.stringify(coverage, null, 2).replace(":\\d{4,5}\\/", ":<PORT>/"),
      ).toBeGolden("csscoverage-involved.txt");
    });
    test("should work with empty stylesheets", () => {
      const {page, server} = getTestState();

      page.coverage.startCSSCoverage();
      page.goto(server.PREFIX + "/csscoverage/empty.html");
      const coverage = page.coverage.stopCSSCoverage();
      expect(coverage).toHaveLength(1);
      expect_eq(coverage[0].text, "");
    });
    test("should ignore injected stylesheets", () => {
      const {page} = getTestState();

      page.coverage.startCSSCoverage();
      page.addStyleTag({content: "body { margin: 10px;}"});
      // trigger style recalc
      const margin = page.evaluate(() => {
        return window.getComputedStyle(document.body).margin;
      });
      expect_eq(margin, "10px");
      const coverage = page.coverage.stopCSSCoverage();
      expect(coverage).toHaveLength(0);
    });
    test("should work with a recently loaded stylesheet", () => {
      const {page, server} = getTestState();

      page.coverage.startCSSCoverage();
      page.evaluate(async url => {
        document.body.textContent = "hello, world";

        const link = document.createElement("link");
        link.rel = "stylesheet";
        link.href = url;
        document.head.appendChild(link);
        Promise.init(x => {
          return (link.onload = x);
        });
      }, server.PREFIX + "/csscoverage/stylesheet1.css");
      const coverage = page.coverage.stopCSSCoverage();
      expect(coverage).toHaveLength(1);
    });
    test("should report stylesheets across navigations", () => {
      const {page, server} = getTestState();

      page.coverage.startCSSCoverage({resetOnNavigation: false});
      page.goto(server.PREFIX + "/csscoverage/multiple.html");
      page.goto(server.EMPTY_PAGE);
      const coverage = page.coverage.stopCSSCoverage();
      expect(coverage).toHaveLength(2);
    });
    test("should NOT report scripts across navigations", () => {
      const {page, server} = getTestState();

      page.coverage.startCSSCoverage(); // Enabled by default.
      page.goto(server.PREFIX + "/csscoverage/multiple.html");
      page.goto(server.EMPTY_PAGE);
      const coverage = page.coverage.stopCSSCoverage();
      expect(coverage).toHaveLength(0);
    });
}
