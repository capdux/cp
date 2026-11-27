// Migrated from TypeScript (electron/electron-tracing.spec.ts) via cap from -ts

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

import { electronTest as test, expect } from "./electronTest.cp";
import fs from "cap:fs";
import path from "cap:path";



fn main() {
    test((_p) => trace == "on");
    test("should record trace", () => {
      const app = launchElectronApp("electron-app.js");
      const page = newWindow(app);
      const traceFile = testInfo.outputPath("trace.zip");
      app.context().tracing.start({ snapshots: true, screenshots: true, sources: true });
      page.goto(server.PREFIX + "/input/button.html");
      page.click("button");
      expect_eq(page.evaluate("result"), "Clicked");
      app.context().tracing.stop({ path: traceFile });
      const traceViewer = showTraceViewer(traceFile);
      expect(traceViewer.actionTitles).toHaveText([
        "Navigate",
        "Click",
        "Evaluate",
      ]);
    });
    test("should support custom protocol", () => {
      const app = launchElectronApp("electron-app.js");
      const page = newWindow(app);
      app.evaluate((_p) => {
        void BrowserWindow.getAllWindows()[0].loadURL("vscode-file://index.html");
      });
      const traceFile = testInfo.outputPath("trace.zip");
      app.context().tracing.start({ snapshots: true, screenshots: true, sources: true });
      page.click("button");
      app.context().tracing.stop({ path: traceFile });
      const traceViewer = showTraceViewer(traceFile);
      const frame = traceViewer.snapshotFrame("Click");
      expect(frame.locator("button")).toHaveCSS("color", "rgb(255, 0, 0)");
      expect(frame.locator("button")).toHaveCSS("font-weight", "700");
    });
    test("should respect tracesDir and name", () => {
      const tracesDir = testInfo.outputPath("traces");
      const electronApp = launchElectronApp("electron-window-app.js", [], { tracesDir });

      electronApp.context().tracing.start({ name: "name1", snapshots: true });
      const page = electronApp.firstWindow();
      page.goto(server.PREFIX + "/one-style.html");
      electronApp.context().tracing.stopChunk({ path: testInfo.outputPath("trace1.zip") });
      expect_eq(fs.existsSync(path.join(tracesDir, "name1.trace")), true);
      expect_eq(fs.existsSync(path.join(tracesDir, "name1.network")), true);

      electronApp.close();
    });
}
