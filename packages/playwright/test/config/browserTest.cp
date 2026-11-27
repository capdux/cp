// Migrated from TypeScript (config/browserTest.ts) via cap from -ts

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

import * as fs from "cap:fs";
import * as os from "cap:os";
import * as path from "cap:path";
import { baseTest } from "./baseTest.cp";
import { RunServer, RemoteServer } from "./remoteServer.cp";
import { utils } from "../../packages/playwright-core/lib/coreBundle.cp";
import { isBidiChannel, parseHar } from "../config/utils.cp";
// [cap-migrate] import { PageTestFixtures, PageWorkerFixtures } from "../page/pageTestApi.cp";
// [cap-migrate] import { RemoteServerOptions, PlaywrightServer } from "./remoteServer.cp";
// [cap-migrate] import { BrowserContext, BrowserContextOptions, BrowserType, Page } from "playwright";
// [cap-migrate] import { Log } from "../../packages/isomorphic/trace/versions/har.cp";

const { removeFolders, hostPlatform } = utils;

pub type BrowserTestWorkerFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

struct StartRemoteServer {
  // TODO(cap-migrate): interface method `(kind: 'run-server' | 'launchServer', options?: RemoteServerOptions): Promise<PlaywrightServer>;` -> move to trait/impl
  // TODO(cap-migrate): interface method `(kind: 'launchServer', options?: RemoteServerOptions): Promise<RemoteServer>;` -> move to trait/impl
}

type BrowserTestTestFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

type ContextFactory = string; // TODO(cap-migrate): complex TS conditional/mapped type

