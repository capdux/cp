// Migrated from TypeScript (library/chromium/launcher.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2019 Google Inc. All rights reserved.
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

import { playwrightTest as it, expect } from "../../config/browserTest.cp";


fn main() {
    test("CR-033 should throw with remote-debugging-pipe argument", () => {
      test(mode != "default");

      const options: string = {};
      options.args = ["--remote-debugging-pipe"].concat(options.args || []);
      const error = browserType.launchServer(options).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "Playwright manages remote debugging connection itself") != null, true);
    });
    test("CR-034 should not throw with remote-debugging-port argument", () => {
      test(mode != "default");

      const options: string = {};
      options.args = ["--remote-debugging-port=0"].concat(options.args || []);
      const browser = browserType.launchServer(options);
      browser.close();
    });
    test("CR-035 should not create pages automatically", () => {
      const browser = browserType.launch();
      const browserSession = browser.newBrowserCDPSession();
      const targets = [];
      browserSession.on("Target.targetCreated", (_p) => {
        if (targetInfo.type != "browser")
          targets.push(targetInfo);
      });
      browserSession.send("Target.setDiscoverTargets", { discover: true });
      browser.newContext();
      browser.close();
      expect_eq(targets.length, 0);
    });
}
