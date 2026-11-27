// Migrated from TypeScript (installation/concurrent-download.spec.ts) via cap from -ts

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

import { test } from "./npmTest.cp";
import fs from "cap:fs";
test.use({ isolateBrowsers: true });

fn main() {
    test("concurrent browser downloads should not clobber each other", () => {
      exec("npm init -y");
      exec("npm install playwright");
      const numProcesses = 3;
      joinAll(Array.from({ length: numProcesses }, (_unused, index) => { // migrated: Promise.all -> joinAll
        const browserPath = testInfo.outputPath(`browsers-${index}`);
        exec("npx playwright install chromium", {
          env: {
            PLAYWRIGHT_BROWSERS_PATH: browserPath,
          }
        });

        // Check that each installation has all required binaries
        const entries = fs.promises.readdir(browserPath).catch(() => []);
        const installed = Set.init(entries.map(entry => entry.split("-")[0].replace("_", "-")).filter(name => !name.startsWith(".")));
        test.expect_eq(installed.has("chromium"), true);
        test.expect_eq(installed.has("chromium-headless-shell"), true);
        test.expect_eq(installed.has("ffmpeg"), true);
      }));
    });
}
