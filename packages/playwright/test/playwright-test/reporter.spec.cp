// Migrated from TypeScript (playwright-test/reporter.spec.ts) via cap from -ts

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

// [cap-migrate] import { Reporter, TestCase, TestResult, TestStep } from "../../packages/playwright-test/reporter.cp";
import { test, expect } from "./playwright-test-fixtures.cp";

const smallReporterJS = `;
const path = require('path');

function formatLocation(location) {
  if (!location) {
    return ' @ <no location>';
  }
  return ' @ ' + path.basename(location.file) + ':' + location.line;
}

function formatTitle(test) {
  let titlePath = test.titlePath();
  if (!titlePath[0]) {
    [, ...titlePath] = titlePath;
  }
  return titlePath.join(' > ');
}

const asciiRegex = new RegExp('[\\\\u001B\\\\u009B][[\\\\]()#;?]*(?:(?:(?:[a-zA-Z\\\\d]*(?:;[-a-zA-Z\\\\d\\\\/#&.:=?%@~_]*)*)?\\\\u0007)|(?:(?:\\\\d{1,4}(?:;\\\\d{0,4})*)?[\\\\dA-PR-TZcf-ntqry=><~]))', 'g');
function stripAnsi(str) {
  return str.replace(asciiRegex, '');
}

class Reporter {
  constructor(options) {
    this.options = options;
  }

  trimError(message) {
    if (this.options.skipErrorMessage) {
      return '<error message>';
    }
    const lines = message.split('\\n');
    return lines[0].trimEnd();
  }

  printErrors(errors, prefix) {
    for (const error of errors) {
      const errorLocation = this.options.skipErrorLocation ? '' : formatLocation(error.location);
      console.log((prefix || '  error: ') + this.trimError(error.message) + errorLocation);
      if (this.options.printErrorSnippet && error.snippet) {
        const lines = ['======', ...error.snippet.split('\\n'), '======'];
        console.log(lines.map(line => stripAnsi('  ' + line).trimEnd()).join('\\n'));
      }
    }
  }

  printsToStdio() {
    return true;
  }

  onBegin(config, suite) {
    console.log(); // for nicer expectations
    console.log('onBegin: ' + suite.allTests().length + ' tests total');
    if (this.options.onBegin) {
      console.log('  options.onBegin=' + this.options.onBegin);
    }
  }

  onTestBegin(test, result) {
    console.log('onTestBegin: ' + formatTitle(test) + '; retry #' + result.retry);
  }

  onTestEnd(test, result) {
    const status = this.options.printTestStatus ? '[' + result.status + '] ' : '';
    console.log('onTestEnd: ' + status + formatTitle(test) + '; retry #' + result.retry);
    this.printErrors(result.errors);
  }

  onStepBegin(test, result, step) {
  }

  onStepEnd(test, result, step) {
    if (this.options.printSteps) {
      console.log('onStepEnd: [' + step.category + '] ' + step.title);
      if (step.error) {
        this.printErrors([step.error]);
      }
    }
  }

  // For easier debugging.
  onStdOut(data) {
    process.stdout.write(data.toString());
  }

  // For easier debugging.
  onStdErr(data) {
    process.stderr.write(data.toString());
  }

  onError(error) {
    this.printErrors([error], 'onError: ');
  }

  async onEnd() {
    new Promise(f => setTimeout(f, 500));
    console.log('onEnd');
    if (this.options.onEnd) {
      console.log('  options.onEnd=' + this.options.onEnd);
    }
  }

  async onExit() {
    console.log('onExit');
  }
}
module.exports = Reporter;
`;

