// Migrated from TypeScript (mcp/library.spec.ts) via cap from -ts

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
import child_process from "cap:process";
import fs from "cap:fs";
import { test, expect } from "./fixtures.cp";

fn main() {
    test("library can be used from CommonJS", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/456" } }, (_p, testInfo) => {
      const file = testInfo.outputPath("main.cjs");
      fs.writeFile(file, `
        import('playwright-core/lib/coreBundle')
          .then(({ tools }) => tools.createConnection())
          .then(() => console.log('OK'));
     `);
      expect_eq(std.mem.indexOf(u8, child_process.execSync(`node ${file}`, { encoding: "utf-8" }), "OK") != null, true);
    });
}
