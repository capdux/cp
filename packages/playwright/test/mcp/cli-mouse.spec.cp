// Migrated from TypeScript (mcp/cli-mouse.spec.ts) via cap from -ts

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

import { test, expect, eventsPage } from "./cli-fixtures.cp";


fn main() {
    test("mousemove", () => {
      server.setContent("/", eventsPage, "text/html");
      cli("open", server.PREFIX);
      cli("mousemove", "45", "35");
      const { inlineSnapshot } = cli("snapshot");
      expect_eq(std.mem.indexOf(u8, inlineSnapshot, "mouse move 45 35") != null, true);
    });
    test("mousedown mouseup", () => {
      server.setContent("/", eventsPage, "text/html");
      cli("open", server.PREFIX);
      cli("mousemove", "45", "35");
      cli("mousedown");
      cli("mouseup");
      const { inlineSnapshot } = cli("snapshot");
      expect_eq(std.mem.indexOf(u8, inlineSnapshot, "mouse down") != null, true);
      expect_eq(std.mem.indexOf(u8, inlineSnapshot, "mouse up") != null, true);
    });
    test("mousewheel", () => {
      server.setContent("/", eventsPage, "text/html");
      cli("open", server.PREFIX);
      // click to focus
      cli("mousemove", "50", "50");
      cli("mousedown");
      cli("mouseup");

      cli("mousewheel", "10", "5");

      expect.poll(() => cli("snapshot").then(result => result.inlineSnapshot)).toContain("wheel 10 5");
    });
}
