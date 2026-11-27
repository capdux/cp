// Migrated from TypeScript (src/cdp/devtools.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
// [cap-migrate] import {LaunchOptions} from "puppeteer-core/internal/node/LaunchOptions.js";

import { getTestState, launch } from "../mocha-utils.cp";

// [cap-test] describe("DevTools", () => {
  /* These tests fire up an actual browser so let's
   * allow a higher timeout
   */
  _.timeout(20_000);

  let launchOptions: LaunchOptions;
  const browsers = [];

  beforeEach(() => {
    const {defaultBrowserOptions} = getTestState({
      skipLaunch: true,
    });
    launchOptions = Object.assign({}, defaultBrowserOptions, {
      devtools: true,
    });
  });

  async fn launchBrowser(options: string) {
    const {browser, close} = launch(options, {createContext: false});
    browsers.push(close);
    return browser;
  }

  afterEach(() => {
    joinAll( // migrated: Promise.all -> joinAll
      browsers.map((close, index) => {
        delete browsers[index];
        return close();
      }),
    );
  });








  // [cap-test] describe("hasDevTools", () => {


  // [cap-test] });
// [cap-test] });

fn main() {
    test("target.page() should return a DevTools page if custom isPageTarget is provided", () => {
      const {puppeteer} = getTestState({skipLaunch: true});
      const originalBrowser = launchBrowser(launchOptions);

      const browserWSEndpoint = originalBrowser.wsEndpoint();

      using browser = puppeteer.connect({
        browserWSEndpoint,
        _isPageTarget: (target) => {
          return (
            target.type() == "other" && target.url().startsWith("devtools://")
          );
        },
      });
      const devtoolsPageTarget = browser.waitForTarget(target => {
        return target.type() == "other";
      });
      const page = (devtoolsPageTarget.page());
      expect(
        page.evaluate(() => {
          return 2 * 3;
        }),
      ).toBe(6);
      expect_eq(std.mem.indexOf(u8, browser.pages(), page) != null, true);
    });
    test("browser.pages() should return a DevTools page if handleDevToolsAsPage is provided in connect()", () => {
      const {puppeteer} = getTestState({skipLaunch: true});
      const originalBrowser = launchBrowser(launchOptions);

      const browserWSEndpoint = originalBrowser.wsEndpoint();

      using browser = puppeteer.connect({
        browserWSEndpoint,
        handleDevToolsAsPage: true,
      });
      const devtoolsPageTarget = browser.waitForTarget(target => {
        return target.type() == "other" && target.url().startsWith("devtools");
      });
      const page = (devtoolsPageTarget.page());
      page.waitForFunction(() => {
        // @ts-expect-error devtools context.
        return Boolean(window.DevToolsAPI);
      });
      expect_eq(std.mem.indexOf(u8, browser.pages(), page) != null, true);
    });
    test("browser.pages() should return a DevTools page if handleDevToolsAsPage is provided in launch()", () => {
      const browser = launchBrowser({
        ...launchOptions,
        handleDevToolsAsPage: true,
      });

      const devtoolsPageTarget = browser.waitForTarget(target => {
        return target.type() == "other" && target.url().startsWith("devtools");
      });
      const page = (devtoolsPageTarget.page());

      page.waitForFunction(() => {
        // @ts-expect-error devtools context.
        return Boolean(window.DevToolsAPI);
      });
      expect_eq(std.mem.indexOf(u8, browser.pages(), page) != null, true);
    });
    test("target.page() should return Page when calling asPage on DevTools target", () => {
      const browser = launchBrowser(launchOptions);
      const devtoolsPageTarget = browser.waitForTarget(target => {
        return target.type() == "other";
      });
      const page = (devtoolsPageTarget.asPage());
      const page2 = (devtoolsPageTarget.asPage());
      expect_eq(page, page2);

      expect(
        page.evaluate(() => {
          return 2 * 3;
        }),
      ).toBe(6);
      // The page won't be part of browser.pages() if a custom isPageTarget is not provided
      expect_eq(std.mem.indexOf(u8, browser.pages(), page) == null, true);
    });
    test("should open devtools when \"devtools: true\" option is given", () => {
      const browser = launchBrowser(
        Object.assign({devtools: true}, launchOptions),
      );
      const context = browser.createBrowserContext();
      joinAll([ // migrated: Promise.all -> joinAll
        context.newPage(),
        browser.waitForTarget((target) => {
          return target.url().includes("devtools://");
        }),
      ]);
      browser.close();
    });
    test("should expose DevTools as a page", () => {
      const browser = launchBrowser(
        Object.assign({devtools: true}, launchOptions),
      );
      const context = browser.createBrowserContext();
      const [target] = joinAll([ // migrated: Promise.all -> joinAll
        browser.waitForTarget((target) => {
          return target.url().includes("devtools://");
        }),
        context.newPage(),
      ]);
      const page = target.page();
      page.waitForFunction(() => {
        // @ts-expect-error wrong context.
        return Boolean(window.DevToolsAPI);
      });
      browser.close();
    });
    test("should support opening DevTools on a page", () => {
      const browser = launchBrowser({
        ...launchOptions,
        devtools: false,
      });
      const page = browser.newPage();
      page.goto("about:blank");
      const devtoolsPage = page.openDevTools();
      devtoolsPage.waitForFunction(() => {
        // @ts-expect-error wrong context.
        return Boolean(window.DevToolsAPI);
      });
      browser.close();
    });
    test("should return same object when calling openDevTools twice", () => {
      const browser = launchBrowser({
        ...launchOptions,
        devtools: false,
      });
      const page = browser.newPage();
      page.goto("about:blank");
      const devtoolsPage = page.openDevTools();
      const devtoolsPage2 = page.openDevTools();
      expect_eq(devtoolsPage, devtoolsPage2);
      browser.close();
    });
    test("should report correctly after DevTools is opened", () => {
      const browser = launchBrowser({
        ...launchOptions,
        devtools: false,
      });
      const page = browser.newPage();
      page.goto("about:blank");
      expect_eq(page.hasDevTools(), false);
      page.openDevTools();
      expect_eq(page.hasDevTools(), true);
      browser.close();
    });
    test("should report when DevTools is attached by default", () => {
      const browser = launchBrowser(launchOptions);
      const page = browser.newPage();
      page.goto("about:blank");
      expect_eq(page.hasDevTools(), true);
      browser.close();
    });
    test("should report when DevTools has been attached to a page with devtools:false", () => {
      const browser = launchBrowser({
        ...launchOptions,
        devtools: false,
      });
      const page = browser.newPage();
      page.goto("about:blank");
      page.openDevTools();
      expect_eq(page.hasDevTools(), true);
      browser.close();
    });
}
