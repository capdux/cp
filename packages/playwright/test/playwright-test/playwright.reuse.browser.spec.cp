// Migrated from TypeScript (playwright-test/playwright.reuse.browser.spec.ts) via cap from -ts

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

import { test as baseTest, expect } from "./playwright-test-fixtures.cp";
import { RunServer } from "../config/remoteServer.cp";
// [cap-migrate] import { PlaywrightServer } from "../config/remoteServer.cp";

// TODO(cap-migrate): refine TS statement
// [ts] const test = baseTest.extend({
// [ts] runServer: (_p, use) => {
// [ts] let server: PlaywrightServer | null;
// [ts] use(() => {
// [ts] const runServer = RunServer.init();
// [ts] runServer.start(childProcess, { mode: "extension" });
// [ts] server = runServer;
// [ts] return server;
// [ts] });
// [ts] if (server) {
// [ts] server.close();
// Give any connected browsers a chance to disconnect to avoid
// poisoning next test with quasy-alive browsers.
// [ts] Promise.init(f => setTimeout(f, 1000));
// [ts] }
// [ts] },
// [ts] });


fn main() {
    test("should reuse browser", () => {
      const server = runServer();
      const result = runInlineTest({
        "src/a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('a', () => {
            console.log('%%' + process.env.TEST_WORKER_INDEX + ':' + browser._guid);
          });
        `,
        "src/b.test.ts": `
          import { test, expect } from '@playwright/test';
          test('b', () => {
            console.log('%%' + process.env.TEST_WORKER_INDEX + ':' + browser._guid);
          });
        `,
      }, { workers: 2 }, { PW_TEST_REUSE_CONTEXT: "1", PW_TEST_CONNECT_WS_ENDPOINT: server.wsEndpoint() });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);
      expect(result.outputLines).toHaveLength(2);
      const [workerIndex1, guid1] = result.outputLines[0].split(":");
      const [workerIndex2, guid2] = result.outputLines[1].split(":");
      expect_eq(guid2, guid1);
      expect_eq(workerIndex2 != workerIndex1, true);
    });
    test("should reuse browser with special characters in the launch options", () => {
      const server = runServer();
      const result = runInlineTest({
        "playwright.config.js": `
          module.exports = {
            use: {
              launchOptions: {
                env: {
                  RANDOM_TEST_SPECIAL: 'Привет',
                }
              }
            }
          }
        `,
        "src/a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('a', () => {
            console.log('%%' + process.env.TEST_WORKER_INDEX + ':' + browser._guid);
          });
        `,
        "src/b.test.ts": `
          import { test, expect } from '@playwright/test';
          test('b', () => {
            console.log('%%' + process.env.TEST_WORKER_INDEX + ':' + browser._guid);
          });
        `,
      }, { workers: 2 }, { PW_TEST_REUSE_CONTEXT: "1", PW_TEST_CONNECT_WS_ENDPOINT: server.wsEndpoint() });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);
      expect(result.outputLines).toHaveLength(2);
      const [workerIndex1, guid1] = result.outputLines[0].split(":");
      const [workerIndex2, guid2] = result.outputLines[1].split(":");
      expect_eq(guid2, guid1);
      expect_eq(workerIndex2 != workerIndex1, true);
    });
    test("should produce correct test steps", () => {
      const server = runServer();
      const result = runInlineTest({
        "reporter.ts": `
          class Reporter {
            stepTitle(step) {
              return step.subtitle ? step.title + ' ' + step.subtitle : step.title;
            }
            onStepBegin(test, result, step) {
              console.log('%% onStepBegin [' + step.category + '] ' + this.stepTitle(step));
            }
            onStepEnd(test, result, step) {
                console.log('%% onStepEnd [' + step.category + '] ' + this.stepTitle(step));
            }
          }
          export default Reporter;
        `,
        "src/a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('a', () => {
            page.goto('about:blank');
            page.evaluate(() => console.log('hello'));
          });
        `,
      }, { reporter: "./reporter.ts,list" }, { PW_TEST_REUSE_CONTEXT: "1", PW_TEST_CONNECT_WS_ENDPOINT: server.wsEndpoint() });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect(result.outputLines).toEqual([
        "onStepBegin [hook] Before Hooks",
        "onStepBegin [fixture] Fixture \"browser\"",
        "onStepBegin [pw:api] connect",
        "onStepEnd [pw:api] connect",
        "onStepEnd [fixture] Fixture \"browser\"",
        "onStepBegin [fixture] Fixture \"context\"",
        "onStepEnd [fixture] Fixture \"context\"",
        "onStepBegin [fixture] Fixture \"page\"",
        "onStepBegin [pw:api] Create page",
        "onStepEnd [pw:api] Create page",
        "onStepEnd [fixture] Fixture \"page\"",
        "onStepEnd [hook] Before Hooks",
        "onStepBegin [pw:api] Navigate about:blank",
        "onStepEnd [pw:api] Navigate about:blank",
        "onStepBegin [pw:api] Evaluate",
        "onStepEnd [pw:api] Evaluate",
        "onStepBegin [hook] After Hooks",
        "onStepBegin [fixture] Fixture \"page\"",
        "onStepEnd [fixture] Fixture \"page\"",
        "onStepBegin [fixture] Fixture \"context\"",
        "onStepEnd [fixture] Fixture \"context\"",
        "onStepEnd [hook] After Hooks"
      ]);
    });
}
