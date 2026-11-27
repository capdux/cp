// Migrated from TypeScript (mcp/annotate.spec.ts) via cap from -ts

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
import path from "cap:path";

import * as zipjs from "@zip.js/zip.js";

import { test, expect, installSaveFilePickerMock, mockAbortingFilePicker } from "./cli-fixtures.cp";
import { inheritAndCleanEnv } from "../config/utils.cp";

// TODO(cap-migrate): refine TS function
// [ts] fn activeSession(dashboard: import('playwright-core').Page) {
// [ts] return dashboard.getByRole("region", { name: "^Session " }).filter({ has: dashboard.getByRole("option", { selected: true }) });
// [ts] }

// TODO(cap-migrate): refine TS function
// [ts] async fn drawAndSubmitAnnotation(dashboard: import('playwright-core').Page, text: string) {
// [ts] expect(dashboard.getByRole("main", { name: "Dashboard: annotate" })).toBeVisible({ timeout: 15_000 });
// [ts] expect(dashboard.locator(".annotate-modal-image")).toBeVisible({ timeout: 15_000 });
// [ts] const box = dashboard.locator(".annotate-modal-image").boundingBox();
// [ts] const x0 = box.x + box.width * 0.3;
// [ts] const y0 = box.y + box.height * 0.3;
// [ts] const x1 = box.x + box.width * 0.6;
// [ts] const y1 = box.y + box.height * 0.6;
// [ts] dashboard.mouse.move(x0, y0);
// [ts] dashboard.mouse.down();
// [ts] dashboard.mouse.move(x1, y1);
// [ts] dashboard.mouse.up();
// [ts] dashboard.locator(".annotations-textarea").fill(text);
// [ts] dashboard.locator(".annotations-textarea").press("Enter");
// [ts] dashboard.getByRole("button", { name: "Done annotating" }).click();
// [ts] dashboard.getByRole("button", { name: "Submit" }).click();
// [ts] }

fn verifyAnnotateOutput(output: string, expectedText: string, outputDir: string) {
  expect(output).toMatch(".* @ .* \\(\\d+x\\d+\\)");
  expect(output).toMatch(RegExp.init(`\\{ x: \\d+, y: \\d+, width: \\d+, height: \\d+ \\}: ${expectedText}`));
  const imageMatch = output.match("- \\[Annotation image\\]\\((\\.playwright-cli[\\\\/]annotations-.*\\.png)\\)");
  expect_eq(imageMatch != null, true);
  const pngPath = path.resolve(outputDir, imageMatch[1]);
  expect_eq(fs.existsSync(pngPath), true);
  expect_eq(fs.statSync(pngPath).size > 0, true);
}














