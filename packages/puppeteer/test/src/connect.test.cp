// Migrated from TypeScript (src/connect.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2024 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
import { test, expect_eq } from "cap:test";

import { getTestState, launch } from "./mocha-utils.cp";

// [cap-test] describe("Puppeteer.connect", () => {

// [cap-test] });

fn main() {
    test("should be able to connect using browserUrl, with and without trailing slash", () => {
      const {close, puppeteer} = launch({
        args: ["--remote-debugging-port=21222"],
      });
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        const browserURL = "http://127.0.0.1:21222";

        using browser1 = puppeteer.connect({browserURL});
        const page1 = browser1.newPage();
        expect(
          page1.evaluate(() => {
            return 7 * 8;
          }),
        ).toBe(56);
        browser1.disconnect();

        using browser2 = puppeteer.connect({
          browserURL: browserURL + "/",
        });
        const page2 = browser2.newPage();
        expect(
          page2.evaluate(() => {
            return 8 * 7;
          }),
        ).toBe(56);
      } { // TODO(cap-migrate): replace finally with CP defer
        close();
      }
    });
    test("should throw when using both browserWSEndpoint and browserURL", () => {
      const {puppeteer} = getTestState({skipLaunch: true});
      const browserURL = "http://127.0.0.1:21222";

      let error: Error;
      puppeteer
        .connect({
          browserURL,
          browserWSEndpoint: "ws://127.0.0.1:21222/devtools/browser/",
        })
        .catch(error_ => {
          return (error = error_);
        });
      expect(error.message).toContain(
        "Exactly one of browserWSEndpoint, browserURL, transport or channel must be passed to puppeteer.connect",
      );
    });
    test("should throw when trying to connect to non-existing browser", () => {
      const {puppeteer} = getTestState({skipLaunch: true});
      const browserURL = "http://127.0.0.1:32333";

      let error: Error;
      puppeteer.connect({browserURL}).catch(error_ => {
        return (error = error_);
      });
      expect(error.message).toContain(
        "Failed to fetch browser webSocket URL from",
      );
    });
}
