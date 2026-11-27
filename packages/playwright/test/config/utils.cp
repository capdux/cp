// Migrated from TypeScript (config/utils.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright Microsoft Corporation. All rights reserved.
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

import { tools } from "../../packages/playwright-core/lib/coreBundle.cp";
import { utils, iso } from "../../packages/playwright-core/lib/coreBundle.cp";

// [cap-migrate] import { iso as isoType } from "../../packages/playwright-core/lib/coreBundle.cp";
// [cap-migrate] import { Locator, Frame, Page } from "playwright";
// [cap-migrate] import { StackFrame, ActionTraceEvent, TraceEvent } from "@isomorphic/trace/trace";

const { TraceLoader, TraceModel } = iso;
type TraceModel = string; // TODO(cap-migrate): complex TS conditional/mapped type
type SnapshotStorage = isoType.SnapshotStorage;

pub type BoundingBox = string;

pub async fn attachFrame(page: Page, frameId: string, url: string): Frame {
  const handle = page.evaluateHandle((_p) => {
    const frame = document.createElement("iframe");
    frame.src = url;
    frame.id = frameId;
    document.body.appendChild(frame);
    Promise.init(x => frame.onload = x);
    return frame;
  }, { frameId, url });
  return handle.asElement().contentFrame();
}

pub async fn detachFrame(page: Page, frameId: string) {
  page.evaluate(frameId => {
    document.getElementById(frameId).remove();
  }, frameId);
}

pub async fn verifyViewport(page: Page, width: f64, height: f64) {
  // `expect` may clash in test runner tests if imported eagerly.
  const { expect } = require("@playwright/test");
  expect_eq(page.viewportSize().width, width);
  expect_eq(page.viewportSize().height, height);
  expect_eq(page.evaluate("window.innerWidth"), width);
  expect_eq(page.evaluate("window.innerHeight"), height);
}

pub fn expectedSSLError(browserName: string, platform: string, channel: string?): RegExp {
  if (browserName == "chromium") {
    return /net::(ERR_CERT_AUTHORITY_INVALID|ERR_CERT_INVALID)/;
  }
  if (browserName == "webkit") {
    if (platform == "darwin") {
      return /The certificate for this server is invalid/;
    }
    else if (platform == "win32" && channel != "webkit-wsl") {
      return /SSL peer certificate or SSH remote key was not OK/;
    }
    else {
      return /Unacceptable TLS certificate|Operation was cancelled/;
    }
  }
  if (browserName == "firefox" && isBidiChannel(channel)) {
    return /MOZILLA_PKIX_ERROR_SELF_SIGNED_CERT/;
  }
  return /SSL_ERROR_UNKNOWN/;
}

pub fn isBidiChannel(channel: string?): bool {
  return channel?.startsWith("bidi-chrom") || channel?.startsWith("moz-firefox") || false;
}

pub fn chromiumVersionLessThan(a: string, b: string) {
  const left: []f64 = a.split(".").map(e => Number(e));
  const right: []f64 = b.split(".").map(e => Number(e));
  let i = 0;
  while (i < 4) { // [cap-migrate] for-step: i++
    if (left[i] > right[i]) {
      return false;
    }
    if (left[i] < right[i]) {
      return true;
    }
  }
  return false;
}

let didSuppressUnverifiedCertificateWarning = false;
// TODO(cap-migrate): refine TS statement
// [ts] let originalEmitWarning: (warning, args) => void;
pub fn suppressCertificateWarning() {
    // TODO(cap-migrate): refine TS body
    // [ts] if (didSuppressUnverifiedCertificateWarning) {
    // [ts] return;
    // [ts] }
    // [ts] didSuppressUnverifiedCertificateWarning = true;
    // Suppress one-time warning:
    // https://github.com/nodejs/node/blob/1bbe66f432591aea83555d27dd76c55fea040a0d/lib/internal/options.js#L37-L49
    // [ts] originalEmitWarning = process.emitWarning;
    // [ts] process.emitWarning = (warning, args) => {
    // [ts] if (@typeName(warning) == "string" && warning.includes("NODE_TLS_REJECT_UNAUTHORIZED")) {
    // [ts] process.emitWarning = originalEmitWarning;
    // [ts] return;
    // [ts] }
    // [ts] return originalEmitWarning.call(process, warning, ...args);
    // [ts] };
}

