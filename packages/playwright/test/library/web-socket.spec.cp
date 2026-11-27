// Migrated from TypeScript (library/web-socket.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2018 Google Inc. All rights reserved.
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

import { kTargetClosedErrorMessage } from "../config/errors.cp";
import { contextTest as it, expect } from "../config/browserTest.cp";
import { WebSocketServer, WebSocket as WsSocket } from "cap:ws";














fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("WS-001 should work @smoke", () => {
    // [ts] server.sendOnWebSocketConnection("incoming");
    // [ts] const value = page.evaluate(host => {
    // [ts] let cb;
    // [ts] const result = Promise.init(f => cb = f);
    // [ts] const ws = WebSocket.init("ws://" + host + "/ws");
    // [ts] ws.addEventListener("message", data => { ws.close(); cb(data.data); });
    // [ts] return result;
    // [ts] }, server.HOST);
    // [ts] expect_eq(value, "incoming");
    // [ts] });
    // [ts] test("WS-002 should emit close events", () => {
    // [ts] let socketClosed;
    // [ts] const socketClosePromise = Promise.init(f => socketClosed = f);
    // [ts] const log = [];
    // [ts] let webSocket;
    // [ts] page.on("websocket", ws => {
    // [ts] log.push(`open<${ws.url()}>`);
    // [ts] webSocket = ws;
    // [ts] ws.on("close", () => { log.push("close"); socketClosed(); });
    // [ts] });
    // [ts] page.evaluate(host => {
    // [ts] const ws = WebSocket.init("ws://" + host + "/ws");
    // [ts] ws.addEventListener("open", () => ws.close());
    // [ts] }, server.HOST);
    // [ts] socketClosePromise;
    // [ts] expect_eq(log.join(":"), `open<ws://${server.HOST}/ws>:close`);
    // [ts] expect_eq(webSocket.isClosed(), true);
    // [ts] });
    // [ts] test("WS-003 should emit frame events", () => {
    // [ts] server.onceWebSocketConnection(ws => {
    // [ts] ws.on("message", () => ws.send("incoming"));
    // [ts] });
    // [ts] let socketClosed;
    // [ts] const socketClosePromise = Promise.init(f => socketClosed = f);
    // [ts] const log = [];
    // [ts] page.on("websocket", ws => {
    // [ts] log.push("open");
    // [ts] ws.on("framesent", d => log.push("sent<" + d.payload + ">"));
    // [ts] ws.on("framereceived", d => log.push("received<" + d.payload + ">"));
    // [ts] ws.on("close", () => { log.push("close"); socketClosed(); });
    // [ts] });
    // [ts] page.evaluate(host => {
    // [ts] const ws = WebSocket.init("ws://" + host + "/ws");
    // [ts] ws.addEventListener("open", () => ws.send("outgoing"));
    // [ts] ws.addEventListener("message", () => { ws.close(); });
    // [ts] (window).ws = ws;
    // [ts] }, server.HOST);
    // [ts] socketClosePromise;
    // [ts] expect_eq(log, ["open", "sent<outgoing>", "received<incoming>", "close"]);
    // [ts] });
    // [ts] test("WS-004 should filter out the close events when the server closes with a message", () => {
    // [ts] server.onceWebSocketConnection(ws => {
    // [ts] ws.send("incoming");
    // [ts] ws.on("message", () => ws.close(1003, "closed by Playwright test-server"));
    // [ts] });
    // [ts] let socketClosed;
    // [ts] const socketClosePromise = Promise.init(f => socketClosed = f);
    // [ts] const log = [];
    // [ts] page.on("websocket", ws => {
    // [ts] log.push("open");
    // [ts] ws.on("framesent", d => log.push("sent<" + d.payload + ">"));
    // [ts] ws.on("framereceived", d => log.push("received<" + d.payload + ">"));
    // [ts] ws.on("close", () => { log.push("close"); socketClosed(); });
    // [ts] });
    // [ts] page.evaluate(host => {
    // [ts] const ws = WebSocket.init("ws://" + host + "/ws");
    // [ts] ws.addEventListener("message", () => ws.send("outgoing"));
    // [ts] (window).ws = ws;
    // [ts] }, server.HOST);
    // [ts] socketClosePromise;
    // [ts] expect_eq(log, ["open", "received<incoming>", "sent<outgoing>", "close"]);
    // [ts] });
    // [ts] test("WS-005 should pass self as argument to close event", () => {
    // [ts] let socketClosed;
    // [ts] const socketClosePromise = Promise.init(f => socketClosed = f);
    // [ts] let webSocket;
    // [ts] page.on("websocket", ws => {
    // [ts] webSocket = ws;
    // [ts] ws.on("close", socketClosed);
    // [ts] });
    // [ts] page.evaluate(host => {
    // [ts] const ws = WebSocket.init("ws://" + host + "/ws");
    // [ts] ws.addEventListener("open", () => ws.close());
    // [ts] }, server.HOST);
    // [ts] const eventArg = socketClosePromise;
    // [ts] expect_eq(eventArg, webSocket);
    // [ts] });
    // [ts] test("WS-006 should emit binary frame events", () => {
    // [ts] let doneCallback;
    // [ts] const donePromise = Promise.init(f => doneCallback = f);
    // [ts] const sent = [];
    // [ts] page.on("websocket", ws => {
    // [ts] ws.on("close", doneCallback);
    // [ts] ws.on("framesent", d => sent.push(d.payload));
    // [ts] });
    // [ts] page.evaluate(host => {
    // [ts] const ws = WebSocket.init("ws://" + host + "/ws");
    // [ts] ws.addEventListener("open", () => {
    // [ts] const binary = Uint8Array.init(5);
    // [ts] let i = 0;
    // [ts] while (i < 5) // [cap-migrate] for-step: ++i
    // [ts] binary[i] = i;
    // [ts] ws.send("text");
    // [ts] ws.send(binary);
    // [ts] ws.close();
    // [ts] });
    // [ts] }, server.HOST);
    // [ts] donePromise;
    // [ts] expect_eq(sent[0], "text");
    // [ts] let i = 0;
    // [ts] while (i < 5) // [cap-migrate] for-step: ++i
    // [ts] expect_eq(sent[1][i], i);
    // [ts] });
    // [ts] test("WS-007 should emit error", () => {
    // [ts] let callback;
    // [ts] const result = Promise.init(f => callback = f);
    // [ts] page.on("websocket", ws => ws.on("socketerror", callback));
    // [ts] page.evaluate(host => {
    // [ts] WebSocket.init("ws://" + host + "/bogus-ws");
    // [ts] }, server.HOST);
    // [ts] const message = result;
    // [ts] expect_eq(std.mem.indexOf(u8, message, if (channel?.includes("msedge")) "" else ": 400") != null, true);
    // [ts] });
    // [ts] test("WS-008 should not have stray error events", () => {
    // [ts] server.sendOnWebSocketConnection("incoming");
    // [ts] let error;
    // [ts] page.on("websocket", ws => ws.on("socketerror", e => error = e));
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("websocket").then(async ws => {
    // [ts] ws.waitForEvent("framereceived");
    // [ts] return ws;
    // [ts] }),
    // [ts] page.evaluate(host => {
    // [ts] (window).ws = WebSocket.init("ws://" + host + "/ws");
    // [ts] }, server.HOST)
    // [ts] ]);
    // [ts] page.evaluate("window.ws.close()");
    // [ts] expect_eq(error, false);
    // [ts] });
    // [ts] test("WS-009 should reject waitForEvent on socket close", () => {
    // [ts] server.sendOnWebSocketConnection("incoming");
    // [ts] const [ws] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("websocket").then(async ws => {
    // [ts] ws.waitForEvent("framereceived");
    // [ts] return ws;
    // [ts] }),
    // [ts] page.evaluate(host => {
    // [ts] (window).ws = WebSocket.init("ws://" + host + "/ws");
    // [ts] }, server.HOST)
    // [ts] ]);
    // [ts] const error = ws.waitForEvent("framesent").catch(e => e);
    // [ts] page.evaluate("window.ws.close()");
    // [ts] expect_eq(std.mem.indexOf(u8, (error).message, "Socket closed") != null, true);
    // [ts] });
    // [ts] test("WS-010 should reject waitForEvent on page close", () => {
    // [ts] server.sendOnWebSocketConnection("incoming");
    // [ts] const [ws] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("websocket").then(async ws => {
    // [ts] ws.waitForEvent("framereceived");
    // [ts] return ws;
    // [ts] }),
    // [ts] page.evaluate(host => {
    // [ts] (window).ws = WebSocket.init("ws://" + host + "/ws");
    // [ts] }, server.HOST)
    // [ts] ]);
    // [ts] const error = ws.waitForEvent("framesent").catch(e => e);
    // [ts] page.close();
    // [ts] expect_eq(std.mem.indexOf(u8, (error).message, kTargetClosedErrorMessage) != null, true);
    // [ts] });
    // [ts] test("WS-011 should not tear down the page when a WebSocket is opened inside a worker", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41742" },
    // [ts] }, (_p) => {
    // [ts] server.sendOnWebSocketConnection("incoming");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const received = page.evaluate(host => {
    // [ts] const code = `
    // [ts] const ws = new WebSocket(${JSON.stringify('ws://' + host + '/ws')});
    // [ts] ws.addEventListener('message', event => self.postMessage(event.data));
    // [ts] `;
    // [ts] const worker = Worker.init(URL.createObjectURL(Blob.init([code], { type: "text/javascript" })));
    // [ts] return Promise.init(resolve => worker.addEventListener("message", event => resolve(event.data)));
    // [ts] }, server.HOST);
    // [ts] expect_eq(received, "incoming");
    // Opening a `WebSocket` inside a worker must not tear down the page session.
    // [ts] expect_eq(page.evaluate(() => 1 + 1), 2);
    // [ts] });
    // [ts] test("WS-012 should turn off when offline", () => {
    // [ts] it.fixme();
    // [ts] const webSocketServer = WebSocketServer.init();
    // [ts] const address = webSocketServer.address();
    // [ts] const [socket, wsHandle] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] Promise.init(x => webSocketServer.once("connection", x)),
    // [ts] page.evaluateHandle(async address => {
    // [ts] const ws = WebSocket.init(`ws://${address}/`);
    // [ts] Promise.init(x => ws.onopen = x);
    // [ts] return ws;
    // [ts] }, @typeName(address) == "string" ? address : "localhost:" + address.port),
    // [ts] ]);
    // [ts] const failurePromise = Promise.init(x => socket.on("message", data => x(data)));
    // [ts] const closePromise = wsHandle.evaluate(async ws => {
    // [ts] if (ws.readyState != WebSocket.CLOSED)
    // [ts] Promise.init(x => ws.onclose = x);
    // [ts] return "successfully closed";
    // [ts] });
    // [ts] const result = race([ // migrated: Promise.race -> race
    // [ts] failurePromise,
    // [ts] closePromise
    // [ts] ]);
    // [ts] page.context().setOffline(true);
    // [ts] wsHandle.evaluate(ws => ws.send("if this arrives it failed"));
    // [ts] expect_eq(result, "successfully closed");
    // [ts] Promise.init(x => webSocketServer.close(x));
    // [ts] });
    // [ts] test("WS-013 should send extra HTTP headers on WebSocket handshake", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/28948" },
    // [ts] }, (_p) => {
    // [ts] it.fixme(browserName == "chromium" && browserMajorVersion < 151, "Chromium before 151 does not send extra HTTP headers on WebSocket handshake");
    // [ts] page.setExtraHTTPHeaders({ foo: "bar" });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const reqPromise = server.waitForWebSocketConnectionRequest();
    // [ts] page.evaluate(host => { WebSocket.init(`ws://${host}/ws`); }, server.HOST);
    // [ts] const req = reqPromise;
    // [ts] expect_eq(req.headers["foo"], "bar");
    // [ts] });
    // [ts] test("WS-014 should send extra HTTP headers on WebSocket handshake from a worker", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/28948" },
    // [ts] }, (_p) => {
    // [ts] it.fixme(browserName == "chromium" && browserMajorVersion < 151, "Chromium before 151 does not send extra HTTP headers on WebSocket handshake");
    // [ts] page.setExtraHTTPHeaders({ foo: "bar" });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const reqPromise = server.waitForWebSocketConnectionRequest();
    // [ts] page.evaluate(host => {
    // [ts] const code = `new WebSocket(${JSON.stringify("ws://" + host + "/ws")});`;
    // [ts] Worker.init(URL.createObjectURL(Blob.init([code], { type: "text/javascript" })));
    // [ts] }, server.HOST);
    // [ts] const req = reqPromise;
    // [ts] expect_eq(req.headers["foo"], "bar");
    // [ts] });
    // [ts] test("WS-041 should navigate while an iframe opens WebSockets", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44005" },
    // [ts] }, (_p) => {
    // [ts] server.setRoute("/parent", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/html");
    // [ts] res.end("<iframe src=\"/child\"></iframe>");
    // [ts] });
    // [ts] server.setRoute("/child", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/html");
    // [ts] res.end(`<script>setInterval(() => new WebSocket('ws://' + location.host + '/ws'), 5)</script>`);
    // [ts] });
    // [ts] let i = 0;
    // [ts] while (i < 20) // [cap-migrate] for-step: ++i
    // [ts] page.goto(server.PREFIX + "/parent");
    // [ts] expect(page.frames()).toHaveLength(2);
    // [ts] expect_eq(page.evaluate(() => 1 + 1), 2);
    // [ts] });
}
