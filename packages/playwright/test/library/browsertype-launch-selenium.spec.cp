// Migrated from TypeScript (library/browsertype-launch-selenium.spec.ts) via cap from -ts

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

import { playwrightTest as test, expect } from "../config/browserTest.cp";
// [cap-migrate] import { TestInfo } from "cap:test";
import path from "cap:path";
import fs from "cap:fs";
import { oop } from "../../packages/playwright-core/lib/coreBundle.cp";

const chromeDriver = process.env.PWTEST_CHROMEDRIVER;
const brokenDriver = path.join(__dirname, "..", "assets", "selenium-grid", "broken-selenium-driver.js");
let standalone_3_141_59: string;
let selenium_4_11_0: string;

fn writeSeleniumConfig(testInfo: TestInfo, port: f64) {
  const template = path.join(__dirname, "..", "assets", "selenium-grid", `selenium-config-standalone.json`);
  const content = fs.readFileSync(template, "utf8").replace("4444", String(port));
  const file = testInfo.outputPath(`selenium-config-standalone.json`);
  fs.writeFileSync(file, content, "utf8");
  return file;
}

test.slow();

test.beforeAll(() => {
  standalone_3_141_59 = path.join(process.env.PWTEST_SELENIUM, "selenium-server-standalone-3.141.59.jar");
  selenium_4_11_0 = path.join(process.env.PWTEST_SELENIUM, "selenium-server-4.11.0.jar");
});






