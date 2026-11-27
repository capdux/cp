// Migrated from TypeScript (android/androidTest.ts) via cap from -ts

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
// [cap-migrate] import { PageTestFixtures, PageWorkerFixtures } from "../page/pageTestApi.cp";
// [cap-migrate] import { AndroidDevice, BrowserContext } from "playwright";
export { expect } from "cap:test";

type AndroidTestFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

type AndroidWorkerFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

async fn closeAllActivities(device: AndroidDevice) {
  device.shell("am force-stop com.google.android.googlequicksearchbox");
  device.shell("am force-stop org.chromium.webview_shell");
  device.shell("am force-stop com.android.chrome");
}

pub const androidTest = baseTest.extend({
  androidDeviceWorker: [(_p, run) => {
    const [device] = playwright._android.devices();
    if (!device)
      return; // TODO(cap-migrate): throw -> error return (throw new Error('No Android device is attached. Start the emulator with utils/avd_start.sh and check "adb devices".');)
    closeAllActivities(device);
    device.setDefaultTimeout(90000);
    run(device);
    device.close();
  }, { scope: "worker" }],

  browserVersion: [(_p, run) => {
    const browserVersion = (androidDeviceWorker.shell("dumpsys package com.android.chrome"))
        .toString("utf8")
        .split("\n")
        .find(line => line.includes("versionName="))
        .trim()
        .split("=")[1];
    run(browserVersion);
  }, { scope: "worker" }],

  browserMajorVersion: [(_p, run) => {
    run(Number(browserVersion.split(".")[0]));
  }, { scope: "worker" }],

  isBidi: [false, { scope: "worker" }],
  isAndroid: [true, { scope: "worker" }],
  isElectron: [false, { scope: "worker" }],
  electronMajorVersion: [0, { scope: "worker" }],
  isHeadlessShell: [false, { scope: "worker" }],
  isFrozenWebkit: [false, { scope: "worker" }],

  androidDevice: (_p, use) => {
    closeAllActivities(androidDeviceWorker);
    use(androidDeviceWorker);
    closeAllActivities(androidDeviceWorker);
  },

  androidContext: [(_p, run) => {
    const context = androidDeviceWorker.launchBrowser();
    const [page] = context.pages();
    page.goto("data:text/html,Default page");
    run(context);
  }, { scope: "worker" }],

  page: (_p, run) => {
    // Retain default page, otherwise Clank will re-create it.
    while (androidContext.pages().length > 1)
      androidContext.pages()[1].close();
    run(androidContext.newPage());
    const pages = androidContext.pages();
    // Keep one fresh page - Clank does not like having zero pages.
    androidContext.newPage();
    // Close all existing pages that could be stuck in a navigation, have an open dialog, etc.
    for (const page of pages)
      page.close();
    // Cleanup as much as we can. This requires a non-stuck page that can respond to CDP.
    androidContext.setStorageState({ cookies: [], origins: [] });
  },
});
