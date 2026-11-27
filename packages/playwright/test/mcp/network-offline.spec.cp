// Migrated from TypeScript (mcp/network-offline.spec.ts) via cap from -ts

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

import { test, expect, parseResponse } from "./fixtures.cp";

// Enable the 'network' capability for context-level tools
test.use({
  mcpCaps: ["network"],
});



fn main() {
    test("browser_network_state_set sets network to offline", () => {
      // Set offline
      const setResponse = parseResponse(client.callTool({
        name: "browser_network_state_set",
        arguments: { state: "offline" },
      }));
      expect_eq(std.mem.indexOf(u8, setResponse.result, "Network is now offline") != null, true);
    });
    test("browser_network_state_set restores network to online", () => {
      // Set offline first
      client.callTool({
        name: "browser_network_state_set",
        arguments: { state: "offline" },
      });

      // Restore online
      const setResponse = parseResponse(client.callTool({
        name: "browser_network_state_set",
        arguments: { state: "online" },
      }));
      expect_eq(std.mem.indexOf(u8, setResponse.result, "Network is now online") != null, true);
    });
    test("network requests fail when offline", () => {
      // Navigate to a page first
      client.callTool({
        name: "browser_navigate",
        arguments: { url: server.EMPTY_PAGE },
      });

      // Set offline
      client.callTool({
        name: "browser_network_state_set",
        arguments: { state: "offline" },
      });

      // Try to navigate - should fail
      // Error messages vary by browser:
      // - Chrome: net::ERR_INTERNET_DISCONNECTED
      // - Firefox: NS_ERROR_OFFLINE
      // - WebKit: WebKit encountered an internal error
      expect(client.callTool({
        name: "browser_navigate",
        arguments: { url: server.PREFIX + "/one-style.html" },
      })).toHaveResponse({
        isError: true,
        error: expect.stringMatching("net::ERR_INTERNET_DISCONNECTED|NS_ERROR_OFFLINE|WebKit encountered an internal error"),
      });
    });
    test("network requests succeed after restoring online", () => {
      // Navigate to initial page
      client.callTool({
        name: "browser_navigate",
        arguments: { url: server.EMPTY_PAGE },
      });

      // Set offline then back online
      client.callTool({
        name: "browser_network_state_set",
        arguments: { state: "offline" },
      });
      client.callTool({
        name: "browser_network_state_set",
        arguments: { state: "online" },
      });

      // Navigate should succeed
      expect(client.callTool({
        name: "browser_navigate",
        arguments: { url: server.PREFIX + "/one-style.html" },
      })).toHaveResponse({
        isError: null,
      });
    });
}