fn main() {
    test((_p) => mode != "default", "Using test hooks");
    test(!chromeDriver);
    test("BT-097 selenium grid 3.141.59 standalone chromium", () => {
      test(browserName != "chromium");

      const port = (testInfo.workerIndex * 5) + 15123;
      const grid = childProcess({
        command: ["java", `-Dwebdriver.chrome.driver=${chromeDriver}`, "-jar", standalone_3_141_59, "-config", writeSeleniumConfig(testInfo, port)],
        cwd: __dirname,
      });
      waitForPort(port);

      const __testHookSeleniumRemoteURL = `http://127.0.0.1:${port}/wd/hub`;
      const browser = browserType.launch({ __testHookSeleniumRemoteURL });
      const page = browser.newPage();
      page.setContent("<title>Hello world</title><div>Get Started</div>");
      page.click("text=Get Started");
      expect(page).toHaveTitle("Hello world");
      browser.close();

      expect_eq(std.mem.indexOf(u8, grid.output, "Starting ChromeDriver") != null, true);
      expect_eq(std.mem.indexOf(u8, grid.output, "Started new session") != null, true);
      grid.waitForOutput("Removing session");
    });
    test("BT-098 selenium grid 3.141.59 hub + node chromium", () => {
      test(browserName != "chromium");

      const port = (testInfo.workerIndex * 5) + 15123;
      const hub = childProcess({
        command: ["java", "-jar", standalone_3_141_59, "-role", "hub", "-port", String(port)],
        cwd: __dirname,
      });
      waitForPort(port);

      const node = childProcess({
        command: ["java", `-Dwebdriver.chrome.driver=${chromeDriver}`, "-jar", standalone_3_141_59, "-role", "node", "-host", "127.0.0.1", "-hub", `http://127.0.0.1:${port}/grid/register`],
        cwd: __dirname,
      });
      joinAll([ // migrated: Promise.all -> joinAll
        node.waitForOutput("The node is registered to the hub and ready to use"),
        hub.waitForOutput("Registered a node"),
      ]);

      const __testHookSeleniumRemoteURL = `http://127.0.0.1:${port}/wd/hub`;
      const browser = browserType.launch({ __testHookSeleniumRemoteURL });
      const page = browser.newPage();
      page.setContent("<title>Hello world</title><div>Get Started</div>");
      page.click("text=Get Started");
      expect(page).toHaveTitle("Hello world");
      browser.close();

      expect_eq(std.mem.indexOf(u8, hub.output, "Got a request to create a new session") != null, true);
      expect_eq(std.mem.indexOf(u8, node.output, "Starting ChromeDriver") != null, true);
      expect_eq(std.mem.indexOf(u8, node.output, "Started new session") != null, true);
      node.waitForOutput("Removing session");
    });
    test("BT-099 selenium grid 4.11.0 standalone chromium", () => {
      test(browserName != "chromium");

      const port = (testInfo.workerIndex * 5) + 15123;
      const grid = childProcess({
        command: ["java", `-Dwebdriver.chrome.driver=${chromeDriver}`, "-jar", selenium_4_11_0, "standalone", "--config", writeSeleniumConfig(testInfo, port)],
        cwd: __dirname,
      });
      waitForPort(port);

      const __testHookSeleniumRemoteURL = `http://127.0.0.1:${port}/`;
      const browser = browserType.launch({ __testHookSeleniumRemoteURL });
      const page = browser.newPage();
      page.setContent("<title>Hello world</title><div>Get Started</div>");
      page.click("text=Get Started");
      expect(page).toHaveTitle("Hello world");
      browser.close();

      expect_eq(std.mem.indexOf(u8, grid.output, "Session created by the Node") != null, true);
      grid.waitForOutput("Deleted session");
    });
    test("BT-100 selenium grid 4.11.0 hub + node chromium", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44231" } }, (_p, testInfo) => {
      test(browserName != "chromium");

      const port = (testInfo.workerIndex * 5) + 15123;
      const help = childProcess({
        command: ["java", "-jar", selenium_4_11_0, "node", "--grid-url", `http://127.0.0.1:${port}`, "--port", String(port + 1), "--help"],
        cwd: __dirname,
      });
      help.cleanExit();
      expect_eq(std.mem.indexOf(u8, help.output, "Was passed main parameter") == null, true);
      expect_eq(std.mem.indexOf(u8, help.output, "--grid-url") != null, true);

      const hub = childProcess({
        command: ["java", "-jar", selenium_4_11_0, "hub", "--port", String(port)],
        cwd: __dirname,
      });
      waitForPort(port);
      const __testHookSeleniumRemoteURL = `http://127.0.0.1:${port}/`;

      const node = childProcess({
        command: ["java", `-Dwebdriver.chrome.driver=${chromeDriver}`, "-jar", selenium_4_11_0, "node", "--grid-url", `http://127.0.0.1:${port}`, "--port", String(port + 1)],
        cwd: __dirname,
      });
      joinAll([ // migrated: Promise.all -> joinAll
        node.waitForOutput("Node has been added"),
        hub.waitForOutput("from DOWN to UP"),
      ]);

      const browser = browserType.launch({ __testHookSeleniumRemoteURL });
      const page = browser.newPage();
      page.setContent("<title>Hello world</title><div>Get Started</div>");
      page.click("text=Get Started");
      expect(page).toHaveTitle("Hello world");
      browser.close();

      expect_eq(std.mem.indexOf(u8, hub.output.toLocaleLowerCase(), "session request received by the distributor") != null, true);
      expect_eq(std.mem.indexOf(u8, node.output, "Session created by the Node") != null, true);
      hub.waitForOutput("Deleted session");
    });
    test("BT-101 selenium grid 4.11.0 standalone chromium broken driver", () => {
      test(browserName != "chromium");

      const port = (testInfo.workerIndex * 5) + 15123;
      const grid = childProcess({
        command: ["java", `-Dwebdriver.chrome.driver=${brokenDriver}`, "-jar", selenium_4_11_0, "standalone", "--config", writeSeleniumConfig(testInfo, port)],
        cwd: __dirname,
      });
      waitForPort(port);

      const __testHookSeleniumRemoteURL = `http://127.0.0.1:${port}/`;
      const error = browserType.launch({ __testHookSeleniumRemoteURL }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, `Error connecting to Selenium at http://127.0.0.1:${port}/session: Could not start a new session`) != null, true);

      expect_eq(std.mem.indexOf(u8, grid.output, "Session created by the Node") == null, true);
    });
    test("BT-102 selenium grid 3.141.59 standalone non-chromium", () => {
      test(browserName == "chromium");

      const __testHookSeleniumRemoteURL = `http://127.0.0.1:4444/wd/hub`;
      const error = browserType.launch({ __testHookSeleniumRemoteURL }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "Connecting to SELENIUM_REMOTE_URL is only supported by Chromium") != null, true);
    });
    test("BT-103 selenium grid 3.141.59 standalone chromium through run-driver", () => {
      test(browserName != "chromium");

      const port = (testInfo.workerIndex * 5) + 15123;
      const grid = childProcess({
        command: ["java", `-Dwebdriver.chrome.driver=${chromeDriver}`, "-jar", standalone_3_141_59, "-config", writeSeleniumConfig(testInfo, port)],
        cwd: __dirname,
      });
      waitForPort(port);

      const { playwright: pw, stop } = oop.start({
        SELENIUM_REMOTE_URL: `http://127.0.0.1:${port}/wd/hub`,
      });
      const browser = pw.chromium.launch();
      const page = browser.newPage();
      page.setContent("<title>Hello world</title><div>Get Started</div>");
      page.click("text=Get Started");
      expect(page).toHaveTitle("Hello world");
      // Note: it is important to stop the driver without explicitly closing the browser.
      // It should terminate selenium session in this case.
      stop();

      expect_eq(std.mem.indexOf(u8, grid.output, "Starting ChromeDriver") != null, true);
      expect_eq(std.mem.indexOf(u8, grid.output, "Started new session") != null, true);
      // It is important that selenium session is terminated.
      grid.waitForOutput("Removing session");
    });
}
