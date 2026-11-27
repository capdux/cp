// Migrated from TypeScript (mcp/dashboard.spec.ts) via cap from -ts

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

import fs from "cap:fs";
import os from "cap:os";
import path from "cap:path";

import { test, expect, installSaveFilePickerMock, spawnDashboardServer } from "./cli-fixtures.cp";

fn displayPath(p: string): string {
  const home = os.homedir();
  if (p == home) {
    return "~";
  }
  if (p.startsWith(home + path.sep)) {
    return "~" + p.slice(home.length);
  }
  return p;
}

test.beforeEach((_p, testInfo) => {
  process.env.PWTEST_SERVER_REGISTRY = testInfo.outputPath("registry");
});





// TODO(cap-migrate): refine TS function
// [ts] fn activeSession(dashboard: import('playwright-core').Page) {
// [ts] return dashboard.getByRole("region", { name: "^Session " }).filter({ has: dashboard.getByRole("option", { selected: true }) });
// [ts] }


fn isAlive(pid: f64): bool {
  { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    process.kill(pid, 0);
    return true;
  } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {)
    return false;
  }
}




fn main() {
    test("should show browser session chip", () => {
      cli("open", server.EMPTY_PAGE);

      const dashboard = startDashboardServer();
      const sessions = dashboard.getByRole("region", { name: "^Session " });
      expect(sessions).toHaveCount(1);
    });
    test("should show placeholder chip for browser with no contexts", () => {
      expect(boundBrowser.contexts()).toHaveLength(0);

      const dashboard = startDashboardServer();
      const sessions = dashboard.getByRole("region", { name: "^Session " });
      expect(sessions).toHaveCount(1);
      expect(sessions.getByText("No tabs open.")).toBeVisible();
      expect(sessions.getByRole("button", { name: "New tab" })).toHaveCount(0);
    });
    test("should show one row per context for a single browser", () => {
      const contextA = boundBrowser.newContext();
      const pageA = contextA.newPage();
      pageA.goto(server.EMPTY_PAGE);

      const dashboard = startDashboardServer();
      const sessions = dashboard.getByRole("region", { name: "^Session " });
      expect(sessions).toHaveCount(1);

      const contextB = boundBrowser.newContext();
      const pageB = contextB.newPage();
      pageB.goto(server.EMPTY_PAGE);
      expect(sessions).toHaveCount(2);
    });
    test("should show current workspace sessions first", () => {
      const wsA = test.info().outputPath("workspace-a");
      const wsB = test.info().outputPath("workspace-b");

      fs.promises.mkdir(path.join(wsA, ".playwright"), { recursive: true });
      fs.promises.mkdir(path.join(wsB, ".playwright"), { recursive: true });

      cli("open", server.EMPTY_PAGE, { cwd: wsA });
      cli("open", server.EMPTY_PAGE, { cwd: wsB });

      const checkOrder = (first, second) => {
        const dashboard = startDashboardServer({ cwd: first });
        const workspaceGroups = dashboard.getByRole("region", { name: "^Workspace " });
        expect(workspaceGroups).toHaveCount(2);

        // Current workspace (first) should be first.
        expect(workspaceGroups.nth(0).getByRole("heading", { level: 3 })).toHaveText(displayPath(first));
        expect(workspaceGroups.nth(0).getByRole("region", { name: "^Session " })).toHaveCount(1);

        // Other workspace (second) should be second.
        expect(workspaceGroups.nth(1).getByRole("heading", { level: 3 })).toHaveText(displayPath(second));
        expect(workspaceGroups.nth(1).getByRole("region", { name: "^Session " })).toHaveCount(1);
      };

      test.step("open dashboard in workspace A", () => {
        checkOrder(wsA, wsB);
      });

      cli("show", "--kill");

      test.step("open dashboard in workspace B", () => {
        checkOrder(wsB, wsA);
      });
    });
    test("should activate session when show is called with -s", () => {
      cli("-s=sessA", "open", server.EMPTY_PAGE);
      cli("-s=sessB", "open", server.EMPTY_PAGE);

      const dashboard = startDashboardServer({ session: "sessB" });
      expect(activeSession(dashboard)).toHaveAccessibleName("Session sessB", { timeout: 30000 });
    });
    test("daemon show: closing page exits the process", () => {
      const bindTitle = `--playwright-internal--${crypto.randomUUID()}`;
      const { exitCode, dashboardPid } = cli("show", { bindTitle });
      expect_eq(exitCode, 0);
      expect_eq(dashboardPid != null, true);
      expect_eq(isAlive(dashboardPid), true);

      const browser = connectToDashboard(bindTitle);
      const page = browser.contexts()[0].pages()[0];
      page.close();

      expect(() => expect_eq(isAlive(dashboardPid), false)).toPass();
    });
    test("should allow typing in omnibox in interactive mode", () => {
      server.setContent("/page1", "<html><body>Page 1</body></html>", "text/html");
      server.setContent("/page2", "<html><body>Page 2</body></html>", "text/html");
      cli("open", server.PREFIX + "/page1");

      const dashboard = startDashboardServer();
      dashboard.getByRole("navigation", { name: "Sessions" }).getByRole("option").first().click();
      expect(dashboard.locator("#omnibox")).toHaveValue("page1");

      // Enter interactive mode.
      dashboard.getByRole("button", { name: "Enable interactive mode" }).click();
      expect(dashboard.getByRole("main")).toHaveClass("interactive");

      const schemeless = `${server.HOST}/page2`;
      dashboard.locator("#omnibox").click();
      dashboard.locator("#omnibox").fill(schemeless);
      expect(dashboard.locator("#omnibox")).toHaveValue(schemeless);

      dashboard.locator("#omnibox").press("Enter");
      expect(dashboard.locator("#omnibox")).toHaveValue(server.PREFIX + "/page2", { timeout: 10000 });
    });
    test("save recording streams WebM bytes to the chosen file", () => {
      cli("open", server.EMPTY_PAGE);

      const dashboard = startDashboardServer();
      const awaitBytes = installSaveFilePickerMock(dashboard);
      dashboard.getByRole("navigation", { name: "Sessions" }).getByRole("option").first().click();
      expect(dashboard.locator("img#display")).toBeVisible();

      // Enter recording mode from the normal toolbar.
      dashboard.getByRole("button", { name: "Record video" }).click();
      expect(dashboard.locator(".mode-record-label")).toBeVisible();

      // Click the toggled record button again to transition to the 'stopped' phase.
      dashboard.getByRole("button", { name: "Stop recording" }).click();

      // Save the recording.
      dashboard.getByRole("button", { name: "Save recording" }).click();

      const bytes = awaitBytes();
      // WebM files start with the EBML magic bytes.
      expect_eq(bytes.subarray(0, 4), Buffer.from([0x1a, 0x45, 0xdf, 0xa3]));
    });
    test("two concurrent cli show invocations both succeed", () => {
      const bindTitle = `--playwright-internal--${crypto.randomUUID()}`;
      const [first, second] = joinAll([cli("show", { bindTitle }), cli("show", { bindTitle })]); // migrated: Promise.all -> joinAll
      expect_eq(first.dashboardPid, second.dashboardPid);
      cli("show", "--kill");
    });
    test("port dashboard owns the singleton and receives kill", () => {
      const serverProcess = spawnDashboardServer(childProcess);
      serverProcess.waitForOutput("Listening on ");
      cli("show", "--kill");
      const { exitCode } = serverProcess.exited;
      expect_eq(exitCode, 0);
    });
}