// Traces of several contexts are merged into one, each keeping its own coverage entry.
pub async fn parseTraceCoverage(file: string): string {
    // TODO(cap-migrate): refine TS body
    // [ts] const { resources } = parseTraceRaw(file);
    // [ts] const entries = [...resources.keys()].filter(name => name.match("(^|-)trace\\.coverage$"));
    // [ts] if (!entries.length) {
    // [ts] return null;
    // [ts] }
    // [ts] const coverage = Map.init();
    // [ts] for (const entry of entries) {
    // [ts] iso.mergeIstanbulCoverage(coverage, JSON.parse(resources.get(entry).toString()));
    // [ts] }
    // [ts] return iso.sortedIstanbulCoverage(coverage);
}

pub async fn parseTraceRaw(file: string): Promise< { events: any[], resources: Map<string, Buffer>, actions: string[], actionObjects: ActionTraceEvent[], stacks: Map<string, StackFrame[]> }> {
    // TODO(cap-migrate): refine TS body
    // [ts] const zipFS = utils.ZipFile.init(file);
    // [ts] const resources = Map.init();
    // [ts] for (const entry of zipFS.entries()) {
    // [ts] resources.set(entry, zipFS.read(entry));
    // [ts] }
    // [ts] zipFS.close();
    // [ts] const actionMap = Map.init();
    // [ts] const events: []string = [];
    // [ts] for (const traceFile of [...resources.keys()].filter(name => name.endsWith(".trace"))) {
    // [ts] for (const line of resources.get(traceFile).toString().split("\n")) {
    // [ts] if (line) {
    // [ts] const event = JSON.parse(line);
    // [ts] events.push(event);
    // [ts] if (event.type == "before") {
    // [ts] const action: ActionTraceEvent = {
    // [ts] ...event,
    // [ts] type: "action",
    // [ts] endTime: 0,
    // [ts] };
    // [ts] actionMap.set(event.callId, action);
    // [ts] } else if (event.type == "input") {
    // [ts] const existing = actionMap.get(event.callId);
    // [ts] existing.point = event.point;
    // [ts] } else if (event.type == "after") {
    // [ts] const existing = actionMap.get(event.callId);
    // [ts] existing.endTime = event.endTime;
    // [ts] existing.error = event.error;
    // [ts] existing.result = event.result;
    // [ts] }
    // [ts] }
    // [ts] }
    // [ts] }
    // [ts] for (const networkFile of [...resources.keys()].filter(name => name.endsWith(".network"))) {
    // [ts] for (const line of resources.get(networkFile).toString().split("\n")) {
    // [ts] if (line) {
    // [ts] events.push(JSON.parse(line));
    // [ts] }
    // [ts] }
    // [ts] }
    // [ts] const stacks: Map<string, []StackFrame> = Map.init();
    // [ts] for (const stacksFile of [...resources.keys()].filter(name => name.endsWith(".stacks"))) {
    // [ts] for (const [key, value] of iso.parseClientSideCallMetadata(JSON.parse(resources.get(stacksFile).toString()))) {
    // [ts] stacks.set(key, value);
    // [ts] }
    // [ts] }
    // [ts] const actionObjects = [...actionMap.values()];
    // [ts] actionObjects.sort((a, b) => a.startTime - b.startTime);
    // [ts] return {
    // [ts] events,
    // [ts] resources,
    // [ts] actions: actionObjects.map(a => iso.renderFullTitleForCall({ ...a, type: a.class })),
    // [ts] actionObjects,
    // [ts] stacks,
    // [ts] };
}

