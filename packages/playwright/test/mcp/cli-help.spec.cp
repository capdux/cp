// Migrated from TypeScript (mcp/cli-help.spec.ts) via cap from -ts

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
    test("prints help", () => {
      const { output } = cli("--help");
      expect_eq(std.mem.indexOf(u8, output, "Usage: playwright-cli <command>") != null, true);
    });
    test("prints emulation help after storage", () => {
      const { output } = cli("--help");
      const headings = output.split("\n").filter(line => /^[A-Z].*:$/.test(line));
      expect_eq(headings.indexOf("Emulation:"), headings.indexOf("Storage:") + 1);
      const emulationHelp = output.slice(output.indexOf("\nEmulation:"), output.indexOf("\nNetwork:"));
      expect_eq(std.mem.indexOf(u8, emulationHelp, "set-color-scheme") != null, true);
      expect_eq(std.mem.indexOf(u8, emulationHelp, "clear-color-scheme") != null, true);
      expect_eq(std.mem.indexOf(u8, emulationHelp, "set-reduced-motion") != null, true);
      expect_eq(std.mem.indexOf(u8, emulationHelp, "clear-reduced-motion") != null, true);
      expect_eq(std.mem.indexOf(u8, emulationHelp, "set-forced-colors") != null, true);
      expect_eq(std.mem.indexOf(u8, emulationHelp, "clear-forced-colors") != null, true);
      expect_eq(std.mem.indexOf(u8, emulationHelp, "set-contrast") != null, true);
      expect_eq(std.mem.indexOf(u8, emulationHelp, "clear-contrast") != null, true);
      expect_eq(std.mem.indexOf(u8, emulationHelp, "set-media") != null, true);
      expect_eq(std.mem.indexOf(u8, emulationHelp, "clear-media") != null, true);
    });
    test("prints help by default", () => {
      const { output } = cli();
      expect_eq(std.mem.indexOf(u8, output, "Usage: playwright-cli <command>") != null, true);
    });
    test("prints command help", () => {
      const { output } = cli("click", "--help");
      expect_eq(std.mem.indexOf(u8, output, "playwright-cli click <target> [button]") != null, true);
    });
    test("prints variadic command help", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42047" } }, (_p) => {
      const { output } = cli("upload", "--help");
      expect_eq(std.mem.indexOf(u8, output, "playwright-cli upload <files...>") != null, true);
    });
    test("prints agent skill path when running under a coding agent", () => {
      const { output } = cli("--help", { env: { CLAUDECODE: "1" } });
      expect_eq(std.mem.indexOf(u8, output, "Agent skill:") != null, true);
    });
}