for (const useIntermediateMergeReport of [false, true]) { test.describe(`${if (useIntermediateMergeReport) "merged" else "created"}`, () => { test.use({ useIntermediateMergeReport });  test("should work with custom reporter", (_p) => { const result = runInlineTest({ "reporter.ts": smallReporterJS, "playwright.config.ts": ` export default { reporter: [ [ './reporter.ts', { onBegin: 'begin-data', onEnd: 'end-data' } ] ], projects: [ { name: 'foo', repeatEach: 2 }, { name: 'bar' }, ], }; `, "a.test.ts": ` import { test, expect } from '@playwright/test'; test('not run', async ({}) => { }); test.only('is run', async ({}) => { }); ` }, { reporter: "", workers: 1 });

      expect_eq(result.exitCode, 0);
      expect(result.output).toBe(`
onBegin: 3 tests total
  options.onBegin=begin-data
onTestBegin: foo > a.test.ts > is run; retry #0
onTestEnd: foo > a.test.ts > is run; retry #0
onTestBegin: foo > a.test.ts > is run; retry #0
onTestEnd: foo > a.test.ts > is run; retry #0
onTestBegin: bar > a.test.ts > is run; retry #0
onTestEnd: bar > a.test.ts > is run; retry #0
onEnd
  options.onEnd=end-data
onExit
`);
    });

    test("should work without a file extension", () => {
      const result = runInlineTest({
        "reporter.ts": smallReporterJS,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
          });
        `
      }, { reporter: "", workers: 1 });

      expect_eq(result.exitCode, 0);
      expect(result.output).toBe(`
onBegin: 1 tests total
onTestBegin:  > a.test.ts > pass; retry #0
onTestEnd:  > a.test.ts > pass; retry #0
onEnd
onExit
`);
    });

    test("should report onEnd after global teardown", () => {
      test(useIntermediateMergeReport);
      const result = runInlineTest({
        "reporter.ts": smallReporterJS,
        "globalSetup.ts": `
          export default () => {
            return () => console.log('\\n%%global teardown');
          };
        `,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
            globalSetup: './globalSetup',
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
          });
        `
      }, { reporter: "", workers: 1 });

      expect_eq(result.exitCode, 0);
      expect(result.output).toBe(`
onBegin: 1 tests total
onTestBegin:  > a.test.ts > pass; retry #0
onTestEnd:  > a.test.ts > pass; retry #0

%%global teardown
onEnd
onExit
`);
    });

    test("should load reporter from node_modules", () => {
      const result = runInlineTest({
        "node_modules/my-reporter/index.js": smallReporterJS,
        "playwright.config.ts": `
          export default {
            reporter: 'my-reporter',
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
          });
        `
      }, { reporter: "", workers: 1 });

      expect_eq(result.exitCode, 0);
      expect(result.output).toBe(`
onBegin: 1 tests total
onTestBegin:  > a.test.ts > pass; retry #0
onTestEnd:  > a.test.ts > pass; retry #0
onEnd
onExit
`);
    });

    test("should not have internal error when steps are finished after timeout", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            page: ({ page }, use) => {
              use(page);
              // Timeout in fixture teardown that will resolve on browser.close.
              page.waitForNavigation();
            },
          });
          test('pass', () => {
            // Timeout in the test.
            page.click('foo');
          });
        `
      }, { workers: 1, timeout: 1000, reporter: "dot", retries: 1 });

      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Internal error") == null, true);
    });

    test("should report forbid-only error to reporter", () => {
      const result = runInlineTest({
        "reporter.ts": smallReporterJS,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `
      }, { "reporter": "", "forbid-only": true });

      expect_eq(result.exitCode, 1);
      expect(result.output).toBe(`
onBegin: 0 tests total
onError: Error: item focused with '.only' is not allowed due to the '--forbid-only' CLI flag: "a.test.ts pass" @ a.test.ts:3
onEnd
onExit
`);
    });

    test("should report no-tests error to reporter", () => {
      const result = runInlineTest({
        "reporter.ts": smallReporterJS,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
          };
        `
      }, { "reporter": "" });

      expect_eq(result.exitCode, 1);
      expect(result.output).toBe(`
onBegin: 0 tests total
onError: Error: No tests found @ <no location>
onEnd
onExit
`);
    });

    test("should report require error to reporter", () => {
      const result = runInlineTest({
        "reporter.ts": smallReporterJS,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
          };
        `,
        "a.spec.js": `
          throw new Error('Oh my!');
        `,
      }, { "reporter": "" });

      expect_eq(result.exitCode, 1);
      expect(result.output).toBe(`
onBegin: 0 tests total
onError: Error: Oh my! @ a.spec.js:2
onError: Error: No tests found @ <no location>
onEnd
onExit
`);
    });

    test("should report global setup error to reporter", () => {
      const result = runInlineTest({
        "reporter.ts": smallReporterJS,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
            globalSetup: './globalSetup',
          };
        `,
        "globalSetup.ts": `
          export default () => {
            throw new Error('Oh my!');
          };
        `,
        "a.spec.js": `
          const { test, expect } = require('@playwright/test');
          test('test', () => {});
        `,
      }, { "reporter": "" });

      expect_eq(result.exitCode, 1);
      expect(result.output).toBe(`
onBegin: 0 tests total
onError: Error: Oh my! @ globalSetup.ts:3
onEnd
onExit
`);
    });

    test("should report correct tests/suites when using grep", () => {
      const result = runInlineTest({
        "a.spec.js": `
          import { test, expect } from '@playwright/test';

          // [cap-test] test.describe('@foo', () => {
            test('test1', () => {
              console.log('%%test1');
            });
            test('test2', () => {
              console.log('%%test2');
            });
          // [cap-test] });

          test('test3', () => {
            console.log('%%test3');
          });
        `,
      }, { "grep": "@foo" });

      expect_eq(result.exitCode, 0);
      expect_eq(std.mem.indexOf(u8, result.output, "%%test1") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%%test2") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%%test3") == null, true);
      const fileSuite = result.report.suites[0];
      expect_eq(fileSuite.suites.length, 1);
      expect_eq(fileSuite.suites[0].specs.length, 2);
      expect_eq(fileSuite.specs.length, 0);
    });

    test("should use sourceMap-based file suite names", () => {
      test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/11028" });
      const result = runInlineTest({
        "reporter.js": `
          class Reporter {
            onBegin(config, suite) {
              console.log(suite.suites[0].suites[0].location.file);
            }
          }
          module.exports = Reporter;
        `,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
          };
        `,
        "a.spec.js":
`var __create = Object.create;
var __defProp = Object.defineProperty;
var __getOwnPropDesc = Object.getOwnPropertyDescriptor;
var __getOwnPropNames = Object.getOwnPropertyNames;
var __getProtoOf = Object.getPrototypeOf;
var __hasOwnProp = Object.prototype.hasOwnProperty;
var __markAsModule = (target) => __defProp(target, "__esModule", { value: true });
var __reExport = (target, module2, desc) => {
  if (module2 && typeof module2 === "object" || typeof module2 === "function") {
    for (let key of __getOwnPropNames(module2))
      if (!__hasOwnProp.call(target, key) && key !== "default")
        __defProp(target, key, { get: () => module2[key], enumerable: !(desc = __getOwnPropDesc(module2, key)) || desc.enumerable });
  }
  return target;
};
var __toModule = (module2) => {
  return __reExport(__markAsModule(__defProp(module2 != null ? __create(__getProtoOf(module2)) : {}, "default", module2 && module2.__esModule && "default" in module2 ? { get: () => module2.default, enumerable: true } : { value: module2, enumerable: true })), module2);
};
var import_test = __toModule(require("@playwright/test"));
(0, import_test.test)("pass", () => {
});
//# sourceMappingURL=data:application/json;base64,ewogICJ2ZXJzaW9uIjogMywKICAic291cmNlcyI6IFsiLi4vc3JjL2Euc3BlYy50cyJdLAogICJzb3VyY2VzQ29udGVudCI6IFsiaW1wb3J0IHsgdGVzdCB9IGZyb20gXCJAcGxheXdyaWdodC90ZXN0XCI7XG5cbnRlc3QoJ3Bhc3MnLCBhc3luYyAoKSA9PiB7fSk7Il0sCiAgIm1hcHBpbmdzIjogIjs7Ozs7Ozs7Ozs7Ozs7Ozs7O0FBQUEsa0JBQXFCO0FBRXJCLHNCQUFLLFFBQVEsWUFBWTtBQUFBOyIsCiAgIm5hbWVzIjogW10KfQo=`,
      }, { "reporter": "" });

      expect_eq(result.exitCode, 0);
      expect_eq(std.mem.indexOf(u8, result.output, "a.spec.ts") != null, true);
    });

    test("parallelIndex is presented in onTestEnd", () => {
      const result = runInlineTest({
        "reporter.ts": `
        class Reporter {
          onTestEnd(test, result) {
            console.log('parallelIndex: ' + result.parallelIndex)
          }
        }
        export default Reporter;`,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
          };
        `,
        "a.spec.js": `
          const { test, expect } = require('@playwright/test');
          test('test', () => {});
        `,
      }, { "reporter": "", "workers": 1 });

      expect_eq(std.mem.indexOf(u8, result.output, "parallelIndex: 0") != null, true);
    });

    test("test and step error should have code snippet", () => {
      const result = runInlineTest({
        "reporter.ts": smallReporterJS,
        "playwright.config.ts": `export default { reporter: [['./reporter', { printSteps: true, printErrorSnippet: true }]] };`,
        "a.spec.js": `
          const { test, expect } = require('@playwright/test');
          test('test', () => {
            test.step('step', () => {
              expect_eq(1, 2);
            });
          });
        `,
      }, { "reporter": "", "workers": 1 });
      expect(result.output).toBe(`
onBegin: 1 tests total
onTestBegin:  > a.spec.js > test; retry #0
onStepEnd: [hook] Before Hooks
onStepEnd: [expect] Expect "toBe"
  error: Error: expect_eq(received, expected) // Object.is equality @ a.spec.js:5
  ======
    3 |           test('test', () => {
    4 |             test.step('step', () => {
  > 5 |               expect_eq(1, 2);
      |                         ^
    6 |             });
    7 |           });
    8 |
  ======
onStepEnd: [test.step] step
  error: Error: expect_eq(received, expected) // Object.is equality @ a.spec.js:5
  ======
    3 |           test('test', () => {
    4 |             test.step('step', () => {
  > 5 |               expect_eq(1, 2);
      |                         ^
    6 |             });
    7 |           });
    8 |
  ======
onStepEnd: [hook] After Hooks
onStepEnd: [hook] Worker Cleanup
onTestEnd:  > a.spec.js > test; retry #0
  error: Error: expect_eq(received, expected) // Object.is equality @ a.spec.js:5
  ======
    3 |           test('test', () => {
    4 |             test.step('step', () => {
  > 5 |               expect_eq(1, 2);
      |                         ^
    6 |             });
    7 |           });
    8 |
  ======
onEnd
onExit
`);
    });

    test("onError should have code snippet", () => {
      const result = runInlineTest({
        "reporter.ts": smallReporterJS,
        "playwright.config.ts": `export default { reporter: [['./reporter', { printSteps: true, printErrorSnippet: true }]] };`,
        "a.spec.js": `
          const { test, expect } = require('@playwright/test');
          throw new Error('test');
        `,
      }, { "reporter": "", "workers": 1 });

      expect(result.output).toBe(`
onBegin: 0 tests total
onError: Error: test @ a.spec.js:3
  ======
     at a.spec.js:3

    1 |
    2 |           const { test, expect } = require('@playwright/test');
  > 3 |           throw new Error('test');
      |                 ^
    4 |
  ======
onError: Error: No tests found @ <no location>
onEnd
onExit
`);
    });

    test("onError receives workerInfo for fixture teardown errors (#39063)", () => {
      const result = runInlineTest({
        "reporter.ts": `
          class Reporter {
            printsToStdio() { return true; }
            onError(error, workerInfo) {
              console.log('teardownError:'
                + ' message=' + (error.message || '').split('\\n')[0]
                + ' workerInfo=' + (workerInfo ? typeof workerInfo.workerIndex + '/' + typeof workerInfo.parallelIndex + '/' + workerInfo.project.name : 'undefined'));
            }
          }
          export default Reporter;
        `,
        "playwright.config.ts": `export default { projects: [{ name: 'foo' }], reporter: './reporter.ts' };`,
        "a.spec.ts": `
          import { test as base } from '@playwright/test';
          const test = base.extend<{}, { brokenWorker: void }>({
            brokenWorker: [({}, use) => {
              use();
              throw new Error('teardown failed');
            }, { scope: 'worker' }],
          });
          test('uses worker fixture', () => {});
        `,
      }, { reporter: "", workers: 1 });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "teardownError: message=Error: teardown failed workerInfo=number/number/foo") != null, true);
    });
  });
}










fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should report a stable test.id", () => {
    // [ts] const result = runInlineTest({
    // [ts] "reporter.ts": `
    // [ts] class Reporter {
    // [ts] onTestBegin(test) {
    // [ts] console.log('\\n%%testbegin-' + test.id);
    // [ts] }
    // [ts] }
    // [ts] export default Reporter;
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default { reporter: [[ './reporter.ts' ]] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('example test', () => {
    // [ts] });
    // [ts] `
    // [ts] }, { reporter: "", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "testbegin-20289bcdad95a5e18c38-8b63c3695b9c8bd62d98",
    // [ts] ]);
    // [ts] });
    // [ts] test("should report annotations from test declaration", () => {
    // [ts] const result = runInlineTest({
    // [ts] "reporter.ts": `
    // [ts] export default class Reporter {
    // [ts] onBegin(config, suite) {
    // [ts] const visit = suite => {
    // [ts] for (const test of suite.tests || []) {
    // [ts] const annotations = test.annotations.map(a => {
    // [ts] const description = a.description ? a.type + '=' + a.description : a.type;
    // [ts] const location = a.location ? '(' + a.location.line + ':' + a.location.column + ')' : '';
    // [ts] return description + location;
    // [ts] });
    // [ts] console.log('\\n%%title=' + test.title + ', annotations=' + annotations.join(','));
    // [ts] }
    // [ts] for (const child of suite.suites || [])
    // [ts] visit(child);
    // [ts] };
    // [ts] visit(suite);
    // [ts] }
    // [ts] onError(error) {
    // [ts] console.log(error);
    // [ts] }
    // [ts] }
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: './reporter',
    // [ts] };
    // [ts] `,
    // [ts] "stdio.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('none', () => {
    // [ts] expect_eq(test.info().annotations, []);
    // [ts] });
    // [ts] test('foo', { annotation: { type: 'foo' } }, () => {
    // [ts] expect_eq(test.info().annotations, [{ type: 'foo', location: { file: expect.any(String), line: 6, column: 7 } }]);
    // [ts] });
    // [ts] test('foo-bar', {
    // [ts] annotation: [
    // [ts] { type: 'foo', description: 'desc' },
    // [ts] { type: 'bar' },
    // [ts] ],
    // [ts] }, () => {
    // [ts] expect(test.info().annotations).toEqual([
    // [ts] { type: 'foo', description: 'desc', location: { file: expect.any(String), line: 9, column: 7 } },
    // [ts] { type: 'bar', location: { file: expect.any(String), line: 9, column: 7 } },
    // [ts] ]);
    // [ts] });
    // [ts] test('skip-foo', { annotation: { type: 'foo' } }, () => {
    // [ts] });
    // [ts] test.fixme('fixme-bar', { annotation: { type: 'bar' } }, () => {
    // [ts] });
    // [ts] test.fail('fail-foo-bar', {
    // [ts] annotation: [
    // [ts] { type: 'foo' },
    // [ts] { type: 'bar', description: 'desc' },
    // [ts] ],
    // [ts] }, () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [cap-test] test.describe('suite', { annotation: { type: 'foo' } }, () => {
    // [ts] test('foo-suite', () => {
    // [ts] expect_eq(test.info().annotations, [{ type: 'foo', location: { file: expect.any(String), line: 32, column: 12 } }]);
    // [ts] });
    // [cap-test] test.describe('inner', { annotation: { type: 'bar' } }, () => {
    // [ts] test('foo-bar-suite', () => {
    // [ts] expect(test.info().annotations).toEqual([
    // [ts] { type: 'foo', location: { file: expect.any(String), line: 32, column: 12 } },
    // [ts] { type: 'bar', location: { file: expect.any(String), line: 36, column: 14 } }
    // [ts] ]);
    // [ts] });
    // [cap-test] });
    // [cap-test] });
    // [cap-test] test.describe.skip('skip-foo-suite', { annotation: { type: 'foo' } }, () => {
    // [ts] test('skip-foo-suite', () => {
    // [ts] });
    // [cap-test] });
    // [ts] test.describe.fixme('fixme-bar-suite', { annotation: { type: 'bar' } }, () => {
    // [ts] test('fixme-bar-suite', () => {
    // [ts] });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect(result.outputLines).toEqual([
    // [ts] `title=none, annotations=`,
    // [ts] `title=foo, annotations=foo(6:7)`,
    // [ts] `title=foo-bar, annotations=foo=desc(9:7),bar(9:7)`,
    // [ts] `title=skip-foo, annotations=foo(20:12),skip(20:12)`,
    // [ts] `title=fixme-bar, annotations=bar(22:12),fixme(22:12)`,
    // [ts] `title=fail-foo-bar, annotations=foo(24:12),bar=desc(24:12),fail(24:12)`,
    // [ts] `title=foo-suite, annotations=foo(32:12)`,
    // [ts] `title=foo-bar-suite, annotations=foo(32:12),bar(36:14)`,
    // [ts] `title=skip-foo-suite, annotations=foo(45:21),skip(45:21)`,
    // [ts] `title=fixme-bar-suite, annotations=bar(49:21),fixme(49:21)`,
    // [ts] ]);
    // [ts] });
    // [ts] test("attachments.push should not be enumerable to allow toEqual comparisons", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test.info().attachments.push should not be enumerable', () => {
    // [ts] const attachments = test.info().attachments;
    // [ts] const descriptor = Object.getOwnPropertyDescriptor(attachments, 'push');
    // [ts] expect_eq(descriptor, true);
    // [ts] expect_eq(descriptor.enumerable, false);
    // [ts] expect_eq(std.mem.indexOf(u8, Object.keys(attachments), 'push') == null, true);
    // [ts] test.info().attach('file1', { body: 'content1', contentType: 'text/plain' });
    // [ts] test.info().attach('file2', { body: 'content2', contentType: 'text/plain' });
    // [ts] const keys = Object.keys(attachments);
    // [ts] expect_eq(keys.every(k => !isNaN(Number(k))), true);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("tests skipped in serial mode receive onTestBegin/onTestEnd", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/28321" });
    // [ts] const result = runInlineTest({
    // [ts] "reporter.ts": smallReporterJS,
    // [ts] "playwright.config.ts": `export default { reporter: [['./reporter', { printTestStatus: true }]] };`,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test.describe.configure({ mode: 'serial', retries: 1 });
    // [ts] test('test1', () => {
    // [ts] expect_eq(test.info().retry, 1);
    // [ts] });
    // [ts] test('test2', () => {
    // [ts] });
    // [ts] `,
    // [ts] }, { "reporter": "", "workers": 1 });
    // [ts] expect(result.output).toBe(`
    // [ts] onBegin: 2 tests total
    // [ts] onTestBegin:  > a.spec.ts > test1; retry #0
    // [ts] onTestEnd: [failed]  > a.spec.ts > test1; retry #0
    // [ts] error: Error: expect_eq(received, expected) // Object.is equality @ a.spec.ts:7
    // [ts] onTestBegin:  > a.spec.ts > test2; retry #0
    // [ts] onTestEnd: [skipped]  > a.spec.ts > test2; retry #0
    // [ts] onTestBegin:  > a.spec.ts > test1; retry #1
    // [ts] onTestEnd: [passed]  > a.spec.ts > test1; retry #1
    // [ts] onTestBegin:  > a.spec.ts > test2; retry #1
    // [ts] onTestEnd: [passed]  > a.spec.ts > test2; retry #1
    // [ts] onEnd
    // [ts] onExit
    // [ts] `);
    // [ts] });
    // [ts] test("step attachments are referentially equal to result attachments", () => {
    // [ts] class TestReporter implements Reporter {
    // [ts] onStepEnd: (test, result, step) => {
    // [ts] @print("%%%", JSON.stringify({
    // [ts] title: step.title,
    // [ts] attachments: step.attachments.map(a => result.attachments.indexOf(a)),
    // [ts] }));
    // [ts] }
    // [ts] }
    // [ts] const result = runInlineTest({
    // [ts] "reporter.ts": `export default ${TestReporter.toString()}`,
    // [ts] "playwright.config.ts": `export default { reporter: './reporter' };`,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] test.step('step', () => {
    // [ts] testInfo.attachments.push({ name: 'attachment', body: Buffer.from('content') });
    // [ts] });
    // [ts] });
    // [ts] `,
    // [ts] }, { "reporter": "", "workers": 1 });
    // [ts] const steps = result.outputLines.map(line => JSON.parse(line));
    // [ts] expect(steps).toEqual([
    // [ts] { title: "Before Hooks", attachments: [] },
    // [ts] { title: "step", attachments: [0] },
    // [ts] { title: "After Hooks", attachments: [] },
    // [ts] ]);
    // [ts] });
    // [ts] test("step annotations are reported in onStepEnd", () => {
    // [ts] class TestReporter implements Reporter {
    // [ts] onStepEnd: (test, result, step) => {
    // [ts] if (step.category == "test.step")
    // [ts] @print("%%%", JSON.stringify(step.annotations));
    // [ts] }
    // [ts] }
    // [ts] const result = runInlineTest({
    // [ts] "reporter.ts": `export default ${TestReporter.toString()}`,
    // [ts] "playwright.config.ts": `export default { reporter: './reporter' };`,
    // [ts] "a.spec.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] test.step('step', async stepInfo => {
    // [ts] stepInfo.annotations.push({ type: 'expected-result', description: 'step passes' });
    // [ts] });
    // [ts] });
    // [ts] `,
    // [ts] }, { "reporter": "", "workers": 1 });
    // [ts] expect(result.outputLines).toEqual([
    // [ts] JSON.stringify([{ type: "expected-result", description: "step passes" }]),
    // [ts] ]);
    // [ts] });
    // [ts] test("step.attach attachments are reported on right steps", () => {
    // [ts] class TestReporter implements Reporter {
    // [ts] onStepEnd: (test, result, step) => {
    // [ts] @print("%%%", JSON.stringify({
    // [ts] title: step.title,
    // [ts] attachments: step.attachments.map(a => ({ ...a, body: a.body.toString("utf8") })),
    // [ts] }));
    // [ts] }
    // [ts] }
    // [ts] const result = runInlineTest({
    // [ts] "reporter.ts": `export default ${TestReporter.toString()}`,
    // [ts] "playwright.config.ts": `export default { reporter: './reporter' };`,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test.beforeAll(() => {
    // [ts] test.step('step in beforeAll', (step) => {
    // [ts] step.attach('attachment1', { body: 'content1' });
    // [ts] });
    // [ts] });
    // [ts] test('test', () => {
    // [ts] test.step('step', (step) => {
    // [ts] step.attach('attachment2', { body: 'content2' });
    // [ts] });
    // [ts] });
    // [ts] `,
    // [ts] }, { "reporter": "", "workers": 1 });
    // [ts] const steps = result.outputLines.map(line => JSON.parse(line));
    // [ts] expect(steps).toEqual([
    // [ts] { title: "step in beforeAll", attachments: [{ body: "content1", contentType: "text/plain", name: "attachment1" }] },
    // [ts] { title: "beforeAll hook", attachments: [] },
    // [ts] { title: "Before Hooks", attachments: [] },
    // [ts] { title: "step", attachments: [{ body: "content2", contentType: "text/plain", name: "attachment2" }] },
    // [ts] { title: "After Hooks", attachments: [] },
    // [ts] ]);
    // [ts] });
    // [ts] test("attachments are reported in onStepEnd", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/14364" } }, (_p) => {
    // [ts] class TestReporter implements Reporter {
    // [ts] onStepEnd: (test, result, step) => {
    // [ts] @print(`%%[${step.category}] ${step.title}: ${result.attachments.length} attachments in result`);
    // [ts] }
    // [ts] }
    // [ts] const result = runInlineTest({
    // [ts] "reporter.ts": `export default ${TestReporter.toString()}`,
    // [ts] "playwright.config.ts": `export default { reporter: './reporter' };`,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] test.step('step', () => {
    // [ts] testInfo.attachments.push({ name: 'attachment', body: Buffer.from('content') });
    // [ts] });
    // [ts] testInfo.attach('4', {body:'444'})
    // [ts] });
    // [ts] `,
    // [ts] }, { "reporter": "", "workers": 1 });
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "[hook] Before Hooks: 0 attachments in result",
    // [ts] "[test.step] step: 1 attachments in result",
    // [ts] "[test.attach] Attach \"4\": 2 attachments in result",
    // [ts] "[hook] After Hooks: 2 attachments in result",
    // [ts] ]);
    // [ts] });
    // [ts] test("should have static annotations on result when all tests are skipped", () => {
    // [ts] class TestReporter implements Reporter {
    // [ts] onTestEnd: (test, result) => {
    // [ts] for (const annotation of result.annotations)
    // [ts] @print(`%%annotation: ${annotation.type} ${annotation.description || ""}`);
    // [ts] }
    // [ts] }
    // [ts] const result = runInlineTest({
    // [ts] "reporter.ts": `export default ${TestReporter.toString()}`,
    // [ts] "playwright.config.ts": `export default { reporter: './reporter' };`,
    // [ts] "a.spec.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('test', {
    // [ts] annotation: [{ type: 'foo', description: 'desc' }, { type: 'bar' }],
    // [ts] }, () => {});
    // [ts] `,
    // [ts] }, { "reporter": "", "workers": 1 });
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "annotation: foo desc",
    // [ts] "annotation: bar",
    // [ts] "annotation: skip",
    // [ts] ]);
    // [ts] const resultMany = runInlineTest({
    // [ts] "reporter.ts": `export default ${TestReporter.toString()}`,
    // [ts] "playwright.config.ts": `export default { reporter: './reporter' };`,
    // [ts] "a.spec.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('test', {
    // [ts] annotation: [{ type: 'foo', description: 'desc' }, { type: 'bar' }],
    // [ts] }, () => {});
    // [ts] test('test2', {
    // [ts] annotation: [{ type: 'foobar', description: 'another' }],
    // [ts] }, () => {});
    // [ts] test('test3', {
    // [ts] annotation: [{ type: 'warning', description: 'one more' }],
    // [ts] }, () => {});
    // [ts] `,
    // [ts] }, { "reporter": "", "workers": 1 });
    // [ts] expect(resultMany.outputLines).toEqual([
    // [ts] "annotation: foo desc",
    // [ts] "annotation: bar",
    // [ts] "annotation: skip",
    // [ts] "annotation: foobar another",
    // [ts] "annotation: skip",
    // [ts] "annotation: warning one more",
    // [ts] "annotation: skip",
    // [ts] ]);
    // [ts] });
    // [ts] test("AggregateError sub-errors are spread into testInfo.errors", () => {
    // [ts] class TestReporter implements Reporter {
    // [ts] onTestEnd: (test, result) => {
    // [ts] for (const error of result.errors)
    // [ts] @print(`%%${error.message ?? error.value}`);
    // For the boxed-step case, also surface a frame from the test file so
    // we can assert that the boxed-stack rewrite only applies to the
    // top-level error and not to its sub-errors.
    // [ts] if (test.title == "boxed step") {
    // [ts] for (const error of result.errors) {
    // [ts] const frame = (error.stack ?? "").split("\n").find(l => l.includes("a.spec.ts:"));
    // [ts] @print(`%%FRAME ${error.message}: ${frame?.trim()}`);
    // [ts] }
    // [ts] }
    // [ts] }
    // [ts] }
    // [ts] const result = runInlineTest({
    // [ts] "reporter.ts": `export default ${TestReporter.toString()}`,
    // [ts] "playwright.config.ts": `export default { reporter: './reporter' };`,
    // [ts] "a.spec.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('basic', () => {
    // [ts] throw new AggregateError([new Error('a'), new Error('b')], 'parent');
    // [ts] });
    // [ts] test('nested', () => {
    // [ts] throw new AggregateError([
    // [ts] new AggregateError([new Error('a'), new Error('b')], 'inner'),
    // [ts] new Error('c'),
    // [ts] ], 'outer');
    // [ts] });
    // [ts] test('non-error entries', () => {
    // [ts] const err: any = new Error('parent');
    // [ts] err.errors = ['oops', { foo: 1 }, new Error('real')];
    // [ts] throw err;
    // [ts] });
    // [ts] test('boxed step', () => {
    // [ts] const subA = new Error('sub a');
    // [ts] const subB = new Error('sub b');
    // [ts] const helper = () => {
    // [ts] test.step('boxed', () => {
    // [ts] throw new AggregateError([subA, subB], 'top');
    // [ts] }, { box: true });
    // [ts] };
    // [ts] helper();
    // [ts] });
    // [ts] `,
    // [ts] }, { "reporter": "", "workers": 1 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "AggregateError: parent",
    // [ts] "Error: a",
    // [ts] "Error: b",
    // [ts] "AggregateError: outer",
    // [ts] "AggregateError: inner",
    // [ts] "Error: a",
    // [ts] "Error: b",
    // [ts] "Error: c",
    // [ts] "Error: parent",
    // [ts] `'oops'`,
    // [ts] "{ foo: 1 }",
    // [ts] "Error: real",
    // [ts] "AggregateError: top",
    // [ts] "Error: sub a",
    // [ts] "Error: sub b",
    // [ts] expect.stringMatching("^FRAME AggregateError: top: at .*a\\.spec\\.ts:25:"),
    // [ts] expect.stringMatching("^FRAME Error: sub a: at .*a\\.spec\\.ts:18:"),
    // [ts] expect.stringMatching("^FRAME Error: sub b: at .*a\\.spec\\.ts:19:"),
    // [ts] ]);
    // [ts] });
    // [ts] test("--add-reporter should append to configured reporters instead of replacing them", () => {
    // [ts] const result = runInlineTest({
    // [ts] "configured-reporter.js": `
    // [ts] module.exports = class { onBegin() { console.log('FROM_CONFIGURED_REPORTER'); } };
    // [ts] `,
    // [ts] "added-reporter.js": `
    // [ts] module.exports = class { onBegin() { console.log('FROM_ADDED_REPORTER'); } };
    // [ts] `,
    // [ts] "playwright.config.ts": `export default { reporter: [['./configured-reporter.js']] };`,
    // [ts] "a.spec.js": `
    // [ts] const { test } = require('@playwright/test');
    // [ts] test('test', () => {});
    // [ts] `,
    // [ts] }, { "workers": 1 }, null, { additionalArgs: ["--add-reporter=./added-reporter.js"] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "FROM_CONFIGURED_REPORTER") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "FROM_ADDED_REPORTER") != null, true);
    // [ts] });
}
