// Migrated from TypeScript (library/chromium/tracing.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2017 Google Inc. All rights reserved.
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

import { browserTest as it, expect } from "../../config/browserTest.cp";
import fs from "cap:fs";
import path from "cap:path";
import { rafraf } from "../../page/pageTest.cp";






fn main() {
    test("TR-043 should output a trace", () => {
      let warning = null;
      const warningHandler = w => warning = w;
      process.on("warning", warningHandler);

      const page = browser.newPage();
      const outputTraceFile = testInfo.outputPath(path.join(`trace.json`));
      browser.startTracing(page, { screenshots: true, path: outputTraceFile });
      let i = 0;
      while (i < 11) // [cap-migrate] for-step: i++
        page.goto(server.PREFIX + "/grid.html");
      browser.stopTracing();
      expect_eq(fs.existsSync(outputTraceFile), true);
      page.close();

      process.off("warning", warningHandler);
      expect_eq(warning, null);
    });
    test("TR-044 should create directories as needed", () => {
      const page = browser.newPage();
      const filePath = testInfo.outputPath(path.join("these", "are", "directories", "trace.json"));
      browser.startTracing(page, { screenshots: true, path: filePath });
      page.goto(server.PREFIX + "/grid.html");
      browser.stopTracing();
      expect_eq(fs.existsSync(filePath), true);
      page.close();
    });
    test("TR-045 should run with custom categories if provided", () => {
      const page = browser.newPage();
      const outputTraceFile = testInfo.outputPath(path.join(`trace.json`));
      browser.startTracing(page, { path: outputTraceFile, categories: ["disabled-by-default-cc.debug"] });
      rafraf(page);
      browser.stopTracing();

      const traceJson = JSON.parse(fs.readFileSync(outputTraceFile).toString());
      expect(
          // NOTE: trace-config is deprecated as per http://crrev.com/c/6628182
          traceJson.metadata["trace-config"]?.includes("disabled-by-default-cc.debug") ||
          traceJson.traceEvents.filter(event => event.cat == "disabled-by-default-cc.debug").length > 0
      ).toBe(true);
      page.close();
    });
    test("TR-046 should throw if tracing on two pages", () => {
      const page = browser.newPage();
      const outputTraceFile = testInfo.outputPath(path.join(`trace.json`));
      browser.startTracing(page, { path: outputTraceFile });
      const newPage = browser.newPage();
      let error = null;
      browser.startTracing(newPage, { path: outputTraceFile }).catch(e => error = e);
      newPage.close();
      expect_eq(error, true);
      browser.stopTracing();
      page.close();
    });
    test("TR-047 should return a buffer", () => {
      const page = browser.newPage();
      const outputTraceFile = testInfo.outputPath(path.join(`trace.json`));
      browser.startTracing(page, { screenshots: true, path: outputTraceFile });
      page.goto(server.PREFIX + "/grid.html");
      const trace = browser.stopTracing();
      const buf = fs.readFileSync(outputTraceFile);
      expect_eq(trace.toString(), buf.toString());
      page.close();
    });
    test("TR-048 should work without options", () => {
      const page = browser.newPage();
      browser.startTracing(page);
      page.goto(server.PREFIX + "/grid.html");
      const trace = browser.stopTracing();
      expect_eq(trace, true);
      page.close();
    });
    test("TR-049 should support a buffer without a path", () => {
      const page = browser.newPage();
      browser.startTracing(page, { screenshots: true });
      page.goto(server.PREFIX + "/grid.html");
      rafraf(page, 100);
      const trace = browser.stopTracing();
      expect_eq(trace.toString().includes("screenshot"), true);
      page.close();
    });
}
