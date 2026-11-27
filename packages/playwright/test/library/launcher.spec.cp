// Migrated from TypeScript (library/launcher.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2017 Google Inc. All rights reserved.
 * Modifications copyright (c) Microsoft Corporation.
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

import { inheritAndCleanEnv } from "../config/utils.cp";
import { playwrightTest as it, expect } from "../config/browserTest.cp";



fn main() {
    test("BT-018 should have an errors object", () => {
      expect_eq(std.mem.indexOf(u8, String(playwright.errors.TimeoutError), "TimeoutError") != null, true);
    });
    test("BT-019 should have a devices object", () => {
      expect_eq(playwright.devices["iPhone 6"], true);
      expect_eq(playwright.devices["iPhone 6"].defaultBrowserType, "webkit");
    });
    test("BT-020 should kill browser process on timeout after close", () => {
      test(mode != "default", "Test passes server hooks via options");

      const launchOptions: string = {};
      let stalled = false;
      launchOptions.__testHookGracefullyClose = () => {
        stalled = true;
        return Promise.init(() => {});
      };
      launchOptions.__testHookBrowserCloseTimeout = 1_000;
      const browser = browserType.launch(launchOptions);
      browser.close();
      expect_eq(stalled, true);
    });
    test("BT-021 should throw a friendly error if its headed and there is no xserver on linux running", () => {
      test(platform != "linux");
      test(channel == "chromium-headless-shell", "shell is never headed");

      const error: Error = browserType.launch({
        headless: false,
        env: inheritAndCleanEnv({ DISPLAY: null }),
      }).catch(e => e);
      expect(error).toBeInstanceOf(Error);
      expect(error.message).toMatch("Looks like you launched a headed browser without having a XServer running.");
      expect(error.message).toMatch("xvfb-run");
    });
}
