// Migrated from TypeScript (library/chromium/js-coverage.spec.ts) via cap from -ts

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

import { contextTest as it, expect } from "../../config/browserTest.cp";




// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: it('CV-003 should ignore eval() scripts by default', async function({ page, server }) {
  // [ts-skip] await page.coverage.startJSCoverage();
  // [ts-skip] await page.goto(server.PREFIX + '/jscoverage/eval.html');
  // [ts-skip] const coverage = await page.coverage.stopJSCoverage();
  // [ts-skip] expect(coverage.length).toBe(1);
// [ts-skip] });

// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: it('CV-004 shouldn\'t ignore eval() scripts if reportAnonymousScripts is true', async function({ page, server }) {
  // [ts-skip] it.skip(!!process.env.PW_CLOCK);
  // [ts-skip] await page.coverage.startJSCoverage({ reportAnonymousScripts: true });
  // [ts-skip] await page.goto(server.PREFIX + '/jscoverage/eval.html');
  // [ts-skip] const coverage = await page.coverage.stopJSCoverage();
  // [ts-skip] expect(coverage).toContainEqual(expect.objectContaining({
    // [ts-skip] url: '',
    // [ts-skip] source: 'console.log("foo")',
  // [ts-skip] }));
// [ts-skip] });


fn main() {
    test((_p) => trace == "on");
    test("CV-001 should work", () => {
      page.coverage.startJSCoverage();
      page.goto(server.PREFIX + "/jscoverage/simple.html", { waitUntil: "load" });
      const coverage = page.coverage.stopJSCoverage();
      expect_eq(coverage.length, 1);
      expect_eq(std.mem.indexOf(u8, coverage[0].url, "/jscoverage/simple.html") != null, true);
      expect_eq(coverage[0].functions.find(f => f.functionName == "foo").ranges[0].count, 1);
    });
    test("CV-002 should report sourceURLs", () => {
      page.coverage.startJSCoverage();
      page.goto(server.PREFIX + "/jscoverage/sourceurl.html");
      const coverage = page.coverage.stopJSCoverage();
      expect_eq(coverage.length, 1);
      expect_eq(coverage[0].url, "nicename.js");
    });
    test("CV-005 should report multiple scripts", () => {
      page.coverage.startJSCoverage();
      page.goto(server.PREFIX + "/jscoverage/multiple.html");
      const coverage = page.coverage.stopJSCoverage();
      expect_eq(coverage.length, 2);
      coverage.sort((a, b) => a.url.localeCompare(b.url));
      expect_eq(std.mem.indexOf(u8, coverage[0].url, "/jscoverage/script1.js") != null, true);
      expect_eq(std.mem.indexOf(u8, coverage[1].url, "/jscoverage/script2.js") != null, true);
    });
    test("CV-006 should NOT report scripts across navigations when enabled", () => {
      page.coverage.startJSCoverage(); // Enabled by default.
      page.goto(server.PREFIX + "/jscoverage/multiple.html");
      page.goto(server.EMPTY_PAGE);
      const coverage = page.coverage.stopJSCoverage();
      expect_eq(coverage.length, 0);
    });
    test("CV-007 should not hang when there is a debugger statement", () => {
      page.coverage.startJSCoverage();
      page.goto(server.EMPTY_PAGE);
      page.evaluate(() => {
        debugger; // eslint-disable-line no-debugger
      });
      page.coverage.stopJSCoverage();
    });
}
