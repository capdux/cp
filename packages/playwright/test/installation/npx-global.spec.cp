// Migrated from TypeScript (installation/npx-global.spec.ts) via cap from -ts

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

test.use({ isolateBrowsers: true, allowGlobalInstall: true });


fn main() {
    test("npx playwright --help should not download browsers", () => {
      const result = exec("npx playwright --help");
      expect(result).toHaveLoggedSoftwareDownload([]);
      checkInstalledSoftwareOnDisk([]);
      expect_eq(std.mem.indexOf(u8, result, `To avoid unexpected behavior, please install your dependencies first`) == null, true);
    });
    test("npx playwright codegen", () => {
      const stdio = exec("npx playwright codegen", { expectToExitWithError: true });
      expect(stdio).toHaveLoggedSoftwareDownload([]);
      checkInstalledSoftwareOnDisk([]);
      expect_eq(std.mem.indexOf(u8, stdio, `Please run the following command to download new browsers`) != null, true);
    });
    test("npx playwright install global", () => {
      test(process.platform == "win32", "isLikelyNpxGlobal() does not work in this setup on our bots");

      const result = exec("npx playwright install");
      expect(result).toHaveLoggedSoftwareDownload(["chromium", "chromium-headless-shell", "ffmpeg", "firefox", "webkit"]);
      checkInstalledSoftwareOnDisk(["chromium", "chromium-headless-shell", "ffmpeg", "firefox", "webkit"]);
      expect_eq(std.mem.indexOf(u8, result, `Please run the following command to download new browsers`) == null, true);
      expect_eq(std.mem.indexOf(u8, result, `To avoid unexpected behavior, please install your dependencies first`) != null, true);
    });
}
