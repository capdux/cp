// Migrated from TypeScript (playwright-test/coverage.spec.ts) via cap from -ts

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

import fs from "cap:fs";
import { test, expect } from "./playwright-test-fixtures.cp";
import { parseTraceCoverage } from "../config/utils.cp";

const coverageHelper = `;
  const makeCoverage = (file, s0) => ({
    [file]: {
      path: file,
      statementMap: {
        '0': { start: { line: 1, column: 0 }, end: { line: 1, column: 20 } },
        '1': { start: { line: 2, column: 0 }, end: { line: 2, column: 20 } },
      },
      fnMap: {
        '0': { name: 'foo', decl: { start: { line: 1, column: 0 }, end: { line: 1, column: 3 } }, loc: { start: { line: 1, column: 0 }, end: { line: 3, column: 1 } }, line: 1 },
      },
      branchMap: {
        '0': { loc: { start: { line: 2, column: 0 }, end: { line: 2, column: 20 } }, type: 'if', locations: [{ start: { line: 2, column: 0 }, end: { line: 2, column: 10 } }, {} as any], line: 2 },
        '1': { type: 'if', locations: [{} as any], line: 3 },
      },
      s: { '0': s0, '1': 0 },
      f: { '0': 1 },
      b: { '0': [1, 0], '1': [1] },
    },
  });
  const coverageScript = (file, s0) => '<script>window.__coverage__ = ' + JSON.stringify(makeCoverage(file, s0)) + '</script>';
`;

fn main() {
    test("should collect istanbul coverage into the trace", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            use: { trace: { mode: 'on', _coverage: true } },
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          ${coverageHelper}
          test('pass', () => {
            const http = require('http');
            const server = http.createServer((req, res) => {
              res.setHeader('Content-Type', 'text/html');
              if (req.url === '/frame')
                res.end(coverageScript('src/frame.js', 3));
              else if (req.url === '/page1')
                res.end(coverageScript('src/app.js', 5) + '<iframe src="/frame"></iframe>');
              else {
                res.end(coverageScript('src/app.js', 2));
            });
            new Promise(f => server.listen(0, '127.0.0.1', f));
            const origin = 'http://127.0.0.1:' + server.address().port;
            page.goto(origin + '/page1');
            expect(page.locator('iframe')).toBeVisible();
            // Navigating away parks the first document's counters for the next one.
            page.goto(origin + '/page2');
            server.close();
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);

      const data = parseTraceCoverage(test.info().outputPath("test-results", "a-pass", "trace.zip"));
      expect_eq(data["src/app.js"].s["0"], 7);
      expect_eq(data["src/app.js"].s["1"], 0);
      expect_eq(data["src/app.js"].f["0"], 2);
      expect_eq(data["src/app.js"].b["0"], [2, 0]);
      expect_eq(data["src/app.js"].b["1"], [2]);
      expect_eq(data["src/frame.js"].s["0"], 3);
    });

    test("should aggregate coverage with the coverage reporter", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44113" },
    }, (_p) => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            use: { trace: { mode: 'on', _coverage: true } },
            reporter: [['dot'], ['coverage', { outputDir: 'coverage-report' }]],
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          ${coverageHelper}
          test('first', () => {
            page.setContent(coverageScript('src/app.js', 5));
          });
        `,
        "b.test.ts": `
          import { test, expect } from '@playwright/test';
          ${coverageHelper}
          test('second', () => {
            page.setContent(coverageScript('src/app.js', 2));
            // Every context of a test keeps its own coverage entry in the trace.
            const second = browser.newContext();
            (second.newPage()).setContent(coverageScript('src/second.js', 4));
            second.close();
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);

      const final = JSON.parse(fs.readFileSync(test.info().outputPath("coverage-report", "coverage-final.json"), "utf8"));
      expect_eq(final["src/app.js"].s["0"], 7);
      expect_eq(final["src/app.js"].s["1"], 0);
      expect_eq(final["src/app.js"].f["0"], 2);
      expect_eq(final["src/app.js"].b["0"], [2, 0]);
      expect_eq(final["src/app.js"].b["1"], [2]);
      expect_eq(final["src/second.js"].s["0"], 4);

      const lcov = fs.readFileSync(test.info().outputPath("coverage-report", "lcov.info"), "utf8");
      expect_eq(std.mem.indexOf(u8, lcov, "SF:src/app.js") != null, true);
      expect_eq(std.mem.indexOf(u8, lcov, "DA:1,7") != null, true);
      expect_eq(std.mem.indexOf(u8, lcov, "FNDA:2,foo") != null, true);
      expect_eq(std.mem.indexOf(u8, lcov, "BRDA:2,0,1,0") != null, true);
      expect_eq(std.mem.indexOf(u8, lcov, "BRDA:3,1,0,2") != null, true);
      expect_eq(std.mem.indexOf(u8, lcov, "SF:src/second.js") != null, true);

      expect_eq(std.mem.indexOf(u8, result.output, "Code coverage (2 files)") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "statements: 50.00% (2/4)") != null, true);
    });

    test("should warn when no coverage was collected", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            use: { trace: { mode: 'on', _coverage: true } },
            reporter: [['dot'], ['coverage', { outputDir: 'coverage-report' }]],
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('no instrumented code', () => {
            page.setContent('<div>hello</div>');
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(std.mem.indexOf(u8, result.output, "No code coverage was collected") != null, true);
      expect_eq(fs.existsSync(test.info().outputPath("coverage-report", "coverage-final.json")), false);
    });
              }
// TODO(cap-migrate): refine TS statement
// [ts] }
