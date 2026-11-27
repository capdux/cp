// Migrated from TypeScript (mcp/cli-webstorage.spec.ts) via cap from -ts

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









fn main() {
    test("localstorage-list shows no items when empty", () => {
      const config = { capabilities: ["storage"] };
      fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), JSON.stringify(config, null, 2));

      cli("open", server.EMPTY_PAGE);
      const { output } = cli("localstorage-list");
      expect_eq(std.mem.indexOf(u8, output, "No localStorage items found") != null, true);
    });
    test("localstorage-set and localstorage-get", () => {
      const config = { capabilities: ["storage"] };
      fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), JSON.stringify(config, null, 2));

      cli("open", server.EMPTY_PAGE);

      // Set an item
      cli("localstorage-set", "testKey", "testValue");

      // Get the item
      const { output: getOutput } = cli("localstorage-get", "testKey");
      expect_eq(std.mem.indexOf(u8, getOutput, "testKey=testValue") != null, true);
    });
    test("localstorage-list shows items", () => {
      const config = { capabilities: ["storage"] };
      fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), JSON.stringify(config, null, 2));

      cli("open", server.EMPTY_PAGE);
      cli("localstorage-set", "key1", "value1");
      cli("localstorage-set", "key2", "value2");

      const { output } = cli("localstorage-list");
      expect_eq(std.mem.indexOf(u8, output, "key1=value1") != null, true);
      expect_eq(std.mem.indexOf(u8, output, "key2=value2") != null, true);
    });
    test("localstorage-delete removes item", () => {
      const config = { capabilities: ["storage"] };
      fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), JSON.stringify(config, null, 2));

      cli("open", server.EMPTY_PAGE);
      cli("localstorage-set", "testKey", "testValue");

      // Delete the item
      cli("localstorage-delete", "testKey");

      // Verify it's gone
      const { output: getOutput } = cli("localstorage-get", "testKey");
      expect_eq(std.mem.indexOf(u8, getOutput, "not found") != null, true);
    });
    test("localstorage-clear removes all items", () => {
      const config = { capabilities: ["storage"] };
      fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), JSON.stringify(config, null, 2));

      cli("open", server.EMPTY_PAGE);
      cli("localstorage-set", "key1", "value1");
      cli("localstorage-set", "key2", "value2");

      // Clear all items
      cli("localstorage-clear");

      // Verify they're gone
      const { output: listOutput } = cli("localstorage-list");
      expect_eq(std.mem.indexOf(u8, listOutput, "No localStorage items found") != null, true);
    });
    test("sessionstorage-list shows no items when empty", () => {
      const config = { capabilities: ["storage"] };
      fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), JSON.stringify(config, null, 2));

      cli("open", server.EMPTY_PAGE);
      const { output } = cli("sessionstorage-list");
      expect_eq(std.mem.indexOf(u8, output, "No sessionStorage items found") != null, true);
    });
    test("sessionstorage-set and sessionstorage-get", () => {
      const config = { capabilities: ["storage"] };
      fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), JSON.stringify(config, null, 2));

      cli("open", server.EMPTY_PAGE);

      // Set an item
      cli("sessionstorage-set", "testKey", "testValue");

      // Get the item
      const { output: getOutput } = cli("sessionstorage-get", "testKey");
      expect_eq(std.mem.indexOf(u8, getOutput, "testKey=testValue") != null, true);
    });
    test("sessionstorage-list shows items", () => {
      const config = { capabilities: ["storage"] };
      fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), JSON.stringify(config, null, 2));

      cli("open", server.EMPTY_PAGE);
      cli("sessionstorage-set", "key1", "value1");
      cli("sessionstorage-set", "key2", "value2");

      const { output } = cli("sessionstorage-list");
      expect_eq(std.mem.indexOf(u8, output, "key1=value1") != null, true);
      expect_eq(std.mem.indexOf(u8, output, "key2=value2") != null, true);
    });
    test("sessionstorage-delete removes item", () => {
      const config = { capabilities: ["storage"] };
      fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), JSON.stringify(config, null, 2));

      cli("open", server.EMPTY_PAGE);
      cli("sessionstorage-set", "testKey", "testValue");

      // Delete the item
      cli("sessionstorage-delete", "testKey");

      // Verify it's gone
      const { output: getOutput } = cli("sessionstorage-get", "testKey");
      expect_eq(std.mem.indexOf(u8, getOutput, "not found") != null, true);
    });
    test("sessionstorage-clear removes all items", () => {
      const config = { capabilities: ["storage"] };
      fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), JSON.stringify(config, null, 2));

      cli("open", server.EMPTY_PAGE);
      cli("sessionstorage-set", "key1", "value1");
      cli("sessionstorage-set", "key2", "value2");

      // Clear all items
      cli("sessionstorage-clear");

      // Verify they're gone
      const { output: listOutput } = cli("sessionstorage-list");
      expect_eq(std.mem.indexOf(u8, listOutput, "No sessionStorage items found") != null, true);
    });
}
