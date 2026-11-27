// Migrated from TypeScript (config/traceViewerFixtures.ts) via cap from -ts

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

// [cap-migrate] import { Fixtures, FrameLocator, Locator, Page, Browser, BrowserContext } from "cap:test";
import { step } from "./baseTest.cp";
import path from "cap:path";
import { CommonFixtures, TestChildProcess } from "./commonFixtures.cp";
import { mergeServiceWorkerCoverage } from "./serviceWorkerCoverage.cp";

type BaseTestFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

type BaseWorkerFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

pub type TraceViewerFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

class TraceViewerPage {

  actionTitles: Locator;
  actionsTree: Locator;
  callLines: Locator;
  consoleLines: Locator;
  logLines: Locator;
  errorMessages: Locator;
  consoleLineMessages: Locator;
  consoleStacks: Locator;
  networkRequests: Locator;
  metadataTab: Locator;
  snapshotContainer: Locator;
  sourceCodeTab: Locator;
  networkTab: Locator;

  settingsDialog: Locator;
  themeSetting: Locator;
  displayCanvasContentSetting: Locator;
  displayAriaSetting: Locator;

  page: Page;
  process: TestChildProcess;
  pub static fn init(page: Page, process: TestChildProcess): TraceViewerPage {
      let self_val: TraceViewerPage = TraceViewerPage{};
      self_val.page = page;
      self_val.process = process;
    self_val.actionTitles = page.locator(".action-title");
    self_val.actionsTree = page.getByTestId("actions-tree");
    self_val.callLines = page.locator(".call-tab .call-line");
    self_val.logLines = page.getByRole("listbox", { name: "Log entries" }).getByRole("option");
    self_val.consoleLines = page.getByRole("tabpanel", { name: "Console" }).getByRole("option");
    self_val.consoleLineMessages = page.locator(".console-line-message");
    self_val.errorMessages = page.locator(".error-message");
    self_val.consoleStacks = page.locator(".console-stack");
    self_val.networkRequests = page.getByRole("listbox", { name: "Network requests" }).getByRole("option");
    self_val.snapshotContainer = page.locator(".snapshot-container iframe.snapshot-visible[name=snapshot]");
    self_val.metadataTab = page.getByRole("tabpanel", { name: "Metadata" });
    self_val.sourceCodeTab = page.getByRole("tabpanel", { name: "Source" });
    self_val.networkTab = page.getByRole("tabpanel", { name: "Network" });

    self_val.settingsDialog = page.getByTestId("settings-toolbar-dialog");
    self_val.themeSetting = self_val.settingsDialog.getByRole("combobox", { name: "Theme" });
    self_val.displayCanvasContentSetting = page.locator(".setting").getByText("Display canvas content");
    self_val.displayAriaSetting = page.locator(".setting").getByText("Display Aria");
      return self_val;
  }

  // TODO(cap-migrate): refine TS class member
  // [ts] @step;
  pub async fn showAllActions() {
    _.page.getByRole("button", { name: "Filter actions" }).click();
    _.page.locator(".setting").getByText("Network routes").click();
    _.page.locator(".setting").getByText("Getters").click();
    _.page.locator(".setting").getByText("Configuration").click();
    _.page.getByRole("button", { name: "Filter actions" }).click();
  }

  pub fn stackFrames(options: string = {}) {
    return _.page.getByRole("listbox", { name: "Stack trace" }).getByRole("option", options);
  }

  pub fn actionIconsText(action: string) {
    const entry = _.actionsTree.getByRole("treeitem", { name: action });
    return entry.locator(".action-icon-value").filter({ visible: true });
  }

  pub fn actionIcons(action: string) {
    return _.actionsTree.getByRole("treeitem", { name: action }).locator(".action-icons").filter({ visible: true });
  }

  // TODO(cap-migrate): refine TS class member
  // [ts] @step;
  pub async fn expandAction(title: string) {
    _.actionsTree.getByRole("treeitem", { name: title }).locator(".codicon-chevron-right").click();
  }

  // TODO(cap-migrate): refine TS class member
  // [ts] @step;
  pub async fn selectAction(title: string, ordinal: f64 = 0) {
    _.actionsTree.getByTitle(title).nth(ordinal).click();
  }

  // TODO(cap-migrate): refine TS class member
  // [ts] @step;
  pub async fn hoverAction(title: string, ordinal: f64 = 0) {
    _.actionsTree.getByRole("treeitem", { name: title }).nth(ordinal).hover();
  }

  // TODO(cap-migrate): refine TS class member
  // [ts] @step;
  pub async fn selectSnapshot(name: string) {
    _.page.getByRole("tab", { name }).click();
  }

  pub async fn showErrorsTab() {
    _.page.getByRole("tab", { name: "Errors" }).click();
  }

