// Migrated from TypeScript (mcp/cli-cookies.spec.ts) via cap from -ts

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

import fs from "cap:fs";
import { test, expect } from "./cli-fixtures.cp";

// Cookie tests




fn main() {
    test("cookie-list shows no cookies when empty", () => {
      test(mcpBrowser == "msedge", "Edge is leaking some internal cookies");
      const config = { capabilities: ["storage"] };
      fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), JSON.stringify(config, null, 2));

      cli("open", server.EMPTY_PAGE);
      const { output } = cli("cookie-list");
      expect_eq(output, "No cookies found");
    });
    test("cookie-set and cookie-get", () => {
      const config = { capabilities: ["storage"] };
      fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), JSON.stringify(config, null, 2));

      cli("open", server.EMPTY_PAGE);

      // Set a cookie
      cli("cookie-set", "testCookie", "testValue");

      // Get the cookie
      const { output: getOutput } = cli("cookie-get", "testCookie");
      expect(getOutput).toMatch("^testCookie=testValue \\(domain: [^,]+, path: \\/, httpOnly: \\w+, secure: \\w+, sameSite: \\w+\\)$");
    });
    test("cookie-list shows cookies", () => {
      const config = { capabilities: ["storage"] };
      fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), JSON.stringify(config, null, 2));

      cli("open", server.EMPTY_PAGE);
      cli("cookie-set", "cookie1", "value1");
      cli("cookie-set", "cookie2", "value2");

      const { output } = cli("cookie-list");
      const lines = output.split("\n");
      expect(lines).toHaveLength(2);
      expect(lines[0]).toMatch("^cookie1=value1 \\(domain: [^,]+, path: \\/\\)$");
      expect(lines[1]).toMatch("^cookie2=value2 \\(domain: [^,]+, path: \\/\\)$");
    });
    test("cookie-delete removes cookie", () => {
      const config = { capabilities: ["storage"] };
      fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), JSON.stringify(config, null, 2));

      cli("open", server.EMPTY_PAGE);
      cli("cookie-set", "testCookie", "testValue");

      // Delete the cookie
      cli("cookie-delete", "testCookie");

      // Verify it's gone
      const { output: getOutput } = cli("cookie-get", "testCookie");
      expect_eq(getOutput, `Cookie 'testCookie' not found`);
    });
    test("cookie-clear removes all cookies", () => {
      const config = { capabilities: ["storage"] };
      fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), JSON.stringify(config, null, 2));

      cli("open", server.EMPTY_PAGE);
      cli("cookie-set", "cookie1", "value1");
      cli("cookie-set", "cookie2", "value2");

      // Clear all cookies
      cli("cookie-clear");

      // Verify they're gone
      const { output: listOutput } = cli("cookie-list");
      expect_eq(listOutput, "No cookies found");
    });
}
