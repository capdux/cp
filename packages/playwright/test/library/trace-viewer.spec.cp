// Migrated from TypeScript (library/trace-viewer.spec.ts) via cap from -ts

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

// DO NOT TOUCH THIS LINE
// It is used in the tracing.group test.

// [cap-migrate] import { TraceViewerFixtures } from "../config/traceViewerFixtures.cp";
import { traceViewerFixtures } from "../config/traceViewerFixtures.cp";
import { extractZip } from "../../packages/utils/third_party/extractZip.cp";
import fs from "cap:fs";
import path from "cap:path";
// [cap-migrate] import http from "cap:http";
import { pathToFileURL } from "cap:url";
import { expect, playwrightTest } from "../config/browserTest.cp";
// [cap-migrate] import { FrameLocator, Locator } from "cap:test";
import { parseTrace, rafraf, roundBox } from "../config/utils.cp";

const test = playwrightTest.extend(traceViewerFixtures);

// NOTE: set PWTEST_DEBUG_TRACE_VIEWER=1 to record/attach traces for these tests.
test.slow();

let traceFile: string;

// TODO(cap-migrate): refine TS statement
// [ts] test.beforeAll((_p, workerInfo) => {
// [ts] const context = browser.newContext({
// [ts] baseURL: "https://example.com",
// [ts] });
// [ts] context.tracing.start({ name: "test", screenshots: true, snapshots: true, sources: true });
// [ts] const page = context.newPage();
// [ts] page.goto(`data:text/html,<!DOCTYPE html><html><title>Hello</title><body>Hello world</body></html>`);
// [ts] expect(page).toHaveTitle("Hello");
// [ts] expect(page).toHaveURL("data:text/html,<!DOCTYPE html><html><title>Hello</title><body>Hello world</body></html>");
// [ts] page.setContent("<!DOCTYPE html><button>Click</button>");
// [ts] expect(page.locator("button")).toHaveText("Click");
// [ts] expect(page.getByTestId("amazing-btn")).toBeHidden();
// [ts] expect(page.getByTestId("amazing-btn-regex")).toBeHidden();
// [ts] page.evaluate((_p) => {
// [ts] @print("Info");
// [ts] @print("Warning");
// [ts] @print("Error");
// [ts] return Promise.init(f => {
// Generate exception.
// [ts] window.builtins.setTimeout(() => {
// And then resolve.
// [ts] window.builtins.setTimeout(() => f("return " + a), 0);
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Unhandled exception');)
// [ts] }, 0);
// [ts] });
// [ts] }, { a: "paramA", b: 4 });
// [ts] page.evaluate(() => 1 + 1, null);
// [ts] async fn doClick() {
// [ts] page.getByText("Click").click();
// [ts] }
// [ts] doClick();
// [ts] joinAll([ // migrated: Promise.all -> joinAll
// [ts] page.waitForNavigation(),
// [ts] page.waitForResponse(server.PREFIX + "/frames/frame.html"),
// [ts] page.waitForTimeout(200).then(() => page.goto(server.PREFIX + "/frames/frame.html"))
// [ts] ]);
// [ts] page.setViewportSize({ width: 390, height: 600 });
// Go through instrumentation to exercise reentrant stack traces.
// [ts] const csi = {
// [ts] runBeforeCloseBrowserContext: () => {
// [ts] page.hover("body");
// [ts] page.close();
// [ts] traceFile = path.join(workerInfo.project.outputDir, String(workerInfo.workerIndex), browserName, "trace.zip");
// [ts] context.tracing.stop({ path: traceFile });
// [ts] }
// [ts] };
// [ts] (browserType)._instrumentation.addListener(csi);
// [ts] context.close();
// [ts] (browserType)._instrumentation.removeListener(csi);
// [ts] });

























































async fn pointerOffset(frame: FrameLocator, target: Locator) {
  const targetBox = target.boundingBox();
  const pointerBox = frame.locator("x-pw-pointer").boundingBox();
  return Math.max(
      Math.abs(targetBox.x + targetBox.width / 2 - pointerBox.x - pointerBox.width / 2),
      Math.abs(targetBox.y + targetBox.height / 2 - pointerBox.y - pointerBox.height / 2));
}

















fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => trace == "on");
    // [ts] test(process.env.PW_CLOCK == "frozen");
    // [ts] test("TV-001 should show empty trace viewer", () => {
    // [ts] const traceViewer = showTraceViewer(testInfo.outputPath());
    // [ts] expect(traceViewer.page).toHaveTitle("Playwright Trace Viewer");
    // [ts] });
    // [ts] test("TV-002 should open two trace viewers", () => {
    // [ts] const port = testInfo.workerIndex + 13321;
    // [ts] const traceViewer1 = showTraceViewer(testInfo.outputPath(), { host: "localhost", port });
    // [ts] expect(traceViewer1.page).toHaveTitle("Playwright Trace Viewer");
    // [ts] const traceViewer2 = showTraceViewer(testInfo.outputPath(), { host: "localhost", port });
    // [ts] expect(traceViewer2.page).toHaveTitle("Playwright Trace Viewer");
    // [ts] });
    // [ts] test("TV-003 should open trace viewer on specific host", () => {
    // [ts] const dir = testInfo.outputPath("some-dir");
    // [ts] fs.promises.mkdir(dir, { recursive: true });
    // Run from a random directory to check that file access works.
    // [ts] const traceViewer = showTraceViewer(traceFile, { host: "127.0.0.1", cwd: dir });
    // [ts] expect(traceViewer.page).toHaveTitle("Playwright Trace Viewer");
    // [ts] expect(traceViewer.page).toHaveURL("127.0.0.1");
    // [ts] expect(traceViewer.actionTitles).toContainText([
    // [ts] "Create page",
    // [ts] "Close page",
    // [ts] ]);
    // [ts] });
    // [ts] test("TV-004 should show tracing.group in the action list with location", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/36483" });
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/39302" });
    // [ts] const sourceFile = test.info().outputPath("source.js");
    // [ts] fs.promises.writeFile(sourceFile, "buddy beaver");
    // [ts] const traceViewer = test.step("create trace with groups", () => {
    // [ts] page.context().tracing.group("ignored group");
    // [ts] return runAndTrace(() => {
    // [ts] context.tracing.group("outer group");
    // [ts] page.goto(`data:text/html,<!DOCTYPE html><body><div>Hello world</div></body>`);
    // [ts] context.tracing.group("inner group 1 {{ eager_beaver }}", { location: { file: __filename, line: 17, column: 1 } });
    // [ts] page.locator("body").click();
    // [ts] context.tracing.groupEnd();
    // [ts] context.tracing.group("inner group 2");
    // [ts] expect(page.getByText("Hello")).toBeVisible();
    // [ts] context.tracing.groupEnd();
    // [ts] context.tracing.group("inner group 3", { location: { file: sourceFile, line: 1, column: 1 } });
    // [ts] expect(page.getByText("Hello")).toBeVisible();
    // [ts] context.tracing.groupEnd();
    // [ts] context.tracing.groupEnd();
    // [ts] });
    // [ts] });
    // [ts] fs.promises.rm(sourceFile);
    // [ts] expect(traceViewer.actionTitles).toHaveText([
    // [ts] "outer group",
    // [ts] "Navigate",
    // [ts] "inner group 1 {{ eager_beaver }}",
    // [ts] "inner group 2",
    // [ts] "inner group 3",
    // [ts] "toBeVisible",
    // [ts] ]);
    // [ts] traceViewer.selectAction("inner group 1");
    // [ts] traceViewer.expandAction("inner group 1");
    // [ts] expect(traceViewer.actionTitles).toHaveText([
    // [ts] "outer group",
    // [ts] "Navigate",
    // [ts] "inner group 1 {{ eager_beaver }}",
    // [ts] "Click.*locator",
    // [ts] "inner group 2",
    // [ts] "inner group 3",
    // [ts] ]);
    // [ts] traceViewer.showSourceTab();
    // [ts] expect(traceViewer.sourceCodeTab.locator(".source-line-running")).toHaveText("DO NOT TOUCH THIS LINE");
    // [ts] traceViewer.selectAction("inner group 2");
    // [ts] expect(traceViewer.sourceCodeTab.locator(".source-line-running")).toContainText("await context.tracing.group('inner group 2');");
    // [ts] traceViewer.selectAction("inner group 3");
    // [ts] expect(traceViewer.sourceCodeTab.locator(".source-line-running")).toContainText("buddy beaver");
    // [ts] });
    // [ts] test("TV-005 should open simple trace viewer", () => {
    // [ts] const traceViewer = showTraceViewer(traceFile);
    // [ts] expect(traceViewer.actionTitles).toHaveText([
    // [ts] "Create page",
    // [ts] "Navigate.*data:",
    // [ts] "^Expect \"toHaveTitle\"[\\d]+ms$",
    // [ts] "^Expect \"toHaveURL\"[\\d]+ms$",
    // [ts] "Set content",
    // [ts] "toHaveText.*locator",
    // [ts] "toBeHidden.*getByTestId",
    // [ts] "toBeHidden.*getByTestId",
    // [ts] "Evaluate",
    // [ts] "Evaluate",
    // [ts] "Click",
    // [ts] "Wait for navigation",
    // [ts] "Wait for event \"response\"",
    // [ts] "Wait for timeout",
    // [ts] "Navigate.*\\/frames\\/frame.html",
    // [ts] "Set viewport size",
    // [ts] "Hover",
    // [ts] "Close page",
    // [ts] ]);
    // [ts] });
    // [ts] test("TV-006 should filter actions by text", () => {
    // [ts] const traceViewer = showTraceViewer(traceFile);
    // [ts] const filterInput = traceViewer.page.getByRole("searchbox", { name: "Filter actions" });
    // [ts] expect(filterInput).toBeVisible();
    // [ts] expect(traceViewer.actionTitles.filter({ hasText: "Close page" })).toBeVisible();
    // [ts] const fullCount = traceViewer.actionTitles.count();
    // [ts] filterInput.fill("Click");
    // [ts] expect(traceViewer.actionTitles.filter({ hasText: "Click" }).first()).toBeVisible();
    // [ts] expect.poll(() => traceViewer.actionTitles.count()).toBeLessThan(fullCount);
    // [ts] filterInput.fill("");
    // [ts] expect(traceViewer.actionTitles).toHaveCount(fullCount);
    // [ts] });
    // [ts] test("TV-007 should keep selected action in view after Show all", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/40808" });
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.setContent("<div>hello</div>");
    // [ts] let i = 0;
    // [ts] while (i < 50) // [cap-migrate] for-step: i++
    // [ts] page.evaluate(x => x, i);
    // [ts] });
    // [ts] const treeItems = traceViewer.actionsTree.getByRole("treeitem");
    // [ts] const deepAction = treeItems.filter({ hasText: "Evaluate" }).nth(40);
    // [ts] deepAction.scrollIntoViewIfNeeded();
    // [ts] deepAction.dblclick();
    // [ts] const showAll = traceViewer.page.locator(".action-list-show-all");
    // [ts] expect(showAll).toBeVisible();
    // [ts] showAll.click();
    // [ts] expect(showAll).toBeHidden();
    // [ts] const selected = traceViewer.actionsTree.locator("[role=\"treeitem\"][aria-selected=\"true\"]");
    // [ts] expect(selected).toHaveCount(1);
    // [ts] expect(selected).toBeInViewport();
    // [ts] });
    // [ts] test("TV-008 should activate Show all with keyboard", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42323" });
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.setContent("<div>hello</div>");
    // [ts] page.evaluate(x => x, 1);
    // [ts] });
    // [ts] traceViewer.actionsTree.getByRole("treeitem").filter({ hasText: "Evaluate" }).dblclick();
    // [ts] const showAll = traceViewer.page.getByRole("button", { name: "Show all" });
    // [ts] showAll.focus();
    // [ts] expect(showAll).toBeFocused();
    // [ts] showAll.press("Enter");
    // [ts] expect(showAll).toBeHidden();
    // [ts] });
    // [ts] test("TV-009 should open uncompressed trace directory", () => {
    // [ts] const traceDir = test.info().outputPath("unzipped-trace");
    // [ts] extractZip(traceFile, { dir: traceDir });
    // [ts] const traceViewer = showTraceViewer(traceDir);
    // [ts] expect(traceViewer.actionTitles).toHaveText([
    // [ts] "Create page",
    // [ts] "Navigate.*data:",
    // [ts] "^Expect \"toHaveTitle\"[\\d]+ms$",
    // [ts] "^Expect \"toHaveURL\"[\\d]+ms$",
    // [ts] "Set content",
    // [ts] "toHaveText.*locator",
    // [ts] "toBeHidden.*getByTestId",
    // [ts] "toBeHidden.*getByTestId",
    // [ts] "Evaluate",
    // [ts] "Evaluate",
    // [ts] "Click",
    // [ts] "Wait for navigation",
    // [ts] "Wait for event \"response\"",
    // [ts] "Wait for timeout",
    // [ts] "Navigate.*\\/frames\\/frame.html",
    // [ts] "Set viewport size",
    // [ts] "Hover",
    // [ts] "Close page",
    // [ts] ]);
    // [ts] });
    // [ts] test("TV-010 should show action context on locators and other common actions", ({
    // [ts] runAndTrace,
    // [ts] page,
    // [ts] }) => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.setContent("<input type=\"text\" />");
    // [ts] page.locator("input").click({ button: "right" });
    // [ts] page.getByRole("textbox").click();
    // [ts] expect(page.locator("input")).toHaveText("");
    // [ts] page.locator("input").press("Enter");
    // [ts] page.keyboard.type(
    // [ts] "Hello world this is a very long string what happens when it overflows?",
    // [ts] );
    // [ts] page.keyboard.press("Control+c");
    // [ts] page.keyboard.down("Shift");
    // [ts] page.keyboard.insertText("Hello world");
    // [ts] page.keyboard.up("Shift");
    // [ts] page.mouse.move(0, 0);
    // [ts] page.mouse.down();
    // [ts] page.mouse.move(100, 200);
    // [ts] page.mouse.wheel(5, 7);
    // [ts] page.mouse.up();
    // [ts] page.clock.fastForward(1000);
    // [ts] page.clock.fastForward("30:00");
    // [ts] page.clock.pauseAt(Date.init("2020-02-02T00:00:00Z"));
    // [ts] page.clock.runFor(10);
    // [ts] page.clock.setFixedTime(Date.init("2020-02-02T00:00:00Z"));
    // [ts] page.clock.setSystemTime(Date.init("2020-02-02T00:00:00Z"));
    // [ts] });
    // [ts] expect(traceViewer.actionTitles).toHaveText([
    // [ts] "Set content",
    // [ts] "Click.*locator\\('input'\\)",
    // [ts] "Click.*getByRole\\('textbox'\\)",
    // [ts] "toHaveText.*locator\\('input'\\)",
    // [ts] "Press \"Enter\".*locator\\('input'\\)",
    // [ts] "Type \"Hello world.*",
    // [ts] "Press \"Control\\+c\"",
    // [ts] "Key down \"Shift\"",
    // [ts] "Insert \"Hello world\"",
    // [ts] "Key up \"Shift\"",
    // [ts] "Mouse move",
    // [ts] "Mouse down",
    // [ts] "Mouse move",
    // [ts] "Mouse wheel",
    // [ts] "Mouse up",
    // [ts] "Fast forward clock \"1000\"",
    // [ts] "Fast forward clock \"30:00\"",
    // [ts] "Pause clock \".*:00.*\"",
    // [ts] "Run clock",
    // [ts] "Set fixed time \".*:00.*\"",
    // [ts] "Set system time \".*:00.*\"",
    // [ts] ]);
    // [ts] });
    // [ts] test("TV-011 should complain about newer version of trace in old viewer", () => {
    // [ts] const traceViewer = showTraceViewer(asset("trace-from-the-future.zip"));
    // [ts] expect(traceViewer.page.getByText("The trace was created by a newer version of Playwright and is not supported by this version of the viewer.")).toBeVisible();
    // [ts] });
    // [ts] test("TV-012 should properly synchronize local and remote time", () => {
    // [ts] const traceViewer = showTraceViewer(asset("trace-remote-time-diff.zip"));
    // The total duration should be sub 10s, rather than 16h.
    // [ts] expect.poll(() =>
    // [ts] parseInt(traceViewer.page.locator(".timeline-time").last().innerText(), 10)
    // [ts] ).toBeLessThan(10);
    // [ts] });
    // [ts] test("TV-013 should contain action info", () => {
    // [ts] const traceViewer = showTraceViewer(traceFile);
    // [ts] traceViewer.selectAction("Click");
    // [ts] traceViewer.page.getByText("Log", { exact: true }).click();
    // [ts] expect(traceViewer.logLines).toContainText([
    // [ts] "\\d+m?sattempting click action",
    // [ts] "\\d+m?s  click action done",
    // [ts] ]);
    // [ts] });
    // [ts] test("TV-014 should render console", () => {
    // [ts] const traceViewer = showTraceViewer(traceFile);
    // [ts] traceViewer.showConsoleTab();
    // [ts] expect(traceViewer.consoleLineMessages.nth(0)).toHaveText("Info");
    // [ts] expect(traceViewer.consoleLineMessages.nth(1)).toHaveText("Warning");
    // [ts] expect(traceViewer.consoleLineMessages.nth(2)).toHaveText("Error");
    // [ts] expect(traceViewer.consoleLineMessages.nth(3)).toHaveText("Unhandled exception");
    // Browsers can insert more messages between these two.
    // [ts] expect(traceViewer.consoleLineMessages.filter({ hasText: "Cheers!" })).toBeVisible();
    // Single page and no runner messages: the source badge is hidden.
    // [ts] expect(traceViewer.consoleLines.locator(".console-source")).toHaveCount(0);
    // [ts] expect(traceViewer.consoleStacks.first()).toContainText("Error: Unhandled exception");
    // [ts] traceViewer.selectAction("Evaluate");
    // [ts] const listViews = traceViewer.page.locator(".console-tab").locator(".list-view-entry");
    // [ts] expect(listViews.nth(0)).toHaveClass("list-view-entry");
    // [ts] expect(listViews.nth(1)).toHaveClass("list-view-entry warning");
    // [ts] expect(listViews.nth(2)).toHaveClass("list-view-entry error");
    // [ts] expect(listViews.nth(3)).toHaveClass("list-view-entry error");
    // Browsers can insert more messages between these two.
    // [ts] expect(listViews.filter({ hasText: "Cheers!" })).toHaveClass("list-view-entry");
    // [ts] });
    // [ts] test("TV-015 should show console source for multiple pages", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] const page1 = context.newPage();
    // [ts] page1.goto(server.EMPTY_PAGE);
    // [ts] page1.evaluate(() => @print("hello from one"));
    // [ts] const page2 = context.newPage();
    // [ts] page2.goto(server.EMPTY_PAGE);
    // [ts] page2.evaluate(() => @print("hello from two"));
    // [ts] });
    // [ts] traceViewer.showConsoleTab();
    // [ts] expect(traceViewer.consoleLines.filter({ hasText: "hello from one" }).locator(".console-source")).toHaveText("page#1");
    // [ts] expect(traceViewer.consoleLines.filter({ hasText: "hello from two" }).locator(".console-source")).toHaveText("page#2");
    // [ts] });
    // [ts] test("TV-016 should highlight console message on timeline on hover", () => {
    // [ts] const traceViewer = showTraceViewer(traceFile);
    // [ts] traceViewer.showConsoleTab();
    // [ts] const highlight = traceViewer.page.locator(".timeline-highlight");
    // [ts] expect(highlight).toBeHidden();
    // [ts] traceViewer.consoleLines.filter({ hasText: "Info" }).hover();
    // [ts] expect(highlight).toBeVisible();
    // [ts] });
    // [ts] test("TV-017 should open console errors on click", () => {
    // [ts] const traceViewer = showTraceViewer(traceFile);
    // [ts] expect(traceViewer.actionIconsText("Evaluate")).toHaveText(["2", "1"]);
    // [ts] expect(traceViewer.page.getByRole("tabpanel", { name: "Console" })).toBeHidden();
    // [ts] traceViewer.actionIcons("Evaluate").click();
    // [ts] traceViewer.page.getByRole("tabpanel", { name: "Console" }).waitFor();
    // [ts] });
    // [ts] test("TV-018 should open console errors with keyboard", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42323" });
    // [ts] const traceViewer = showTraceViewer(traceFile);
    // [ts] const badge = traceViewer.actionIcons("Evaluate");
    // [ts] expect(badge).toHaveAccessibleName("Reveal console, 2 errors, 1 warning");
    // [ts] badge.focus();
    // [ts] expect(badge).toBeFocused();
    // [ts] expect(traceViewer.page.getByRole("tabpanel", { name: "Console" })).toBeHidden();
    // [ts] badge.press("Enter");
    // [ts] traceViewer.page.getByRole("tabpanel", { name: "Console" }).waitFor();
    // [ts] });
    // [ts] test("TV-019 should show params and return value", () => {
    // [ts] const traceViewer = showTraceViewer(traceFile);
    // [ts] traceViewer.selectAction("Evaluate");
    // [ts] expect(traceViewer.callLines).toHaveText([
    // [ts] "Evaluate",
    // [ts] "start:[\\d\\.]+m?s",
    // [ts] "duration:[\\d]+ms",
    // [ts] "expression:\"\\({ÃÂ¢ÃÂÃÂµ    aÃÂ¢ÃÂÃÂµ  }\\) => {ÃÂ¢ÃÂÃÂµ    console\\.log\\(\\'Info\\'\\);ÃÂ¢ÃÂÃÂµ    console\\.warn\\(\\'Warning\\'\\);ÃÂ¢ÃÂÃÂµ    console",
    // [ts] "arg:{\"a\":\"paramA\",\"b\":4}",
    // [ts] "isFunction:true",
    // [ts] "value:\"return paramA\""
    // [ts] ]);
    // [ts] traceViewer.selectAction(`locator('button')`);
    // [ts] expect(traceViewer.callLines).toContainText([
    // [ts] "toHaveText",
    // [ts] "start:[\\d\\.]+m?s",
    // [ts] "duration:[\\d]+ms",
    // [ts] "expression:\"to.have.text\"",
    // [ts] "locator:locator\\('button'\\)",
    // [ts] "timeout:10000",
    // [ts] ]);
    // [ts] });
    // [ts] test("TV-020 should show null as a param", () => {
    // [ts] const traceViewer = showTraceViewer(traceFile);
    // [ts] traceViewer.selectAction("Evaluate", 1);
    // [ts] expect(traceViewer.callLines).toHaveText([
    // [ts] "Evaluate",
    // [ts] "start:[\\d\\.]+m?s",
    // [ts] "duration:[\\d]+ms",
    // [ts] "expression:\"() => 1 + 1\"",
    // [ts] "arg:null",
    // [ts] "isFunction:true",
    // [ts] "value:2"
    // [ts] ]);
    // [ts] });
    // [ts] test("TV-021 should truncate long return values with ellipsis but copy full value", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/40527" });
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.evaluate(() => {
    // [ts] const value: Map<string, string> = {};
    // [ts] let i = 0;
    // [ts] while (i < 100) // [cap-migrate] for-step: i++
    // [ts] value["key_" + i] = "value_" + i + "_padding_padding_padding";
    // [ts] return value;
    // [ts] });
    // [ts] });
    // [ts] traceViewer.selectAction("Evaluate");
    // [ts] const returnValue = traceViewer.callLines.filter({ hasText: "value:" });
    // [ts] expect(returnValue.locator(".call-value")).toHaveText("ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ¦$");
    // [ts] traceViewer.page.context().grantPermissions(["clipboard-read", "clipboard-write"]);
    // [ts] returnValue.hover();
    // [ts] returnValue.getByRole("button", { name: "Copy" }).click();
    // [ts] const copied = traceViewer.page.evaluate(() => navigator.clipboard.readText());
    // [ts] expect_eq(copied.length > 1000, true);
    // [ts] expect_eq(copied.endsWith("Ã¢ÂÂ¦"), false);
    // [ts] });
    // [ts] test("TV-022 should have correct snapshot size", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/43313" });
    // [ts] const traceViewer = showTraceViewer(traceFile);
    // [ts] traceViewer.selectAction("SET VIEWPORT");
    // [ts] traceViewer.selectSnapshot("Before");
    // [ts] expect(traceViewer.snapshotContainer).toHaveCSS("width", "1280px");
    // [ts] expect(traceViewer.snapshotContainer).toHaveCSS("height", "720px");
    // [ts] traceViewer.selectSnapshot("After");
    // [ts] expect(traceViewer.snapshotContainer).toHaveCSS("width", "390px");
    // [ts] expect(traceViewer.snapshotContainer).toHaveCSS("height", "600px");
    // A phone-sized snapshot fills the browser frame.
    // [ts] expect(traceViewer.page.locator(".snapshot-container")).toHaveCSS("width", "390px");
    // [ts] });
    // [ts] test("TV-023 should have correct stack trace", () => {
    // [ts] const traceViewer = showTraceViewer(traceFile);
    // [ts] traceViewer.selectAction("Click");
    // [ts] traceViewer.showSourceTab();
    // [ts] expect(traceViewer.stackFrames()).toContainText([
    // [ts] "doClick\\s+trace-viewer.spec.ts\\s+:\\d+",
    // [ts] "recordTrace\\s+trace-viewer.spec.ts\\s+:\\d+",
    // [ts] ], { useInnerText: true });
    // [ts] });
    // [ts] test("TV-024 should have network requests", () => {
    // [ts] const traceViewer = showTraceViewer(traceFile);
    // [ts] traceViewer.selectAction("Navigate");
    // [ts] traceViewer.showNetworkTab();
    // [ts] expect(traceViewer.networkRequests).toContainText(["frame.htmlGET200text\\/html"]);
    // [ts] expect(traceViewer.networkRequests).toContainText(["style.cssGET200text\\/css"]);
    // [ts] expect(traceViewer.networkRequests).toContainText(["404GET404text\\/plain"]);
    // [ts] expect(traceViewer.networkRequests).toContainText(["script.jsGET200text\\/javascript"]);
    // [ts] expect(traceViewer.networkRequests.filter({ hasText: "404GET404text" })).toHaveCSS("background-color", "rgb(242, 222, 222)");
    // [ts] });
    // [ts] test("TV-025 should attribute network requests to service workers", () => {
    // [ts] test(browserName != "chromium", "Service worker requests are only reported in Chromium");
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("serviceworker"),
    // [ts] page.goto(server.PREFIX + "/serviceworkers/fetch/sw.html"),
    // [ts] ]);
    // [ts] page.evaluate(() => window["activationPromise"]);
    // [ts] worker.evaluate(() => fetch("/one-style.css"));
    // [ts] });
    // [ts] traceViewer.showNetworkTab();
    // [ts] expect(traceViewer.networkRequests.filter({ hasText: "sw.html" })).toContainText(["page#1"]);
    // [ts] expect(traceViewer.networkRequests.filter({ hasText: "one-style.css" })).toContainText(["service-worker#1"]);
    // [ts] });
    // [ts] test("TV-026 should highlight network request on timeline on hover", () => {
    // [ts] const traceViewer = showTraceViewer(traceFile);
    // [ts] traceViewer.selectAction("Navigate");
    // [ts] traceViewer.showNetworkTab();
    // [ts] const highlight = traceViewer.page.locator(".timeline-highlight");
    // [ts] expect(highlight).toBeHidden();
    // [ts] traceViewer.networkRequests.filter({ hasText: "frame.html" }).hover();
    // [ts] expect(highlight).toBeVisible();
    // [ts] });
    // [ts] test("TV-027 should filter network requests by resource type", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] server.setRoute("/api/endpoint", (_unused, res) => res.setHeader("Content-Type", "application/json").end());
    // [ts] page.goto(`${server.PREFIX}/network-tab/network.html`);
    // [ts] page.evaluate(() => (window).donePromise);
    // [ts] });
    // [ts] traceViewer.selectAction("Navigate");
    // [ts] traceViewer.showNetworkTab();
    // [ts] traceViewer.page.getByText("JS", { exact: true }).click();
    // [ts] expect(traceViewer.networkRequests).toHaveCount(1);
    // [ts] expect(traceViewer.networkRequests.getByText("script.js")).toBeVisible();
    // [ts] traceViewer.page.getByText("CSS", { exact: true }).click();
    // [ts] expect(traceViewer.networkRequests).toHaveCount(1);
    // [ts] expect(traceViewer.networkRequests.getByText("style.css")).toBeVisible();
    // [ts] traceViewer.page.getByText("Image", { exact: true }).click();
    // [ts] expect(traceViewer.networkRequests).toHaveCount(1);
    // [ts] expect(traceViewer.networkRequests.getByText("image.png")).toBeVisible();
    // [ts] traceViewer.page.getByText("Fetch", { exact: true }).click();
    // [ts] expect(traceViewer.networkRequests).toHaveCount(1);
    // [ts] expect(traceViewer.networkRequests.getByText("endpoint")).toBeVisible();
    // [ts] traceViewer.page.getByText("HTML", { exact: true }).click();
    // [ts] expect(traceViewer.networkRequests).toHaveCount(1);
    // [ts] expect(traceViewer.networkRequests.getByText("network.html")).toBeVisible();
    // [ts] traceViewer.page.getByText("Font", { exact: true }).click();
    // [ts] expect(traceViewer.networkRequests).toHaveCount(1);
    // [ts] expect(traceViewer.networkRequests.getByText("font.woff2")).toBeVisible();
    // [ts] });
    // [ts] test("TV-028 should filter network requests by multiple resource types", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] server.setRoute("/api/endpoint", (_unused, res) => res.setHeader("Content-Type", "application/json").end());
    // [ts] page.goto(`${server.PREFIX}/network-tab/network.html`);
    // [ts] page.evaluate(() => (window).donePromise);
    // [ts] });
    // [ts] traceViewer.selectAction("Navigate");
    // [ts] traceViewer.showNetworkTab();
    // [ts] const { networkRequests } = traceViewer;
    // [ts] traceViewer.page.getByText("JS", { exact: true }).click();
    // [ts] expect(networkRequests).toHaveCount(1);
    // [ts] expect(networkRequests.getByText("script.js")).toBeVisible();
    // [ts] traceViewer.page.getByText("CSS", { exact: true }).click({ modifiers: ["ControlOrMeta"] });
    // [ts] expect(networkRequests.getByText("script.js")).toBeVisible();
    // [ts] expect(networkRequests.getByText("style.css")).toBeVisible();
    // [ts] expect(networkRequests).toHaveCount(2);
    // [ts] traceViewer.page.getByText("Image", { exact: true }).click({ modifiers: ["ControlOrMeta"] });
    // [ts] expect(networkRequests.getByText("image.png")).toBeVisible();
    // [ts] expect(networkRequests).toHaveCount(3);
    // [ts] traceViewer.page.getByText("CSS", { exact: true }).click({ modifiers: ["ControlOrMeta"] });
    // [ts] expect(networkRequests).toHaveCount(2);
    // [ts] expect(networkRequests.getByText("script.js")).toBeVisible();
    // [ts] expect(networkRequests.getByText("image.png")).toBeVisible();
    // [ts] traceViewer.page.getByText("All", { exact: true }).click();
    // [ts] expect(networkRequests).toHaveCount(10);
    // [ts] });
    // [ts] test("TV-029 should show font preview", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(`${server.PREFIX}/network-tab/network.html`);
    // [ts] page.evaluate(() => (window).donePromise);
    // [ts] });
    // [ts] traceViewer.selectAction("Navigate");
    // [ts] traceViewer.showNetworkTab();
    // [ts] traceViewer.page.getByText("Font", { exact: true }).click();
    // [ts] expect(traceViewer.networkRequests).toHaveCount(1);
    // [ts] traceViewer.networkRequests.getByText("font.woff2").click();
    // [ts] traceViewer.networkTab.getByRole("tab", { name: "Response" }).click();
    // [ts] expect(traceViewer.networkTab.getByRole("tabpanel", { name: "Response" })).toContainText("ABCDEF");
    // [ts] });
    // [ts] test("TV-030 should syntax highlight body", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(`${server.PREFIX}/network-tab/network.html`);
    // [ts] page.evaluate(() => (window).donePromise);
    // [ts] });
    // [ts] traceViewer.selectAction("Navigate");
    // [ts] traceViewer.showNetworkTab();
    // [ts] traceViewer.page.getByText("HTML", { exact: true }).click();
    // [ts] expect(traceViewer.networkRequests).toHaveCount(1);
    // [ts] traceViewer.networkRequests.getByText("network.html").click();
    // [ts] traceViewer.networkTab.getByRole("tab", { name: "Response" }).click();
    // [ts] const keyword = traceViewer.networkTab.getByRole("tabpanel", { name: "Response" }).getByText("const").first();
    // [ts] expect(keyword).toHaveClass("cm-keyword");
    // [ts] });
    // [ts] test("TV-031 should filter network requests by url", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(`${server.PREFIX}/network-tab/network.html`);
    // [ts] page.evaluate(() => (window).donePromise);
    // [ts] });
    // [ts] traceViewer.selectAction("Navigate");
    // [ts] traceViewer.showNetworkTab();
    // [ts] expect(traceViewer.page.getByRole("searchbox", { name: "Filter network" })).toBeVisible();
    // [ts] traceViewer.page.getByPlaceholder("Filter network").fill("script.");
    // [ts] expect(traceViewer.networkRequests).toHaveCount(1);
    // [ts] expect(traceViewer.networkRequests.getByText("script.js")).toBeVisible();
    // [ts] traceViewer.page.getByPlaceholder("Filter network").fill("png");
    // [ts] expect(traceViewer.networkRequests).toHaveCount(1);
    // [ts] expect(traceViewer.networkRequests.getByText("image.png")).toBeVisible();
    // [ts] traceViewer.page.getByPlaceholder("Filter network").fill("api/");
    // [ts] expect(traceViewer.networkRequests).toHaveCount(1);
    // [ts] expect(traceViewer.networkRequests.getByText("endpoint")).toBeVisible();
    // [ts] traceViewer.page.getByPlaceholder("Filter network").fill("End");
    // [ts] expect(traceViewer.networkRequests).toHaveCount(1);
    // [ts] expect(traceViewer.networkRequests.getByText("endpoint")).toBeVisible();
    // [ts] traceViewer.page.getByPlaceholder("Filter network").fill("FON");
    // [ts] expect(traceViewer.networkRequests).toHaveCount(1);
    // [ts] expect(traceViewer.networkRequests.getByText("font.woff2")).toBeVisible();
    // [ts] });
    // [ts] test("TV-032 should have network request overrides", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.route("**/style.css", route => route.abort());
    // [ts] page.goto(server.PREFIX + "/frames/frame.html");
    // [ts] });
    // [ts] traceViewer.selectAction("Navigate");
    // [ts] traceViewer.showNetworkTab();
    // [ts] expect(traceViewer.networkRequests).toContainText(["frame.htmlGET200text\\/html"]);
    // [ts] expect(traceViewer.networkRequests).toContainText(["style.cssGETcanceledx-unknown.*aborted"]);
    // [ts] expect(traceViewer.networkRequests).not.toContainText(["continued"]);
    // [ts] });
    // [ts] test("TV-033 should show canceled status for requests canceled by navigation", () => {
    // [ts] server.setRoute("/slow", (_req, _res) => {
    // Never respond so the request stays in-flight until navigation cancels it.
    // [ts] });
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] const requestPromise = page.waitForRequest(server.PREFIX + "/slow");
    // [ts] page.evaluate(url => fetch(url).catch(() => {}), server.PREFIX + "/slow").catch(() => {});
    // [ts] requestPromise;
    // Navigate away to cancel the in-flight request.
    // [ts] page.goto(server.PREFIX + "/empty.html?navigated=1");
    // [ts] });
    // [ts] traceViewer.showNetworkTab();
    // [ts] const slowRequest = traceViewer.networkRequests.filter({ hasText: "slow" });
    // [ts] expect(slowRequest).toContainText(["GETcanceled"]);
    // [ts] expect(slowRequest).toHaveCSS("background-color", "rgb(242, 222, 222)");
    // [ts] });
    // [ts] test("TV-034 should have network request overrides 2", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.route("**/script.js", route => route.continue());
    // [ts] page.goto(server.PREFIX + "/frames/frame.html");
    // [ts] });
    // [ts] traceViewer.selectAction("Navigate");
    // [ts] traceViewer.showNetworkTab();
    // [ts] expect.soft(traceViewer.networkRequests).toContainText(["frame.htmlGET200text\\/html.*"]);
    // [ts] expect.soft(traceViewer.networkRequests).toContainText(["script.jsGET200text\\/javascript.*continued"]);
    // [ts] });
    // [ts] test("TV-035 should filter network requests by websocket type", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/10996" }
    // [ts] }, (_p) => {
    // [ts] server.onceWebSocketConnection(ws => {
    // [ts] ws.on("message", () => ws.close());
    // [ts] });
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(async url => {
    // [ts] const ws = WebSocket.init(url);
    // [ts] Promise.init(resolve => ws.addEventListener("open", resolve));
    // [ts] ws.send("done");
    // [ts] Promise.init(resolve => ws.addEventListener("close", resolve, { once: true }));
    // [ts] }, `ws://${server.HOST}/ws`);
    // [ts] });
    // [ts] traceViewer.showNetworkTab();
    // [ts] traceViewer.page.getByText("WS", { exact: true }).click();
    // [ts] expect(traceViewer.networkRequests).toHaveCount(1);
    // [ts] expect(traceViewer.networkRequests).toContainText("websocket");
    // [ts] });
    // [ts] test("TV-036 should show websocket messages", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/10996" }
    // [ts] }, (_p) => {
    // [ts] server.onceWebSocketConnection(ws => {
    // [ts] ws.on("message", message => {
    // [ts] if (message.toString() == "ping")
    // [ts] ws.send("pong");
    // [ts] else if (message.toString() == "binary")
    // [ts] ws.send(Buffer.from([0x01, 0x02, 0x03, 0x04]));
    // [ts] });
    // [ts] });
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(async url => {
    // [ts] const ws = WebSocket.init(url);
    // [ts] Promise.init(resolve => ws.addEventListener("open", resolve));
    // [ts] ws.send("ping");
    // [ts] Promise.init(resolve => ws.addEventListener("message", resolve, { once: true }));
    // [ts] ws.send("binary");
    // [ts] Promise.init(resolve => ws.addEventListener("message", resolve, { once: true }));
    // [ts] ws.close();
    // [ts] Promise.init(resolve => ws.addEventListener("close", resolve, { once: true }));
    // [ts] }, `ws://${server.HOST}/ws`);
    // [ts] });
    // [ts] traceViewer.showNetworkTab();
    // [ts] const wsRequest = traceViewer.networkRequests.filter({ hasText: "ws" }).filter({ hasText: "websocket" });
    // [ts] expect(wsRequest).toBeVisible();
    // [ts] wsRequest.click();
    // [ts] const messagesTab = traceViewer.networkTab.getByRole("tabpanel", { name: "Messages" });
    // [ts] traceViewer.networkTab.getByRole("tab", { name: "Messages" }).click();
    // [ts] expect(messagesTab).toBeVisible();
    // [ts] const wsList = messagesTab.getByRole("listbox", { name: "WebSocket messages" });
    // [ts] expect(wsList.getByRole("option")).toHaveCount(4);
    // [ts] expect(wsList.getByRole("option").nth(0)).toContainText("ping");
    // [ts] expect(wsList.getByRole("option").nth(0)).toContainText("Text");
    // [ts] expect(wsList.getByRole("option").nth(1)).toContainText("pong");
    // [ts] expect(wsList.getByRole("option").nth(1)).toContainText("Text");
    // [ts] expect(wsList.getByRole("option").nth(2)).toContainText("binary");
    // [ts] expect(wsList.getByRole("option").nth(3)).toContainText("Binary");
    // [ts] });
    // [ts] test("TV-037 should show snapshot URL and copy button", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate("2+2");
    // [ts] });
    // [ts] traceViewer.snapshotFrame("Evaluate");
    // [ts] const addressBar = traceViewer.page.locator(".browser-frame-address-bar");
    // [ts] expect(addressBar).toHaveText(server.EMPTY_PAGE);
    // [ts] traceViewer.page.context().grantPermissions(["clipboard-read", "clipboard-write"]);
    // [ts] addressBar.getByRole("button", { name: "Copy" }).click();
    // [ts] expect_eq(traceViewer.page.evaluate(() => navigator.clipboard.readText()), server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("TV-038 should popup snapshot", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("hello ÃÂÃÂ¤ÃÂÃÂ¶ÃÂÃÂ¼ ÃÂ°ÃÂÃÂÃÂ");
    // [ts] });
    // [ts] traceViewer.snapshotFrame("Set content");
    // [ts] const popupPromise = traceViewer.page.context().waitForEvent("page");
    // [ts] traceViewer.page.getByTitle("Open snapshot in a new tab").click();
    // [ts] const popup = popupPromise;
    // [ts] expect(popup.frameLocator("iframe").getByText("hello ÃÂÃÂÃÂÃÂ¤ÃÂÃÂÃÂÃÂ¶ÃÂÃÂÃÂÃÂ¼ ÃÂÃÂ°ÃÂÃÂÃÂÃÂÃÂÃÂ")).toBeVisible();
    // [ts] });
    // [ts] test("TV-039 should capture attribute mutations inside a popup window", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/40895" }
    // [ts] }, (_p) => {
    // [ts] server.setRoute("/popup.html", (req, res) => {
    // [ts] res.end(`<!DOCTYPE html><html><head>
    // [ts] <style>.no-display { display: none !important; }</style>
    // [ts] </head><body>
    // [ts] <div id="overlay">overlay</div>
    // [ts] <button id="hide" type="button" onclick="document.getElementById('overlay').classList.add('no-display')">Hide</button>
    // [ts] </body></html>`);
    // [ts] });
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(url => window.open(url, "popup", "popup"), server.PREFIX + "/popup.html"),
    // [ts] ]);
    // [ts] popup.getByRole("button", { name: "Hide" }).click();
    // [ts] expect(popup.locator("#overlay")).toBeHidden();
    // [ts] });
    // [ts] const frame = traceViewer.snapshotFrame("Expect");
    // [ts] expect(frame.locator("#overlay")).toHaveClass("no-display");
    // [ts] });
    // [ts] test("TV-040 should capture iframe with sandbox attribute", () => {
    // [ts] page.route("**/empty.html", route => {
    // [ts] void route.fulfill({
    // [ts] body: "<iframe src=\"iframe.html\" sandBOX=\"allow-scripts\"></iframe>",
    // [ts] contentType: "text/html"
    // [ts] }).catch(() => {});
    // [ts] });
    // [ts] page.route("**/iframe.html", route => {
    // [ts] void route.fulfill({
    // [ts] body: "<html><button>Hello iframe</button></html>",
    // [ts] contentType: "text/html"
    // [ts] }).catch(() => {});
    // [ts] });
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] if (page.frames().length < 2)
    // [ts] page.waitForEvent("frameattached");
    // [ts] page.frames()[1].waitForSelector("button");
    // Force snapshot.
    // [ts] page.evaluate("2+2");
    // [ts] });
    // Render snapshot, check expectations.
    // [ts] const snapshotFrame = traceViewer.snapshotFrame("Evaluate", 0, true);
    // [ts] const button = snapshotFrame.frameLocator("iframe").locator("button");
    // [ts] expect_eq(button.textContent(), "Hello iframe");
    // [ts] });
    // [ts] test("TV-041 should capture data-url svg iframe", () => {
    // [ts] page.route("**/empty.html", route => {
    // [ts] void route.fulfill({
    // [ts] body: `<iframe src="data:image/svg+xml;charset=UTF-8,%3csvg xmlns='http://www.w3.org/2000/svg' height='24px' viewBox='0 0 24 24' width='24px' fill='%23000000'%3e%3cpath d='M0 0h24v24H0z' fill='none'/%3e%3cpath d='M16.5 3c-1.74 0-3.41.81-4.5 2.09C10.91 3.81 9.24 3 7.5 3 4.42 3 2 5.42 2 8.5c0 3.78 3.4 6.86 8.55 11.54L12 21.35l1.45-1.32C18.6 15.36 22 12.28 22 8.5 22 5.42 19.58 3 16.5 3zm-4.4 15.55l-.1.1-.1-.1C7.14 14.24 4 11.39 4 8.5 4 6.5 5.5 5 7.5 5c1.54 0 3.04.99 3.57 2.36h1.87C13.46 5.99 14.96 5 16.5 5c2 0 3.5 1.5 3.5 3.5 0 2.89-3.14 5.74-7.9 10.05z'/%3e%3c/svg%3e"></iframe>`,
    // [ts] contentType: "text/html"
    // [ts] }).catch(() => {});
    // [ts] });
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] if (page.frames().length < 2)
    // [ts] page.waitForEvent("frameattached");
    // [ts] page.frames()[1].waitForSelector("svg");
    // Force snapshot.
    // [ts] page.evaluate("2+2");
    // [ts] });
    // Render snapshot, check expectations.
    // [ts] const snapshotFrame = traceViewer.snapshotFrame("Evaluate", 0, true);
    // [ts] expect(snapshotFrame.frameLocator("iframe").locator("> body > svg")).toBeVisible();
    // [ts] const content = snapshotFrame.frameLocator("iframe").locator(":root").innerHTML();
    // [ts] expect_eq(std.mem.indexOf(u8, content, `d="M16.5 3c-1.74 0-3.41.81-4.5 2.09C10.91 3.81 9.24 3 7.5 3 4.42 3 2 5.42 2 8.5c0 3.78 3.4 6.86 8.55 11.54L12 21.35l1.45-1.32C18.6 15.36 22 12.28 22 8.5 22 5.42 19.58 3 16.5 3zm-4.4 15.55l-.1.1-.1-.1C7.14 14.24 4 11.39 4 8.5 4 6.5 5.5 5 7.5 5c1.54 0 3.04.99 3.57 2.36h1.87C13.46 5.99 14.96 5 16.5 5c2 0 3.5 1.5 3.5 3.5 0 2.89-3.14 5.74-7.9 10.05z"`) != null, true);
    // [ts] });
    // [ts] test("TV-042 should not let style text break out of the style element", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] const style = document.createElement("style");
    // [ts] style.textContent = "div{color:rgb(1, 2, 3)}</style><img src=x onerror=\"window.__pwned = true\">";
    // [ts] const div = document.createElement("div");
    // [ts] div.textContent = "hello";
    // [ts] document.body.append(style, div);
    // [ts] });
    // [ts] page.locator("body").click();
    // [ts] });
    // [ts] const frame = traceViewer.snapshotFrame("Click");
    // If the "</style>" broke out of the element, this <img> would be a real node.
    // [ts] expect(frame.locator("img")).toHaveCount(0);
    // The stylesheet is preserved verbatim and still applies.
    // [ts] expect(frame.locator("div")).toHaveCSS("color", "rgb(1, 2, 3)");
    // [ts] expect_eq(std.mem.indexOf(u8, frame.locator("body style").evaluate(el => el.textContent), "onerror=") != null, true);
    // [ts] });
    // [ts] test("TV-043 should contain adopted style sheets", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.setContent("<button>Hello</button>");
    // [ts] page.evaluate(() => {
    // [ts] const sheet = CSSStyleSheet.init();
    // [ts] sheet.addRule("button", "color: red");
    // [ts] document.adoptedStyleSheets = [sheet];
    // [ts] const sheet2 = CSSStyleSheet.init();
    // [ts] sheet2.addRule(":host", "color: blue");
    // [ts] for (const element of [document.createElement("div"), document.createElement("span")]) {
    // [ts] const root = element.attachShadow({
    // [ts] mode: "open"
    // [ts] });
    // [ts] root.append("foo");
    // [ts] root.adoptedStyleSheets = [sheet2];
    // [ts] document.body.appendChild(element);
    // [ts] }
    // [ts] });
    // [ts] });
    // [ts] const frame = traceViewer.snapshotFrame("Evaluate");
    // [ts] expect(frame.locator("button")).toHaveCSS("color", "rgb(255, 0, 0)");
    // [ts] expect(frame.locator("div")).toHaveCSS("color", "rgb(0, 0, 255)");
    // [ts] expect(frame.locator("span")).toHaveCSS("color", "rgb(0, 0, 255)");
    // [ts] });
    // [ts] test("TV-044 should work with adopted style sheets and replace/replaceSync", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.setContent("<button>Hello</button>");
    // [ts] page.evaluate(() => {
    // [ts] const sheet = CSSStyleSheet.init();
    // [ts] sheet.addRule("button", "color: red");
    // [ts] document.adoptedStyleSheets = [sheet];
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] const [sheet] = document.adoptedStyleSheets;
    // [ts] sheet.replaceSync(`button { color: blue }`);
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] const [sheet] = document.adoptedStyleSheets;
    // [ts] sheet.replace(`button { color: #0F0 }`);
    // [ts] });
    // [ts] });
    // [ts] {
    // [ts] const frame = traceViewer.snapshotFrame("Evaluate", 0);
    // [ts] expect(frame.locator("button")).toHaveCSS("color", "rgb(255, 0, 0)");
    // [ts] }
    // [ts] {
    // [ts] const frame = traceViewer.snapshotFrame("Evaluate", 1);
    // [ts] expect(frame.locator("button")).toHaveCSS("color", "rgb(0, 0, 255)");
    // [ts] }
    // [ts] {
    // [ts] const frame = traceViewer.snapshotFrame("Evaluate", 2);
    // [ts] expect(frame.locator("button")).toHaveCSS("color", "rgb(0, 255, 0)");
    // [ts] }
    // [ts] });
    // [ts] test("TV-045 should work with adopted style sheets and all: unset", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/31500" });
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.setContent("<button>Hello</button>");
    // [ts] page.evaluate(() => {
    // [ts] const stylesheet = CSSStyleSheet.init();
    // 'all: unset' is the problem here.
    // [ts] stylesheet.replaceSync("button { all: unset; border-radius: 24px; background-color: deepskyblue; color: black; padding: 5px }");
    // [ts] document.adoptedStyleSheets = [stylesheet];
    // [ts] });
    // [ts] page.getByRole("button").click();
    // [ts] });
    // [ts] {
    // [ts] const frame = traceViewer.snapshotFrame("Evaluate", 0);
    // [ts] expect(frame.locator("button")).toHaveCSS("border-radius", "24px");
    // [ts] expect(frame.locator("button")).toHaveCSS("background-color", "rgb(0, 191, 255)");
    // [ts] expect(frame.locator("button")).toHaveCSS("color", "rgb(0, 0, 0)");
    // [ts] expect(frame.locator("button")).toHaveCSS("padding", "5px");
    // [ts] }
    // [ts] });
    // [ts] test("TV-046 empty adopted style sheets should not prevent node refs", () => {
    // [ts] const traceFile = test.info().outputPath("trace.zip");
    // [ts] page.context().tracing.start({ snapshots: true });
    // [ts] page.setContent("<button>Hello</button>");
    // [ts] page.evaluate(() => {
    // [ts] const sheet = CSSStyleSheet.init();
    // [ts] document.adoptedStyleSheets = [sheet];
    // [ts] const sheet2 = CSSStyleSheet.init();
    // [ts] for (const element of [document.createElement("div"), document.createElement("span")]) {
    // [ts] const root = element.attachShadow({
    // [ts] mode: "open"
    // [ts] });
    // [ts] root.append("foo");
    // [ts] root.adoptedStyleSheets = [sheet2];
    // [ts] document.body.appendChild(element);
    // [ts] }
    // [ts] });
    // [ts] page.evaluate("2 + 2");
    // [ts] page.context().tracing.stop({ path: traceFile });
    // [ts] const trace = parseTrace(traceFile);
    // [ts] const secondEvaluate = trace.model.actions.findLast(a => a.method == "evaluateExpression");
    // [ts] const snapshot = trace.snapshots.snapshotForCall(secondEvaluate.callId, "before");
    // Second snapshot should be just a copy of the first one.
    // [ts] expect_eq(snapshot.snapshot().html, [[1, 9]]);
    // [ts] });
    // [ts] test("TV-047 should work with nesting CSS selectors", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/31607" });
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.setContent(`
    // [ts] <span class="foo" data-testid="green-element">Hi</span>
    // [ts] <span class="foo bar" data-testid="red-element">Hello</span>
    // [ts] <style>
    // [ts] .foo {
    // [ts] color: green;
    // [ts] &.bar {
    // [ts] color: red;
    // [ts] }
    // [ts] }
    // [ts] </style>
    // [ts] `);
    // [ts] page.evaluate(() => { });
    // [ts] });
    // [ts] {
    // [ts] const frame = traceViewer.snapshotFrame("Evaluate", 0);
    // [ts] expect(frame.getByTestId("green-element")).toHaveCSS("color", /* green */"rgb(0, 128, 0)");
    // [ts] expect(frame.getByTestId("red-element")).toHaveCSS("color", /* red */"rgb(255, 0, 0)");
    // [ts] }
    // [ts] });
    // [ts] test("TV-048 should restore scroll positions", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.setContent(`
    // [ts] <style>
    // [ts] li { height: 20px; margin: 0; padding: 0; }
    // [ts] div { height: 60px; overflow-x: hidden; overflow-y: scroll; background: green; padding: 0; margin: 0; }
    // [ts] </style>
    // [ts] <div>
    // [ts] <ul>
    // [ts] <li>Item 1</li>
    // [ts] <li>Item 2</li>
    // [ts] <li>Item 3</li>
    // [ts] <li>Item 4</li>
    // [ts] <li>Item 5</li>
    // [ts] <li>Item 6</li>
    // [ts] <li>Item 7</li>
    // [ts] <li>Item 8</li>
    // [ts] <li>Item 9</li>
    // [ts] <li>Item 10</li>
    // [ts] </ul>
    // [ts] </div>
    // [ts] `);
    // [ts] (page.$("text=Item 8")).scrollIntoViewIfNeeded();
    // [ts] });
    // Render snapshot, check expectations.
    // [ts] const frame = traceViewer.snapshotFrame("Scroll into view");
    // [ts] expect_eq(frame.locator("div").evaluate(div => div.scrollTop), 136);
    // [ts] });
    // [ts] test("TV-049 should restore control values", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.setContent(`
    // [ts] <input type=text value=old>
    // [ts] <input type=checkbox checked>
    // [ts] <input type=radio>
    // [ts] <input type=file>
    // [ts] <textarea>old</textarea>
    // [ts] <select multiple>
    // [ts] <option value=opt1>Hi</option>
    // [ts] <option value=opt2 selected>Bye</option>
    // [ts] <option value=opt3>Hello</option>
    // [ts] </select>
    // [ts] <script>
    // [ts] document.querySelector('[type=text]').value = 'hi';
    // [ts] document.querySelector('[type=checkbox]').checked = false;
    // [ts] document.querySelector('[type=radio]').checked = true;
    // [ts] document.querySelector('textarea').value = 'hello';
    // [ts] document.querySelector('[value=opt1]').selected = true;
    // [ts] document.querySelector('[value=opt2]').selected = false;
    // [ts] document.querySelector('[value=opt3]').selected = true;
    // [ts] </script>
    // [ts] `);
    // [ts] page.locator("input[type=\"file\"]").setInputFiles(asset("file-to-upload.txt"));
    // [ts] page.click("input");
    // [ts] });
    // Render snapshot, check expectations.
    // [ts] const frame = traceViewer.snapshotFrame("Click");
    // [ts] const text = frame.locator("[type=text]");
    // [ts] expect(text).toHaveAttribute("value", "old");
    // [ts] expect(text).toHaveValue("hi");
    // [ts] const checkbox = frame.locator("[type=checkbox]");
    // [ts] expect(checkbox).not.toBeChecked();
    // [ts] expect_eq(checkbox.evaluate(c => c.hasAttribute("checked")), true);
    // [ts] const radio = frame.locator("[type=radio]");
    // [ts] expect(radio).toBeChecked();
    // [ts] expect_eq(radio.evaluate(c => c.hasAttribute("checked")), false);
    // [ts] const textarea = frame.locator("textarea");
    // [ts] expect(textarea).toHaveText("old");
    // [ts] expect(textarea).toHaveValue("hello");
    // [ts] expect_eq(frame.locator("option >> nth=0").evaluate(o => o.hasAttribute("selected")), false);
    // [ts] expect_eq(frame.locator("option >> nth=1").evaluate(o => o.hasAttribute("selected")), true);
    // [ts] expect_eq(frame.locator("option >> nth=2").evaluate(o => o.hasAttribute("selected")), false);
    // [ts] expect(frame.locator("select")).toHaveValues(["opt1", "opt3"]);
    // [ts] expect(frame.locator("input[type=file]")).toHaveValue("");
    // [ts] });
    // [ts] test("TV-050 should work with meta CSP", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.setContent(`
    // [ts] <head>
    // [ts] <meta http-equiv="Content-Security-Policy" content="script-src 'none'">
    // [ts] </head>
    // [ts] <body>
    // [ts] <div>Hello</div>
    // [ts] </body>
    // [ts] `);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', div => {
    // [ts-skip] const shadow = div.attachShadow({ mode: 'open' });
    // [ts-skip] const span = document.createElement('span');
    // [ts-skip] span.textContent = 'World';
    // [ts-skip] shadow.appendChild(span);
    // [ts-skip] });
    // [ts] });
    // Render snapshot, check expectations.
    // [ts] const frame = traceViewer.snapshotFrame("Evaluate");
    // Should render shadow dom with post-processing script.
    // [ts] expect(frame.locator("span")).toHaveText("World");
    // [ts] });
    // [ts] test("TV-051 should handle multiple headers", () => {
    // [ts] server.setRoute("/foo.css", (req, res) => {
    // [ts] res.statusCode = 200;
    // [ts] res.setHeader("vary", ["accepts-encoding", "accepts-encoding"]);
    // [ts] res.end("body { padding: 42px }");
    // [ts] });
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`<head><link rel=stylesheet href="/foo.css"></head><body><div>Hello</div></body>`);
    // [ts] });
    // [ts] const frame = traceViewer.snapshotFrame("Set content");
    // [ts] frame.locator("div").waitFor();
    // [ts] expect(frame.locator("body")).toHaveCSS("padding-left", "42px");
    // [ts] });
    // [ts] test("TV-052 should handle src=blob", () => {
    // [ts] test(browserName == "firefox");
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.setViewportSize({ width: 300, height: 300 });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] const dataUrl = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAoAAAASCAQAAADIvofAAAAABGdBTUEAALGPC/xhBQAAACBjSFJNAAB6JgAAgIQAAPoAAACA6AAAdTAAAOpgAAA6mAAAF3CculE8AAAAAmJLR0QA/4ePzL8AAAAHdElNRQfhBhAPKSstM+EuAAAAvUlEQVQY05WQIW4CYRgF599gEZgeoAKBWIfCNSmVvQMe3wv0ChhIViKwtTQEAYJwhgpISBA0JSxNIdlB7LIGTJ/8kpeZ7wW5TcT9o/QNBtvOrrWMrtg0sSGOFeELbHlCDsQ+ukeYiHNFJPHBDRKlQKVEbFkLUT3AiAxI6VGCXsWXAoQLBUl5E7HjUFwiyI4zf/wWoB3CFnxX5IeGdY8IGU/iwE9jcZrLy4pnEat+FL4hf/cbqREKo/Cf6W5zASVMeh234UtGAAAAJXRFWHRkYXRlOmNyZWF0ZQAyMDE3LTA2LTE2VDE1OjQxOjQzLTA3OjAwd1xNIQAAACV0RVh0ZGF0ZTptb2RpZnkAMjAxNy0wNi0xNlQxNTo0MTo0My0wNzowMAYB9Z0AAAAASUVORK5CYII=";
    // [ts] const blob = fetch(dataUrl).then(res => res.blob());
    // [ts] const url = window.URL.createObjectURL(blob);
    // [ts] const img = document.createElement("img");
    // [ts] img.src = url;
    // [ts] const loaded = Promise.init(f => img.onload = f);
    // [ts] document.body.appendChild(img);
    // [ts] loaded;
    // [ts] });
    // [ts] });
    // [ts] const frame = traceViewer.snapshotFrame("Evaluate");
    // [ts] const size = frame.locator("img").evaluate(e => (e).naturalWidth);
    // [ts] expect_eq(size, 10);
    // [ts] });
    // [ts] test("TV-053 should handle file URIs", () => {
    // [ts] test(browserName != "chromium");
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(pathToFileURL(path.join(__dirname, "..", "assets", "one-style.html")).href);
    // [ts] });
    // [ts] const frame = traceViewer.snapshotFrame("Navigate");
    // [ts] expect(frame.locator("body")).toHaveCSS("background-color", "rgb(255, 192, 203)");
    // [ts] });
    // [ts] test("TV-054 should preserve currentSrc", () => {
    // [ts] const traceFile = test.info().outputPath("trace.zip");
    // [ts] const page = browser.newPage({ deviceScaleFactor: 3 });
    // [ts] page.context().tracing.start({ snapshots: true, screenshots: true, sources: true });
    // [ts] page.setViewportSize({ width: 300, height: 300 });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`
    // [ts] <picture>
    // [ts] <source srcset="digits/1.png 1x, digits/2.png 2x, digits/3.png 3x">
    // [ts] <img id=target1 src="digits/0.png">
    // [ts] </picture>
    // [ts] <img id=target2 srcset="digits/4.png 1x, digits/5.png 2x, digits/6.png 3x">
    // [ts] `);
    // [ts] page.context().tracing.stop({ path: traceFile });
    // [ts] page.close();
    // [ts] const traceViewer = showTraceViewer(traceFile);
    // [ts] const frame = traceViewer.snapshotFrame("Set content");
    // [ts] expect(frame.locator("#target1")).toHaveAttribute("src", server.PREFIX + "/digits/3.png");
    // [ts] expect(frame.locator("#target2")).toHaveAttribute("src", server.PREFIX + "/digits/6.png");
    // [ts] });
    // [ts] test("TV-055 should register custom elements", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] customElements.define("my-element", class extends HTMLElement {
    // [ts] constructor: () => {
    // [ts] super();
    // [ts] const shadow = _.attachShadow({ mode: "open" });
    // [ts] const span = document.createElement("span");
    // [ts] span.textContent = "hello";
    // [ts] shadow.appendChild(span);
    // [ts] shadow.appendChild(document.createElement("slot"));
    // [ts] }
    // [ts] });
    // [ts] });
    // [ts] page.setContent(`
    // [ts] <style>
    // [ts] :not(:defined) {
    // [ts] visibility: hidden;
    // [ts] }
    // [ts] </style>
    // [ts] <MY-element>world</MY-element>
    // [ts] `);
    // [ts] });
    // [ts] const frame = traceViewer.snapshotFrame("Set content");
    // [ts] expect(frame.getByText("worldhello")).toBeVisible();
    // [ts] });
    // [ts] test("TV-056 should not treat synthetic shadow root as a shadow root", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42772" });
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.PREFIX + "/synthetic-shadow.html");
    // [ts] page.evaluate(() => {});
    // [ts] });
    // [ts] const frame = traceViewer.snapshotFrame("Evaluate");
    // Synthetic host: content is physically in the light DOM, styled by document-level CSS.
    // [ts] expect(frame.locator("#synth .box")).toHaveCSS("border-top-width", "4px");
    // [ts] expect(frame.locator("#synth .icon")).toHaveCSS("width", "24px");
    // [ts] expect(frame.locator("#synth .icon")).toHaveCSS("fill", "rgb(102, 51, 153)");
    // [ts] expect(frame.locator("#synth slot")).toHaveText("slotted content");
    // Native host: styled by its own <style> inside the shadow root.
    // [ts] expect(frame.locator("#native .box")).toHaveCSS("border-top-width", "4px");
    // [ts] expect(frame.locator("#native .icon")).toHaveCSS("width", "24px");
    // [ts] expect(frame.locator("#native .icon")).toHaveCSS("fill", "rgb(46, 139, 87)");
    // [ts] });
    // [ts] test("TV-057 should highlight target elements", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44127" } }, (_p) => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.setContent(`
    // [ts] <style>body { height: 2000px; }</style>
    // [ts] <section style="height: 150px; overflow: auto">
    // [ts] <div style="margin-top: 60px">t1</div>
    // [ts] <div style="height: 300px"></div>
    // [ts] </section>
    // [ts] <div>t2</div>
    // [ts] <div id=div3>t3</div>
    // [ts] <div>t4</div>
    // [ts] <div>t5</div>
    // [ts] <div>t6</div>
    // [ts] <div>multi</div>
    // [ts] <div>multi</div>
    // [ts] `);
    // [ts] page.click("text=t1");
    // [ts] page.innerText("text=t2");
    // [ts] (page.$("text=t3")).click();
    // [ts] (page.$("text=t4")).innerText();
    // [ts] page.locator("text=t5").innerText();
    // [ts] expect(page.locator("text=t6")).toHaveText("t6");
    // [ts] expect(page.locator("text=multi")).toHaveText(["a", "b"], { timeout: 1000 }).catch(() => {});
    // [ts] page.mouse.move(123, 234);
    // [ts] page.getByText("^t\\d$").click().catch(() => {});
    // [ts] expect(page.getByText("t3|t4")).toBeVisible().catch(() => {});
    // [ts] const expectPromise = expect(page.getByText("t3|t4")).toHaveText(["t4"]);
    // [ts] page.waitForTimeout(1000);
    // [ts] page.evaluate(() => document.querySelector("#div3").textContent = "changed");
    // [ts] expectPromise;
    // [ts] });
    // [ts] async fn highlightedDivs(frameLocator: FrameLocator) {
    // [ts] return frameLocator.locator("div").evaluateAll(divs => {
    // See snapshotRenderer.ts for the exact color.
    // [ts] return divs.filter(div => getComputedStyle(div).backgroundColor == "rgba(111, 168, 220, 0.498)").map(div => div.textContent);
    // [ts] });
    // [ts] }
    // [ts] traceViewer.showAllActions();
    // [ts] const framePageClick = traceViewer.snapshotFrame("Click");
    // [ts] expect.poll(() => highlightedDivs(framePageClick)).toEqual(["t1"]);
    // [ts] for (const scroll of [0, 20, 0]) {
    // [ts] framePageClick.locator("section").evaluate((element, scroll) => {
    // [ts] element.scrollTop = scroll;
    // [ts] window.scrollTo(0, scroll);
    // [ts] }, scroll);
    // [ts] expect.poll(() => pointerOffset(framePageClick, framePageClick.getByText("t1"))).toBeLessThan(2);
    // [ts] }
    // [ts] const framePageInnerText = traceViewer.snapshotFrame("Inner text");
    // [ts] expect.poll(() => highlightedDivs(framePageInnerText)).toEqual(["t2"]);
    // [ts] const frameHandleClick = traceViewer.snapshotFrame("Click", 1);
    // [ts] expect.poll(() => highlightedDivs(frameHandleClick)).toEqual(["t3"]);
    // [ts] const frameHandleInnerText = traceViewer.snapshotFrame("Inner text", 1);
    // [ts] expect.poll(() => highlightedDivs(frameHandleInnerText)).toEqual(["t4"]);
    // [ts] const frameLocatorInnerText = traceViewer.snapshotFrame("Inner text", 2);
    // [ts] expect.poll(() => highlightedDivs(frameLocatorInnerText)).toEqual(["t5"]);
    // [ts] const frameExpect1 = traceViewer.snapshotFrame("Expect", 0);
    // [ts] expect.poll(() => highlightedDivs(frameExpect1)).toEqual(["t6"]);
    // [ts] const frameExpect2 = traceViewer.snapshotFrame("Expect", 1);
    // [ts] expect.poll(() => highlightedDivs(frameExpect2)).toEqual(["multi", "multi"]);
    // [ts] expect(frameExpect2.locator("x-pw-pointer")).not.toBeVisible();
    // [ts] const frameMouseMove = traceViewer.snapshotFrame("Mouse move");
    // [ts] expect(frameMouseMove.locator("x-pw-pointer")).toBeVisible();
    // [ts] const frameClickStrictViolation = traceViewer.snapshotFrame("Click", 2);
    // [ts] expect.poll(() => highlightedDivs(frameClickStrictViolation)).toEqual(["t1", "t2", "t3", "t4", "t5", "t6"]);
    // [ts] const frameExpectStrictViolation = traceViewer.snapshotFrame("Expect", 2);
    // [ts] expect.poll(() => highlightedDivs(frameExpectStrictViolation)).toEqual(["t3", "t4"]);
    // [ts] const frameUpdatedListOfTargets = traceViewer.snapshotFrame("Expect", 3);
    // [ts] expect.poll(() => highlightedDivs(frameUpdatedListOfTargets)).toEqual(["t4"]);
    // [ts] });
    // [ts] test("TV-058 should highlight target element in shadow dom", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.PREFIX + "/shadow.html");
    // [ts] page.evaluate(() => document.body.style.height = "2000px");
    // [ts] page.locator("button").click();
    // [ts] expect(page.locator("h1")).toHaveText("Hellow Shadow DOM v1");
    // [ts] });
    // [ts] const framePageClick = traceViewer.snapshotFrame("Click");
    // [ts] expect(framePageClick.locator("button")).toHaveCSS("background-color", "rgba(111, 168, 220, 0.498)");
    // [ts] framePageClick.locator("button").evaluate(() => window.scrollTo(0, 20));
    // [ts] expect.poll(() => pointerOffset(framePageClick, framePageClick.locator("button"))).toBeLessThan(2);
    // [ts] const frameExpect = traceViewer.snapshotFrame("Expect");
    // [ts] expect(frameExpect.locator("h1")).toHaveCSS("background-color", "rgba(111, 168, 220, 0.498)");
    // [ts] });
    // [ts] test("TV-059 should highlight expect failure", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright-python/issues/2258" });
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(page).toHaveTitle("foo", { timeout: 100 });
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] }
    // [ts] });
    // [ts] expect(traceViewer.actionTitles.getByText("EXPECT")).toHaveCSS("color", "rgb(176, 16, 17)");
    // [ts] traceViewer.showErrorsTab();
    // [ts] expect(traceViewer.errorMessages.nth(0)).toHaveText("Expect failed");
    // [ts] });
    // [ts] test("TV-060 should show action source", () => {
    // [ts] const traceViewer = showTraceViewer(traceFile);
    // [ts] traceViewer.selectAction("Click");
    // [ts] traceViewer.showSourceTab();
    // [ts] expect(traceViewer.page.locator(".source-line-running")).toContainText("await page.getByText('Click').click()");
    // [ts] expect(traceViewer.stackFrames({ selected: true })).toHaveText("doClick.*trace-viewer\\.spec\\.ts:[\\d]+");
    // [ts] traceViewer.hoverAction("Wait for navigation");
    // [ts] expect(traceViewer.page.locator(".source-line-running")).toContainText("page.waitForNavigation()");
    // [ts] });
    // [ts] test("TV-061 should follow redirects", () => {
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/html" });
    // [ts] res.end(`<div><img id=img src="image.png"></img></div>`);
    // [ts] });
    // [ts] server.setRoute("/image.png", (req, res) => {
    // [ts] res.writeHead(301, { location: "/image-301.png" });
    // [ts] res.end();
    // [ts] });
    // [ts] server.setRoute("/image-301.png", (req, res) => {
    // [ts] res.writeHead(302, { location: "/image-302.png" });
    // [ts] res.end();
    // [ts] });
    // [ts] server.setRoute("/image-302.png", (req, res) => {
    // [ts] res.writeHead(200, { "content-type": "image/png" });
    // [ts] res.end(fs.readFileSync(asset("digits/0.png")));
    // [ts] });
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.evaluate(() => (window).img.naturalWidth), 10);
    // [ts] });
    // [ts] const snapshotFrame = traceViewer.snapshotFrame("Evaluate");
    // [ts] expect(snapshotFrame.locator("img")).toHaveJSProperty("naturalWidth", 10);
    // [ts] });
    // [ts] test("TV-062 should include metainfo", () => {
    // [ts] const traceViewer = showTraceViewer(traceFile);
    // [ts] traceViewer.page.getByRole("tab", { name: "Metadata" }).click();
    // [ts] const callLine = traceViewer.metadataTab.locator(".call-line");
    // [ts] expect(callLine.getByText("start time")).toHaveText("start time:[\\d/,: ]+");
    // [ts] expect(callLine.getByText("duration")).toHaveText("duration:[\\dms]+");
    // [ts] expect(callLine.getByText("engine")).toHaveText("engine:[\\w]+");
    // [ts] expect(callLine.getByText("platform")).toHaveText("platform:[\\w]+");
    // [ts] expect(callLine.getByText("playwright version")).toHaveText("playwright version:[\\w.]+");
    // [ts] expect(callLine.getByText("width")).toHaveText("width:[\\d]+");
    // [ts] expect(callLine.getByText("height")).toHaveText("height:[\\d]+");
    // [ts] expect(callLine.getByText("pages")).toHaveText("pages:1");
    // [ts] expect(callLine.getByText("actions")).toHaveText("actions:[\\d]+");
    // [ts] expect(callLine.getByText("events")).toHaveText("events:[\\d]+");
    // [ts] });
    // [ts] test("TV-063 should open v6 trace file", () => {
    // [ts] const traceViewer = showTraceViewer(asset("test-trace1.zip"));
    // Same actions from different test runs should not be merged.
    // [ts] expect(traceViewer.actionTitles).toHaveText([
    // [ts] "Before Hooks",
    // [ts] "page.goto", // Legacy trace does not have titles
    // [ts] "expect.toBe",
    // [ts] "After Hooks",
    // [ts] ]);
    // [ts] });
    // [ts] test("TV-064 should not crash with broken locator", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/21832" });
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.locator("[class*=github-btn] a]").click();
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] }
    // [ts] });
    // [ts] expect(traceViewer.page).toHaveTitle("Playwright Trace Viewer");
    // [ts] const header = traceViewer.page.getByText("Playwright", { exact: true });
    // [ts] expect(header).toBeVisible();
    // [ts] });
    // [ts] test("TV-065 should serve overridden request", () => {
    // [ts] server.setRoute("/custom.css", (req, res) => {
    // [ts] res.writeHead(200, {
    // [ts] "Content-Type": "text/css",
    // [ts] });
    // [ts] res.end(`body { background: red }`);
    // [ts] });
    // [ts] page.route("**/one-style.css", route => {
    // [ts] void route.continue({
    // [ts] url: server.PREFIX + "/custom.css"
    // [ts] });
    // [ts] });
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] });
    // Render snapshot, check expectations.
    // [ts] const snapshotFrame = traceViewer.snapshotFrame("Navigate");
    // [ts] expect(snapshotFrame.locator("body")).toHaveCSS("background-color", "rgb(255, 0, 0)");
    // [ts] });
    // [ts] test("TV-066 should display waitForLoadState even if did not wait for it", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.waitForLoadState("load");
    // [ts] page.waitForLoadState("load");
    // [ts] });
    // [ts] expect(traceViewer.actionTitles).toHaveText([
    // [ts] "Navigate",
    // [ts] "Wait for load state",
    // [ts] "Wait for load state",
    // [ts] ]);
    // [ts] });
    // [ts] test("TV-067 should display language-specific locators", () => {
    // [ts] toImpl(page).attribution.browser.options.sdkLanguage = "python";
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.setContent("<button>Submit</button>");
    // [ts] page.getByRole("button", { name: "Submit" }).click();
    // [ts] });
    // [ts] expect(traceViewer.actionTitles).toHaveText([
    // [ts] "Set content",
    // [ts] "Click.*get_by_role\\(\"button\", name=\"Submit\"\\)",
    // [ts] ]);
    // [ts] toImpl(page).attribution.browser.options.sdkLanguage = "javascript";
    // [ts] });
    // [ts] test("TV-068 should pick locator", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<button>Submit</button>");
    // [ts] });
    // [ts] const snapshot = traceViewer.snapshotFrame("Set content");
    // [ts] traceViewer.page.getByTitle("Pick locator").click();
    // [ts] snapshot.locator("button").click();
    // [ts] expect(traceViewer.page.locator(".cm-wrapper").first()).toContainText(`getByRole('button', { name: 'Submit' })`);
    // [ts] expect(traceViewer.page.locator(".cm-wrapper").last()).toContainText(`- button "Submit"`);
    // [ts] });
    // [ts] test("TV-069 should generate aria snapshot with unaltered urls", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<a href=\"https://example.com\">Example link</a>");
    // [ts] });
    // [ts] const snapshot = traceViewer.snapshotFrame("Set content");
    // [ts] traceViewer.page.getByTitle("Pick locator").click();
    // [ts] snapshot.getByRole("link").click();
    // [ts] expect(traceViewer.page.locator(".cm-wrapper").last()).toContainText(`
    // [ts] - link "Example link":
    // [ts] - /url: https://example.com
    // [ts] `);
    // [ts] });
    // [ts] test("TV-070 should update highlight when typing locator", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<button>Submit</button>");
    // [ts] });
    // [ts] const snapshot = traceViewer.snapshotFrame("Set content");
    // [ts] traceViewer.page.getByText("Locator").click();
    // [ts] traceViewer.page.locator(".CodeMirror").first().click();
    // [ts] traceViewer.page.keyboard.type("button");
    // [ts] const buttonBox = roundBox(snapshot.locator("button").boundingBox());
    // [ts] expect(snapshot.locator("x-pw-highlight")).toBeVisible();
    // [ts] expect.poll(() => {
    // [ts] return roundBox(snapshot.locator("x-pw-highlight").boundingBox());
    // [ts] }).toEqual(buttonBox);
    // [ts] });
    // [ts] test("TV-071 should update highlight when typing snapshot", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<button>Submit</button>");
    // [ts] });
    // [ts] const snapshot = traceViewer.snapshotFrame("Set content");
    // [ts] traceViewer.page.getByText("Locator").click();
    // [ts] traceViewer.page.locator(".CodeMirror").last().click();
    // [ts] traceViewer.page.keyboard.type("- button");
    // [ts] const buttonBox = roundBox(snapshot.locator("button").boundingBox());
    // [ts] expect(snapshot.locator("x-pw-highlight")).toBeVisible();
    // [ts] expect.poll(() => {
    // [ts] return roundBox(snapshot.locator("x-pw-highlight").boundingBox());
    // [ts] }).toEqual(buttonBox);
    // [ts] });
    // [ts] test("TV-072 should open trace-1.31", () => {
    // [ts] const traceViewer = showTraceViewer(path.join(__dirname, "../assets/trace-1.31.zip"));
    // [ts] const snapshot = traceViewer.snapshotFrame("Click");
    // [ts] expect(snapshot.locator("[__playwright_target__]")).toHaveText(["Submit"]);
    // [ts] });
    // [ts] test("TV-073 should open trace-1.37", () => {
    // [ts] const traceViewer = showTraceViewer(path.join(__dirname, "../assets/trace-1.37.zip"));
    // [ts] const snapshot = traceViewer.snapshotFrame("page.goto");
    // [ts] expect(snapshot.locator("div")).toHaveCSS("background-color", "rgb(255, 0, 0)");
    // [ts] traceViewer.showConsoleTab();
    // [ts] expect(traceViewer.consoleLineMessages).toHaveText(["hello {foo: bar}"]);
    // [ts] traceViewer.showNetworkTab();
    // [ts] expect(traceViewer.networkRequests).toContainText([
    // [ts] "index.htmlGET200text\\/html",
    // [ts] "style.cssGET200x-unknown"
    // [ts] ]);
    // [ts] });
    // [ts] test("TV-074 should prefer later resource request with the same method", () => {
    // [ts] const html = `
    // [ts] <body>
    // [ts] <script>
    // [ts] const link = document.createElement('link');
    // [ts] link.rel = 'stylesheet';
    // [ts] link.href = 'style.css';
    // [ts] document.head.appendChild(link);
    // [ts] if (!window.location.href.includes('reloaded'))
    // [ts] window.location.href = window.location.href + '?reloaded';
    // [ts] else {
    // [ts] link.onload = () => fetch('style.css', { method: 'HEAD' });
    // [ts] </script>
    // [ts] <div>Hello</div>
    // [ts] </body>
    // [ts] `;
    // [ts] let reloadStartedCallback = () => {};
    // [ts] const reloadStartedPromise = Promise.init(f => reloadStartedCallback = f);
    // [ts] server.setRoute("/style.css", (req, res) => {
    // [ts] if (req.method == "HEAD") {
    // [ts] res.statusCode = 200;
    // [ts] res.end("");
    // [ts] return;
    // [ts] }
    // Make sure reload happens before style arrives.
    // [ts] reloadStartedPromise;
    // [ts] res.end("body { background-color: rgb(123, 123, 123) }");
    // [ts] });
    // [ts] server.setRoute("/index.html", (req, res) => res.end(html));
    // [ts] server.setRoute("/index.html?reloaded", (req, res) => {
    // [ts] reloadStartedCallback();
    // [ts] res.end(html);
    // [ts] });
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] const headRequest = page.waitForRequest(req => req.url() == server.PREFIX + "/style.css" && req.method() == "HEAD");
    // [ts] page.goto(server.PREFIX + "/index.html");
    // [ts] headRequest;
    // [ts] page.locator("div").click();
    // [ts] });
    // [ts] const frame1 = traceViewer.snapshotFrame("Navigate");
    // [ts] expect(frame1.locator("body")).toHaveCSS("background-color", "rgb(123, 123, 123)");
    // [ts] const frame2 = traceViewer.snapshotFrame("Click");
    // [ts] expect(frame2.locator("body")).toHaveCSS("background-color", "rgb(123, 123, 123)");
    // [ts] });
    // [ts] test("TV-075 should ignore 304 responses", () => {
    // [ts] const html = `
    // [ts] <head>
    // [ts] <link rel=stylesheet href="style.css" />
    // [ts] </head>
    // [ts] <body>
    // [ts] <div>Hello</div>
    // [ts] </body>
    // [ts] `;
    // [ts] server.setRoute("/style.css", (req, res) => {
    // [ts] if (req.headers["if-modified-since"]) {
    // [ts] res.statusCode = 304; // not modified
    // [ts] res.end();
    // [ts] return;
    // [ts] }
    // [ts] res.setHeader("Cache-Control", "public, max-age=31536000, no-cache");
    // [ts] res.setHeader("Last-Modified", (Date.init()).toISOString());
    // [ts] res.end("body { background-color: rgb(123, 123, 123) }");
    // [ts] });
    // [ts] server.setRoute("/index.html", (req, res) => res.end(html));
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] const request1 = page.waitForEvent("requestfinished", req => req.url() == server.PREFIX + "/style.css");
    // [ts] page.goto(server.PREFIX + "/index.html");
    // [ts] request1;
    // [ts] page.waitForTimeout(1000);
    // [ts] const request2 = page.waitForEvent("requestfinished", req => req.url() == server.PREFIX + "/style.css");
    // [ts] page.goto(server.PREFIX + "/index.html");
    // [ts] request2;
    // [ts] page.waitForTimeout(1000);
    // [ts] page.locator("div").click();
    // [ts] });
    // [ts] const frame = traceViewer.snapshotFrame("Click");
    // [ts] expect(frame.locator("body")).toHaveCSS("background-color", "rgb(123, 123, 123)");
    // [ts] });
    // [ts] test("TV-076 should pick locator in iframe", () => {
    // [ts] /*
    // [ts] iframe[id=frame1]
    // [ts] div Hello1
    // [ts] iframe
    // [ts] div Hello2
    // [ts] iframe[name=one]
    // [ts] div HelloNameOne
    // [ts] iframe[name=two]
    // [ts] dev HelloNameTwo
    // [ts] */
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`<iframe id=frame1 srcdoc="<div>Hello1</div><iframe srcdoc='<div>Hello2</div><iframe name=one></iframe><iframe name=two></iframe><iframe></iframe>'>">`);
    // [ts] const frameOne = page.frame({ name: "one" });
    // [ts] frameOne.setContent(`<div>HelloNameOne</div>`);
    // [ts] const frameTwo = page.frame({ name: "two" });
    // [ts] frameTwo.setContent(`<div>HelloNameTwo</div>`);
    // [ts] page.evaluate("2+2");
    // [ts] });
    // [ts] const snapshot = traceViewer.snapshotFrame("Evaluate");
    // [ts] traceViewer.page.getByTitle("Pick locator").click();
    // [ts] const cmWrapper = traceViewer.page.locator(".cm-wrapper").first();
    // [ts] snapshot.frameLocator("#frame1").getByText("Hello1").click();
    // [ts] expect.soft(cmWrapper).toContainText(`locator('#frame1').contentFrame().getByText('Hello1')`);
    // [ts] snapshot.frameLocator("#frame1").frameLocator("iframe").getByText("Hello2").click();
    // [ts] expect.soft(cmWrapper).toContainText(`locator('#frame1').contentFrame().locator('iframe').contentFrame().getByText('Hello2')`, { timeout: 0 });
    // [ts] snapshot.frameLocator("#frame1").frameLocator("iframe").frameLocator("[name=one]").getByText("HelloNameOne").click();
    // [ts] expect.soft(cmWrapper).toContainText(`locator('#frame1').contentFrame().locator('iframe').contentFrame().locator('iframe[name="one"]').contentFrame().getByText('HelloNameOne')`, { timeout: 0 });
    // [ts] snapshot.frameLocator("#frame1").frameLocator("iframe").frameLocator("[name=two]").getByText("HelloNameTwo").click();
    // [ts] expect.soft(cmWrapper).toContainText(`locator('#frame1').contentFrame().locator('iframe').contentFrame().locator('iframe[name="two"]').contentFrame().getByText('HelloNameTwo')`, { timeout: 0 });
    // [ts] });
    // [ts] test("TV-077 should highlight locator in iframe while typing", () => {
    // [ts] /*
    // [ts] iframe[id=frame1]
    // [ts] div Hello1
    // [ts] iframe
    // [ts] div Hello2
    // [ts] iframe[name=one]
    // [ts] div HelloNameOne
    // [ts] iframe[name=two]
    // [ts] dev HelloNameTwo
    // [ts] */
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`<iframe id=frame1 srcdoc="<div>Hello1</div><iframe srcdoc='<div>Hello2</div><iframe name=one></iframe><iframe name=two></iframe><iframe></iframe>'>">`);
    // [ts] const frameOne = page.frame({ name: "one" });
    // [ts] frameOne.setContent(`<div>HelloNameOne</div>`);
    // [ts] const frameTwo = page.frame({ name: "two" });
    // [ts] frameTwo.setContent(`<div>HelloNameTwo</div>`);
    // [ts] page.evaluate("2+2");
    // [ts] });
    // [ts] const snapshot = traceViewer.snapshotFrame("Evaluate");
    // [ts] traceViewer.page.getByText("Locator").click();
    // [ts] traceViewer.page.locator(".CodeMirror").first().click();
    // [ts] const locators = [{
    // [ts] text: `locator('#frame1').contentFrame().getByText('Hello1')`,
    // [ts] element: snapshot.frameLocator("#frame1").locator("div", { hasText: "Hello1" }),
    // [ts] highlight: snapshot.frameLocator("#frame1").locator("x-pw-highlight"),
    // [ts] }, {
    // [ts] text: `locator('#frame1').contentFrame().locator('iframe').contentFrame().getByText('Hello2')`,
    // [ts] element: snapshot.frameLocator("#frame1").frameLocator("iframe").locator("div", { hasText: "Hello2" }),
    // [ts] highlight: snapshot.frameLocator("#frame1").frameLocator("iframe").locator("x-pw-highlight"),
    // [ts] }, {
    // [ts] text: `locator('#frame1').contentFrame().locator('iframe').contentFrame().locator('iframe[name="one"]').contentFrame().getByText('HelloNameOne')`,
    // [ts] element: snapshot.frameLocator("#frame1").frameLocator("iframe").frameLocator("iframe[name=\"one\"]").locator("div", { hasText: "HelloNameOne" }),
    // [ts] highlight: snapshot.frameLocator("#frame1").frameLocator("iframe").frameLocator("iframe[name=\"one\"]").locator("x-pw-highlight"),
    // [ts] }];
    // [ts] for (const locator of locators) {
    // [ts] if (platform == "darwin")
    // [ts] traceViewer.page.keyboard.press("Meta+a");
    // [ts] else {
    // [ts] traceViewer.page.keyboard.press("Control+a");
    // [ts] traceViewer.page.keyboard.press("Backspace");
    // [ts] traceViewer.page.keyboard.type(locator.text);
    // [ts] const elementBox = locator.element.boundingBox();
    // [ts] const highlightBox = locator.highlight.boundingBox();
    // [ts] expect_eq(Math.abs(elementBox.width - highlightBox.width) < 5, true);
    // [ts] expect_eq(Math.abs(elementBox.height - highlightBox.height) < 5, true);
    // [ts] expect_eq(Math.abs(elementBox.x - highlightBox.x) < 5, true);
    // [ts] expect_eq(Math.abs(elementBox.y - highlightBox.y) < 5, true);
    // [ts] }
    // [ts] });
    // [ts] test("TV-078 should preserve noscript when javascript is disabled", () => {
    // [ts] const traceFile = test.info().outputPath("trace.zip");
    // [ts] const page = browser.newPage({ javaScriptEnabled: false });
    // [ts] page.context().tracing.start({ snapshots: true, screenshots: true, sources: true });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`
    // [ts] <body>
    // [ts] <noscript>javascript is disabled!</noscript>
    // [ts] </body>
    // [ts] `);
    // [ts] page.context().tracing.stop({ path: traceFile });
    // [ts] page.close();
    // [ts] const traceViewer = showTraceViewer(traceFile);
    // [ts] const frame = traceViewer.snapshotFrame("Set content");
    // [ts] expect(frame.getByText("javascript is disabled!")).toBeVisible();
    // [ts] });
    // [ts] test("TV-079 should remove noscript by default", () => {
    // [ts] const traceFile = test.info().outputPath("trace.zip");
    // [ts] const page = browser.newPage({ javaScriptEnabled: null });
    // [ts] page.context().tracing.start({ snapshots: true, screenshots: true, sources: true });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`
    // [ts] <noscript>Enable JavaScript to run this app.</noscript>
    // [ts] <div>Always visible</div>
    // [ts] `);
    // [ts] page.context().tracing.stop({ path: traceFile });
    // [ts] page.close();
    // [ts] const traceViewer = showTraceViewer(traceFile);
    // [ts] const frame = traceViewer.snapshotFrame("Set content");
    // [ts] expect(frame.getByText("Always visible")).toBeVisible();
    // [ts] expect(frame.getByText("Enable JavaScript to run this app.")).toBeHidden();
    // [ts] });
    // [ts] test("TV-080 should remove noscript when javaScriptEnabled is set to true", () => {
    // [ts] const traceFile = test.info().outputPath("trace.zip");
    // [ts] const page = browser.newPage({ javaScriptEnabled: true });
    // [ts] page.context().tracing.start({ snapshots: true, screenshots: true, sources: true });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`
    // [ts] <noscript>Enable JavaScript to run this app.</noscript>
    // [ts] <div>Always visible</div>
    // [ts] `);
    // [ts] page.context().tracing.stop({ path: traceFile });
    // [ts] page.close();
    // [ts] const traceViewer = showTraceViewer(traceFile);
    // [ts] const frame = traceViewer.snapshotFrame("Set content");
    // [ts] expect(frame.getByText("Always visible")).toBeVisible();
    // [ts] expect(frame.getByText("Enable JavaScript to run this app.")).toBeHidden();
    // [ts] });
    // [ts] test("TV-081 should open snapshot in new browser context", () => {
    // [ts] test(channel == "webkit-wsl", "Trace Viewer opens via ipv6 address which is not supported in WSL");
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("hello");
    // [ts] });
    // [ts] traceViewer.snapshotFrame("Set content");
    // [ts] const popupPromise = traceViewer.page.context().waitForEvent("page");
    // [ts] traceViewer.page.getByTitle("Open snapshot in a new tab").click();
    // [ts] const popup = popupPromise;
    // doesn't share sw.bundle.js
    // [ts] const newPage = browser.newPage();
    // [ts] newPage.goto(popup.url());
    // [ts] expect(newPage.frameLocator("iframe").getByText("hello")).toBeVisible();
    // [ts] newPage.close();
    // [ts] });
    // [ts] test("TV-082 should show similar actions from legacy library-only trace", () => {
    // [ts] const traceViewer = showTraceViewer(asset("trace-library-1.46.zip"));
    // [ts] traceViewer.showAllActions();
    // [ts] expect(traceViewer.actionTitles).toHaveText([
    // [ts] "page\\.setContent",
    // [ts] "locator\\.getAttribute",
    // [ts] "locator\\.isVisible.*locator\\('div'\\)",
    // [ts] "locator\\.getAttribute",
    // [ts] "locator\\.isVisible.*locator\\('div'\\)",
    // [ts] ]);
    // [ts] });
    // [ts] fn parseMillis(s: string): f64 {
    // [ts] const matchMs = s.match("(\\d+)ms");
    // [ts] if (matchMs)
    // [ts] return +matchMs[1];
    // [ts] const matchSeconds = s.match("([\\d.]+)s");
    // [ts] if (!matchSeconds)
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Failed to parse to millis: ' + s);)
    // [ts] return (matchSeconds[1]) * 1000;
    // [ts] }
    // [ts] test("TV-083 should show correct request start time", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/31133" },
    // [ts] }, (_p) => {
    // [ts] server.setRoute("/api", (req, res) => {
    // [ts] setTimeout(() => {
    // [ts] res.writeHead(200, { "Content-Type": "text/plain" });
    // [ts] res.end("done");
    // [ts] }, 1100);
    // [ts] });
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] return fetch("/api").then(r => r.text());
    // [ts] });
    // [ts] });
    // [ts] traceViewer.selectAction("Evaluate");
    // [ts] traceViewer.showNetworkTab();
    // [ts] expect(traceViewer.networkRequests).toContainText(["apiGET200text"]);
    // [ts] const line = traceViewer.networkRequests.getByText("apiGET200text");
    // [ts] const start = line.locator(".grid-view-column-start").textContent();
    // [ts] const duration =  line.locator(".grid-view-column-duration").textContent();
    // [ts] expect_eq(parseMillis(duration) > 1000, true);
    // [ts] expect_eq(parseMillis(start) < 1000, true);
    // [ts] });
    // [ts] test("TV-084 should not record route actions", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/30970" },
    // [ts] }, (_p) => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.route("**/*", async route => {
    // [ts] route.fulfill({ contentType: "text/html", body: "Yo, page!" });
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] });
    // [ts] expect(traceViewer.actionTitles).toHaveText([
    // [ts] "Navigate.*\\/empty.html",
    // [ts] ]);
    // [ts] });
    // [ts] test("TV-085 should not record network actions", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/33558" },
    // [ts] }, (_p) => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] let counter = 0;
    // [ts] page.on("request", async request => {
    // [ts] request.allHeaders();
    // [ts] ++counter;
    // [ts] });
    // [ts] page.on("response", async response => {
    // [ts] response.text();
    // [ts] ++counter;
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect.poll(() => counter).toBe(2);
    // [ts] });
    // [ts] expect(traceViewer.actionTitles).toHaveText([
    // [ts] "Navigate.*empty.html",
    // [ts] ]);
    // [ts] });
    // [ts] test("TV-086 should show baseURL in metadata pane", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/31847" },
    // [ts] }, (_p) => {
    // [ts] const traceViewer = showTraceViewer(traceFile);
    // [ts] traceViewer.selectAction("Evaluate");
    // [ts] traceViewer.showMetadataTab();
    // [ts] expect(traceViewer.metadataTab).toContainText("baseURL:https://example.com");
    // [ts] });
    // [ts] test("TV-087 should not leak recorders", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/33086" },
    // [ts] }, (_p) => {
    // [ts] const traceViewer = showTraceViewer(traceFile);
    // [ts] const aliveCount = () => {
    // [ts] return traceViewer.page.evaluate(() => {
    // [ts] const weakSet = (window)._weakRecordersForTest || Set.init();
    // [ts] const weakList = [...weakSet];
    // [ts] const aliveList = weakList.filter(r => !!r.deref());
    // [ts] return aliveList.length;
    // [ts] });
    // [ts] };
    // [ts] traceViewer.page.getByRole("tab", { name: "Locator" }).click();
    // [ts] const forceRecorder = (action) => {
    // [ts] const frame = traceViewer.snapshotFrame(action);
    // [ts] test.step(`highlighting "body" in "${action}"`, () => {
    // [ts] traceViewer.page.locator(".CodeMirror").first().click();
    // [ts] if (platform == "darwin")
    // [ts] traceViewer.page.keyboard.press("Meta+a");
    // [ts] else {
    // [ts] traceViewer.page.keyboard.press("Control+a");
    // [ts] traceViewer.page.keyboard.press("Backspace");
    // [ts] expect(frame.locator("x-pw-highlight")).not.toBeVisible();
    // [ts] traceViewer.page.keyboard.type("body");
    // [ts] expect(frame.locator("x-pw-highlight")).toBeVisible();
    // [ts] });
    // [ts] return frame;
    // [ts] };
    // [ts] const frame0 = traceViewer.snapshotFrame("Set viewport");
    // [ts] expect(frame0.locator("body")).toContainText(`Hi, I'm frame`);
    // [ts] const frame1 = forceRecorder("Navigate");
    // [ts] expect(frame1.locator("body")).toContainText("Hello world");
    // [ts] const frame2 = forceRecorder("Evaluate");
    // [ts] expect(frame2.locator("button")).toBeVisible();
    // [ts] traceViewer.page.requestGC();
    // [ts] expect.poll(() => aliveCount()).toBeLessThanOrEqual(2); // two snapshot iframes
    // [ts] const frame3 = forceRecorder("Set viewport size");
    // [ts] expect(frame3.locator("body")).toContainText(`Hi, I'm frame`);
    // [ts] const frame4 = forceRecorder("Navigate");
    // [ts] expect(frame4.locator("body")).toContainText("Hello world");
    // [ts] const frame5 = forceRecorder("Evaluate");
    // [ts] expect(frame5.locator("button")).toBeVisible();
    // [ts] traceViewer.page.requestGC();
    // [ts] expect.poll(() => aliveCount()).toBeLessThanOrEqual(2); // two snapshot iframes
    // [ts] });
    // [ts] test("TV-088 should serve css without content-type", () => {
    // [ts] server.setRoute("/one-style.css", (req, res) => {
    // [ts] res.writeHead(200);
    // [ts] res.end(`body { background: red }`);
    // [ts] });
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] });
    // [ts] const snapshotFrame = traceViewer.snapshotFrame("Navigate");
    // [ts] expect(snapshotFrame.locator("body")).toHaveCSS("background-color", "rgb(255, 0, 0)", { timeout: 0 });
    // [ts] });
    // [ts] test("TV-089 canvas disabled title", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.PREFIX + "/screenshots/canvas.html#canvas-on-edge");
    // [ts] rafraf(page, 5);
    // [ts] });
    // [ts] const snapshot = traceViewer.snapshotFrame("Navigate");
    // [ts] expect(snapshot.locator("canvas")).toHaveAttribute("title", `Canvas content display is disabled.`);
    // [ts] });
    // [ts] test("TV-090 canvas clipping", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.PREFIX + "/screenshots/canvas.html#canvas-on-edge");
    // [ts] rafraf(page, 5);
    // [ts] });
    // Enable canvas display
    // [ts] traceViewer.showSettings();
    // [ts] const [msg] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] traceViewer.page.waitForEvent("console", { predicate: msg => msg.text().startsWith("canvas drawn:") }),
    // [ts] traceViewer.displayCanvasContentSetting.click(),
    // [ts] ]);
    // [ts] expect_eq(msg.text(), "canvas drawn: [0,91,11,20]");
    // [ts] const snapshot = traceViewer.snapshotFrame("Navigate");
    // [ts] expect(snapshot.locator("canvas")).toHaveAttribute("title", `Playwright couldn't capture full canvas contents because it's located partially outside the viewport.`);
    // [ts] });
    // [ts] test("TV-091 canvas clipping in iframe", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.setContent(`
    // [ts] <iframe src="${server.PREFIX}/screenshots/canvas.html#canvas-on-edge"></iframe>
    // [ts] `);
    // [ts] page.locator("iframe").contentFrame().locator("canvas").scrollIntoViewIfNeeded();
    // [ts] rafraf(page, 5);
    // [ts] });
    // Enable canvas display
    // [ts] traceViewer.showSettings();
    // [ts] const [msg] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] traceViewer.page.waitForEvent("console", { predicate: msg => msg.text().startsWith("canvas drawn:") }),
    // [ts] traceViewer.displayCanvasContentSetting.click(),
    // [ts] ]);
    // [ts] expect_eq(msg.text(), "canvas drawn: [1,1,11,20]");
    // [ts] const snapshot = traceViewer.snapshotFrame("Evaluate");
    // [ts] const canvas = snapshot.locator("iframe").contentFrame().locator("canvas");
    // [ts] expect(canvas).toHaveAttribute("title", "Canvas contents are displayed on a best-effort basis based on viewport screenshots taken during test execution.");
    // [ts] });
    // [ts] test("TV-092 should show only one pointer with multilevel iframes", () => {
    // [ts] server.setRoute("/level-0.html", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/html" });
    // [ts] res.end(`<iframe src="/level-1.html" style="position: absolute; left: 100px"></iframe>`);
    // [ts] });
    // [ts] server.setRoute("/level-1.html", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/html" });
    // [ts] res.end(`<iframe src="/level-2.html"></iframe>`);
    // [ts] });
    // [ts] server.setRoute("/level-2.html", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/html" });
    // [ts] res.end(`<button>Click me</button>`);
    // [ts] });
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.PREFIX + "/level-0.html");
    // [ts] page.frameLocator("iframe").frameLocator("iframe").locator("button").click({ position: { x: 5, y: 5 } });
    // [ts] });
    // [ts] const snapshotFrame = traceViewer.snapshotFrame("Click");
    // [ts] expect.soft(snapshotFrame.locator("x-pw-pointer")).toBeAttached();
    // [ts] expect.soft(snapshotFrame.frameLocator("iframe").locator("x-pw-pointer")).not.toBeAttached();
    // [ts] expect.soft(snapshotFrame.frameLocator("iframe").frameLocator("iframe").locator("x-pw-pointer")).not.toBeAttached();
    // [ts] });
    // [ts] test("TV-093 should show a popover", () => {
    // [ts] test(platform == "darwin" && macVersion == 13 && browserName == "webkit", "WebKit on macOS 13.7 reliably fails on this test for some reason");
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.setContent(`
    // [ts] <button popovertarget="pop">Click me</button>
    // [ts] <article id="pop" popover="auto">
    // [ts] <div>I'm a popover</div>
    // [ts] </article>
    // [ts] `);
    // [ts] page.getByRole("button").click();
    // [ts] expect(page.locator("div")).toBeVisible();
    // [ts] });
    // [ts] const snapshot = traceViewer.snapshotFrame("Expect");
    // [ts] const popover = snapshot.locator("#pop");
    // [ts] expect.poll(() => popover.evaluate(e => e.matches(":popover-open"))).toBe(true);
    // [ts] });
    // [ts] test("TV-094 should show a modal dialog", () => {
    // [ts] test(platform == "darwin" && macVersion == 13 && browserName == "webkit", "WebKit on macOS 13.7 reliably fails on this test for some reason");
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.setContent(`
    // [ts] <button>Show the dialog</button>
    // [ts] <dialog>
    // [ts] <p>This is a modal dialog</p>
    // [ts] </dialog>
    // [ts] <script>
    // [ts] document.querySelector('button').addEventListener('click', () => {
    // [ts] document.querySelector('dialog').showModal();
    // [ts] });
    // [ts] </script>
    // [ts] `);
    // [ts] page.getByRole("button").click();
    // [ts] expect(page.locator("p")).toBeVisible();
    // [ts] });
    // [ts] const snapshot = traceViewer.snapshotFrame("Expect");
    // [ts] const dialog = snapshot.locator("dialog");
    // [ts] expect.poll(() => dialog.evaluate(e => e.matches(":open"))).toBe(true);
    // [ts] expect.poll(() => dialog.evaluate(e => e.matches(":modal"))).toBe(true);
    // [ts] });
    // [ts] test("TV-095 should open settings dialog", () => {
    // [ts] const traceViewer = showTraceViewer(traceFile);
    // [ts] traceViewer.selectAction("Navigate");
    // [ts] traceViewer.showSettings();
    // [ts] expect(traceViewer.settingsDialog).toBeVisible();
    // [ts] });
    // [ts] test("TV-096 should toggle theme color", () => {
    // [ts] const traceViewer = showTraceViewer(traceFile);
    // [ts] traceViewer.selectAction("Navigate");
    // [ts] traceViewer.showSettings();
    // [ts] expect(traceViewer.themeSetting).toHaveValue("system");
    // [ts] traceViewer.themeSetting.selectOption("Dark mode");
    // [ts] expect(traceViewer.themeSetting).toHaveValue("dark-mode");
    // [ts] expect(traceViewer.page.locator("html")).toHaveClass("dark-mode");
    // [ts] traceViewer.themeSetting.selectOption("Light mode");
    // [ts] expect(traceViewer.themeSetting).toHaveValue("light-mode");
    // [ts] expect(traceViewer.page.locator("html")).toHaveClass("light-mode");
    // [ts] });
    // [ts] test("TV-097 should reflect system color scheme changes in document theme", () => {
    // [ts] const traceViewer = showTraceViewer(traceFile);
    // [ts] traceViewer.selectAction("Navigate");
    // [ts] expect(traceViewer.page.locator("html")).toHaveClass("light-mode");
    // [ts] traceViewer.page.emulateMedia({ colorScheme: "dark" });
    // [ts] expect(traceViewer.page.locator("html")).toHaveClass("dark-mode");
    // [ts] traceViewer.page.emulateMedia({ colorScheme: "no-preference" });
    // [ts] expect(traceViewer.page.locator("html")).toHaveClass("light-mode");
    // [ts] });
    // [ts] test("TV-098 should toggle canvas rendering", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(`data:text/html,<!DOCTYPE html><body><div>Hello world</div><canvas /></body>`);
    // [ts] page.goto(`data:text/html,<!DOCTYPE html><body><div>Hello world</div></body>`);
    // [ts] });
    // [ts] let snapshotRequestPromise = traceViewer.page.waitForRequest(request => request.url().includes("/snapshot/"));
    // Click on the action with a canvas snapshot
    // [ts] traceViewer.selectAction("Navigate", 0);
    // [ts] let snapshotRequest = snapshotRequestPromise;
    // [ts] expect_eq(std.mem.indexOf(u8, snapshotRequest.url(), "shouldPopulateCanvasFromScreenshot") == null, true);
    // [ts] traceViewer.showSettings();
    // [ts] expect(traceViewer.displayCanvasContentSetting).toBeChecked({ checked: false });
    // [ts] traceViewer.displayCanvasContentSetting.click();
    // [ts] expect(traceViewer.displayCanvasContentSetting).toBeChecked({ checked: true });
    // Deselect canvas
    // [ts] traceViewer.selectAction("Navigate", 1);
    // [ts] snapshotRequestPromise = traceViewer.page.waitForRequest(request => request.url().includes("/snapshot/"));
    // Select canvas again
    // [ts] traceViewer.selectAction("Navigate", 0);
    // [ts] snapshotRequest = snapshotRequestPromise;
    // [ts] expect_eq(std.mem.indexOf(u8, snapshotRequest.url(), "shouldPopulateCanvasFromScreenshot") != null, true);
    // [ts] });
    // [ts] test("TV-099 should display aria mode", () => {
    // [ts] let buttonBox: { x: number, y: number, width: number, height: number };
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.setContent("<!DOCTYPE html><button>Click me</button>");
    // [ts] buttonBox = (page.locator("button").boundingBox());
    // [ts] page.locator("button").click();
    // [ts] page.locator("button").press("Enter");
    // [ts] }, { snapshots: { dom: true, aria: true, screen: true } });
    // [ts] traceViewer.showSettings();
    // [ts] expect(traceViewer.displayAriaSetting).toBeChecked({ checked: false });
    // [ts] traceViewer.displayAriaSetting.click();
    // [ts] expect(traceViewer.displayAriaSetting).toBeChecked({ checked: true });
    // [ts] traceViewer.selectAction("Click");
    // [ts] const ariaModeView = traceViewer.page.locator(".aria-mode-view");
    // [ts] expect(ariaModeView.locator("img")).toBeVisible();
    // [ts] expect(ariaModeView).toContainText("button \"Click me\"");
    // The screenshot is scaled to fit into the available area.
    // [ts] const imgBox = (ariaModeView.locator("img").boundingBox());
    // [ts] const scale = imgBox.width / page.viewportSize().width;
    // [ts] const toImage = (point) => ({ x: imgBox.x + point.x * scale, y: imgBox.y + point.y * scale });
    // Hovering an aria node highlights its box on the screenshot, scaled to match the image.
    // [ts] const highlight = ariaModeView.locator(".aria-mode-highlight");
    // [ts] expect(highlight).not.toBeVisible();
    // [ts] ariaModeView.locator(".aria-mode-line", { hasText: "button \"Click me\"" }).hover();
    // [ts] expect(highlight).toBeVisible();
    // [ts] {
    // [ts] const highlightBox = (highlight.boundingBox());
    // [ts] const expected = toImage(buttonBox);
    // [ts] expect_eq(Math.abs(highlightBox.x - expected.x) < 2, true);
    // [ts] expect_eq(Math.abs(highlightBox.y - expected.y) < 2, true);
    // [ts] expect_eq(Math.abs(highlightBox.width - buttonBox.width * scale) < 2, true);
    // [ts] expect_eq(Math.abs(highlightBox.height - buttonBox.height * scale) < 2, true);
    // [ts] }
    // [ts] ariaModeView.locator("img").hover();
    // [ts] expect(highlight).not.toBeVisible();
    // The action point and the target box are rendered on the "Action" screenshot.
    // [ts] const actionPoint = ariaModeView.locator(".aria-mode-action-point");
    // [ts] const actionHighlight = ariaModeView.locator(".aria-mode-action-highlight");
    // [ts] expect(actionPoint).toBeVisible();
    // [ts] expect(actionHighlight).toBeVisible();
    // [ts] {
    // [ts] const pointBox = (actionPoint.boundingBox());
    // [ts] const expected = toImage({ x: buttonBox.x + buttonBox.width / 2, y: buttonBox.y + buttonBox.height / 2 });
    // [ts] expect_eq(Math.abs(pointBox.x + pointBox.width / 2 - expected.x) < 2, true);
    // [ts] expect_eq(Math.abs(pointBox.y + pointBox.height / 2 - expected.y) < 2, true);
    // [ts] const actionHighlightBox = (actionHighlight.boundingBox());
    // [ts] const expectedBox = toImage(buttonBox);
    // [ts] expect_eq(Math.abs(actionHighlightBox.x - expectedBox.x) < 2, true);
    // [ts] expect_eq(Math.abs(actionHighlightBox.y - expectedBox.y) < 2, true);
    // [ts] expect_eq(Math.abs(actionHighlightBox.width - buttonBox.width * scale) < 2, true);
    // [ts] expect_eq(Math.abs(actionHighlightBox.height - buttonBox.height * scale) < 2, true);
    // [ts] }
    // Keyboard actions have no point, but still highlight the target box.
    // [ts] traceViewer.selectAction("Press");
    // [ts] expect(actionHighlight).toBeVisible();
    // [ts] expect(actionPoint).not.toBeVisible();
    // [ts] {
    // [ts] const actionHighlightBox = (actionHighlight.boundingBox());
    // [ts] const expectedBox = toImage(buttonBox);
    // [ts] expect_eq(Math.abs(actionHighlightBox.x - expectedBox.x) < 2, true);
    // [ts] expect_eq(Math.abs(actionHighlightBox.y - expectedBox.y) < 2, true);
    // [ts] expect_eq(Math.abs(actionHighlightBox.width - buttonBox.width * scale) < 2, true);
    // [ts] expect_eq(Math.abs(actionHighlightBox.height - buttonBox.height * scale) < 2, true);
    // [ts] }
    // [ts] traceViewer.selectAction("Click");
    // The "Before" tab shows the state before the click, without the action point.
    // [ts] traceViewer.selectSnapshot("Before");
    // [ts] expect(ariaModeView.locator("img")).toBeVisible();
    // [ts] expect(ariaModeView).toContainText("button \"Click me\"");
    // [ts] expect(actionPoint).not.toBeVisible();
    // [ts] expect(actionHighlight).not.toBeVisible();
    // Toggling the setting off restores the DOM snapshot.
    // [ts] traceViewer.showSettings();
    // [ts] traceViewer.displayAriaSetting.click();
    // [ts] expect(ariaModeView).not.toBeVisible();
    // [ts] expect(traceViewer.snapshotContainer).toBeVisible();
    // [ts] });
    // [ts] test("TV-100 should disable aria setting when there are no aria snapshots", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.setContent("<!DOCTYPE html><button>Click me</button>");
    // [ts] page.locator("button").click();
    // [ts] }, { snapshots: { dom: true } });
    // [ts] traceViewer.selectAction("Click");
    // [ts] expect(traceViewer.snapshotContainer).toBeVisible();
    // [ts] traceViewer.showSettings();
    // [ts] expect(traceViewer.displayAriaSetting).toBeDisabled();
    // [ts] expect(traceViewer.displayAriaSetting).toBeChecked({ checked: false });
    // [ts] });
    // [ts] test("TV-101 should force aria mode when there are no dom snapshots", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.setContent("<!DOCTYPE html><button>Click me</button>");
    // [ts] page.locator("button").click();
    // [ts] }, { snapshots: { aria: true, screen: true } });
    // [ts] traceViewer.selectAction("Click");
    // [ts] expect(traceViewer.page.locator(".aria-mode-view")).toContainText("button \"Click me\"");
    // [ts] traceViewer.showSettings();
    // [ts] expect(traceViewer.displayAriaSetting).toBeDisabled();
    // [ts] expect(traceViewer.displayAriaSetting).toBeChecked({ checked: true });
    // [ts] });
    // [ts] test("TV-102 should notify parent when ready to receive messages", () => {
    // [ts] const traceViewer = showTraceViewer(null, { host: "localhost" });
    // [ts] const traceViewerURL = traceViewer.page.url();
    // [ts] using context = browser.newContext();
    // [ts] const host = context.newPage();
    // [ts] host.goto(server.EMPTY_PAGE);
    // [ts] const readyMessage = host.evaluate(traceViewerURL => {
    // [ts] return Promise.init(resolve => {
    // [ts] const traceViewerOrigin = URL.init(traceViewerURL).origin;
    // [ts] const iframe = document.createElement("iframe");
    // [ts] fn onMessage(event: MessageEvent) {
    // [ts] if (event.origin != traceViewerOrigin)
    // [ts] return;
    // [ts] if (event.source != iframe.contentWindow)
    // [ts] return;
    // [ts] if (event.data?.method != "ready")
    // [ts] return;
    // [ts] window.removeEventListener("message", onMessage);
    // [ts] iframe.remove();
    // [ts] resolve(event.data);
    // [ts] }
    // [ts] window.addEventListener("message", onMessage);
    // [ts] iframe.src = traceViewerURL;
    // [ts] document.body.appendChild(iframe);
    // [ts] });
    // [ts] }, traceViewerURL);
    // [ts] expect_eq(readyMessage, { method: "ready" });
    // [ts] });
    // [ts] test("TV-103 should render blob trace received from opener", () => {
    // [ts] const traceViewer = showTraceViewer(null, { host: "localhost" });
    // [ts] const traceViewerURL = traceViewer.page.url();
    // [ts] using context = browser.newContext();
    // [ts] const opener = context.newPage();
    // [ts] opener.goto(server.EMPTY_PAGE);
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] opener.waitForEvent("popup"),
    // [ts] opener.evaluate((_p) => {
    // [ts] return Promise.init(resolve => {
    // [ts] const traceViewerOrigin = URL.init(traceViewerURL).origin;
    // [ts] fn onMessage(event: MessageEvent) {
    // [ts] if (event.origin != traceViewerOrigin)
    // [ts] return;
    // [ts] const target = traceViewerWindow;
    // [ts] if (!target || event.source != target)
    // [ts] return;
    // [ts] if (event.data?.method != "ready")
    // [ts] return;
    // [ts] window.removeEventListener("message", onMessage);
    // [ts] const uint8Array = Uint8Array.from(atob(trace), c => c.charCodeAt(0));
    // [ts] target.postMessage({
    // [ts] method: "load",
    // [ts] params: {
    // [ts] trace: Blob.init([uint8Array], { type: "application/zip" }),
    // [ts] }
    // [ts] }, traceViewerOrigin);
    // [ts] resolve();
    // [ts] }
    // [ts] window.addEventListener("message", onMessage);
    // [ts] const traceViewerWindow = window.open(traceViewerURL);
    // [ts] if (!traceViewerWindow) {
    // [ts] window.removeEventListener("message", onMessage);
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Failed to open Trace Viewer');)
    // [ts] }
    // [ts] });
    // [ts] }, { traceViewerURL, trace: fs.readFileSync(traceFile, "base64") }),
    // [ts] ]);
    // [ts] expect(popup.locator(".drop-target")).not.toBeVisible();
    // [ts] expect(popup.locator(".action-title")).toHaveText([
    // [ts] "Create page",
    // [ts] "Navigate.*data:",
    // [ts] "toHaveTitle",
    // [ts] "toHaveURL",
    // [ts] "Set content",
    // [ts] "toHaveText.*locator",
    // [ts] "toBeHidden.*getByTestId",
    // [ts] "toBeHidden.*getByTestId",
    // [ts] "Evaluate",
    // [ts] "Evaluate",
    // [ts] "Click.*getByText\\('Click'\\)",
    // [ts] "Wait for navigation",
    // [ts] "Wait for event \"response\"",
    // [ts] "Wait for timeout",
    // [ts] "Navigate.*\\/frames\\/frame.html",
    // [ts] "Set viewport size",
    // [ts] "Hover",
    // [ts] "Close page",
    // [ts] ]);
    // [ts] });
    // [ts] test("TV-104 shouldn't render not-blob trace received from message", () => {
    // [ts] const traceViewer = showTraceViewer(null, { host: "localhost" });
    // [ts] expect(traceViewer.page.locator(".drop-target")).toBeVisible();
    // [ts] expect(traceViewer.actionTitles).not.toBeVisible();
    // [ts] traceViewer.page.evaluate(trace => {
    // [ts] window.postMessage({
    // [ts] method: "load",
    // [ts] params: {
    // [ts] trace,
    // [ts] }
    // [ts] }, "*");
    // [ts] }, fs.readFileSync(traceFile, "base64"));
    // [ts] expect(traceViewer.page.locator(".drop-target")).toBeVisible();
    // [ts] expect(traceViewer.actionTitles).not.toBeVisible();
    // [ts] });
    // [ts] test("TV-105 should not trip over complex urls in style tags", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/35681" },
    // [ts] }, (_p) => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.setContent(`
    // [ts] <html>
    // [ts] <head>
    // [ts] <style>
    // [ts] </style>
    // [ts] </head>
    // [ts] <body>
    // [ts] <div>hello</div>
    // [ts] <span>world</span>
    // [ts] </body>
    // [ts] </html>
    // [ts] `);
    // [ts] const css = `
    // [ts] div {
    // [ts] background-image: url(   'data:image/svg+xml,<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><defs><style>.cls-1{fill:none}</style></defs><title>Custom Title</title><polyline class="cls-1" points="1 2 3 4"/></svg>');
    // [ts] color: rgb(1, 2, 3);
    // [ts] }
    // [ts] span {
    // [ts] color: rgb(4, 5, 6);
    // [ts] background-image: url("data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'><defs><style>.cls-1{fill:none}</style></defs><title>Custom Title</title><polyline class='cls-1' points='1 2 3 4'/></svg>" );
    // [ts] }
    // [ts] `;
    // [ts] page.evaluate(css => document.querySelector("style").textContent = css, css);
    // [ts] });
    // [ts] const snapshot = traceViewer.snapshotFrame("Evaluate");
    // Just "hello world", no gibberish from the <style>.
    // [ts] expect(snapshot.locator("body")).toHaveText("hello world");
    // [ts] expect(snapshot.locator("div")).toHaveCSS("color", "rgb(1, 2, 3)");
    // [ts] expect(snapshot.locator("span")).toHaveCSS("color", "rgb(4, 5, 6)");
    // [ts] });
    // [ts] test("TV-106 should render locator descriptions", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.setContent("<input type=\"text\" />");
    // [ts] page.locator("input").describe("custom").click();
    // [ts] page.locator("input").describe("custom").first().click();
    // [ts] });
    // [ts] expect(traceViewer.page).toMatchAriaSnapshot(`
    // [ts] - treeitem /Click.*custom/
    // [ts] - treeitem /Click.*input.*first/
    // [ts] `);
    // [ts] });
    // [ts] test("TV-107 should load trace from HTTP with progress indicator", () => {
    // [ts] const [traceViewer, res] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] showTraceViewer(server.PREFIX),
    // [ts] Promise.init(resolve => {
    // [ts] server.setRoute("/", (req, res) => resolve(res));
    // [ts] }),
    // [ts] ]);
    // [ts] const file = fs.promises.readFile(traceFile);
    // [ts] const dialog = traceViewer.page.locator("dialog", { hasText: "Loading" });
    // [ts] res.setHeader("Access-Control-Allow-Origin", "*");
    // [ts] res.setHeader("Content-Length", file.byteLength);
    // [ts] res.writeHead(200);
    // Should become visible after ~200ms
    // [ts] expect(dialog).toBeVisible();
    // [ts] res.end(file);
    // [ts] expect(dialog).not.toBeVisible();
    // [ts] expect(traceViewer.actionTitles).toContainText(["Create page"]);
    // [ts] });
    // [ts] test("TV-108 should filter actions", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.route("**/*", async route => {
    // [ts] route.fulfill({ contentType: "text/html", body: "<input type=checkbox checked>" });
    // [ts] });
    // [ts] page.goto("https://does.not.exist");
    // [ts] page.locator("input").getAttribute("checked");
    // [ts] expect(page.locator("input")).toBeChecked();
    // [ts] });
    // [ts] expect(traceViewer.actionTitles).toHaveText([
    // [ts] "Navigate",
    // [ts] "Expect \"toBeChecked\"",
    // [ts] ]);
    // [ts] expect(traceViewer.page.getByText("3 hidden", { exact: true })).toBeVisible();
    // [ts] traceViewer.page.getByRole("button", { name: "Filter actions" }).click();
    // [ts] expect(traceViewer.page).toMatchAriaSnapshot(`
    // [ts] - dialog:
    // [ts] - checkbox "Getters 1"
    // [ts] - checkbox "Network routes 2"
    // [ts] - checkbox "Configuration"
    // [ts] `);
    // [ts] traceViewer.page.locator(".setting").getByText("Getters").click();
    // [ts] expect(traceViewer.actionTitles).toHaveText([
    // [ts] "Navigate",
    // [ts] "Get attribute \"checked\"",
    // [ts] "Expect \"toBeChecked\"",
    // [ts] ]);
    // [ts] expect(traceViewer.page.getByText("2 hidden", { exact: true })).toBeVisible();
    // [ts] traceViewer.page.locator(".setting").getByText("Network routes").click();
    // [ts] expect(traceViewer.actionTitles).toHaveText([
    // [ts] "Route requests",
    // [ts] "Navigate",
    // [ts] "Fulfill request",
    // [ts] "Get attribute \"checked\"",
    // [ts] "Expect \"toBeChecked\"",
    // [ts] ]);
    // [ts] });
    // [cap-test] test.describe(() => {
    // NOTE: In Firefox/WebKit, history.pushState() requires a SecureContext.
    // On http/about:blank it throws "The operation is insecure".
    // [ts] test.use({ ignoreHTTPSErrors: true });
    // [ts] test("TV-109 should handle failed snapshots due to dialog", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(httpsServer.EMPTY_PAGE);
    // [ts] page.setContent(`
    // [ts] <head>
    // [ts] <style>
    // [ts] button { color: red; }
    // [ts] </style>
    // [ts] </head>
    // [ts] <body>
    // [ts] <button>Click me</button>
    // [ts] <script>
    // [ts] const button = document.querySelector('button');
    // [ts] window.history.pushState({ page: 'stay' }, '', window.location.href);
    // [ts] window.addEventListener('popstate', () => {
    // [ts] if (window.confirm('ready?'))
    // [ts] button.textContent = 'Clicked';
    // [ts] });
    // [ts] </script>
    // [ts] </body>
    // [ts] `);
    // [ts] let dialogMessage = "";
    // [ts] page.on("dialog", async dialog => {
    // [ts] dialogMessage = dialog.message();
    // [ts] dialog.accept();
    // [ts] });
    // [ts] page.goBack();
    // [ts] expect.poll(() => dialogMessage).toBe("ready?");
    // [ts] expect(page.getByRole("button")).toHaveText("Clicked");
    // [ts] });
    // [ts] const frame = traceViewer.snapshotFrame("Expect");
    // [ts] expect(frame.getByRole("button")).toHaveCSS("color", "rgb(255, 0, 0)");
    // [ts] });
    // [cap-test] });
    // [ts] test("TV-110 should survive service worker restart", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("Old world");
    // [ts] page.evaluate(() => document.body.textContent = "New world");
    // [ts] });
    // [ts] const snapshot1 = traceViewer.snapshotFrame("Evaluate");
    // [ts] expect(snapshot1.locator("body")).toHaveText("New world");
    // [ts] const status = traceViewer.page.evaluate(() => {
    // [ts] const response = fetch("restartServiceWorker");
    // [ts] return response.status;
    // [ts] });
    // [ts] expect_eq(status, 200);
    // [ts] const snapshot2 = traceViewer.snapshotFrame("Set content");
    // [ts] expect(snapshot2.locator("body")).toHaveText("Old world");
    // [ts] });
    // [ts] test("TV-111 should not navigate on anchor clicks", () => {
    // [ts] const url = server.EMPTY_PAGE;
    // [ts] const traceViewer = runAndTrace(() => {
    // An invalid custom element name must not stop the snapshot script before it guards the links.
    // [ts] page.setContent(`
    // [ts] <body __playwright_custom_elements__="not a valid name">
    // [ts] <a href="${url}">link1</a>
    // [ts] <a href="${url}" target="_blank">link2</a>
    // [ts] <a href="${url}" target="_top">link3</a>
    // [ts] <map name="map"><area href="${url}" alt="link4" shape="default"></map>
    // [ts] <img usemap="#map" width="100" height="100">
    // [ts] </body>
    // [ts] `);
    // [ts] });
    // [ts] const snapshot = traceViewer.snapshotFrame("Set content");
    // [ts] const frame = snapshot.owner().elementHandle().then(handle => handle.contentFrame());
    // [ts] const checkLink = (name) => {
    // [ts] let newPageOpened = false;
    // [ts] traceViewer.page.context().on("page", () => {
    // [ts] newPageOpened = true;
    // [ts] });
    // [ts] snapshot.getByRole("link", { name }).click();
    // A middle click opens a link in a new tab.
    // [ts] snapshot.getByRole("link", { name }).click({ button: "middle" });
    // [ts] traceViewer.page.waitForTimeout(500);
    // [ts] expect(traceViewer.page).not.toHaveURL(url, { timeout: 500 });
    // [ts] expect_eq(frame.url() != url, true);
    // [ts] expect_eq(newPageOpened, false);
    // [ts] };
    // [ts] checkLink("link1");
    // [ts] checkLink("link2");
    // [ts] checkLink("link3");
    // [ts] snapshot.locator("area").dispatchEvent("click");
    // [ts] traceViewer.page.waitForTimeout(500);
    // [ts] expect_eq(frame.url() != url, true);
    // [ts] });
    // [ts] test("TV-112 should respect CSSOM changes", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.setContent("<style>button { color: red; }</style><button>Hello</button>");
    // [ts] page.evaluate(() => { (document.styleSheets[0].cssRules[0]).style.color = "blue"; });
    // [ts] page.setContent("<style>@media { button { color: red; } }</style><button>Hello</button>");
    // [ts] page.evaluate(() => {
    // [ts] window["rule"] = document.styleSheets[0].cssRules[0];
    // [ts] void 0;
    // [ts] });
    // [ts] page.evaluate(() => { window["rule"].cssRules[0].style.color = "black"; });
    // [ts] page.evaluate(() => { window["rule"].insertRule("button:not(.disabled) { color: green; }", 1); });
    // [ts] page.route("**/style.css", route => {
    // [ts] route.fulfill({ body: "button { color: red; }", }).catch(() => {});
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<link rel=\"stylesheet\" href=\"style.css\"><button>Hello</button>");
    // [ts] page.evaluate(() => { (document.styleSheets[0].cssRules[0]).style.color = "blue"; });
    // [ts] page.evaluate(() => "1 + 1");
    // [ts] page.evaluate(() => { document.styleSheets[0].disabled = true; });
    // [ts] });
    // [ts] const frame1 = traceViewer.snapshotFrame("Set content", 0);
    // [ts] expect(frame1.locator("button")).toHaveCSS("color", "rgb(255, 0, 0)");
    // [ts] const frame2 = traceViewer.snapshotFrame("Evaluate", 0);
    // [ts] expect(frame2.locator("button")).toHaveCSS("color", "rgb(0, 0, 255)");
    // [ts] const frame3 = traceViewer.snapshotFrame("Set content", 1);
    // [ts] expect(frame3.locator("button")).toHaveCSS("color", "rgb(255, 0, 0)");
    // [ts] const frame4 = traceViewer.snapshotFrame("Evaluate", 2);
    // [ts] expect(frame4.locator("button")).toHaveCSS("color", "rgb(0, 0, 0)");
    // [ts] const frame5 = traceViewer.snapshotFrame("Evaluate", 3);
    // [ts] expect(frame5.locator("button")).toHaveCSS("color", "rgb(0, 128, 0)");
    // [ts] const frame6 = traceViewer.snapshotFrame("Set content", 2);
    // [ts] expect(frame6.locator("button")).toHaveCSS("color", "rgb(255, 0, 0)");
    // [ts] const frame7 = traceViewer.snapshotFrame("Evaluate", 4);
    // [ts] expect(frame7.locator("button")).toHaveCSS("color", "rgb(0, 0, 255)");
    // [ts] const frame8 = traceViewer.snapshotFrame("Evaluate", 5);
    // [ts] traceViewer.page.waitForTimeout(1000); // give it time to render wrong color
    // [ts] expect(frame8.locator("button")).toHaveCSS("color", "rgb(0, 0, 255)");
    // [ts] const frame9 = traceViewer.snapshotFrame("Evaluate", 6);
    // [ts] expect(frame9.locator("button")).toHaveCSS("color", "rgb(0, 0, 0)");
    // [ts] });
    // [ts] test("TV-113 should preserve custom doctype", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.setContent("<!DOCTYPE foo><body>hi</body>");
    // [ts] });
    // [ts] const frame = traceViewer.snapshotFrame("Set content");
    // [ts] expect.poll(() => frame.locator("body").evaluate(() => document.doctype.name)).toBe("foo");
    // [ts] });
    // [ts] test("TV-114 should replace meta charset attr that specifies charset", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<meta charset=\"shift-jis\" />");
    // [ts] });
    // [ts] const frame = traceViewer.snapshotFrame("Set content");
    // [ts] expect.poll(() => frame.locator("body").evaluate(() => document.querySelector("meta")?.outerHTML.toLowerCase())).toBe("<meta charset=\"utf-8\">");
    // [ts] });
    // [ts] test("TV-115 should replace meta content attr that specifies charset", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=Shift_JIS\">");
    // [ts] });
    // [ts] const frame = traceViewer.snapshotFrame("Set content");
    // [ts] expect.poll(() => frame.locator("body").evaluate(() => document.querySelector("meta")?.outerHTML.toLowerCase())).toBe("<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\">");
    // [ts] });
    // [ts] test("TV-116 should drop meta http-equiv refresh during recording", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // Use a very large delay so the recorded page does not actually navigate before the snapshot is taken.
    // [ts] page.setContent(`
    // [ts] <head>
    // [ts] <meta http-equiv="refresh" content="999999; url=https://example.com/evil">
    // [ts] </head>
    // [ts] <body><div>safe</div></body>
    // [ts] `);
    // [ts] });
    // [ts] const frame = traceViewer.snapshotFrame("Set content");
    // [ts] expect(frame.locator("div")).toHaveText("safe");
    // Refresh META must be stripped from the snapshot, so the iframe is not navigated.
    // [ts] expect.poll(() => frame.locator("head").evaluate(head => head.querySelector("meta[http-equiv]")?.outerHTML.toLowerCase() ?? "")).toBe("");
    // The trace viewer top-level title and body must remain untouched.
    // [ts] expect(traceViewer.page).toHaveTitle("Playwright Trace Viewer");
    // [ts] expect(traceViewer.page.locator("body")).not.toHaveAttribute("data-pwned", ".*");
    // [ts] });
    // [ts] test("TV-117 should neutralize meta http-equiv refresh during rendering", () => {
    // Inject a META refresh directly via DOM manipulation that bypasses the
    // recording-time strip (so this test specifically validates the renderer-side defense).
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] const meta = document.createElement("meta");
    // [ts] meta.setAttribute("http-equiv", "refresh");
    // [ts] meta.setAttribute("content", "999999; url=https://example.com/evil");
    // [ts] document.head.appendChild(meta);
    // [ts] const div = document.createElement("div");
    // [ts] div.textContent = "safe";
    // [ts] document.body.appendChild(div);
    // [ts] });
    // [ts] });
    // [ts] const frame = traceViewer.snapshotFrame("Evaluate");
    // [ts] expect(frame.locator("div")).toHaveText("safe");
    // Even if a META refresh is in the recorded snapshot, the renderer must
    // neutralize the http-equiv and content attributes so it has no effect.
    // [ts] expect.poll(() => frame.locator("head").evaluate(head => !!head.querySelector("meta[http-equiv=\"refresh\"]"))).toBe(false);
    // [ts] expect(traceViewer.page).toHaveTitle("Playwright Trace Viewer");
    // [ts] });
    // [ts] test("TV-118 snapshots should be served with a script-src policy", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<div>hello</div>");
    // [ts] });
    // [ts] const frame = traceViewer.snapshotFrame("Set content");
    // [ts] expect(frame.locator("div")).toHaveText("hello");
    // [ts] const scripted = frame.locator("body").evaluate(body => {
    // [ts] const script = body.ownerDocument.createElement("script");
    // [ts] script.textContent = `document.body.setAttribute('data-scripted', 'yes')`;
    // [ts] body.appendChild(script);
    // [ts] return body.getAttribute("data-scripted");
    // [ts] });
    // [ts] expect_eq(scripted, null);
    // [ts] });
    // [ts] test("TV-119 snapshot iframes should be sandboxed", () => {
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<div>hello</div>");
    // [ts] });
    // [ts] traceViewer.snapshotFrame("Set content");
    // [ts] const sandboxValues = traceViewer.page.locator("iframe[name=snapshot]").evaluateAll(frames => frames.map(f => f.getAttribute("sandbox")));
    // [ts] for (const value of sandboxValues) {
    // [ts] expect_eq(value, true);
    // [ts] expect_eq(std.mem.indexOf(u8, value, "allow-same-origin") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, value, "allow-scripts") != null, true);
    // [ts] }
    // [ts] });
    // [ts] test("TV-120 should capture iframe with srcdoc", () => {
    // [ts] page.route("**/empty.html", route => {
    // [ts] void route.fulfill({
    // [ts] body: `<iframe srcdoc="<button>Hello iframe</button>"></iframe>`,
    // [ts] contentType: "text/html"
    // [ts] }).catch(() => {});
    // [ts] });
    // [ts] const traceViewer = runAndTrace(() => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(page.frameLocator("iframe").locator("button")).toHaveText("Hello iframe");
    // [ts] page.evaluate("2+2");
    // [ts] });
    // [ts] const frame = traceViewer.snapshotFrame("Evaluate");
    // [ts] expect(frame.frameLocator("iframe").getByRole("button")).toHaveText("Hello iframe");
    // [ts] });
    // [ts] test("TV-121 should render snapshots from the second chunk", () => {
    // [ts] context.tracing.start({ screenshots: true, snapshots: true });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<button>Click</button>");
    // [ts] context.tracing.startChunk();
    // [ts] page.click("\"Click\"");
    // [ts] context.tracing.stopChunk({ path: testInfo.outputPath("trace1.zip") });
    // [ts] context.tracing.startChunk();
    // [ts] page.hover("\"Click\"");
    // [ts] context.tracing.stopChunk({ path: testInfo.outputPath("trace2.zip") });
    // Snapshots in the second chunk must be self-contained and not reference
    // snapshot state from the first chunk.
    // [ts] const traceViewer = showTraceViewer(testInfo.outputPath("trace2.zip"));
    // [ts] const frame = traceViewer.snapshotFrame("Hover");
    // [ts] expect(frame.locator("button")).toHaveText("Click");
    // [ts] });
    // [ts] test("TV-122 take trace paths via stdin", () => {
    // [ts] const traceViewer = showTraceViewer(null, { stdin: true });
    // [ts] expect(traceViewer.page).toHaveTitle("Playwright Trace Viewer");
    // [ts] traceViewer.process.write(traceFile);
    // [ts] expect(traceViewer.actionTitles).toContainText([
    // [ts] "Create page",
    // [ts] ]);
    // [ts] });
    // [ts] }
    // [ts] }
}
// TODO(cap-migrate): refine TS statement
// [ts] }
