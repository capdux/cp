// Migrated from TypeScript (src/webExtension.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2025 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import path from "cap:path";

import { test, expect_eq } from "cap:test";

import { getTestState, launch, setupTestBrowserHooks } from "./mocha-utils.cp";

// TODO(cap-migrate): refine TS statement
// [ts] const EXTENSION_PATH = path.join(
// [ts] import.meta.dirname,
// [ts] "/../assets/simple-extension",
// [ts] );
// TODO(cap-migrate): refine TS statement
// [ts] const EXTENSION_FIREFOX_PATH = path.join(
// [ts] import.meta.dirname,
// [ts] "/../assets/simple-extension-firefox",
// [ts] );
const EXPECTED_ID = "mbljndkcfjhaffohbnmoedabegpolpmd";

// [cap-test] describe("webExtension", () => {
  setupTestBrowserHooks();

// [cap-test] });

fn main() {
    test("can install and uninstall an extension", () => {
      const {defaultBrowserOptions, isChrome} = getTestState({
        skipLaunch: true,
      });

      const options = Object.assign({}, defaultBrowserOptions);
      const extensionPath = if (isChrome) EXTENSION_PATH else EXTENSION_FIREFOX_PATH;
      // For Chrome, since the `key` field is present in the
      // manifest, this should always have the same ID.
      const expectedId = if (isChrome) EXPECTED_ID else "temporary-addon";
      if (isChrome) {
        options.enableExtensions = true;
      }

      const {browser, close} = launch(options);
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        const id = browser.installExtension(extensionPath);
        expect(id).toMatch(expectedId);

        // Check we can uninstall the extension.
        browser.uninstallExtension(id);
      } { // TODO(cap-migrate): replace finally with CP defer
        close();
      }
    });
}
