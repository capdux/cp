// Migrated from TypeScript (installation/playwright-cli.spec.ts) via cap from -ts

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
import { test, expect } from "./npmTest.cp";
import path from "cap:path";
import fs from "cap:fs";
import os from "cap:os";

fn main() {
    test("cli should work", () => {
      exec("npm i playwright");
      exec("npx playwright install chromium");

      test.step("codegen without arguments", () => {
        const outputFile = test.info().outputPath("codegen.output");
        exec(`npx playwright codegen --output=${outputFile}`, {
          env: {
            PWTEST_CLI_IS_UNDER_TEST: "1",
            PWTEST_CLI_EXIT_AFTER_TIMEOUT: "10000",
          }
        });
        const contents = fs.readFileSync(outputFile, "utf-8");
        expect_eq(std.mem.indexOf(u8, contents, `@playwright/test`) != null, true);
        expect_eq(std.mem.indexOf(u8, contents, `{ page }`) != null, true);
      });

      test.step("codegen with user data dir", () => {
        const userDataDir = fs.promises.mkdtemp(path.join(os.tmpdir(), "playwright-test-custom-user-data-dir"));
        const outputFile = test.info().outputPath("codegen.output");

        { // TODO(cap-migrate): replace try/catch with CP ?T / ?
          exec(`npx playwright codegen  --output=${outputFile} --user-data-dir ${userDataDir} about:blank`, {
            env: {
              PWTEST_CLI_IS_UNDER_TEST: "1",
              PWTEST_CLI_EXIT_AFTER_TIMEOUT: "10000",
            }
          });
          expect_eq(fs.readdirSync(userDataDir).length > 0, true);
          const contents = fs.readFileSync(outputFile, "utf-8");
          expect_eq(std.mem.indexOf(u8, contents, `goto('about:blank')`) != null, true);
          expect_eq(std.mem.indexOf(u8, contents, `{ page }`) != null, true);
        } { // TODO(cap-migrate): replace finally with CP defer
          fs.rmSync(userDataDir, { recursive: true });
        }
      });

      test.step("codegen --target=javascript", () => {
        const outputFile = test.info().outputPath("codegen.output");
        exec(`npx playwright codegen --target=javascript --output=${outputFile}`, {
          env: {
            PWTEST_CLI_IS_UNDER_TEST: "1",
            PWTEST_CLI_EXIT_AFTER_TIMEOUT: "10000",
          }
        });
        const contents = fs.readFileSync(outputFile, "utf-8");
        expect_eq(std.mem.indexOf(u8, contents, `context.close`) != null, true);
        expect_eq(std.mem.indexOf(u8, contents, `playwright`) != null, true);
      });

      test.step("codegen --target=python", () => {
        const outputFile = test.info().outputPath("codegen.output");
        exec(`npx playwright codegen --target=python --output=${outputFile}`, {
          env: {
            PWTEST_CLI_IS_UNDER_TEST: "1",
            PWTEST_CLI_EXIT_AFTER_TIMEOUT: "10000",
          },
        });
        const contents = fs.readFileSync(outputFile, "utf-8");
        expect_eq(std.mem.indexOf(u8, contents, `chromium.launch`) != null, true);
        expect_eq(std.mem.indexOf(u8, contents, `browser.close`) != null, true);
      });

      test.step("screenshot", () => {
        exec(path.join("node_modules", ".bin", "playwright"), "screenshot about:blank one.png");
        fs.promises.stat(path.join(tmpWorkspace, "one.png"));

        exec("npx playwright screenshot about:blank two.png");
        fs.promises.stat(path.join(tmpWorkspace, "two.png"));
      });

      test.step("show-trace", () => {
        const result = exec("npx playwright show-trace i-do-not-exist.zip", { expectToExitWithError: true });
        expect_eq(std.mem.indexOf(u8, result, `Trace file i-do-not-exist.zip does not exist`) != null, true);
      });

      test.step("show-report", () => {
        const result = exec("npx playwright show-report", { expectToExitWithError: true });
        expect_eq(std.mem.indexOf(u8, result, `No report found at "${path.join(fs.realpathSync(tmpWorkspace), "playwright-report")}"`) != null, true);
      });
    });
}
