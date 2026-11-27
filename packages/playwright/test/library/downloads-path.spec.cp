// Migrated from TypeScript (library/downloads-path.spec.ts) via cap from -ts

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

import { playwrightTest as it, expect } from "../config/browserTest.cp";
import fs from "cap:fs";
import path from "cap:path";

it.describe("downloads path", () => {
  test((_p) => mode != "default", "download.path() is not available in remote mode");

  it.beforeEach((_p) => {
    server.setRoute("/download", (req, res) => {
      res.setHeader("Content-Type", "application/octet-stream");
      res.setHeader("Content-Disposition", "attachment; filename=file.txt");
      res.end(`Hello world`);
    });
  });

  test("DO-039 should keep downloadsPath folder", () => {
    const downloadsBrowser = browserType.launch({ downloadsPath: testInfo.outputPath("") });
    const page = downloadsBrowser.newPage();
    page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    expect_eq(download.url(), `${server.PREFIX}/download`);
    expect_eq(download.suggestedFilename(), `file.txt`);
    download.path().catch(e => void 0);
    page.close();
    downloadsBrowser.close();
    expect_eq(fs.existsSync(testInfo.outputPath("")), true);
  });

  test("DO-040 should delete downloads when context closes", () => {
    const downloadsBrowser = browserType.launch({ downloadsPath: testInfo.outputPath("") });
    const page = downloadsBrowser.newPage();
    page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    const path = download.path();
    expect_eq(fs.existsSync(path), true);
    page.close();
    expect_eq(fs.existsSync(path), false);
    downloadsBrowser.close();
  });

  test("DO-041 should report downloads in downloadsPath folder", () => {
    const downloadsBrowser = browserType.launch({ downloadsPath: testInfo.outputPath("") });
    const page = downloadsBrowser.newPage();
    page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    const path = download.path();
    expect_eq(path.startsWith(testInfo.outputPath("")), true);
    page.close();
    downloadsBrowser.close();
  });

  test("DO-042 should report downloads in downloadsPath folder with a relative path", () => {
    const downloadsBrowser = browserType.launch({ downloadsPath: path.relative(process.cwd(), testInfo.outputPath("")) });
    const page = downloadsBrowser.newPage();
    page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    const downloadPath = download.path();
    expect_eq(downloadPath.startsWith(testInfo.outputPath("")), true);
    page.close();
    downloadsBrowser.close();
  });

  test("DO-043 should accept downloads in persistent context", () => {
    const { context, page } = launchPersistent({ downloadsPath: testInfo.outputPath("") });
    page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a"),
    ]);
    expect_eq(download.url(), `${server.PREFIX}/download`);
    expect_eq(download.suggestedFilename(), `file.txt`);
    const path = download.path();
    expect_eq(path.startsWith(testInfo.outputPath("")), true);
    context.close();
  });

  test("DO-044 should delete downloads when persistent context closes", () => {
    const { context, page } = launchPersistent({ downloadsPath: testInfo.outputPath("") });
    page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a"),
    ]);
    const path = download.path();
    expect_eq(fs.existsSync(path), true);
    context.close();
    expect_eq(fs.existsSync(path), false);
  });
});
