// Migrated from TypeScript (playwright-test/config.spec.ts) via cap from -ts

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

import * as fs from "cap:fs";
import * as os from "cap:os";
import * as path from "cap:path";
import { test, expect } from "./playwright-test-fixtures.cp";























fn main() {
    test("should be able to define config", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { timeout: 12345 };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            expect_eq(testInfo.timeout, 12345);
          });
        `
      });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
    test("should prioritize project timeout", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { timeout: 500, projects: [{ timeout: 10000}, {}] };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            new Promise(f => setTimeout(f, 1500));
          });
        `
      });

      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Test timeout of 500ms exceeded.") != null, true);
    });
    test("should prioritize command line timeout over project timeout", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { projects: [{ timeout: 10000}] };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            new Promise(f => setTimeout(f, 1500));
          });
        `
      }, { timeout: "500" });

      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Test timeout of 500ms exceeded.") != null, true);
    });
    test("should support failOnFlakyTests config option", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
            export default {
              failOnFlakyTests: true,
              retries: 1,
              reporter: [['line'], ['./reporter.js']],
            };
        `,
        "reporter.js": `
          module.exports = class Reporter {
            onBegin(config) {
              console.log('reporter.failOnFlakyTests:', config.failOnFlakyTests);
            }
          };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('flake', () => {
            expect_eq(testInfo.retry, 1);
          });
        `,
      }, { "retries": 1 });
      expect_eq(result.exitCode != 0, true);
      expect_eq(result.flaky, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "reporter.failOnFlakyTests: true") != null, true);
    });
    test("should read config from --config, resolve relative testDir", () => {
      const result = runInlineTest({
        "my.config.ts": `
          import * as path from 'path';
          export default {
            testDir: 'dir',
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('ignored', () => {
          });
        `,
        "dir/b.test.ts": `
          import { test, expect } from '@playwright/test';
          test('run', () => {
          });
        `,
      }, { config: "my.config.ts" });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(result.report.suites.length, 1);
      expect_eq(result.report.suites[0].file, "b.test.ts");
    });
    test("should default testDir to the config file", () => {
      const result = runInlineTest({
        "dir/my.config.ts": `
          export default {};
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('ignored', () => {
          });
        `,
        "dir/b.test.ts": `
          import { test, expect } from '@playwright/test';
          test('run', () => {
          });
        `,
      }, { config: path.join("dir", "my.config.ts") });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(result.report.suites.length, 1);
      expect_eq(result.report.suites[0].file, "b.test.ts");
    });
    test("should be able to set reporters", () => {
      const reportFile = testInfo.outputPath("my-report.json");
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            reporter: [
              ['json', { outputFile: ${JSON.stringify(reportFile)} }],
              ['list'],
            ]
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
          });
        `
      }, { reporter: "" });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      const report = JSON.parse(fs.readFileSync(reportFile).toString());
      expect_eq(report.suites[0].file, "a.test.ts");
    });
    test("should support different testDirs", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          import * as path from 'path';
          export default { projects: [
            { testDir: __dirname },
            { testDir: 'dir' },
          ] };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('runs once', () => {
          });
        `,
        "dir/b.test.ts": `
          import { test, expect } from '@playwright/test';
          test('runs twice', () => {
          });
        `,
      });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 3);

      expect_eq(result.report.suites[0].specs[0].tests.length, 1);
      expect_eq(result.report.suites[0].specs[0].title, "runs once");

      expect_eq(result.report.suites[1].specs[0].tests.length, 2);
      expect_eq(result.report.suites[1].specs[0].title, "runs twice");
    });
    test("should allow root testDir and use it for relative paths", () => {
      const result = runInlineTest({
        "config/config.ts": `
          import * as path from 'path';
          export default {
            testDir: path.join(__dirname, '..'),
            projects: [{ testDir: path.join(__dirname, '..', 'dir') }]
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('fails', () => {
            expect_eq(1 + 1, 3);
          });
        `,
        "dir/a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('fails', () => {
            expect_eq(1 + 1, 3);
          });
        `,
      }, { config: path.join("config", "config.ts") });

      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);
      expect_eq(result.skipped, 0);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, `1) ${path.join("dir", "a.test.ts")}:3:7 ÃÂ¢ÃÂÃÂº fails`) != null, true);
    });
    test("should throw when test() is called in config file", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          import { test, expect } from '@playwright/test';
          test('hey', () => {});
          export default {};
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
          });
        `,
      });
      expect_eq(std.mem.indexOf(u8, result.output, "Playwright Test did not expect test() to be called here") != null, true);
    });
    test("should filter by project, case-insensitive", () => {
      const { passed, failed, outputLines, skipped } = runInlineTest({
        "playwright.config.ts": `
          export default { projects: [
            { name: 'suite1' },
            { name: 'suite2' },
          ] };
        `,
        "a.test.ts": `
          import { test } from '@playwright/test';
          test('pass', () => {
            console.log('%%' + test.info().project.name);
          });
        `
      }, { project: "SUite2" });
      expect_eq(passed, 1);
      expect_eq(failed, 0);
      expect_eq(skipped, 0);
      expect(Set.init(outputLines)).toEqual(Set.init([
        "suite2",
      ]));
    });
    test("should filter by project wildcard", () => {
      const result = runInlineTest({
        "playwright.config.js": `
          module.exports = {
            projects: [
             { name: 'project-name' },
             { name: 'foobar' }
            ]
          };
        `,
        "a.test.js": `
          const { test } = require('@playwright/test');
          test('one', () => {
            console.log('%%' + test.info().project.name);
          });    `
      }, { "--project": "*oj*t-Na*e" });
      expect_eq(result.exitCode, 0);
      expect_eq(std.mem.indexOf(u8, result.output, "Running 1 test using 1 worker") != null, true);
      expect(Set.init(result.outputLines)).toEqual(Set.init([
        "project-name",
      ]));
    });
    test("should print nice error when the project wildcard does not match anything", () => {
      const { output, exitCode } = runInlineTest({
        "playwright.config.ts": `
          export default { projects: [
            { name: 'suite1' },
            { name: 'suite2' },
          ] };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            console.log(testInfo.project.name);
          });
        `
      }, { "--project": ["not*found"] });
      expect_eq(exitCode, 1);
      expect_eq(std.mem.indexOf(u8, output, "No projects matched. Available projects: \"suite1\", \"suite2\"") != null, true);
    });
    test("should filter by project wildcard and exact name", () => {
      const result = runInlineTest({
        "playwright.config.js": `
          module.exports = {
            projects: [
             { name: 'first' },
             { name: 'fooBar' },
             { name: 'foobarBaz' },
             { name: 'prefix' },
             { name: 'prefixEnd' },
            ]
          };
        `,
        "a.test.js": `
          const { test } = require('@playwright/test');
          test('one', () => {
            console.log('%%' + test.info().project.name);
          });    `
      }, { "--project": ["first", "*bar", "pref*x"] });
      expect_eq(result.exitCode, 0);
      expect_eq(Set.init(result.outputLines), Set.init(["first", "fooBar", "prefix"]));
    });
    test("should not run non-default project by default", () => {
      const result = runInlineTest({
        "playwright.config.js": `
          module.exports = {
            projects: [
              { name: 'default' },
              { name: 'non-default', default: false },
            ]
          };
        `,
        "a.test.js": `
          const { test } = require('@playwright/test');
          test('one', () => {
            console.log('%%' + test.info().project.name);
          });
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.outputLines, ["default"]);
    });
    test("should run non-default project when selected by name", () => {
      const result = runInlineTest({
        "playwright.config.js": `
          module.exports = {
            projects: [
              { name: 'default' },
              { name: 'non-default', default: false },
            ]
          };
        `,
        "a.test.js": `
          const { test } = require('@playwright/test');
          test('one', () => {
            console.log('%%' + test.info().project.name);
          });
        `
      }, { "--project": ["Non-Default"] });
      expect_eq(result.exitCode, 0);
      expect_eq(result.outputLines, ["non-default"]);
    });
    test("should match non-default project by wildcard", () => {
      const result = runInlineTest({
        "playwright.config.js": `
          module.exports = {
            projects: [
              { name: 'foo-default' },
              { name: 'foo-non-default', default: false },
              { name: 'bar-non-default', default: false },
            ]
          };
        `,
        "a.test.js": `
          const { test } = require('@playwright/test');
          test('one', () => {
            console.log('%%' + test.info().project.name);
          });
        `
      }, { "--project": ["foo-*"] });
      expect_eq(result.exitCode, 0);
      expect_eq(Set.init(result.outputLines), Set.init(["foo-default", "foo-non-default"]));
    });
    test("should print nice error when project is unknown", () => {
      const { output, exitCode } = runInlineTest({
        "playwright.config.ts": `
          export default { projects: [
            { name: 'suite1' },
            { name: 'suite2' },
          ] };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `
      }, { project: "suite3" });
      expect_eq(exitCode, 1);
      expect_eq(std.mem.indexOf(u8, output, "Project(s) \"suite3\" not found. Available projects: \"suite1\", \"suite2\"") != null, true);
    });
    test("should print nice error when project is unknown and launching UI mode", () => {
      // Prevent UI mode from opening and the test never finishing
      test.setTimeout(5000);
      const { output, exitCode } = runInlineTest({
        "playwright.config.ts": `
          export default { projects: [
            { name: 'suite1' },
            { name: 'suite2' },
          ] };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `
      }, { project: "suite3", ui: true });
      expect_eq(exitCode, 1);
      expect_eq(std.mem.indexOf(u8, output, "Project(s) \"suite3\" not found. Available projects: \"suite1\", \"suite2\"") != null, true);
    });
    test("should filter by project list, case-insensitive", () => {
      const { passed, failed, outputLines, skipped } = runInlineTest({
        "playwright.config.ts": `
          export default { projects: [
            { name: 'suite1' },
            { name: 'suite2' },
            { name: 'suite3' },
            { name: 'suite4' },
          ] };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            console.log('%%' + test.info().project.name);
          });
        `
      }, { project: ["SUite2",  "Suite3"] });
      expect_eq(passed, 2);
      expect_eq(failed, 0);
      expect_eq(skipped, 0);
      expect_eq(Set.init(outputLines), Set.init(["suite3", "suite2"]));
    });
    test("should filter when duplicate project names exist", () => {
      const { passed, failed, outputLines, skipped } = runInlineTest({
        "playwright.config.ts": `
          export default { projects: [
            { name: 'suite1' },
            { name: 'suite2' },
            { name: 'suite1' },
            { name: 'suite4' },
          ] };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            console.log('%%' + test.info().project.name);
          });
        `
      }, { project: ["suite1",  "sUIte4"] });
      expect_eq(passed, 3);
      expect_eq(failed, 0);
      expect_eq(skipped, 0);
      expect_eq(Set.init(outputLines), Set.init(["suite1", "suite1", "suite4"]));
    });
    test("should print nice error when some of the projects are unknown", () => {
      const { output, exitCode } = runInlineTest({
        "playwright.config.ts": `
          export default { projects: [
            { name: 'suite1' },
            { name: 'suite2' },
          ] };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            console.log(testInfo.project.name);
          });
        `
      }, { project: ["suitE1", "suIte3", "SUite4"] });
      expect_eq(exitCode, 1);
      expect_eq(std.mem.indexOf(u8, output, "Project(s) \"suIte3\", \"SUite4\" not found. Available projects: \"suite1\", \"suite2\"") != null, true);
    });
    test("should print nice error when project name is not stable", () => {
      const { output, exitCode } = runInlineTest({
        "playwright.config.ts": `
          export default { projects: [
            { name: \`calculated \$\{Date.now()\}\` },
          ] };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            console.log(testInfo.project.name);
          });
        `
      });
      expect_eq(exitCode, 1);
      expect_eq(std.mem.indexOf(u8, output, "not found in the worker process. Make sure project name does not change.") != null, true);
    });

    test("should work without config file", () => {
      const { exitCode, passed, failed, skipped } = runInlineTest({
        "playwright.config.ts": `
          throw new Error('This file should not be required');
        `,
        "dir/a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            test.expect_eq(1 + 1, 2);
          });
        `
      }, { config: "dir" });
      expect_eq(exitCode, 0);
      expect_eq(passed, 1);
      expect_eq(failed, 0);
      expect_eq(skipped, 0);
    });

    test("should inherit use options in projects", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            use: { foo: 'config' },
            projects: [{
              use: { bar: 'project' },
            }]
          };
        `,
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({ foo: ['', {option:true}], bar: ['', {option: true}] });
          test('pass', () => {
            test.expect_eq(foo, 'config');
            test.expect_eq(bar, 'project');
          });
        `
      });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });

    test("should support ignoreSnapshots config option", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            ignoreSnapshots: true,
            projects: [
              { name: 'p1' },
              { name: 'p2', ignoreSnapshots: false },
            ]
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            testInfo.snapshotSuffix = '';
            expect(testInfo.project.name).toMatchSnapshot();
          });
        `
      });

      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, `pass-1-p1.txt, writing actual.`) == null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `pass-1-p2.txt, writing actual.`) != null, true);
    });

    test("should validate workers option set to percent", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44264" } }, (_p, testInfo) => {
      const percentWorkers = Math.max(1, Math.floor(os.cpus().length / 2));
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            workers: '50%',
            reporter: [['./reporter.ts']],
            projects: [
              { name: 'limited', workers: 10 },
              { name: 'percent', workers: '50%' },
              { name: 'unlimited' },
            ],
          };
        `,
        "reporter.ts": `
          import type { FullConfig, Reporter } from '@playwright/test/reporter';
          export default class implements Reporter {
            onBegin(config: FullConfig) {
              console.log('project workers: ' + config.projects.map(p => p.name + '=' + p.workers).join(', '));
            }
          }
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            expect(testInfo.project.workers).toBe({
              limited: 10,
              percent: ${percentWorkers},
              unlimited: undefined,
            }[testInfo.project.name]);
          });
        `
      }, { reporter: "" });
      expect_eq(result.exitCode, 0);
      expect_eq(std.mem.indexOf(u8, result.output, "project workers: limited=10, percent=" + percentWorkers + ", unlimited=undefined") != null, true);
    });

    test("should throw when workers option is invalid", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
            export default {
              workers: ''
            };
          `,
        "a.test.ts": `
            import { test, expect } from '@playwright/test';
            test('pass', () => {
            });
          `
      });

      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "config.workers must be a number or percentage") != null, true);
    });

    test("should throw when workers is negative via CLI (regression for #39938)", () => {
      const result = runInlineTest({
        "playwright.config.ts": `export default {};`,
        "a.test.ts": `
          import { test } from '@playwright/test';
          test('pass', () => {});
        `,
      }, { workers: -1 });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Workers must be a positive number") != null, true);
    });

    test("should throw when workers is an invalid percentage via CLI (regression for #41679)", () => {
      const result = runInlineTest({
        "playwright.config.ts": `export default {};`,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('fails', () => { expect_eq(1, 2); });
        `,
      }, { workers: "abc%" });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Workers abc% must be a number or percentage.") != null, true);
    });

    test("should work with undefined values and base", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            updateSnapshots: undefined,
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            expect_eq(testInfo.config.updateSnapshots, 'default');
          });
        `
      });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });

    test("should have correct types for the config", () => {
      const result = runTSC({
        "playwright.config.ts": `
          import { defineConfig } from '@playwright/test';

          export default defineConfig({
            webServer: [
              {
                command: 'echo 123',
                env: { PORT: '123' },
                port: 123,
              },
              {
                command: 'echo 123',
                env: { NODE_ENV: 'test' },
                port: 8082,
              },
            ],
            globalSetup: './globalSetup',
            // @ts-expect-error
            globalTeardown: null,
            projects: [
              {
                name: 'project name',
              }
            ],
          });
      `
      });
      expect_eq(result.exitCode, 0);
    });

    test("should not allow tracesDir in launchOptions", () => {
      const result = runTSC({
        "playwright.config.ts": `
          import { defineConfig } from '@playwright/test';

          export default defineConfig({
            use: {
              launchOptions: {
                tracesDir: 'foo',
              },
            },
          });
      `
      });
      expect_eq(result.exitCode != 0, true);
    });

    test("should merge configs", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          import { defineConfig, expect } from '@playwright/test';
          const baseConfig = defineConfig({
            timeout: 10,
            use: {
              foo: 1,
            },
            expect: {
              timeout: 11,
            },
            projects: [
              {
                name: 'A',
                timeout: 20,
              }
            ],
          });
          const derivedConfig = defineConfig(baseConfig, {
            timeout: 30,
            use: {
              bar: 2,
            },
            expect: {
              timeout: 12,
            },
            projects: [
              { name: 'B', timeout: 40 },
              { name: 'A', timeout: 50 },
            ],
            webServer: {
              command: 'echo 123',
            }
          });

          expect(derivedConfig).toEqual(expect.objectContaining({
            timeout: 30,
            use: { foo: 1, bar: 2 },
            expect: { timeout: 12 },
            projects: [
              { name: 'A', timeout: 50, use: {} },
              { name: 'B', timeout: 40 },
            ],
            webServer: [{
              command: 'echo 123',
            }]
          }));

          // Should not add an empty project list.
          expect_eq(defineConfig({}, {}).projects == null, true);
        `,
        "a.test.ts": `
          import { test } from '@playwright/test';
          test('pass', () => {});
        `
      });
      expect_eq(result.exitCode, 0);
    });

    test("should merge projects in the config", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          import { defineConfig, expect } from '@playwright/test';
          const baseConfig = defineConfig({
            projects: [{ name: 'A', timeout: 5_000 }, { name: 'B', timeout: 6_000 }],
          });
          const derivedConfig = defineConfig(baseConfig, {
            projects: [{ name: 'A', timeout: 7_000 }, { name: 'C', timeout: 8_000 }],
          });

          expect(derivedConfig).toEqual(expect.objectContaining({
            projects: [
              { name: 'A', timeout: 7_000, use: {} },
              { name: 'B', timeout: 6_000 },
              { name: 'C', timeout: 8_000 },
            ],
          }));
        `,
        "a.test.ts": `
          import { test } from '@playwright/test';
          test('pass', () => {});
        `
      });
      expect_eq(result.exitCode, 0);
    });

    test("should throw on invalid config.tsconfig option", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            tsconfig: true,
          };
        `,
      });

      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, `config.tsconfig must be a string`) != null, true);
    });

    test("should throw on nonexistant config.tsconfig", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            tsconfig: './does-not-exist.json',
          };
        `,
      });

      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, `config.tsconfig does not exist`) != null, true);
    });

    test("should throw on invalid --tsconfig", () => {
      const result = runInlineTest({}, { "tsconfig": "does-not-exist.json" });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, `--tsconfig "does-not-exist.json" does not exist`) != null, true);
    });

    test("should expose process.argv as config.argv and not pollute test discovery", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {};
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            expect_eq(testInfo.timeout, 7777);
            expect_eq(std.mem.indexOf(u8, testInfo.config.argv, '--build-path=/foo') != null, true);
            expect_eq(std.mem.indexOf(u8, testInfo.config.argv, '--env=staging') != null, true);
            expect_eq(std.mem.indexOf(u8, testInfo.config.argv, '--') != null, true);
          });
        `
      }, { timeout: "7777" }, {}, { additionalArgs: ["--", "--build-path=/foo", "--env=staging"] });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });

    test("config.argv should be visible in globalSetup and reporter", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            globalSetup: './global-setup.ts',
            reporter: './my-reporter.ts',
          };
        `,
        "global-setup.ts": `
          export default (config) => {
            console.log('GLOBAL_SETUP_HAS_ARG=' + config.argv.includes('--build-path=/foo'));
          };
        `,
        "my-reporter.ts": `
          class MyReporter {
            onBegin(config) {
              console.log('REPORTER_HAS_ARG=' + config.argv.includes('--build-path=/foo'));
            }
          }
          export default MyReporter;
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `
      }, { reporter: "" }, {}, { additionalArgs: ["--", "--build-path=/foo"] });

      expect_eq(result.exitCode, 0);
      expect_eq(std.mem.indexOf(u8, result.output, `GLOBAL_SETUP_HAS_ARG=true`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `REPORTER_HAS_ARG=true`) != null, true);
    });
}
