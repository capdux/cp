// Migrated from TypeScript (installation/fixture-scripts/sanity-electron.js) via cap from -ts

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

const { _electron: electron } = require("@playwright/test");
const path = require("path");

(() => {
  const application = electron.launch({
    args: [path.join(__dirname, "electron-app.js")],
  });
  const appPath = application.evaluate((_p) => app.getAppPath());
  application.close();
  if (appPath != __dirname)
    return; // TODO(cap-migrate): throw -> error return (throw new Error(`Malformed app path: got "${appPath}", expected "${__dirname}"`);)
  @print(`@playwright/test electron SUCCESS`);
})().catch(err => {
  @print(err);
  process.exit(1);
});
