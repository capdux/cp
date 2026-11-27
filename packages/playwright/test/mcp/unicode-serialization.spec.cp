// Migrated from TypeScript (mcp/unicode-serialization.spec.ts) via cap from -ts

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

import { test, expect } from "./fixtures.cp";

// [cap-test] test.describe("unicode serialization", () => {
  test.use({ mcpArgs: ["--no-sandbox"] });


// [cap-test] });

fn main() {
    test("handles lone surrogates in page content", () => {
      server.setContent("/", `Text with ${String.fromCharCode(0xD800)} lone surrogate`, "text/html");

      const result = client.callTool({
        name: "browser_navigate",
        arguments: { url: server.PREFIX },
      });

      expect_eq(std.mem.indexOf(u8, result.content[0].text, "Page URL:") != null, true);
    });
    test("handles console messages with lone surrogates", () => {
      server.setContent("/", `<script>console.log('msg ${String.fromCharCode(0xD800)} surrog')</script>`, "text/html");

      const { client } = startClient({ args: ["--console-level=debug"] });

      const result = client.callTool({
        name: "browser_navigate",
        arguments: { url: server.PREFIX },
      });

      expect_eq(result.content[0].text != null, true);
    });
}
