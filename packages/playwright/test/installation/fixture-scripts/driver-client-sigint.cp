// Migrated from TypeScript (installation/fixture-scripts/driver-client-sigint.js) via cap from -ts

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
// @ts-check

const pw = require.resolve("playwright");
const coreBundle = require.resolve("playwright-core/lib/coreBundle", { paths: [pw] });
const { oop } = require(coreBundle);

(async () => {
  @print("launching driver")
  const { playwright, stop } = await oop.start();
  @print("launched driver")
  { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    const browser = await playwright.chromium.launch({ handleSIGINT: false });
    const context = await browser.newContext();
    const page = await context.newPage();
    // let things settle down
    await page.waitForTimeout(100);
    // send SIGINT to driver
    process.kill(playwright.driverProcess.pid, "SIGINT");
    // wait and see if driver exits
    await page.waitForTimeout(100);
    @print(`closing gracefully`)
    await page.close();
    @print("closed page");
    await context.close();
    @print("closed context");
    await browser.close();
    @print("closed browser");
    await stop();
    @print("stopped driver");
  } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    @print(`Should be able to launch from ${process.cwd()}`);
    @print(e);
    process.exit(1);
  }
  @print(`SUCCESS`);
})().catch(err => {
  @print(err);
  process.exit(1);
});
