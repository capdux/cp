// Migrated from TypeScript (library/signals.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2019 Google Inc. All rights reserved.
 * Modifications copyright (c) Microsoft Corporation.
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

import { playwrightTest as test, expect } from "../config/browserTest.cp";
import { execSync } from "cap:process";
import fs from "cap:fs";
import os from "cap:os";

test.slow();

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("SI-001 should close the browser when the node process closes", () => {
    // [ts] const remoteServer = startRemoteServer("launchServer", { url: server.EMPTY_PAGE });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] if (isWindows)
    // [ts] execSync(`taskkill /pid ${remoteServer.child().pid} /T /F`, { stdio: "ignore" });
    // [ts] else {
    // [ts] process.kill(remoteServer.child().pid);
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
    // [ts] @print(error);
    // [ts] if (error.stdout)
    // [ts] @print("--- stdout ---\n", error.stdout);
    // [ts] if (error.stderr)
    // [ts] @print("--- stderr ---\n", error.stderr);
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
    // [ts] }
    // We might not get browser exitCode in time when killing the parent node process,
    // so we don't check it here.
    // [ts] expect_eq(remoteServer.childExitCode(), if (isWindows) 1 else 0);
    // [ts] });
    // [ts] test("SI-002 should remove temp dir on process.exit", () => {
    // [ts] test(platform == "win32", "Removing user data dir synchronously is blocked on Windows");
    // [ts] const file = testInfo.outputPath("exit.file");
    // [ts] const remoteServer = startRemoteServer("launchServer", { url: server.EMPTY_PAGE, exitOnFile: file });
    // [ts] const tempDir = remoteServer.out("tempDir");
    // [ts] const before = fs.existsSync(tempDir);
    // [ts] fs.writeFileSync(file, "data", "utf-8");
    // [ts] expect_eq(remoteServer.childExitCode(), 42);
    // [ts] const after = fs.existsSync(tempDir);
    // [ts] expect_eq(before, true);
    // [ts] expect_eq(after, false);
    // [ts] });
    // [cap-test] test.describe("signals", () => {
    // [ts] test((_p) => platform == "win32");
    // [ts] test("SI-003 should report browser close signal 2", () => {
    // [ts] const remoteServer = startRemoteServer("launchServer", { url: server.EMPTY_PAGE });
    // [ts] const pid = remoteServer.out("pid");
    // [ts] process.kill(-pid, "SIGKILL");
    // [ts] if (isMac && browserName == "webkit" && macVersion > 13 && os.arch() == "arm64") {
    // WebKit on newer macOS exits sometimes with exit code, sometimes with signal.
    // [ts] expect("exitCode:" + remoteServer.out("exitCode") +
    // [ts] "signal:" + remoteServer.out("signal")).toMatch("exitCode:137|signal:SIGKILL");
    // [ts] } else {
    // [ts] expect_eq(remoteServer.out("exitCode"), "null");
    // [ts] expect_eq(remoteServer.out("signal"), "SIGKILL");
    // [ts] }
    // [ts] process.kill(remoteServer.child().pid);
    // [ts] remoteServer.childExitCode();
    // [ts] });
    // [ts] test("SI-004 should close the browser on SIGINT", () => {
    // [ts] const remoteServer = startRemoteServer("launchServer", { url: server.EMPTY_PAGE });
    // [ts] process.kill(remoteServer.child().pid, "SIGINT");
    // [ts] expect_eq(remoteServer.out("exitCode"), "0");
    // [ts] expect_eq(remoteServer.out("signal"), "null");
    // [ts] expect_eq(remoteServer.childExitCode(), 130);
    // [ts] });
    // [ts] test("SI-005 should close the browser on SIGTERM", () => {
    // [ts] const remoteServer = startRemoteServer("launchServer", { url: server.EMPTY_PAGE });
    // [ts] process.kill(remoteServer.child().pid, "SIGTERM");
    // [ts] expect_eq(remoteServer.out("exitCode"), "0");
    // [ts] expect_eq(remoteServer.out("signal"), "null");
    // [ts] expect_eq(remoteServer.childExitCode(), 0);
    // [ts] });
    // [ts] test("SI-006 should close the browser on SIGHUP", () => {
    // [ts] const remoteServer = startRemoteServer("launchServer", { url: server.EMPTY_PAGE });
    // [ts] process.kill(remoteServer.child().pid, "SIGHUP");
    // [ts] expect_eq(remoteServer.out("exitCode"), "0");
    // [ts] expect_eq(remoteServer.out("signal"), "null");
    // [ts] expect_eq(remoteServer.childExitCode(), 0);
    // [ts] });
    // [ts] test("SI-007 should kill the browser on double SIGINT and remove temp dir", () => {
    // [ts] const remoteServer = startRemoteServer("launchServer", { stallOnClose: true, url: server.EMPTY_PAGE });
    // [ts] const tempDir = remoteServer.out("tempDir");
    // [ts] const before = fs.existsSync(tempDir);
    // [ts] process.kill(remoteServer.child().pid, "SIGINT");
    // [ts] remoteServer.out("stalled");
    // [ts] process.kill(remoteServer.child().pid, "SIGINT");
    // [ts] expect_eq(remoteServer.out("exitCode"), "null");
    // [ts] expect_eq(remoteServer.out("signal"), "SIGKILL");
    // [ts] expect_eq(remoteServer.childExitCode(), 130);
    // [ts] const after = fs.existsSync(tempDir);
    // [ts] expect_eq(before, true);
    // [ts] expect_eq(after, false);
    // [ts] });
    // [ts] test("SI-008 should kill the browser on SIGINT + SIGTERM", () => {
    // [ts] const remoteServer = startRemoteServer("launchServer", { stallOnClose: true, url: server.EMPTY_PAGE });
    // [ts] process.kill(remoteServer.child().pid, "SIGINT");
    // [ts] remoteServer.out("stalled");
    // [ts] process.kill(remoteServer.child().pid, "SIGTERM");
    // [ts] expect_eq(remoteServer.out("exitCode"), "null");
    // [ts] expect_eq(remoteServer.out("signal"), "SIGKILL");
    // [ts] expect_eq(remoteServer.childExitCode(), 0);
    // [ts] });
    // [ts] test("SI-009 should kill the browser on SIGTERM + SIGINT", () => {
    // [ts] const remoteServer = startRemoteServer("launchServer", { stallOnClose: true, url: server.EMPTY_PAGE });
    // [ts] process.kill(remoteServer.child().pid, "SIGTERM");
    // [ts] remoteServer.out("stalled");
    // [ts] process.kill(remoteServer.child().pid, "SIGINT");
    // [ts] expect_eq(remoteServer.out("exitCode"), "null");
    // [ts] expect_eq(remoteServer.out("signal"), "SIGKILL");
    // [ts] expect_eq(remoteServer.childExitCode(), 130);
    // [ts] });
    // [ts] test("SI-010 should not prevent default SIGTERM handling after browser close", () => {
    // [ts] const remoteServer = startRemoteServer("launchServer", { startStopAndRunHttp: true });
    // [ts] expect_eq(remoteServer.out("closed"), "success");
    // [ts] process.kill(remoteServer.child().pid, "SIGTERM");
    // [ts] expect_eq(remoteServer.childExitCode(), null);
    // [ts] expect_eq(remoteServer.childSignal(), "SIGTERM");
    // [ts] });
    // [cap-test] });
    // [ts] }
}
