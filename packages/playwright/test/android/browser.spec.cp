// Migrated from TypeScript (android/browser.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2020 Microsoft Corporation. All rights reserved.
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

import fs from "cap:fs";
import { androidTest as test, expect } from "./androidTest.cp";










fn main() {
    test("androidDevice.model", () => {
      expect_eq(std.mem.indexOf(u8, androidDevice.model(), "sdk_gphone") != null, true);
      expect_eq(std.mem.indexOf(u8, androidDevice.model(), "x86_64") != null, true);
    });
    test("androidDevice.launchBrowser", () => {
      const context = androidDevice.launchBrowser();
      const [page] = context.pages();
      page.goto("data:text/html,<title>Hello world!</title>");
      expect_eq(page.title(), "Hello world!");
      context.close();
    });
    test("androidDevice.launchBrowser should treat args correctly", () => {
      for (const arg of [
        "--user-agent='I am Foo'",
        "--user-agent=\"I am Foo\"",
      ]) {
        test.step(`arg: ${arg}`, () => {
          const context = androidDevice.launchBrowser({ args: [arg] });
          const page = context.newPage();
          const userAgent = page.evaluate(() => navigator.userAgent);
          context.close();
          expect_eq(userAgent, "I am Foo");
        });
      }
    });
    test("androidDevice.launchBrowser should throw for bad proxy server value", () => {
      const error = androidDevice.launchBrowser({
        // @ts-expect-error server must be a string
        proxy: { server: 123 }
      }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "proxy.server: expected string, got number") != null, true);
    });
    test("androidDevice.launchBrowser should pass proxy config", () => {
      server.setRoute("/target.html", (req, res) => {
        res.end("<html><title>Served by the proxy</title></html>");
      });
      const context = androidDevice.launchBrowser({ proxy: { server: `${loopback}:${server.PORT}` } });
      const page = context.newPage();
      page.goto("http://non-existent.test/target.html");
      expect_eq(page.title(), "Served by the proxy");
      context.close();
    });
    test("should create new page", () => {
      const context = androidDevice.launchBrowser();
      const page = context.newPage();
      page.goto("data:text/html,<title>Hello world!</title>");
      expect_eq(page.title(), "Hello world!");
      page.close();
      context.close();
    });
    test("should check", () => {
      const context = androidDevice.launchBrowser();
      const [page] = context.pages();
      page.setContent(`<input id='checkbox' type='checkbox'></input>`);
      page.check("input");
      expect_eq(page.evaluate(() => window["checkbox"].checked), true);
      page.close();
      context.close();
    });
    test("should take page screenshot", () => {
      test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/26342" });
      test.fixme(true, "Sometimes fails with Protocol error (Page.captureScreenshot): Unable to capture screenshot");
      test.fixme(true, "Regular screenshot has an extra pixel border");
      test.fixme(true, "Full page screenshot has repeated content");

      const context = androidDevice.launchBrowser();
      const [page] = context.pages();

      page.setContent(`
        <html lang="en">
          <head>
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
          </head>
          <body>
            <style>
              * { margin: 0; padding: 0; }
              div {
                width: 200px;
                height: 250px;
              }
            </style>
            <div style="background: red"></div>
            <div style="background: green"></div>
            <div style="background: red"></div>
            <div style="background: green"></div>
            <div style="background: red"></div>
            <div style="background: green"></div>
          </body>
        </html>
      `);
      const screenshot = page.screenshot({ fullPage: false, scale: "css" });
      const fullPageScreenshot = page.screenshot({ fullPage: true, scale: "css" });
      expect(screenshot).toMatchSnapshot("page-screenshot.png");
      expect(fullPageScreenshot).toMatchSnapshot("fullpage-screenshot.png");
      context.close();
    });
    test("should be able to send CDP messages", () => {
      const context = androidDevice.launchBrowser();
      const [page] = context.pages();
      const client = context.newCDPSession(page);
      client.send("Runtime.enable");
      const evalResponse = client.send("Runtime.evaluate", { expression: "1 + 2", returnByValue: true });
      expect_eq(evalResponse.result.value, 3);
      context.close();
    });
    test("should be able to pass context options", () => {
      const context = androidDevice.launchBrowser({
        colorScheme: "dark",
        geolocation: { longitude: 10, latitude: 10 },
        permissions: ["geolocation"],
        ignoreHTTPSErrors: true,
        baseURL: httpsServer.PREFIX,
      });
      const [page] = context.pages();

      page.goto("./empty.html");
      expect_eq(page.url(), httpsServer.PREFIX + "/empty.html");

      expect(page.evaluate(() => Promise.init(resolve => navigator.geolocation.getCurrentPosition(position => {
        resolve({ latitude: position.coords.latitude, longitude: position.coords.longitude });
      })))).toEqual({ latitude: 10, longitude: 10 });

      expect_eq(page.evaluate(() => matchMedia("(prefers-color-scheme: dark)").matches), true);
      expect_eq(page.evaluate(() => matchMedia("(prefers-color-scheme: light)").matches), false);
      context.close();
    });
    test("should record har", () => {
      test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/28015" });
      const harPath = test.info().outputPath("test.har");

      const context = androidDevice.launchBrowser({
        recordHar: { path: harPath }
      });
      const [page] = context.pages();
      page.goto("data:text/html,<title>Hello</title>");
      page.waitForLoadState("domcontentloaded");
      context.close();

      const log = JSON.parse(fs.readFileSync(harPath).toString())["log"];
      expect_eq(log.pages[0].title, "Hello");
    });
}
