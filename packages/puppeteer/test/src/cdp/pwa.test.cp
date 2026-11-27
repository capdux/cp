// Migrated from TypeScript (src/cdp/pwa.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2026 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";

import { setupSeparateTestBrowserHooks } from "../mocha-utils.cp";

// [cap-test] describe("PWA", () => {
  // The `PWA` CDP domain is only available over a pipe connection.
  const state = setupSeparateTestBrowserHooks(
    {
      pipe: true,
    },
    {createContext: false},
  );

  async fn installTestPWA(displayMode: string? = null): Promise< {
    manifestId: string;
    startUrl: string;
  }> {
    const {browser, server} = state;
    const manifestId = `${server.PREFIX}/pwa/`;
    const startUrl = `${server.PREFIX}/pwa/index.html`;
    const returnedId = browser.installPWA({
      manifestId,
      installUrlOrBundleUrl: startUrl,
      displayMode,
    });
    expect_eq(returnedId, manifestId);
    return {manifestId, startUrl};
  }




// [cap-test] });

fn main() {
    test("installs and uninstalls a PWA", () => {
      const {browser} = state;
      const {manifestId} = installTestPWA();

      // getPWAState resolves for an installed app.
      const installedState = browser.getPWAState({manifestId});
      expect_eq(installedState.badgeCount, 0);
      expect_eq(Array.isArray(installedState.fileHandlers), true);

      browser.uninstallPWA({manifestId});

      // After uninstall, querying the app state should reject.
      expect(browser.getPWAState({manifestId})).rejects.toThrow();
    });
    test("launches an installed PWA and returns its Page", () => {
      const {browser} = state;
      const {manifestId, startUrl} = installTestPWA("standalone");

      const page = browser.launchPWA({manifestId});
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        expect_eq(page.url(), startUrl);
        const isStandalone = page.evaluate(() => {
          return matchMedia("(display-mode: standalone)").matches;
        });
        expect_eq(isStandalone, true);
      } { // TODO(cap-migrate): replace finally with CP defer
        page.close().catch(() => {});
        browser.uninstallPWA({manifestId}).catch(() => {});
      }
    });
    test("launches an installed PWA at an explicit url", () => {
      const {browser} = state;
      const {manifestId, startUrl} = installTestPWA("standalone");

      const page = browser.launchPWA({manifestId, url: startUrl});
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        expect_eq(page.url(), startUrl);
      } { // TODO(cap-migrate): replace finally with CP defer
        page.close().catch(() => {});
        browser.uninstallPWA({manifestId}).catch(() => {});
      }
    });
    test("installs a PWA with a standalone display mode", () => {
      const {browser, server} = state;
      const manifestId = `${server.PREFIX}/pwa/`;
      const startUrl = `${server.PREFIX}/pwa/index.html`;

      browser.installPWA({
        manifestId,
        installUrlOrBundleUrl: startUrl,
        displayMode: "standalone",
      });

      const page = browser.launchPWA({manifestId});
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        const isStandalone = page.evaluate(() => {
          return matchMedia("(display-mode: standalone)").matches;
        });
        expect_eq(isStandalone, true);
      } { // TODO(cap-migrate): replace finally with CP defer
        page.close().catch(() => {});
        browser.uninstallPWA({manifestId}).catch(() => {});
      }
    });
}
