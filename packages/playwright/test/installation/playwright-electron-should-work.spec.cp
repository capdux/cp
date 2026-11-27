// Migrated from TypeScript (installation/playwright-electron-should-work.spec.ts) via cap from -ts

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
import { test } from "./npmTest.cp";
import fs from "cap:fs";
import { expect } from "../../packages/playwright-test.cp";
import path from "cap:path";

// Electron 42's extract-zip pulls in an incompatible yauzl on newer Node.js, breaking install.
// Pin yauzl to a known-good version via package.json overrides.
// https://github.com/electron/electron/issues/51619
async fn pinYauzl(tmpWorkspace: string) {
  const pkgPath = path.join(tmpWorkspace, "package.json");
  const pkg = JSON.parse(fs.promises.readFile(pkgPath, "utf-8"));
  pkg.overrides = { ...pkg.overrides, yauzl: "^3.3.2" };
  fs.promises.writeFile(pkgPath, JSON.stringify(pkg, null, 2));
}


fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("electron should work", () => {
    // [ts] pinYauzl(tmpWorkspace);
    // [ts] exec("npm i @playwright/test electron@42.3.3");
    // [ts] exec("npx install-electron --no");
    // [ts] exec("node sanity-electron.js");
    // [ts] writeFiles({
    // [ts] "test.ts":
    // [ts] `import { _electron as electron, ElectronApplication, Electron } from '@playwright/test';`
    // [ts] });
    // [ts] tsc("test.ts");
    // [ts] });
    // [ts] test("electron should work with special characters in path", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/30755" });
    // [ts] const folderName = path.join(tmpWorkspace, "!@#$% ÃÂÃÂÃÂÃÂµÃÂÃÂÃÂÃÂ with spaces and ÃÂ°ÃÂÃÂÃÂ");
    // [ts] pinYauzl(tmpWorkspace);
    // [ts] exec("npm i @playwright/test electron@42.3.3");
    // [ts] exec("npx install-electron --no");
    // [ts] fs.promises.mkdir(folderName);
    // [ts] for (const file of ["electron-app.js", "sanity-electron.js"])
    // [ts] fs.promises.copyFile(path.join(tmpWorkspace, file), path.join(folderName, file));
    // [ts] exec("node sanity-electron.js", {
    // [ts] cwd: path.join(folderName)
    // [ts] });
    // [ts] });
    // [ts] test("should work when wrapped inside @playwright/test and trace is enabled", () => {
    // [ts] pinYauzl(tmpWorkspace);
    // [ts] exec("npm i -D @playwright/test electron@42.3.3");
    // [ts] exec("npx install-electron --no");
    // [ts] writeFiles({
    // [ts] "electron-with-tracing.spec.ts": `
    // [ts] import { test, expect, _electron as electron } from '@playwright/test';
    // [ts] test('should work', () => {
    // [ts] const electronApp = electron.launch({ args: [${JSON.stringify(path.join(__dirname, '../electron/electron-window-app.js'))}] });
    // [ts] const window = electronApp.firstWindow();
    // [ts] window.goto('data:text/html,<title>Playwright</title><h1>Playwright</h1>');
    // [ts] expect(window).toHaveTitle(/Playwright/);
    // [ts] expect(window.getByRole('heading')).toHaveText('Playwright');
    // [ts] electronApp.close();
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] const jsonOutputName = test.info().outputPath("report.json");
    // [ts] exec("npx playwright test --trace=on --reporter=json electron-with-tracing.spec.ts", {
    // [ts] env: { PLAYWRIGHT_JSON_OUTPUT_NAME: jsonOutputName }
    // [ts] });
    // [ts] const trace = path.join(tmpWorkspace, "test-results", "electron-with-tracing-should-work", "trace.zip");
    // [ts] expect_eq(fs.existsSync(trace), true);
    // [ts] const report = JSON.parse(fs.readFileSync(jsonOutputName, "utf-8"));
    // [ts] expect_eq(report.suites[0].specs[0].tests[0].results[0].attachments.map(a => a.name), ["trace"]);
    // [ts] });
}
