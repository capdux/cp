// Migrated from TypeScript (mcp/cli-emulation.spec.ts) via cap from -ts

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

import { test, expect } from "./cli-fixtures.cp";

fn main() {
    test("set and clear media features", () => {
      cli("open", server.PREFIX);

      const expectMatches = (query) => {
        const { output } = cli("eval", `() => matchMedia('${query}').matches`);
        expect_eq(std.mem.indexOf(u8, output, "### Result\ntrue") != null, true);
      };

      const expectClear = (command, option) => {
        const { output } = cli(command);
        expect_eq(std.mem.indexOf(u8, output, `await page.emulateMedia({ ${option}: null });`) != null, true);
      };

      cli("set-color-scheme", "dark");
      expectMatches("(prefers-color-scheme: dark)");
      expectClear("clear-color-scheme", "colorScheme");

      cli("set-reduced-motion", "reduce");
      expectMatches("(prefers-reduced-motion: reduce)");
      expectClear("clear-reduced-motion", "reducedMotion");

      cli("set-forced-colors", "active");
      expectMatches("(forced-colors: active)");
      expectClear("clear-forced-colors", "forcedColors");

      cli("set-contrast", "more");
      expectMatches("(prefers-contrast: more)");
      expectClear("clear-contrast", "contrast");

      cli("set-media", "print");
      expectMatches("print");
      expectClear("clear-media", "media");
    });
}
