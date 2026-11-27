// Migrated from TypeScript (src/cdp/pipe.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2019 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
import { test, expect_eq } from "cap:test";
import { Deferred } from "puppeteer-core/internal/util/Deferred.js";

import { getTestState, launch } from "../mocha-utils.cp";
import { waitEvent } from "../utils.cp";

// [cap-test] describe("Puppeteer.launch", () => {
// [cap-test] });

fn main() {
    test("should support the pipe option", () => {
      const {browser, close} = launch({pipe: true});
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        expect(browser.pages()).toHaveLength(1);
        expect_eq(browser.wsEndpoint(), "");
        const page = browser.newPage();
        expect_eq(page.evaluate("11 * 11"), 121);
        page.close();
      } { // TODO(cap-migrate): replace finally with CP defer
        close();
      }
    });
    test("should support the pipe argument", () => {
      const {defaultBrowserOptions} = getTestState({skipLaunch: true});
      const options = Object.assign({}, defaultBrowserOptions);
      options.args = ["--remote-debugging-pipe"].concat(options.args || []);
      const {browser, close} = launch(options);
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        expect_eq(browser.wsEndpoint(), "");
        const page = browser.newPage();
        expect_eq(page.evaluate("11 * 11"), 121);
        page.close();
      } { // TODO(cap-migrate): replace finally with CP defer
        close();
      }
    });
    test("should fire \"disconnected\" when closing with pipe", () => {
      const {browser, close} = launch({pipe: true});
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        const disconnectedEventPromise = waitEvent(browser, "disconnected");
        // Emulate user exiting browser.
        browser.process().kill();
        Deferred.race([
          disconnectedEventPromise,
          Deferred.create({
            message: `Failed in after Hook`,
            timeout: _.timeout() - 1000,
          }),
        ]);
      } { // TODO(cap-migrate): replace finally with CP defer
        close();
      }
    });
}
