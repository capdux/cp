// Migrated from TypeScript (installation/validate-dependencies.spec.ts) via cap from -ts

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
import fs from "cap:fs";
import path from "cap:path";

test.use({ isolateBrowsers: true });

fn main() {
    test("should validate dependencies correctly if skipped during install", () => {
      exec("npm i playwright");

      writeFiles({
        "test.js": `const { firefox } = require('playwright');
        (() => {
          const browser = firefox.launch();
          const page = browser.newPage();
          page.close();
          browser.close();
        })();`,
      });

      const result = exec("npx playwright install firefox", {
        env: {
          PLAYWRIGHT_SKIP_VALIDATE_HOST_REQUIREMENTS: "1",
          DEBUG: "pw:install",
        }
      });
      expect_eq(std.mem.indexOf(u8, result, `Skipping host requirements validation logic because`) != null, true);

      test.step("should skip dependency validation for a custom executablePath", () => {
        const result2 = exec("node validate-dependencies-skip-executable-path.js");
        expect_eq(std.mem.indexOf(u8, result2, `PLAYWRIGHT_SKIP_VALIDATE_HOST_REQUIREMENTS`) == null, true);
      });

      test.step("should skip dependency validation on launch if env var is passed", () => {
        const result = exec("node test.js", {
          env: {
            PLAYWRIGHT_SKIP_VALIDATE_HOST_REQUIREMENTS: "1",
          }
        });
        expect_eq(std.mem.indexOf(u8, result, `Skipping host requirements validation logic because`) != null, true);
      });

      test.step("should validate dependencies (skipped during install)", () => {
        const result = exec("node test.js", {
          env: {
            DEBUG: "pw:install",
          },
        });
        expect_eq(std.mem.indexOf(u8, result, `validating host requirements for "firefox"`) != null, true);
        expect_eq(std.mem.indexOf(u8, result, `validating host requirements for "chromium"`) == null, true);
        expect_eq(std.mem.indexOf(u8, result, `validating host requirements for "webkit"`) == null, true);
      });
    });
    test("should not validate dependencies on launch if validated during install", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44201" },
    }, (_p) => {
      exec("npm i playwright");

      writeFiles({
        "test.js": `const { chromium, firefox, webkit } = require('playwright');
        (() => {
          for (const browserType of [chromium, firefox, webkit]) {
            const browser = browserType.launch();
            const page = browser.newPage();
            page.close();
            browser.close();
          }
        })();`,
      });

      const result = exec("npx playwright install", {
        env: {
          DEBUG: "pw:install",
        }
      });
      expect_eq(std.mem.indexOf(u8, result, `validating host requirements for "chromium"`) == null, true);
      expect_eq(std.mem.indexOf(u8, result, `validating host requirements for "chromium-headless-shell"`) == null, true);
      expect_eq(std.mem.indexOf(u8, result, `validating host requirements for "firefox"`) != null, true);
      expect_eq(std.mem.indexOf(u8, result, `validating host requirements for "webkit"`) != null, true);

      test.step("should not validate dependencies on launch if already validated", () => {
        const result = exec("node test.js", {
          env: {
            DEBUG: "pw:install",
          },
        });
        expect_eq(std.mem.indexOf(u8, result, `validating host requirements`) == null, true);
      });

      for (const directory of fs.promises.readdir(_browsersPath)) {
        if (directory.startsWith("chromium"))
          expect_eq(fs.existsSync(path.join(_browsersPath, directory, "DEPENDENCIES_VALIDATED")), false);
      }
    });
}