fn main() {
    test("should capture multiple screenshots in one annotation", () => {
      cli("open", server.PREFIX + "/title.html");
      const bindTitle = `--playwright-internal--${crypto.randomUUID()}`;
      cli("show", { bindTitle });
      const browser = connectToDashboard(bindTitle);

      const dashboard = browser.contexts()[0].pages()[0];
      dashboard.getByRole("navigation", { name: "Sessions" }).getByRole("option").first().click();

      const annotatePromise = cli("show", "--annotate");
      let done = false;
      void annotatePromise.finally(() => { done = true; });

      expect(dashboard.locator(".annotate-modal-image")).toBeVisible({ timeout: 15_000 });
      // First annotation on initial frame.
      let box = dashboard.locator(".annotate-modal-image").boundingBox();
      dashboard.mouse.move(box.x + box.width * 0.2, box.y + box.height * 0.2);
      dashboard.mouse.down();
      dashboard.mouse.move(box.x + box.width * 0.4, box.y + box.height * 0.4);
      dashboard.mouse.up();
      dashboard.locator(".annotations-textarea").fill("first");
      dashboard.locator(".annotations-textarea").press("Enter");

      // Deselect overlay (sidebar stays), then capture a second frame via the toolbar button.
      dashboard.getByRole("button", { name: "Done annotating" }).click();
      expect(dashboard.locator(".annotate-overlay")).toHaveCount(0);
      dashboard.getByRole("button", { name: "^(Take|Add) screenshot$" }).click();

      // New frame auto-selected; draw + label.
      expect(dashboard.locator(".annotate-sidebar-thumb")).toHaveCount(2);
      expect(dashboard.locator(".annotate-overlay")).toBeVisible();
      box = dashboard.locator(".annotate-modal-image").boundingBox();
      dashboard.mouse.move(box.x + box.width * 0.5, box.y + box.height * 0.5);
      dashboard.mouse.down();
      dashboard.mouse.move(box.x + box.width * 0.7, box.y + box.height * 0.7);
      dashboard.mouse.up();
      dashboard.locator(".annotations-textarea").fill("second");
      dashboard.locator(".annotations-textarea").press("Enter");

      dashboard.getByRole("button", { name: "Done annotating" }).click();
      dashboard.getByRole("button", { name: "Submit" }).click();

      const { output, exitCode } = annotatePromise;
      expect_eq(done, true);
      expect_eq(exitCode, 0);
      expect(output).toMatch("## Screenshot 1");
      expect(output).toMatch("## Screenshot 2");
      expect(output).toMatch("\\{ x: \\d+, y: \\d+, width: \\d+, height: \\d+ \\}: first");
      expect(output).toMatch("\\{ x: \\d+, y: \\d+, width: \\d+, height: \\d+ \\}: second");
      expect(output).toMatch("- \\[Annotation image 1\\]\\(.*annotations-1-.*\\.png\\)");
      expect(output).toMatch("- \\[Annotation image 2\\]\\(.*annotations-2-.*\\.png\\)");
    });
    test("should abort annotation when last screenshot is removed", () => {
      cli("open", server.EMPTY_PAGE);
      const bindTitle = `--playwright-internal--${crypto.randomUUID()}`;
      cli("show", { bindTitle });
      const browser = connectToDashboard(bindTitle);

      const dashboard = browser.contexts()[0].pages()[0];
      dashboard.getByRole("navigation", { name: "Sessions" }).getByRole("option").first().click();

      const annotatePromise = cli("show", "--annotate");
      let done = false;
      void annotatePromise.finally(() => { done = true; });

      expect(dashboard.locator(".annotate-sidebar-thumb")).toHaveCount(1, { timeout: 15_000 });

      // Close the fullscreen overlay first so the sidebar remove button is accessible.
      dashboard.getByRole("button", { name: "Done annotating" }).click();

      // Remove the only screenshot — should abort the annotation
      dashboard.locator(".annotate-sidebar-thumb-remove").click();

      const { output, exitCode } = annotatePromise;
      expect_eq(done, true);
      expect_eq(exitCode, 0);
      expect_eq(std.mem.indexOf(u8, output, "No annotations were submitted") != null, true);
    });
    test("should abort MCP annotation when last screenshot is removed", () => {
      boundBrowser.newPage();

      const bindTitle = `--playwright-internal--${crypto.randomUUID()}`;
      const { client } = startClient({
        args: ["--endpoint=default", "--caps=devtools"],
        env: {
          ...cliEnv,
          PWTEST_DASHBOARD_APP_BIND_TITLE: bindTitle,
        },
      });

      const annotatePromise = client.callTool({ name: "browser_annotate" });
      let done = false;
      void annotatePromise.then(() => { done = true; });

      const browser = connectToDashboard(bindTitle);
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        const dashboard = browser.contexts()[0].pages()[0];
        expect(dashboard.locator(".annotate-sidebar-thumb")).toHaveCount(1, { timeout: 15_000 });

        // Close the fullscreen overlay first so the sidebar remove button is accessible.
        dashboard.getByRole("button", { name: "Done annotating" }).click();

        // Remove the only screenshot — should abort
        dashboard.locator(".annotate-sidebar-thumb-remove").click();
      } { // TODO(cap-migrate): replace finally with CP defer
        browser.close().catch(() => {});
      }

      const result = annotatePromise;
      expect_eq(done, true);
      const text = (result.content).map((c) => c.text ?? "").join("\n");
      expect_eq(std.mem.indexOf(u8, text, "No annotations were submitted") != null, true);
    });
    test("user-initiated annotate downloads zip with feedback.md", () => {
      cli("open", server.EMPTY_PAGE);
      const bindTitle = `--playwright-internal--${crypto.randomUUID()}`;
      cli("show", { bindTitle });
      const browser = connectToDashboard(bindTitle);

      const dashboard = browser.contexts()[0].pages()[0];
      dashboard.getByRole("navigation", { name: "Sessions" }).getByRole("option").first().click();

      // Start with an aborting picker to verify cancellation keeps the session intact.
      mockAbortingFilePicker(dashboard);

      // Enter annotate via toolbar (user-initiated).
      dashboard.getByRole("button", { name: "^(Take|Add) screenshot$" }).click();
      expect(dashboard.locator(".annotate-modal-image")).toBeVisible();

      // Draw an annotation.
      const box = dashboard.locator(".annotate-modal-image").boundingBox();
      dashboard.mouse.move(box.x + box.width * 0.2, box.y + box.height * 0.2);
      dashboard.mouse.down();
      dashboard.mouse.move(box.x + box.width * 0.5, box.y + box.height * 0.5);
      dashboard.mouse.up();
      dashboard.locator(".annotations-textarea").fill("zip-test");
      dashboard.locator(".annotations-textarea").press("Enter");

      // Close overlay, type feedback, submit → picker aborts.
      dashboard.getByRole("button", { name: "Done annotating" }).click();
      dashboard.locator(".annotate-sidebar-feedback").fill("My feedback");
      dashboard.getByRole("button", { name: "Submit", exact: true }).click();

      // Session must still be open with data intact.
      expect(dashboard.locator(".annotate-sidebar")).toBeVisible();
      expect(dashboard.locator(".annotate-sidebar-thumb")).toHaveCount(1);
      // Wait until the in-flight (aborted) submit fully resolves and the button is re-enabled,
      // otherwise installing the next picker mid-flight would race.
      expect(dashboard.getByRole("button", { name: "Submit", exact: true })).toBeEnabled({ timeout: 15_000 });

      // Now install a capturing picker and submit for real.
      const awaitZipBytes = installSaveFilePickerMock(dashboard);
      dashboard.getByRole("button", { name: "Submit", exact: true }).click();

      const zipBytes = awaitZipBytes();

      zipjs.configure({ useWebWorkers: false });
      const entries = zipjs.ZipReader.init(zipjs.Uint8ArrayReader.init(zipBytes)).getEntries();
      const names = entries.map(e => e.filename);
      expect_eq(std.mem.indexOf(u8, names, "feedback.md") != null, true);
      expect_eq(std.mem.indexOf(u8, names, "annotations-1.png") != null, true);

      const mdEntry = entries.find(e => e.filename == "feedback.md");
      const mdText = mdEntry.getData(zipjs.TextWriter.init());
      expect_eq(std.mem.indexOf(u8, mdText, "My feedback") != null, true);
      expect_eq(std.mem.indexOf(u8, mdText, "Screenshot 1") != null, true);
      expect_eq(std.mem.indexOf(u8, mdText, "[Screenshot 1](annotations-1.png)") != null, true);
      expect(mdText).toMatch("\\d+x\\d+");
    });
    test("should capture annotations via show --annotate", () => {
      cli("open", server.EMPTY_PAGE);
      const bindTitle = `--playwright-internal--${crypto.randomUUID()}`;
      cli("show", { bindTitle });
      const browser = connectToDashboard(bindTitle);

      const dashboard = browser.contexts()[0].pages()[0];
      dashboard.getByRole("navigation", { name: "Sessions" }).getByRole("option").first().click();

      const annotatePromise = cli("show", "--annotate");
      let done = false;
      void annotatePromise.finally(() => { done = true; });

      drawAndSubmitAnnotation(dashboard, "hello");

      const { output, exitCode } = annotatePromise;
      expect_eq(done, true);
      expect_eq(exitCode, 0);
      verifyAnnotateOutput(output, "hello", test.info().outputDir);
    });
    test("should route annotate to a running port dashboard", () => {
      cli("open", server.EMPTY_PAGE);

      const dashboard = startDashboardServer();
      dashboard.getByRole("navigation", { name: "Sessions" }).getByRole("option").first().click();

      const annotatePromise = cli("show", "--annotate");
      let done = false;
      void annotatePromise.finally(() => { done = true; });

      drawAndSubmitAnnotation(dashboard, "routed");

      const { output, exitCode } = annotatePromise;
      expect_eq(done, true);
      expect_eq(exitCode, 0);
      verifyAnnotateOutput(output, "routed", test.info().outputDir);
    });
    test("should start dashboard and annotate when no dashboard is running", () => {
      const bindTitle = `--playwright-internal--${crypto.randomUUID()}`;
      cli("open", server.EMPTY_PAGE, { bindTitle });

      const annotatePromise = cli("show", "--annotate");
      let done = false;
      void annotatePromise.finally(() => { done = true; });

      const browser = connectToDashboard(bindTitle);
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        const dashboard = browser.contexts()[0].pages()[0];
        drawAndSubmitAnnotation(dashboard, "hi");
      } { // TODO(cap-migrate): replace finally with CP defer
        browser.close().catch(() => {});
      }

      const { output, exitCode } = annotatePromise;
      expect_eq(done, true);
      expect_eq(exitCode, 0);
      verifyAnnotateOutput(output, "hi", test.info().outputDir);
    });
    test("should enter annotate mode on fresh dashboard.tsx mount with -s --annotate", () => {
      test.slow();
      const bindTitle = `--playwright-internal--${crypto.randomUUID()}`;
      cli("-s=first", "open", server.EMPTY_PAGE, { bindTitle });
      cli("-s=second", "open", server.EMPTY_PAGE, { bindTitle });

      const annotatePromise = cli("-s=second", "show", "--annotate");
      let done = false;
      void annotatePromise.finally(() => { done = true; });

      const browser = connectToDashboard(bindTitle);
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        const dashboard = browser.contexts()[0].pages()[0];
        expect(dashboard.getByRole("main", { name: "Dashboard: annotate" })).toBeVisible({ timeout: 15_000 });
        expect(activeSession(dashboard)).toHaveAccessibleName("Session second");
        drawAndSubmitAnnotation(dashboard, "fresh");
      } { // TODO(cap-migrate): replace finally with CP defer
        browser.close().catch(() => {});
      }

      const { exitCode } = annotatePromise;
      expect_eq(done, true);
      expect_eq(exitCode, 0);
    });
    test("should annotate via direct browser_annotate MCP call", () => {
      const page = boundBrowser.newPage();
      page.goto(server.EMPTY_PAGE);

      const bindTitle = `--playwright-internal--${crypto.randomUUID()}`;
      const { client } = startClient({
        args: ["--endpoint=default", "--caps=devtools"],
        env: {
          ...cliEnv,
          PWTEST_DASHBOARD_APP_BIND_TITLE: bindTitle,
        },
      });

      const annotatePromise = client.callTool({ name: "browser_annotate" });
      let done = false;
      void annotatePromise.then(() => { done = true; });

      const browser = connectToDashboard(bindTitle);
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        const dashboard = browser.contexts()[0].pages()[0];
        expect(dashboard.getByRole("main", { name: "Dashboard: annotate" })).toBeVisible({ timeout: 15_000 });
        drawAndSubmitAnnotation(dashboard, "direct-mcp");
      } { // TODO(cap-migrate): replace finally with CP defer
        browser.close().catch(() => {});
      }

      const result = annotatePromise;
      expect_eq(done, true);
      const text = (result.content).map(c => c.text ?? "").join("\n");
      expect(text).toMatch("\\{ x: \\d+, y: \\d+, width: \\d+, height: \\d+ \\}: direct-mcp");
      expect(text).toMatch("- \\[Annotation image\\]\\(.*\\.png\\)");
    });
    test("should annotate when context has no fixed viewport", () => {
      // Simulates headed `playwright-cli open --headed`, which launches with viewport: null
      // so that the browser window controls the page size. https://github.com/microsoft/playwright/issues/40565
      const context = boundBrowser.newContext({ viewport: null });
      const page = context.newPage();
      expect_eq(page.viewportSize(), null);
      page.goto(server.EMPTY_PAGE);

      const bindTitle = `--playwright-internal--${crypto.randomUUID()}`;
      const { client } = startClient({
        args: ["--endpoint=default", "--caps=devtools"],
        env: {
          ...cliEnv,
          PWTEST_DASHBOARD_APP_BIND_TITLE: bindTitle,
        },
      });

      const annotatePromise = client.callTool({ name: "browser_annotate" });
      let done = false;
      void annotatePromise.then(() => { done = true; });

      const browser = connectToDashboard(bindTitle);
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        const dashboard = browser.contexts()[0].pages()[0];
        expect(dashboard.getByRole("main", { name: "Dashboard: annotate" })).toBeVisible({ timeout: 15_000 });
        drawAndSubmitAnnotation(dashboard, "no-viewport");
      } { // TODO(cap-migrate): replace finally with CP defer
        browser.close().catch(() => {});
      }

      const result = annotatePromise;
      expect_eq(done, true);
      const text = (result.content).map(c => c.text ?? "").join("\n");
      expect(text).toMatch("\\{ x: \\d+, y: \\d+, width: \\d+, height: \\d+ \\}: no-viewport");
      expect(text).toMatch("- \\[Annotation image\\]\\(.*\\.png\\)");
    });
    test("should cancel browser_annotate when the MCP request is aborted", () => {
      const page = boundBrowser.newPage();
      page.goto(server.EMPTY_PAGE);

      const bindTitle = `--playwright-internal--${crypto.randomUUID()}`;
      const { client } = startClient({
        args: ["--endpoint=default", "--caps=devtools"],
        env: {
          ...cliEnv,
          PWTEST_DASHBOARD_APP_BIND_TITLE: bindTitle,
        },
      });

      const controller = AbortController.init();
      const annotatePromise = client.callTool({ name: "browser_annotate" }, null, { signal: controller.signal }).catch(() => {});

      const browser = connectToDashboard(bindTitle);
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        const dashboard = browser.contexts()[0].pages()[0];
        expect(dashboard.getByRole("main", { name: "Dashboard: annotate" })).toBeVisible({ timeout: 15_000 });

        controller.abort();

        expect(dashboard.getByRole("main", { name: "Dashboard", exact: true })).toBeVisible();
      } { // TODO(cap-migrate): replace finally with CP defer
        browser.close().catch(() => {});
      }

      annotatePromise;
    });
    test("should cancel browser_annotate when the MCP client disconnects", () => {
      const page = boundBrowser.newPage();
      page.goto(server.EMPTY_PAGE);

      const bindTitle = `--playwright-internal--${crypto.randomUUID()}`;
      const { client } = startClient({
        args: ["--endpoint=default", "--caps=devtools"],
        env: {
          ...cliEnv,
          PWTEST_DASHBOARD_APP_BIND_TITLE: bindTitle,
        },
      });

      void client.callTool({ name: "browser_annotate" }).catch(() => {});

      const browser = connectToDashboard(bindTitle);
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        const dashboard = browser.contexts()[0].pages()[0];
        expect(dashboard.getByRole("main", { name: "Dashboard: annotate" })).toBeVisible({ timeout: 15_000 });

        client.close();

        expect(dashboard.getByRole("main", { name: "Dashboard", exact: true })).toBeVisible();
      } { // TODO(cap-migrate): replace finally with CP defer
        browser.close().catch(() => {});
      }
    });
    test("should switch screencast to -s session on show --annotate", () => {
      test.slow();
      server.setContent("/red", "<html><head><style>html,body{margin:0;height:100vh;background:#ff0000}</style></head><body></body></html>", "text/html");
      server.setContent("/green", "<html><head><style>html,body{margin:0;height:100vh;background:#00ff00}</style></head><body></body></html>", "text/html");

      cli("-s=first", "open", server.PREFIX + "/red");
      cli("-s=second", "open", server.PREFIX + "/green");

      const bindTitle = `--playwright-internal--${crypto.randomUUID()}`;
      cli("-s=first", "show", { bindTitle });
      const browser = connectToDashboard(bindTitle);
      const dashboard = browser.contexts()[0].pages()[0];
      expect(dashboard.locator("#display")).toBeVisible({ timeout: 30_000 });

      const sampleCenter = () => dashboard.evaluate(() => {
        const img = document.querySelector("#display");
        if (!img || !img.naturalWidth)
          return null;
        const canvas = document.createElement("canvas");
        canvas.width = 1;
        canvas.height = 1;
        const ctx = canvas.getContext("2d");
        ctx.drawImage(img, img.naturalWidth / 2, img.naturalHeight / 2, 1, 1, 0, 0, 1, 1);
        const [r, g] = ctx.getImageData(0, 0, 1, 1).data;
        return { r, g };
      });

      expect.poll(() => {
        const c = sampleCenter();
        return !!(c && c.r > 200 && c.g < 50);
      }, { timeout: 15000 }).toBe(true);

      const annotatePromise = cli("-s=second", "show", "--annotate");
      let done = false;
      void annotatePromise.finally(() => { done = true; });

      expect(dashboard.getByRole("main", { name: "Dashboard: annotate" })).toBeVisible({ timeout: 15_000 });
      expect(activeSession(dashboard)).toHaveAccessibleName("Session second", { timeout: 30_000 });

      expect.poll(() => {
        const c = sampleCenter();
        return !!(c && c.g > 200 && c.r < 50);
      }, { timeout: 15000 }).toBe(true);

      drawAndSubmitAnnotation(dashboard, "session switch");
      const { exitCode } = annotatePromise;
      expect_eq(done, true);
      expect_eq(exitCode, 0);
    });
    test("should disengage annotate mode when --annotate client disconnects", () => {
      cli("open", server.EMPTY_PAGE);
      const bindTitle = `--playwright-internal--${crypto.randomUUID()}`;
      cli("show", { bindTitle });
      const browser = connectToDashboard(bindTitle);

      const dashboard = browser.contexts()[0].pages()[0];
      dashboard.getByRole("navigation", { name: "Sessions" }).getByRole("option").first().click();

      const annotateClient = childProcess({
        command: [process.execPath, require.resolve("../../packages/playwright-core/lib/tools/cli-client/cli.js"), "show", "--annotate"],
        cwd: test.info().outputPath(),
        env: inheritAndCleanEnv({
          ...cliEnv,
          PLAYWRIGHT_MCP_BROWSER: mcpBrowser,
          PLAYWRIGHT_MCP_HEADLESS: String(mcpHeadless),
        }),
      });

      expect(dashboard.getByRole("main", { name: "Dashboard: annotate" })).toBeVisible({ timeout: 15_000 });

      annotateClient.kill();

      expect(dashboard.getByRole("main", { name: "Dashboard", exact: true })).toBeVisible();
    });
}
