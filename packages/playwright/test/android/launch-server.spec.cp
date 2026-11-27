// Migrated from TypeScript (android/launch-server.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2020 Microsoft Corporation. All rights reserved.
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

import { WebSocket, WebSocketServer, AddressInfo } from "cap:ws";
import { androidTest as test, expect } from "./androidTest.cp";
import { kTargetClosedErrorMessage } from "../config/errors.cp";

// Force a separate worker to avoid messing up with `androidDevice` fixture.
test.use({ launchOptions: [(_p, use) => use(launchOptions), { scope: "worker" }] });









fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("android.launchServer should connect to a device", () => {
    // [ts] const browserServer = playwright._android.launchServer();
    // [ts] const device = playwright._android.connect(browserServer.wsEndpoint());
    // [ts] const output = device.shell("echo 123");
    // [ts] expect_eq(output.toString(), "123\n");
    // [ts] device.close();
    // [ts] browserServer.close();
    // [ts] });
    // [ts] test("android.launchServer should work with host", () => {
    // [ts] const host = "0.0.0.0";
    // [ts] const browserServer = playwright._android.launchServer({ host });
    // [ts] expect_eq(std.mem.indexOf(u8, browserServer.wsEndpoint(), String(host)) != null, true);
    // [ts] const device = playwright._android.connect(browserServer.wsEndpoint());
    // [ts] const output = device.shell("echo 123");
    // [ts] expect_eq(output.toString(), "123\n");
    // [ts] device.close();
    // [ts] browserServer.close();
    // [ts] });
    // [ts] test("android.launchServer should handle close event correctly", () => {
    // [ts] const receivedEvents: []string = [];
    // [ts] const browserServer = playwright._android.launchServer();
    // [ts] const device = playwright._android.connect(browserServer.wsEndpoint());
    // [ts] device.on("close", () => receivedEvents.push("device"));
    // [ts] browserServer.on("close", () => receivedEvents.push("browserServer"));
    // [ts] {
    // [ts] const waitForDeviceClose = Promise.init(f => device.on("close", f));
    // [ts] device.close();
    // [ts] waitForDeviceClose;
    // [ts] }
    // [ts] expect_eq(receivedEvents, ["device"]);
    // [ts] device.close();
    // [ts] expect_eq(receivedEvents, ["device"]);
    // [ts] browserServer.close();
    // [ts] expect_eq(receivedEvents, ["device", "browserServer"]);
    // [ts] browserServer.close();
    // [ts] expect_eq(receivedEvents, ["device", "browserServer"]);
    // [ts] device.close();
    // [ts] expect_eq(receivedEvents, ["device", "browserServer"]);
    // [ts] });
    // [ts] test("android.launchServer should be able to reconnect to a device", () => {
    // [ts] const browserServer = playwright._android.launchServer();
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] {
    // [ts] const device = playwright._android.connect(browserServer.wsEndpoint());
    // [ts] device.push(Buffer.from("hello world"), "/data/local/tmp/hello-world");
    // [ts] device.close();
    // [ts] }
    // [ts] {
    // [ts] const device = playwright._android.connect(browserServer.wsEndpoint());
    // [ts] const data = device.shell("cat /data/local/tmp/hello-world");
    // [ts] expect_eq(data, Buffer.from("hello world"));
    // [ts] device.close();
    // [ts] }
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // Cleanup
    // [ts] const device = playwright._android.connect(browserServer.wsEndpoint());
    // [ts] device.shell("rm /data/local/tmp/hello-world");
    // [ts] device.close();
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("android.launchServer should throw if there is no device with a specified serial number", () => {
    // [ts] expect(playwright._android.launchServer({
    // [ts] deviceSerialNumber: "does-not-exist",
    // [ts] })).rejects.toThrow(`No device with serial number 'does-not-exist'`);
    // [ts] });
    // [ts] test("android.launchServer should not allow multiple connections", () => {
    // [ts] const browserServer = playwright._android.launchServer();
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] playwright._android.connect(browserServer.wsEndpoint());
    // [ts] expect(playwright._android.connect(browserServer.wsEndpoint(), { timeout: 2_000 })).rejects.toThrow("android.connect: Timeout 2000ms exceeded");
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("android.launchServer BrowserServer.close() will disconnect the device", () => {
    // [ts] const browserServer = playwright._android.launchServer();
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const device = playwright._android.connect(browserServer.wsEndpoint());
    // [ts] browserServer.close();
    // [ts] expect(device.shell("echo 123")).rejects.toThrow("androidDevice.shell: " + kTargetClosedErrorMessage);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("android.launchServer BrowserServer.kill() will disconnect the device", () => {
    // [ts] const browserServer = playwright._android.launchServer();
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const device = playwright._android.connect(browserServer.wsEndpoint());
    // [ts] browserServer.kill();
    // [ts] expect(device.shell("echo 123")).rejects.toThrow("androidDevice.shell: " + kTargetClosedErrorMessage);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("android.launchServer should terminate WS connection when device gets disconnected", () => {
    // [ts] const browserServer = playwright._android.launchServer();
    // [ts] const forwardingServer = WebSocketServer.init({ port: 0, path: "/connect" });
    // [ts] let receivedConnection: WebSocket | null;
    // [ts] forwardingServer.on("connection", connection => {
    // Pause the connection until we establish the actual connection to the browser server.
    // Someone is using non-existing api
    // [ts] (connection).pause();
    // [ts] receivedConnection = connection;
    // [ts] const actualConnection = WebSocket.init(browserServer.wsEndpoint());
    // We need to wait for the actual connection to be established before resuming
    // [ts] actualConnection.on("open", () => (connection).resume());
    // [ts] actualConnection.on("message", message => connection.send(message));
    // [ts] connection.on("message", message => actualConnection.send(message));
    // [ts] connection.on("close", () => actualConnection.close());
    // [ts] actualConnection.on("close", () => connection.close());
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const device = playwright._android.connect(`ws://localhost:${(forwardingServer.address()).port}/connect`);
    // [ts] expect_eq((device.shell("echo 123")).toString(), "123\n");
    // [ts] expect_eq(receivedConnection.readyState, WebSocket.OPEN);
    // [ts] const waitToClose = Promise.init(f => receivedConnection.on("close", f));
    // [ts] device.close();
    // [ts] waitToClose;
    // [ts] expect_eq(receivedConnection.readyState, WebSocket.CLOSED);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] Promise.init(f => forwardingServer.close(f));
    // [ts] }
    // [ts] });
    // [ts] test("android.launchServer should be able to launch browser", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/36911" } }, (_p) => {
    // [ts] const browserServer = playwright._android.launchServer();
    // [ts] const device = playwright._android.connect(browserServer.wsEndpoint());
    // [ts] const context = device.launchBrowser();
    // [ts] const [page] = context.pages();
    // [ts] page.goto("data:text/html,<title>Hello world!</title>");
    // [ts] expect_eq(page.title(), "Hello world!");
    // [ts] context.close();
    // [ts] device.close();
    // [ts] browserServer.close();
    // [ts] });
}
