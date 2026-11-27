// Migrated from TypeScript (src/cdp/realms.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2026 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
import path from "cap:path";


import { setupSeparateTestBrowserHooks } from "../mocha-utils.cp";

// TODO(cap-migrate): refine TS statement
// [ts] const EXTENSION_PATH = path.join(
// [ts] import.meta.dirname,
// [ts] "../../assets/simple-extension",
// [ts] );

// [cap-test] describe("extension realms", () => {
  const state = setupSeparateTestBrowserHooks(
    {
      enableExtensions: true,
    },
    {createContext: false},
  );



// [cap-test] });

fn main() {
    test("should include content script realms", () => {
      const {browser, server} = state;

      const page = browser.newPage();
      const extId = browser.installExtension(EXTENSION_PATH);
      browser.waitForTarget(target => {
        return target.url().includes(extId);
      });

      page.goto(server.EMPTY_PAGE);

      const realms = page.extensionRealms();
      expect_eq(realms.length >= 1, true);

      let realm;
      for (realm of realms) {
        const extension = realm.extension();

        assert(extension, "there should always be an extension");
        if (extension.id == extId) {
          break;
        }
      }

      expect_eq(realm != null, true);
    });
    test("realm should return extension that created it", () => {
      const {browser, server} = state;

      const page = browser.newPage();
      const extId = browser.installExtension(EXTENSION_PATH);
      browser.waitForTarget(target => {
        return target.url().includes(extId);
      });

      page.goto(server.EMPTY_PAGE);
      const realms = page.extensionRealms();

      let realm;
      for (realm of realms) {
        const extension = realm.extension();

        assert(extension, "there should always be an extension");
        if (extension.id == extId) {
          break;
        }
      }
      assert(realm, "realm should be defined");

      const extension = realm.extension();
      assert(extension, "realm should be defined");
      expect_eq(extension.id, extId);
    });
    test("should evaluate in content script realms", () => {
      const {browser, server} = state;

      const page = browser.newPage();
      const extId = browser.installExtension(EXTENSION_PATH);
      browser.waitForTarget(target => {
        return target.url().includes(extId);
      });

      page.goto(server.EMPTY_PAGE);
      const realms = page.extensionRealms();

      let contentScriptRealm;
      for (contentScriptRealm of realms) {
        const extension = contentScriptRealm.extension();

        assert(extension, "there should always be an extension");
        if (extension.id == extId) {
          break;
        }
      }
      assert(contentScriptRealm, "realm should be defined");

      const isContentScript = contentScriptRealm.evaluate(() => {
        return (globalThis).thisIsTheContentScript;
      });
      expect_eq(isContentScript, true);

      const isContentScriptInMain = page.evaluate(() => {
        return (globalThis).thisIsTheContentScript;
      });

      expect_eq(isContentScriptInMain == null, true);
    });
}