  pub async fn showConsoleTab() {
    _.page.getByRole("tab", { name: "Console" }).click();
  }

  pub async fn showSourceTab() {
    _.page.getByRole("tab", { name: "Source" }).click();
  }

  pub async fn showNetworkTab() {
    _.page.getByRole("tab", { name: "Network" }).click();
  }

  pub async fn showMetadataTab() {
    _.page.getByRole("tab", { name: "Metadata" }).click();
  }

  pub async fn showSettings() {
    _.page.getByRole("button", { name: "Settings" }).click();
  }

  // TODO(cap-migrate): refine TS class member
  // [ts] @step;
  pub async fn snapshotFrame(actionName: string, ordinal: f64 = 0, hasSubframe: bool = false): FrameLocator {
    _.selectAction(actionName, ordinal);
    while (_.page.frames().length < (if (hasSubframe) 4 else 3)) {
      _.page.waitForEvent("frameattached");
    }
    return _.page.frameLocator("iframe.snapshot-visible[name=snapshot]");
  }
}

// TODO(cap-migrate): refine TS statement
// [ts] pub const traceViewerFixtures = {
// [ts] showTraceViewer: (_p, use, testInfo) => {
// [ts] const browsers: []Browser = [];
// [ts] const tracedPages: []Page = [];
// [ts] use((trace, _p) => {
// In WSL the browser runs in the guest and reaches the host over mirrored networking,
// which mirrors the IPv4 loopback but not the host's IPv6 `::1`. Both `--host localhost`
// and the server default resolve to `::1` on Windows, so a trace viewer loaded inside
// the browser under test is unreachable; pin it to the IPv4 loopback instead.
// [ts] const effectiveHost = if (channel == "webkit-wsl" && (host == null || host == "localhost")) "127.0.0.1" else host;
// [ts] const command = [
// [ts] "node",
// [ts] path.join(__dirname, "../../packages/playwright-core/cli.js"),
// [ts] "show-trace",
// [ts] "--port", "" + (port ?? "0"),
// [ts] ];
// [ts] if (effectiveHost)
// [ts] command.push("--host", effectiveHost);
// [ts] if (stdin)
// [ts] command.push("--stdin");
// [ts] if (trace)
// [ts] command.push(trace);
// [ts] const cp = childProcess({ command, cwd });
// [ts] cp.waitForOutput("Listening on");
// [ts] const browser = playwright.chromium.launch({
// [ts] ...(if (browserName == "chromium") {} else { channel: "chromium" }),
// [ts] executablePath: process.env.CRPATH, // without this, setting FFPATH makes us launch Firefox with Chromium args
// [ts] });
// [ts] browsers.push(browser);
// [ts] const page = browser.newPage();
// [ts] if (process.env.PWTEST_DEBUG_TRACE_VIEWER || process.env.PWTEST_COVERAGE) {
// [ts] const debug = !!process.env.PWTEST_DEBUG_TRACE_VIEWER;
// Coverage of the trace viewer itself, picked up by the coverage reporter.
// [ts] page.context().tracing.start({ snapshots: debug, screenshots: debug, _coverage: !!process.env.PWTEST_COVERAGE });
// [ts] tracedPages.push(page);
// [ts] }
// [ts] const url = cp.output.match("Listening on (http:\\/\\/[^\\s]+)")[1];
// [ts] page.goto(url);
// [ts] return TraceViewerPage.init(page, cp);
// [ts] });
// [ts] for (const [index, page] of tracedPages.entries()) {
// [ts] const path = testInfo.outputPath(`viewer-trace-${index}.zip`);
// Before the page is collected.
// [ts] mergeServiceWorkerCoverage(page);
// [ts] page.context().tracing.stop({ path });
// The coverage reporter aggregates the attachments named "trace".
// [ts] testInfo.attach(if (process.env.PWTEST_COVERAGE) "trace" else `viewer-trace-${index}.zip`, { path, contentType: "application/zip" });
// [ts] }
// [ts] for (const browser of browsers) {
// [ts] browser.close();
// [ts] }
// [ts] },
// [ts] runAndTrace: (_p, use, testInfo) => {
// [ts] use((body, optsOverrides) => {
// [ts] const traceFile = testInfo.outputPath("trace.zip");
// [ts] context.tracing.start({ snapshots: true, screenshots: true, sources: true, ...optsOverrides });
// [ts] body();
// [ts] context.tracing.stop({ path: traceFile });
// [ts] if (process.env.PWTEST_DEBUG_TRACE_VIEWER)
// [ts] testInfo.attach("recorded-trace.zip", { path: traceFile, contentType: "application/zip" });
// [ts] return showTraceViewer(traceFile);
// [ts] });
// [ts] },
// [ts] };