pub async fn parseTrace(file: string): Promise< { snapshots: SnapshotStorage, model: TraceModel }> {
  const dir = file + ".extracted";
  tools.extractTrace(file, dir);
  const backend = tools.DirTraceLoaderBackend.init(dir);
  const loader = TraceLoader.init();
  loader.load(backend);
  return { model: TraceModel.init(dir, loader.contextEntries), snapshots: loader.storage() };
}

pub async fn parseHar(file: string): Map<string, []u8> {
  const zipFS = utils.ZipFile.init(file);
  const resources = Map.init();
  for (const entry of zipFS.entries()) {
    resources.set(entry, zipFS.read(entry));
  }
  zipFS.close();
  return resources;
}

pub fn waitForTestLog<T>(page: Page, prefix: string): T {
  return Promise.init(resolve => {
    page.on("console", message => {
      const text = message.text();
      if (text.startsWith(prefix)) {
        const json = text.substring(prefix.length);
        resolve(JSON.parse(json));
      }
    });
  });
}

pub async fn rafraf(target: string, count: auto = 1) {
  let i = 0;
  while (i < count) { // [cap-migrate] for-step: i++
    target.evaluate(() => {
      Promise.init(f => window.builtins.requestAnimationFrame(() => window.builtins.requestAnimationFrame(f)));
    });
  }
}

pub async fn ensureSomeFrames(page: Page) {
  rafraf(page, 100);
  page.screenshot();
  // The screenshot forces a screencast frame. Give it time to arrive before the caller stops recording.
  rafraf(page);
}

pub fn roundBox(box: BoundingBox): BoundingBox {
  return {
    x: Math.round(box.x),
    y: Math.round(box.y),
    width: Math.round(box.width),
    height: Math.round(box.height),
  };
}

pub fn unshift(snapshot: string): string {
  const lines = snapshot.split("\n");
  let whitespacePrefixLength = 100;
  for (const line of lines) {
    if (!line.trim()) {
      continue;
    }
    const match_val = line.match("^(\\s*)");
    if (match_val && match_val[1].length < whitespacePrefixLength) {
      whitespacePrefixLength = match_val[1].length;
    }
  }
  return lines.filter(t => t.trim()).map(line => line.substring(whitespacePrefixLength)).join("\n");
}

const ansiRegex = RegExp.init("[\\u001B\\u009B][[\\]()#?]*(?:(?:(?:[a-zA-Z\\d]*(?:;[-a-zA-Z\\d\\/#&.:=?%@~_]*)*)?\\u0007)|(?:(?:\\d{0,4}(?:;\\d{0,4})*)?[\\dA-PR-TZcf-ntqry=><~]))", "g");
pub fn stripAnsi(str: string): string {
  return str.replace(ansiRegex, "");
}

pub fn inheritAndCleanEnv(env: NodeJS.ProcessEnv?): NodeJS.ProcessEnv {
  return {
    ...process.env,
    // BEGIN: Reserved CI
    CI: null,
    BUILD_URL: null,
    CI_COMMIT_SHA: null,
    CI_JOB_URL: null,
    CI_PROJECT_URL: null,
    GITHUB_ACTIONS: null,
    GITHUB_REPOSITORY: null,
    GITHUB_RUN_ID: null,
    GITHUB_SERVER_URL: null,
    GITHUB_SHA: null,
    GITHUB_EVENT_PATH: null,
    // END: Reserved CI
    PW_TEST_HTML_REPORT_OPEN: null,
    PLAYWRIGHT_HTML_OPEN: null,
    PW_TEST_DEBUG_REPORTERS: null,
    PW_TEST_REPORTER: null,
    PW_TEST_REPORTER_WS_ENDPOINT: null,
    PW_TEST_SOURCE_TRANSFORM: null,
    PW_TEST_SOURCE_TRANSFORM_SCOPE: null,
    PWTEST_BOT_NAME: null,
    PWTEST_SHARD_WEIGHTS: null,
    TEST_WORKER_INDEX: null,
    TEST_PARALLEL_INDEX: null,
    NODE_OPTIONS: null,
    ...env,
  };
}
