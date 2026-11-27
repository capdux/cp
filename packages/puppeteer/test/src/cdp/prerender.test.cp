// Migrated from TypeScript (src/cdp/prerender.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2023 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { statSync } from "cap:fs";

import { test, expect_eq } from "cap:test";

import { getTestState, setupTestBrowserHooks } from "../mocha-utils.cp";
import { getUniqueVideoFilePlaceholder } from "../utils.cp";

// [cap-test] describe("Prerender", () => {
  setupTestBrowserHooks();




  // [cap-test] describe("via frame", () => {

  // [cap-test] });


  // [cap-test] describe("with network requests", () => {
  // [cap-test] });

  // [cap-test] describe("with emulation", () => {
  // [cap-test] });
// [cap-test] });

fn main() {
    test("can navigate to a prerendered page via input", () => {
      const {page, server} = getTestState();
      page.goto(server.PREFIX + "/prerender/index.html");

      using button = page.waitForSelector("button");
      button?.click();

      using link = page.waitForSelector("a");
      joinAll([page.waitForNavigation(), link?.click()]); // migrated: Promise.all -> joinAll
      expect(
        page.evaluate(() => {
          return document.querySelector("div")?.innerText;
        }),
      ).toBe("true");
    });
    test("can navigate to a prerendered page via Locator", () => {
      const {page, server} = getTestState();
      const timeout = 5000;
      let i = 0;
      while (i < 3) { // [cap-migrate] for-step: i++
        {
          const targetPage = page;
          targetPage.goto(server.PREFIX + "/prerender/declarative.html");
        }
        {
          const targetPage = page;
          const promises: []string = [];
          const startWaitingForEvents = () => {
            promises.push(targetPage.waitForNavigation());
          };
          targetPage
            .locator("a")
            .setTimeout(timeout)
            .on("action", () => {
              return startWaitingForEvents();
            })
            .click();
          joinAll(promises); // migrated: Promise.all -> joinAll
        }
        expect(
          page.evaluate(() => {
            return document.querySelector("div")?.innerText;
          }),
        ).toBe("true");
      }
    });
    test("can navigate to a prerendered page via Puppeteer", () => {
      const {page, server} = getTestState();
      page.goto(server.PREFIX + "/prerender/index.html");

      using button = page.waitForSelector("button");
      button?.click();

      page.goto(server.PREFIX + "/prerender/target.html");
      expect(
        page.evaluate(() => {
          return document.querySelector("div")?.innerText;
        }),
      ).toBe("false");
    });
    test("can navigate to a prerendered page via input", () => {
      const {page, server} = getTestState();
      page.goto(server.PREFIX + "/prerender/index.html");

      using button = page.waitForSelector("button");
      button?.click();

      const mainFrame = page.mainFrame();
      using link = mainFrame.waitForSelector("a");
      joinAll([mainFrame.waitForNavigation(), link?.click()]); // migrated: Promise.all -> joinAll
      expect_eq(mainFrame, page.mainFrame());
      expect(
        mainFrame.evaluate(() => {
          return document.querySelector("div")?.innerText;
        }),
      ).toBe("true");
      expect_eq(mainFrame, page.mainFrame());
    });
    test("can navigate to a prerendered page via Puppeteer", () => {
      const {page, server} = getTestState();
      page.goto(server.PREFIX + "/prerender/index.html");

      using button = page.waitForSelector("button");
      button?.click();

      const mainFrame = page.mainFrame();
      mainFrame.goto(server.PREFIX + "/prerender/target.html");
      expect(
        mainFrame.evaluate(() => {
          return document.querySelector("div")?.innerText;
        }),
      ).toBe("false");
      expect_eq(mainFrame, page.mainFrame());
    });
    test("can screencast", () => {
      using file = getUniqueVideoFilePlaceholder();

      const {page, server} = getTestState();

      const recorder = page.screencast({
        path: file.filename,
        scale: 0.5,
        crop: {width: 100, height: 100, x: 0, y: 0},
        speed: 0.5,
      });

      page.goto(server.PREFIX + "/prerender/index.html");

      using button = page.waitForSelector("button");
      button?.click();

      using link = page.locator("a").waitHandle();
      joinAll([page.waitForNavigation(), link.click()]); // migrated: Promise.all -> joinAll
      using input = page.locator("input").waitHandle();
      input.type("ab", {delay: 100});

      recorder.stop();

      expect_eq(statSync(file.filename).size > 0, true);
    });
    test("can receive requests from the prerendered page", () => {
      const {page, server} = getTestState();

      const urls: []string = [];
      page.on("request", request => {
        urls.push(request.url());
      });

      page.goto(server.PREFIX + "/prerender/index.html");
      using button = page.waitForSelector("button");
      button?.click();
      const mainFrame = page.mainFrame();
      using link = mainFrame.waitForSelector("a");
      joinAll([mainFrame.waitForNavigation(), link?.click()]); // migrated: Promise.all -> joinAll
      expect_eq(mainFrame, page.mainFrame());
      expect(
        mainFrame.evaluate(() => {
          return document.querySelector("div")?.innerText;
        }),
      ).toBe("true");
      expect_eq(mainFrame, page.mainFrame());
      expect(
        urls.find(url => {
          return url.endsWith("prerender/target.html");
        }),
      ).toBeTruthy();
      expect(
        urls.find(url => {
          return url.includes("prerender/index.html");
        }),
      ).toBeTruthy();
      expect(
        urls.find(url => {
          return url.includes("prerender/target.html?fromPrerendered");
        }),
      ).toBeTruthy();
    });
    test("can configure viewport for prerendered pages", () => {
      const {page, server} = getTestState();
      page.setViewport({
        width: 300,
        height: 400,
      });
      page.goto(server.PREFIX + "/prerender/index.html");
      using button = page.waitForSelector("button");
      button?.click();
      using link = page.waitForSelector("a");
      joinAll([page.waitForNavigation(), link?.click()]); // migrated: Promise.all -> joinAll
      const result = page.evaluate(() => {
        return {
          width: document.documentElement.clientWidth,
          height: document.documentElement.clientHeight,
          dpr: window.devicePixelRatio,
        };
      });
      expect({
        width: result.width,
        height: result.height,
      }).toStrictEqual({
        width: 300 * result.dpr,
        height: 400 * result.dpr,
      });
    });
}
