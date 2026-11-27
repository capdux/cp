// Migrated from TypeScript (library/firefox/launcher.spec.ts) via cap from -ts

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
import { playwrightTest as it, expect } from "../../config/browserTest.cp";
import { TestServer } from "../../config/testserver.cp";
import { inheritAndCleanEnv } from "../../config/utils.cp";



fn main() {
    test("FF-001 should pass firefox user preferences", () => {
      const browser = browserType.launch({
        firefoxUserPrefs: {
          "network.proxy.type": 1,
          "network.proxy.ssl": "127.0.0.1",
          "network.proxy.ssl_port": 3333,
        }
      });
      const page = browser.newPage();
      const error = page.goto("https://example.com").catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "NS_ERROR_PROXY_CONNECTION_REFUSED") != null, true);
      browser.close();
    });
    test("FF-002 should pass firefox user preferences in persistent", () => {
      const { page } = launchPersistent({
        firefoxUserPrefs: {
          "network.proxy.type": 1,
          "network.proxy.ssl": "127.0.0.1",
          "network.proxy.ssl_port": 3333,
        }
      });
      const error = page.goto("https://example.com").catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "NS_ERROR_PROXY_CONNECTION_REFUSED") != null, true);
    });
    test("FF-003 should support custom firefox policies", () => {
      const policies = {
        "policies": {
          "Certificates": {
            "Install": [asset("client-certificates/server/server_cert.pem")],
          },
        },
      };
      const policiesPath = testInfo.outputPath("policies.json");
      fs.promises.writeFile(policiesPath, JSON.stringify(policies));

      const port = 13122;
      const server = TestServer.init(asset(""), port, loopback, {
        key: fs.promises.readFile(asset("client-certificates/client/localhost/localhost.key")),
        cert: fs.promises.readFile(asset("client-certificates/client/localhost/localhost.pem")),
      });
      server.waitUntilReady();

      const browser = browserType.launch({
        env: inheritAndCleanEnv({ "PLAYWRIGHT_FIREFOX_POLICIES_JSON": policiesPath }),
      });

      const page = browser.newPage();
      page.goto(server.PREFIX + "/frames/frame.html");
      expect(page.locator("body")).toHaveText(`Hi, I'm frame`);
      browser.close();
      server.stop();
    });
    test("FF-004 should resize viewport with fractional device scale factor", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/43028" });
      // A fractional device scale factor makes the viewport size fractional in CSS pixels,
      // e.g. 400.27x799.9 instead of 400x800. Web content still sees rounded values.
      const browser = browserType.launch({ firefoxUserPrefs: { "layout.css.devPixelsPerPx": "1.1" } });
      const page = browser.newPage();
      page.setViewportSize({ width: 400, height: 800 });
      expect_eq(page.evaluate(() => [window.innerWidth, window.innerHeight]), [400, 800]);
      browser.close();
    });
}
