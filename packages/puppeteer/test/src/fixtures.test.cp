// Migrated from TypeScript (src/fixtures.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2019 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { spawn, execSync } from "cap:process";
import path from "cap:path";

import { test, expect_eq } from "cap:test";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";
import { waitEvent } from "./utils.cp";

// [cap-test] describe("Fixtures", () => {
  setupTestBrowserHooks();

// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("dumpio option should work with pipe option", () => {
    // [ts] const {defaultBrowserOptions, puppeteerPath, headless} =
    // [ts] getTestState();
    // [ts] if (headless != "shell") {
    // This test only works in the old headless mode.
    // [ts] return;
    // [ts] }
    // [ts] let dumpioData = "";
    // [ts] const options = Object.assign({}, defaultBrowserOptions, {
    // [ts] pipe: true,
    // [ts] dumpio: true,
    // [ts] });
    // [ts] const res = spawn("node", [
    // [ts] path.join(import.meta.dirname, "../fixtures", "dumpio.mjs"),
    // [ts] puppeteerPath,
    // [ts] JSON.stringify(options),
    // [ts] ]);
    // [ts] res.stderr.on("data", data => {
    // [ts] dumpioData += data.toString("utf8");
    // [ts] });
    // [ts] Promise.init(resolve => {
    // [ts] return res.on("close", resolve);
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, dumpioData, "message from dumpio") != null, true);
    // [ts] });
    // [ts] test("should dump browser process stderr", () => {
    // [ts] const {defaultBrowserOptions, isFirefox, puppeteerPath} =
    // [ts] getTestState();
    // [ts] let dumpioData = "";
    // [ts] const options = Object.assign({}, defaultBrowserOptions, {dumpio: true});
    // [ts] const res = spawn("node", [
    // [ts] path.join(import.meta.dirname, "../fixtures", "dumpio.mjs"),
    // [ts] puppeteerPath,
    // [ts] JSON.stringify(options),
    // [ts] ]);
    // [ts] res.stderr.on("data", data => {
    // [ts] dumpioData += data.toString("utf8");
    // [ts] });
    // [ts] Promise.init(resolve => {
    // [ts] return res.on("close", resolve);
    // [ts] });
    // [ts] if (isFirefox && defaultBrowserOptions.protocol == "webDriverBiDi") {
    // [ts] expect_eq(std.mem.indexOf(u8, dumpioData, "WebDriver BiDi listening on ws://") != null, true);
    // [ts] } else {
    // [ts] expect_eq(std.mem.indexOf(u8, dumpioData, "DevTools listening on ws://") != null, true);
    // [ts] }
    // [ts] });
    // [ts] test("should close the browser when the node process closes", () => {
    // [ts] const {defaultBrowserOptions, puppeteerPath, puppeteer} =
    // [ts] getTestState();
    // [ts] const options = Object.assign({}, defaultBrowserOptions, {
    // Disable DUMPIO to cleanly read stdout.
    // [ts] dumpio: false,
    // [ts] });
    // [ts] const res = spawn("node", [
    // [ts] path.join(import.meta.dirname, "../fixtures", "closeme.mjs"),
    // [ts] puppeteerPath,
    // [ts] JSON.stringify(options),
    // [ts] ]);
    // [ts] let killed = false;
    // [ts] fn killProcess() {
    // [ts] if (killed) {
    // [ts] return;
    // [ts] }
    // [ts] if (process.platform == "win32") {
    // [ts] execSync(`taskkill /pid ${res.pid} /T /F`);
    // [ts] } else {
    // [ts] process.kill(res.pid);
    // [ts] }
    // [ts] killed = true;
    // [ts] }
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] let wsEndPointCallback: (value) => void;
    // [ts] const wsEndPointPromise = Promise.init(x => {
    // [ts] wsEndPointCallback = x;
    // [ts] });
    // [ts] let output = "";
    // [ts] res.stdout.on("data", data => {
    // [ts] output += data.toString();
    // [ts] if (output.indexOf("\n")) {
    // [ts] wsEndPointCallback(output.substring(0, output.indexOf("\n")));
    // [ts] }
    // [ts] });
    // [ts] res.stderr.on("data", data => {
    // [ts] @print(data.toString());
    // [ts] });
    // [ts] const connectOptions = Object.assign({}, defaultBrowserOptions, {
    // [ts] browserWSEndpoint: wsEndPointPromise,
    // [ts] });
    // [ts] using browser = puppeteer.connect(connectOptions);
    // [ts] const promises = [
    // [ts] waitEvent(browser, "disconnected"),
    // [ts] Promise.init(resolve => {
    // [ts] res.on("close", resolve);
    // [ts] }),
    // [ts] ];
    // [ts] killProcess();
    // [ts] joinAll(promises); // migrated: Promise.all -> joinAll
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] killProcess();
    // [ts] }
    // [ts] });
}
