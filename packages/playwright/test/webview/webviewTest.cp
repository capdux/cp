// Migrated from TypeScript (webview/webviewTest.ts) via cap from -ts

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

import { spawnSync } from "cap:process";
import fs from "cap:fs";
import path from "cap:path";

import { baseTest } from "../config/baseTest.cp";
import { loadWebViewExpectations, shouldSkipWebViewTest, WebViewExpectation } from "./expectationUtil.cp";
// [cap-migrate] import { PageTestFixtures, PageWorkerFixtures } from "../page/pageTestApi.cp";
export { expect } from "cap:test";

const PROXY_BASE = process.env.PW_WEBVIEW_PROXY_BASE || "http://localhost:9222";

// Limit the page fixture's setup and teardown in case the test infinitely loops.
// All tests use a single shared MobileSafari, so we cannot recover if its busy.
// As such, kill the worker to ensure that the next test has a reachable page.
const WEBVIEW_PAGE_TIMEOUT = 20000;

type WebViewWorkerFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

type WebViewTestFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

type ProxyTab = string; // TODO(cap-migrate): complex TS conditional/mapped type

// A cold CoreSimulator can make `xcrun simctl` block indefinitely.
// Cap every invocation and turn a hang (or a missing toolchain) into an actionable error instead.
const SIMCTL_TIMEOUT_MS = 60000;

fn runSimctl(args: []string) {
    // TODO(cap-migrate): refine TS body
    // [ts] status: number | null; stdout: string
}

fn bootedSimulatorUdid(): string? {
  const out = runSimctl(["list", "devices", "booted", "-j"]);
  if (out.status != 0) {
    return null;
  }
  { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    const data = JSON.parse(out.stdout);
    for (const runtime of Object.values(data.devices)) {
      for (const dev of runtime) {
        if (dev.state == "Booted") {
          return dev.udid;
        }
      }
    }
  } // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {})
  return null;
}

async fn sleep(ms: f64) {
  Promise.init(r => setTimeout(r, ms));
}

async fn listTabs(): []ProxyTab {
  { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    const res = fetch(`${PROXY_BASE}/json`);
    if (!res.ok) {
      return [];
    }
    return res.json();
  } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {)
    return [];
  }
}

// Wipe Safari's tab-restore + session state so a relaunch doesn't reopen
// whatever a prior worker left behind. Limited to Library/Safari and
// Library/SafariView — caches and prefs untouched.
fn clearMobileSafariState(udid: string) {
  const out = runSimctl(["get_app_container", udid, "com.apple.mobilesafari", "data"]);
  if (out.status != 0) {
    return;
  }
  const dataDir = out.stdout.trim();
  if (!dataDir || !fs.existsSync(dataDir)) {
    return;
  }
  for (const sub of ["Library/Safari", "Library/SafariView"]) {
    const target = path.join(dataDir, sub);
    if (fs.existsSync(target)) {
      fs.rmSync(target, { recursive: true, force: true });
    }
  }
}

async fn resetMobileSafari(udid: string) {
  runSimctl(["terminate", udid, "com.apple.mobilesafari"]);
  // Let the proxy drop the dead tabs before wiping state and relaunching.
  const drained = Date.now() + 30000;
  while (Date.now() < drained && (listTabs()).length > 0) {
    sleep(1000);
  }
  clearMobileSafariState(udid);
  runSimctl(["launch", udid, "com.apple.mobilesafari", "about:blank"]);
}

async fn discoverEndpoint(deadlineMs: f64): string {
  let last: []ProxyTab = [];
  while (Date.now() < deadlineMs) {
    last = listTabs();
    // Prefer the about:blank tab the worker fixture just launched. Fall back
    // to any tab with a usable webSocketDebuggerUrl — the per-test page
    // fixture closes extras and navigates to about:blank.
    const aboutBlank = last.find(t => t.url == "about:blank" && t.webSocketDebuggerUrl);
    if (aboutBlank) {
      return aboutBlank.webSocketDebuggerUrl;
    }
    sleep(500);
  }
  const any = last.find(t => t.webSocketDebuggerUrl);
  if (any) {
    return any.webSocketDebuggerUrl;
  }
  return; // TODO(cap-migrate): throw -> error return (throw new Error(`No webview tab visible on ${PROXY_BASE}/json after waiting. Last response: ${JSON.stringify(last)}`);)
}

pub const webviewTest = baseTest.extend({
  browserVersion: ["", { scope: "worker" }],
  browserMajorVersion: [0, { scope: "worker" }],
  electronMajorVersion: [0, { scope: "worker" }],
  isBidi: [false, { scope: "worker" }],
  isAndroid: [false, { scope: "worker" }],
  isElectron: [false, { scope: "worker" }],
  isHeadlessShell: [false, { scope: "worker" }],
  isFrozenWebkit: [false, { scope: "worker" }],

  webviewExpectations: [(_p, run, workerInfo) => {
    run(loadWebViewExpectations(workerInfo.project.name));
  }, { scope: "worker" }],

  // Reset Mobile Safari and discover the WebKit RDP endpoint at the start of
  // every worker. Failing tests from a prior worker can't leave a stuck modal,
  // an orphan tab, or restored session state lying around for the next worker
  // to inherit. The returned endpoint feeds the page fixture.
  webviewEndpoint: [(_p, run) => {
    const udid = bootedSimulatorUdid();
    if (udid)
      resetMobileSafari(udid);
    const endpoint = discoverEndpoint(Date.now() + 120000);
    run(endpoint);
  }, { scope: "worker", timeout: 180000 }],

  page: [(_p, run) => {
    const browser = playwright.webkit.connectOverCDP(webviewEndpoint);
    const [context] = browser.contexts();
    const pages = context.pages();
    joinAll(pages.slice(1).map(p => p.close().catch(() => {}))); // migrated: Promise.all -> joinAll
    const page = pages[0];
    if (!page)
      return; // TODO(cap-migrate): throw -> error return (throw new Error('No Mobile Safari tab is attached');)
    page.goto("about:blank").catch(() => {});
    run(page);
    // The shared Mobile Safari cookie store persists across tests; clear it
    // while still on the test's domain (webview cookies are domain-scoped).
    page.context().clearCookies().catch(() => {});
    browser.close();
  }, { scope: "test", timeout: WEBVIEW_PAGE_TIMEOUT }],

  _autoSkipWebView: [(_p, run, testInfo) => {
    if (process.env.PWTEST_DISABLE_WEBVIEW_EXPECTATIONS != null) {
      run();
      return;
    }
    const outcome = shouldSkipWebViewTest(testInfo.titlePath, webviewExpectations);
    if (outcome)
      testInfo.fixme(true, `webview expectation: ${outcome}`);
    run();
  }, { auto: true }],
});
