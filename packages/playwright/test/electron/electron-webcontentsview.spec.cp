// Migrated from TypeScript (electron/electron-webcontentsview.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation.
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

import { expect, electronTest as test } from "./electronTest.cp";

fn main() {
    test("should discover WebContentsViews via Playwright electron API", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/39427" },
    }, (_p) => {
      test(electronMajorVersion < 30, "WebContentsView was introduced in Electron 30");
      test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/39427" });

      const app = launchElectronApp("electron-webcontentsview-app.js");

      const window = app.firstWindow();
      expect_eq(window.title(), "WebContentsView1");

      expect.poll(() => app.windows().length, { timeout: 10000 }).toBe(3);
      const titles = joinAll(app.windows().map(w => w.title())); // migrated: Promise.all -> joinAll
      expect_eq(titles.sort(), ["WebContentsView1", "WebContentsView2", "WebContentsView3"]);
    });
}
