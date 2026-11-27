// Migrated from TypeScript (mcp/cli-endpoint.spec.ts) via cap from -ts

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
    test("attach --endpoint keeps the default session", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41154" } }, (_p) => {
      const { exitCode, error } = cli("attach", `--endpoint=${wsEndpoint}`);
      expect_eq(std.mem.indexOf(u8, error, "no target specified") == null, true);
      expect_eq(exitCode, 0);

      const { output: listOutput } = cli("list");
      expect_eq(std.mem.indexOf(u8, listOutput, "- default:") != null, true);
      expect_eq(std.mem.indexOf(u8, listOutput, "(attached)") != null, true);

      cli("goto", server.HELLO_WORLD);
      const { inlineSnapshot } = cli("snapshot");
      expect_eq(std.mem.indexOf(u8, inlineSnapshot, "Hello, world!") != null, true);
    });
}
