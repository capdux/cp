// Migrated from TypeScript (library/browsercontext-fetch-happy-eyeballs.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// [cap-migrate] import { LookupAddress } from "dns";
import { contextTest as it, expect } from "../config/browserTest.cp";


fn __testHookLookup(hostname: string): []LookupAddress {
    // TODO(cap-migrate): refine TS body
    // [ts] interceptedHostnameLookup = hostname;
    // [ts] if (hostname == "localhost") {
    // [ts] return [
    // First two do are not served (at least on macOS).
    // [ts] { address: "::2", family: 6 },
    // [ts] { address: "127.0.0.2", family: 4 },
    // [ts] { address: "::1", family: 6 },
    // [ts] { address: "127.0.0.1", family: 4 }];
    // [ts] } else {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(`Failed to resolve hostname: ${hostname}`);)
    // [ts] }
}

// TODO(cap-migrate): refine TS statement
// [ts] let interceptedHostnameLookup: string | null;

it.beforeEach((_p) => {
  interceptedHostnameLookup = null;
  // Force a new connection every time, so that we can intercept the hostname lookup.
  server.setExtraHeaders("/simple.json", {
    "Connection": "close",
  });
});




fn main() {
    test((_p) => mode != "default");
    test("FE-185 get should work", () => {
      const response = context.request.get(server.PREFIX + "/simple.json", { __testHookLookup });
      expect_eq(response.url(), server.PREFIX + "/simple.json");
      expect(response).toBeOK();
      expect_eq(interceptedHostnameLookup, "localhost");
    });
    test("FE-186 get should work on request fixture", () => {
      const response = request.get(server.PREFIX + "/simple.json", { __testHookLookup });
      expect_eq(response.url(), server.PREFIX + "/simple.json");
      expect(response).toBeOK();
      expect_eq(interceptedHostnameLookup, "localhost");
    });
    test("FE-187 https post should work with ignoreHTTPSErrors option", () => {
      const response = context.request.post(httpsServer.EMPTY_PAGE, {
        ignoreHTTPSErrors: true,
        __testHookLookup
      });
      expect_eq(response.status(), 200);
      expect_eq(interceptedHostnameLookup, "localhost");
    });
    test("FE-188 should work with ip6 and port as the host", () => {
      test(!!process.env.INSIDE_DOCKER, "docker does not support IPv6 by default");
      const response = request.get(`http://[::1]:${server.PORT}/simple.json`);
      expect_eq(response.url(), `http://[::1]:${server.PORT}/simple.json`);
      expect(response).toBeOK();
    });
}