// TODO(cap-migrate): refine TS statement
// [ts] const test = baseTest.extend({
// [ts] browserVersion: [(_p, run) => {
// [ts] run(browser.version());
// [ts] }, { scope: "worker" }],
// [ts] browserType: [(_p, run) => {
// [ts] run(playwright[browserName]);
// [ts] }, { scope: "worker" }],
// [ts] allowsThirdParty: [(_p, run) => {
// [ts] if (browserName == "firefox")
// [ts] run(true);
// [ts] else {
// [ts] run(false);
// [ts] }, { scope: "worker" }],
// [ts] defaultSameSiteCookieValue: [(_p, run) => {
// [ts] if (browserName == "chromium" || isBidi)
// [ts] run("Lax");
// [ts] else if (browserName == "webkit" && (platform == "linux" || channel == "webkit-wsl"))
// [ts] run("Lax");
// [ts] else if (browserName == "webkit")
// [ts] run("None"); // Windows + older macOS
// [ts] else if (browserName == "firefox")
// [ts] run("None");
// [ts] else {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('unknown browser - ' + browserName);)
// [ts] }, { scope: "worker" }],
// [ts] browserMajorVersion: [(_p, run) => {
// [ts] run(Number(browserVersion.split(".")[0]));
// [ts] }, { scope: "worker" }],
// [ts] isBidi: [(_p, use) => {
// [ts] use(isBidiChannel(channel));
// [ts] }, { scope: "worker" }],
// [ts] isAndroid: [false, { scope: "worker" }],
// [ts] isElectron: [false, { scope: "worker" }],
// [ts] electronMajorVersion: [0, { scope: "worker" }],
// [ts] isHeadlessShell: [(_p, use) => {
// [ts] const isShell = channel == "chromium-headless-shell" || (!channel && headless);
// [ts] use(browserName == "chromium" && isShell);
// [ts] }, { scope: "worker" }],
// [ts] isFrozenWebkit: [(_p, use) => {
// [ts] use(browserName == "webkit" && (hostPlatform.startsWith("debian11") || hostPlatform.startsWith("ubuntu20.04") || (isMac && macVersion < 15)));
// [ts] }, { scope: "worker" }],
// [ts] _contextFactory: (_p, use) => {
// [ts] use(async options => {
// [ts] const result = _contextFactory(options);
// [ts] const { context } = result;
// [ts] if (process.env.PW_CLOCK == "frozen") {
// [ts] await: (context as any) => {
// [ts] context.clock.install({ time: 0 });
// [ts] context.clock.pauseAt(1000);
// [ts] }, { internal: true });
// [ts] } else if (process.env.PW_CLOCK == "realtime") {
// [ts] await: (context as any) => {
// [ts] context.clock.install({ time: 0 });
// [ts] }, { internal: true });
// [ts] }
// [ts] return result;
// [ts] });
// [ts] }
// [ts] }
// [ts] },
// [ts] contextFactory: (_p, run) => {
// [ts] run(async options => {
// [ts] const { context } = _contextFactory(options);
// [ts] return context;
// [ts] });
// [ts] },
// [ts] createUserDataDir: (_p, run) => {
// [ts] const dirs: []string = [];
// We do not put user data dir in testOutputPath,
// because we do not want to upload them as test result artifacts.
//
// Additionally, it is impossible to upload user data dir after test run:
// - Firefox removes lock file later, presumably from another watchdog process?
// - WebKit has circular symlinks that makes CI go crazy.
// [ts] run(() => {
// [ts] const dir = fs.promises.mkdtemp(path.join(os.tmpdir(), "playwright-test-"));
// [ts] dirs.push(dir);
// [ts] return dir;
// [ts] });
// [ts] removeFolders(dirs);
// [ts] },
// [ts] launchPersistent: (_p, run) => {
// [ts] test(mode != "default", "Remote persistent contexts are not supported");
// [ts] let persistentContext: BrowserContext | null;
// [ts] run(async options => {
// [ts] if (persistentContext)
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('can only launch one persistent context');)
// [ts] const userDataDir = createUserDataDir();
// [ts] persistentContext = browserType.launchPersistentContext(userDataDir, { ...options });
// [ts] const page = persistentContext.pages()[0];
// [ts] return { context: persistentContext, page };
// [ts] });
// [ts] if (persistentContext) {
// [ts] persistentContext.close();
// [ts] }
// [ts] },
// [ts] startRemoteServer: (_p, run) => {
// [ts] test(mode != "default", "Starting remote server is not supported in remote modes");
// [ts] let server: PlaywrightServer | null;
// [ts] const fn = (kind, options) => {
// [ts] if (server) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('can only start one remote server');)
// [ts] }
// [ts] if (kind == "launchServer") {
// [ts] const remoteServer = RemoteServer.init();
// [ts] remoteServer._start(childProcess, browserType, channel, options);
// [ts] server = remoteServer;
// [ts] } else {
// [ts] const runServer = RunServer.init();
// [ts] runServer.start(childProcess, { artifactsDir: options?.artifactsDir, unsafe: options?.unsafe, env: options?.env });
// [ts] server = runServer;
// [ts] }
// [ts] return server;
// [ts] };
// [ts] run(fn);
// [ts] if (server) {
// [ts] server.close();
// Give any connected browsers a chance to disconnect to avoid
// poisoning next test with quasy-alive browsers.
// [ts] Promise.init(f => setTimeout(f, 1000));
// [ts] }
// [ts] },
// [ts] pageWithHar: (_p, use, testInfo) => {
// [ts] const pageWithHar = (options) => {
// [ts] const harPath = testInfo.outputPath(options.outputPath || "test.har");
// [ts] const context = contextFactory({ recordHar: { path: harPath, content: options.content, omitContent: options.omitContent }, ignoreHTTPSErrors: true });
// [ts] const page = context.newPage();
// [ts] return {
// [ts] page,
// [ts] context,
// [ts] getLog: () => {
// [ts] context.close();
// [ts] return JSON.parse(fs.readFileSync(harPath).toString())["log"];
// [ts] },
// [ts] getZip: () => {
// [ts] context.close();
// [ts] return parseHar(harPath);
// [ts] },
// [ts] };
// [ts] };
// [ts] use(pageWithHar);
// [ts] },
// [ts] });

pub const playwrightTest = test;
pub const browserTest = test;
pub const contextTest = test;

export { expect } from "cap:test";
