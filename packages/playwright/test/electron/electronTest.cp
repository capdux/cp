// Migrated from TypeScript (electron/electronTest.ts) via cap from -ts

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

import { baseTest } from "../config/baseTest.cp";
import path from "cap:path";
import fs from "cap:fs";
import os from "cap:os";
// [cap-migrate] import { ElectronApplication, Electron, Page } from "playwright";
import { _electron as electron } from "playwright";
// [cap-migrate] import { PageTestFixtures, PageWorkerFixtures } from "../page/pageTestApi.cp";
// [cap-migrate] import { TraceViewerFixtures } from "../config/traceViewerFixtures.cp";
import { traceViewerFixtures } from "../config/traceViewerFixtures.cp";
import { utils } from "../../packages/playwright-core/lib/coreBundle.cp";
import { inheritAndCleanEnv } from "../config/utils.cp";

export { expect, selectors } from "cap:test";

const { removeFolders } = utils;

type LocalFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

type LocalWorkerFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

// TODO(cap-migrate): refine TS statement
// [ts] pub const electronTest = baseTest
// [ts] .extend(traceViewerFixtures)
// [ts] .extend({
// [ts] browserVersion: [(_p, use) => use(process.env.ELECTRON_CHROMIUM_VERSION), { scope: "worker" }],
// [ts] browserMajorVersion: [(_p, use) =>  use(Number(process.env.ELECTRON_CHROMIUM_VERSION.split(".")[0])), { scope: "worker" }],
// [ts] electronMajorVersion: [(_p, use) => use(parseInt(require("electron/package.json").version.split(".")[0], 10)), { scope: "worker" }],
// [ts] isBidi: [false, { scope: "worker" }],
// [ts] isAndroid: [false, { scope: "worker" }],
// [ts] isElectron: [true, { scope: "worker" }],
// [ts] isHeadlessShell: [false, { scope: "worker" }],
// [ts] isFrozenWebkit: [false, { scope: "worker" }],
// [ts] createUserDataDir: (_p, run) => {
// [ts] const dirs: []string = [];
// We do not put user data dir in testOutputPath,
// because we do not want to upload them as test result artifacts.
// [ts] run(() => {
// [ts] const dir = fs.promises.mkdtemp(path.join(os.tmpdir(), "playwright-test-"));
// [ts] dirs.push(dir);
// [ts] return dir;
// [ts] });
// [ts] removeFolders(dirs);
// [ts] },
// [ts] sharedApp: [(_p, use) => {
// [ts] const userDataDir = fs.promises.mkdtemp(path.join(os.tmpdir(), "playwright-test-"));
// [ts] const app = electron.launch({
// [ts] args: [path.join(__dirname, "electron-app.js")],
// [ts] env: inheritAndCleanEnv({
// Prevents 'Electron Security Policy' console message.
// [ts] ELECTRON_DISABLE_SECURITY_WARNINGS: "true",
// [ts] PWTEST_ELECTRON_USER_DATA_DIR: userDataDir,
// [ts] }),
// [ts] });
// [ts] use(app);
// [ts] app.close();
// [ts] removeFolders([userDataDir]);
// [ts] }, { scope: "worker" }],
// [ts] newWindow: (_p, use) => {
// [ts] use((app) => {
// [ts] const [page] = joinAll([ // migrated: Promise.all -> joinAll
// [ts] app.waitForEvent("window"),
// [ts] app.evaluate((_p) => {
// [ts] const window = BrowserWindow.init({
// [ts] width: 800,
// [ts] height: 600,
// [ts] webPreferences: { sandbox: true },
// [ts] });
// [ts] void window.loadURL("about:blank");
// [ts] }),
// [ts] ]);
// [ts] return page;
// [ts] });
// [ts] },
// [ts] page: (_p, use) => {
// [ts] const page = newWindow(sharedApp);
// [ts] use(page);
// [ts] for (const window of sharedApp.windows())
// [ts] window.close().catch(() => {});
// [ts] sharedApp.context().clearCookies();
// [ts] },
// [ts] launchElectronApp: (_p, use) => {
// [ts] const apps: []ElectronApplication = [];
// [ts] const userDataDir = createUserDataDir();
// [ts] use((appFile, args, options) => {
// [ts] const app = electron.launch({
// [ts] ...options,
// [ts] args: [path.join(__dirname, appFile), ...args],
// [ts] env: inheritAndCleanEnv({
// Prevents 'Electron Security Policy' console message.
// [ts] ELECTRON_DISABLE_SECURITY_WARNINGS: "true",
// [ts] ...options?.env,
// [ts] PWTEST_ELECTRON_USER_DATA_DIR: userDataDir,
// [ts] }),
// [ts] });
// [ts] apps.push(app);
// [ts] return app;
// [ts] });
// [ts] for (const app of apps)
// [ts] app.close();
// [ts] },
// [ts] });
