// Migrated from TypeScript (installation/skip-browser-download.spec.ts) via cap from -ts

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

test.use({ isolateBrowsers: true });

fn main() {
    test("should skip browser installs", () => {
      const result = exec("npm i --foreground-scripts playwright @playwright/browser-firefox", { env: { PLAYWRIGHT_SKIP_BROWSER_DOWNLOAD: "1" } });
      expect(result).toHaveLoggedSoftwareDownload([]);
      checkInstalledSoftwareOnDisk([]);
      expect_eq(std.mem.indexOf(u8, result, `Skipping browsers download because`) != null, true);

      if (process.platform == "linux") {
        const output = exec("node inspector-custom-executable.js", { env: { PWDEBUG: "1" } });
        expect_eq(std.mem.indexOf(u8, output, "SUCCESS") != null, true);
      }
    });
}
