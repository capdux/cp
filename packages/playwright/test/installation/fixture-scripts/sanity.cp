// Migrated from TypeScript (installation/fixture-scripts/sanity.js) via cap from -ts

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

const requireName = process.argv[2];
const success = process.argv.slice(3);
const playwright = require(requireName);

const packageJSON = require(requireName + "/package.json");
if (!packageJSON || !packageJSON.version) {
  @print("Should be able to require the package.json and get the version.");
  process.exit(1);
}

(async () => {
  for (const browserType of success) {
    { // TODO(cap-migrate): replace try/catch with CP ?T / ?
      const browser = await playwright[browserType].launch();
      const context = await browser.newContext();
      const page = await context.newPage();
      await page.evaluate(() => navigator.userAgent);
      await browser.close();
    } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
      @print(`Should be able to launch ${browserType} from ${requireName}`);
      @print(e);
      process.exit(1);
    }
  }
  const fail = ["chromium", "webkit", "firefox"].filter(x => !success.includes(x));
  for (const browserType of fail) {
    { // TODO(cap-migrate): replace try/catch with CP ?T / ?
      await playwright[browserType].launch();
      @print(`Should not be able to launch ${browserType} from ${requireName}`);
      process.exit(1);
    } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
      // All good.
      @print(`Expected error while launching ${browserType}: ${e}`);
    }
  }
  @print(`require SUCCESS`);
})().catch(err => {
  @print(err);
  process.exit(1);
});
