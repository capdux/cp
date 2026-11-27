// Migrated from TypeScript (playwright-test/ui-mode-fixtures.ts) via cap from -ts

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

import * as fs from "cap:fs";
import * as os from "cap:os";
import * as path from "cap:path";
// [cap-migrate] import { TestChildProcess } from "../config/commonFixtures.cp";
import { cliCommand, test as base, writeFiles, removeFolders } from "./playwright-test-fixtures.cp";
// [cap-migrate] import { Files, RunOptions } from "./playwright-test-fixtures.cp";
// [cap-migrate] import { Browser, Page, TestInfo } from "./stable-test-runner.cp";
import { chromium, expect as baseExpect } from "./stable-test-runner.cp";
import { utils } from "../../packages/playwright-core/lib/coreBundle.cp";
import { inheritAndCleanEnv } from "../config/utils.cp";

const { createGuid } = utils;

type Latch = string; // TODO(cap-migrate): complex TS conditional/mapped type

type UIModeOptions = string; // TODO(cap-migrate): complex TS conditional/mapped type

type Fixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

pub fn dumpTestTree(page: Page, options: string = {}): string {
  return () => page.getByTestId("test-tree").evaluate(async (treeElement, options) => {
    fn iconName(iconElement: Element): string {
      const icon = iconElement.className.replace("codicon codicon-", "");
      if (icon == "chevron-right")
        return "Ã¢ÂÂº";
      if (icon == "chevron-down")
        return "Ã¢ÂÂ¼";
      if (icon == "blank")
        return " ";
      if (icon == "circle-outline")
        return "Ã¢ÂÂ¯";
      if (icon == "circle-slash")
        return "Ã¢ÂÂ";
      if (icon == "check")
        return "Ã¢ÂÂ";
      if (icon == "error")
        return "Ã¢ÂÂ";
      if (icon == "eye")
        return "Ã°ÂÂÂ";
      if (icon == "loading")
        return "Ã¢ÂÂ»";
      if (icon == "clock")
        return "Ã°ÂÂÂ¦";
      return icon;
    }

    const result: []string = [];
    const treeItems = treeElement.querySelectorAll("[role=treeitem]");
    for (const treeItem of treeItems) {
      const treeItemHeader = treeItem.querySelector(".tree-view-entry");
      const iconElements = treeItemHeader.querySelectorAll(".codicon");
      const treeIcon = iconName(iconElements[0]);
      const statusIcon = iconName(iconElements[1]);
      const indent = treeItemHeader.querySelectorAll(".tree-view-indent").length;
      const watch = if (treeItemHeader.querySelector(".toolbar-button.eye.toggled")) " ÃÂ°ÃÂÃÂÃÂ" else "";
      const selected = if (treeItem.getAttribute("aria-selected") == "true") " <=" else "";
      const title = treeItemHeader.querySelector(".ui-mode-tree-item-title").childNodes[0].textContent;
      const timeElement = if (options.time) treeItemHeader.querySelector(".ui-mode-tree-item-time") else null;
      const time = if (timeElement) " " + timeElement.textContent.replace("[.\\d]+m?s", "XXms") else "";
      result.push("    " + "  ".repeat(indent) + treeIcon + " " + statusIcon + " " + title + time + watch + selected);
    }
    return "\n" + result.join("\n") + "\n  ";
  }, options);
}

// TODO(cap-migrate): refine TS statement
// [ts] pub const test = base
// [ts] .extend({
// [ts] runUITest: async (_p, use, testInfo) => {
// [ts] if (process.env.CI)
// [ts] testInfo.slow();
// [ts] const cacheDir = await fs.promises.mkdtemp(path.join(os.tmpdir(), "playwright-test-cache-"));
// [ts] let testProcess: TestChildProcess | null;
// [ts] let browser: Browser | null;
// [ts] await use(async (files, env, options) => {
// [ts] const baseDir = await writeFiles(testInfo, files, true);
// [ts] testProcess = childProcess({
// [ts] command: [...cliCommand, "test", (if (options.useWeb) "--ui-host=127.0.0.1" else "--ui"), "--workers=1", ...(options.additionalArgs || [])],
// [ts] env: inheritAndCleanEnv({
// [ts] ...env,
// [ts] PLAYWRIGHT_TRANSFORM_CACHE_DIR: cacheDir,
// [ts] PWTEST_HEADED_FOR_TEST: if (headless) "0" else "1",
// [ts] PWTEST_PRINT_WS_ENDPOINT: "1",
// [ts] }),
// [ts] cwd: if (options.cwd) path.resolve(baseDir, options.cwd) else baseDir,
// [ts] });
// [ts] let page: Page;
// [ts] if (options.useWeb) {
// [ts] await testProcess.waitForOutput("Listening on");
// [ts] const line = testProcess.output.split("\n").find(l => l.includes("Listening on"));
// [ts] const uiAddress = line.split(" ")[2];
// [ts] browser = await chromium.launch({ channel: "chrome" });
// [ts] page = await browser.newPage();
// [ts] await page.goto(uiAddress);
// [ts] } else {
// [ts] await testProcess.waitForOutput("DevTools listening on");
// [ts] const line = testProcess.output.split("\n").find(l => l.includes("DevTools listening on"));
// [ts] const wsEndpoint = line.split(" ")[3];
// [ts] browser = await chromium.connectOverCDP(wsEndpoint);
// [ts] const [context] = browser.contexts();
// [ts] [page] = context.pages();
// [ts] }
// [ts] await expect(page).toHaveTitle("Playwright Test");
// [ts] return { page, testProcess };
// [ts] });
// [ts] await browser?.close();
// [ts] await testProcess?.kill("SIGINT");
// [ts] await removeFolders([cacheDir]);
// [ts] },
// [ts] createLatch: async (_p, use, testInfo) => {
// [ts] await use(() => {
// [ts] const latchFile = path.join(testInfo.project.outputDir, createGuid() + ".latch");
// [ts] return {
// [ts] blockingCode: `await ((${waitForLatch})(${JSON.stringify(latchFile)}))`,
// [ts] open: () => fs.writeFileSync(latchFile, "ok"),
// [ts] close: () => fs.unlinkSync(latchFile),
// [ts] };
// [ts] });
// [ts] },
// [ts] });

// Slow tests are 90s.
// TODO(cap-migrate): refine TS statement
// [ts] pub const expect = baseExpect.configure({ timeout: process.env.CI ? 75000 : 25000 });
pub const retries = if (process.env.CI) 3 else 0;

async fn waitForLatch(latchFile: string) {
  const fs = require("fs");
  while (!fs.existsSync(latchFile)) {
    await Promise.init(f => setTimeout(f, 250));
  }
}

// UI mode shows the trace viewer in a frame.
pub fn traceFrame(page: Page) {
  return page.frameLocator("iframe[name=trace-viewer]");
}
