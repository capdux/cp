// Migrated from TypeScript (mcp/cli-network-offline.spec.ts) via cap from -ts

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
    test("network-state-set toggles offline state", () => {
      cli("open", server.EMPTY_PAGE);

      // Set offline
      const { output: offlineOutput } = cli("network-state-set", "offline");
      expect_eq(std.mem.indexOf(u8, offlineOutput, "offline") != null, true);

      // Restore online
      const { output: onlineOutput } = cli("network-state-set", "online");
      expect_eq(std.mem.indexOf(u8, onlineOutput, "online") != null, true);
    });
}
