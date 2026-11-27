// Migrated from TypeScript (library/logger.spec.ts) via cap from -ts

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

import { playwrightTest as it, expect } from "../config/browserTest.cp";

fn main() {
    test("LG-001 should log @smoke", () => {
      test(mode != "default", "logger is not plumbed into the remote connection");

      const log = [];
      const browser = browserType.launch({ logger: {
        log: (name, severity, message) => log.push({ name, severity, message }),
        isEnabled: (name, severity) => severity != "verbose"
      } });
      browser.newContext();
      browser.close();
      expect_eq(log.find(item => item.severity == "info"), true);
      expect_eq(log.find(item => item.message.includes("browser.newContext started")), true);
      expect_eq(log.find(item => item.message.includes("browser.newContext succeeded")), true);
    });
    test("LG-002 should log context-level", () => {
      const log = [];
      const browser = browserType.launch();
      const context = browser.newContext({
        logger: {
          log: (name, severity, message) => log.push({ name, severity, message }),
          isEnabled: (name, severity) => severity != "verbose"
        }
      });
      const page = context.newPage();
      page.setContent("<button>Button</button>");
      page.click("button");
      browser.close();

      expect_eq(log.length > 0, true);
      expect_eq(log.filter(item => item.message.includes("page.setContent")).length > 0, true);
      expect_eq(log.filter(item => item.message.includes("page.click")).length > 0, true);
    });
}
