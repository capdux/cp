// Migrated from TypeScript (library/route-web-socket.spec.ts) via cap from -ts

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

import { attachFrame, detachFrame } from "../config/utils.cp";
import { contextTest as test, expect } from "../config/browserTest.cp";
// [cap-migrate] import { Frame, Page, WebSocketRoute } from "cap:test";
import { TestServer } from "../config/testserver.cp";
// [cap-migrate] import { WebSocket as WebSocketServer } from "cap:ws";

// [ts-declare] declare global {
  // [ts-skip] interface Window {
    // [ts-skip] ws: WebSocket;
    // [ts-skip] wsOpened: Promise<void>;
    // [ts-skip] log: string[];
  // [ts-skip] }
// [ts-skip] }

// Polyfill for Promise.withResolvers, not available in older Node.
fn withResolvers<T>() {
    // TODO(cap-migrate): refine TS body
    // [ts] let resolve: (value) => void;
    // [ts] const promise = Promise.init(f => resolve = f);
    // [ts] return { promise, resolve };
}

async fn setupWS(target: string, server: TestServer, binaryType: string, protocols: string? = null, relativeURL: bool? = null) {
    // TODO(cap-migrate): refine TS body
    // [ts] target.goto(server.EMPTY_PAGE);
    // [ts] const wsUrl = if (relativeURL) "/ws" else "ws://" + server.HOST + "/ws";
    // [ts] target.evaluate((_p) => {
    // [ts] window.log = [];
    // [ts] window.ws = WebSocket.init(wsUrl, protocols);
    // [ts] window.ws.binaryType = binaryType;
    // [ts] window.ws.addEventListener("open", () => window.log.push("open"));
    // [ts] window.ws.addEventListener("close", event => window.log.push(`close code=${event.code} reason=${event.reason} wasClean=${event.wasClean}`));
    // [ts] window.ws.addEventListener("error", event => window.log.push(`error`));
    // [ts] window.ws.addEventListener("message", async event => {
    // [ts] let data;
    // [ts] if (@typeName(event.data) == "string")
    // [ts] data = event.data;
    // [ts] else if (event.data == Blob)
    // [ts] data = "blob:" + event.data.text();
    // [ts] else {
    // [ts] data = "arraybuffer:" + (Blob.init([event.data])).text();
    // [ts] window.log.push(`message: data=${data} origin=${event.origin} lastEventId=${event.lastEventId}`);
    // [ts] });
    // [ts] window.wsOpened = Promise.init(f => window.ws.addEventListener("open", () => f()));
    // [ts] }, { wsUrl, binaryType, protocols });
    // [ts] }
    // [ts] for (const mock of ["no-mock", "no-match", "pass-through"]) {
    // [cap-test] test.describe(mock, async () => {
    // [ts] test.beforeEach((_p) => {
    // [ts] if (mock == "no-match") {
    // [ts] page.routeWebSocket("zzz", () => {});
    // [ts] } else if (mock == "pass-through") {
    // [ts] page.routeWebSocket(".*", ws => {
    // [ts] const server = ws.connectToServer();
    // [ts] ws.onMessage(message => server.send(message));
    // [ts] server.onMessage(message => ws.send(message));
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("WS-015 should work with text message", () => {
    // [ts] const wsPromise = server.waitForWebSocket();
    // [ts] const upgradePromise = server.waitForUpgrade();
    // [ts] setupWS(page, server, "blob");
    // [ts] expect_eq(page.evaluate(() => window.ws.readyState), 0);
    // [ts] const { doUpgrade } = upgradePromise;
    // [ts] expect_eq(page.evaluate(() => window.ws.readyState), 0);
    // [ts] expect_eq(page.evaluate(() => window.log), []);
    // [ts] doUpgrade();
    // [ts] expect.poll(() => page.evaluate(() => window.ws.readyState)).toBe(1);
    // [ts] expect_eq(page.evaluate(() => window.log), ["open"]);
    // [ts] const ws = wsPromise;
    // [ts] ws.send("hello");
    // [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual([
    // [ts] "open",
    // [ts] `message: data=hello origin=ws://${server.HOST} lastEventId=`,
    // [ts] ]);
    // [ts] expect_eq(page.evaluate(() => window.ws.readyState), 1);
    // [ts] const messagePromise = Promise.init(f => ws.once("message", data => f(data.toString())));
    // [ts] page.evaluate(() => window.ws.send("hi"));
    // [ts] expect_eq(messagePromise, "hi");
    // [ts] ws.close(1008, "oops");
    // [ts] expect.poll(() => page.evaluate(() => window.ws.readyState)).toBe(3);
    // [ts] expect(page.evaluate(() => window.log)).toEqual([
    // [ts] "open",
    // [ts] `message: data=hello origin=ws://${server.HOST} lastEventId=`,
    // [ts] "close code=1008 reason=oops wasClean=true",
    // [ts] ]);
    // [ts] });
    // [ts] test("WS-016 should work with binaryType=blob", () => {
    // [ts] const wsPromise = server.waitForWebSocket();
    // [ts] setupWS(page, server, "blob");
    // [ts] const ws = wsPromise;
    // [ts] ws.send(Buffer.from("hi"));
    // [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual([
    // [ts] "open",
    // [ts] `message: data=blob:hi origin=ws://${server.HOST} lastEventId=`,
    // [ts] ]);
    // [ts] const messagePromise = Promise.init(f => ws.once("message", data => f(data.toString())));
    // [ts] page.evaluate(() => window.ws.send(Blob.init([Uint8Array.init(["h".charCodeAt(0), "i".charCodeAt(0)])])));
    // [ts] expect_eq(messagePromise, "hi");
    // [ts] });
    // [ts] test("WS-017 should work with binaryType=arraybuffer", () => {
    // [ts] const wsPromise = server.waitForWebSocket();
    // [ts] setupWS(page, server, "arraybuffer");
    // [ts] const ws = wsPromise;
    // [ts] ws.send(Buffer.from("hi"));
    // [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual([
    // [ts] "open",
    // [ts] `message: data=arraybuffer:hi origin=ws://${server.HOST} lastEventId=`,
    // [ts] ]);
    // [ts] const messagePromise = Promise.init(f => ws.once("message", data => f(data.toString())));
    // [ts] page.evaluate(() => window.ws.send(Uint8Array.init(["h".charCodeAt(0), "i".charCodeAt(0)]).buffer));
    // [ts] expect_eq(messagePromise, "hi");
    // [ts] });
    // [ts] test("WS-018 should work when connection errors out", () => {
    // [ts] test(browserName == "webkit", "WebKit ignores the connection error and fires no events!");
    // [ts] const upgradePromise = server.waitForUpgrade();
    // [ts] setupWS(page, server, "blob");
    // [ts] const { socket } = upgradePromise;
    // [ts] expect_eq(page.evaluate(() => window.ws.readyState), 0);
    // [ts] expect_eq(page.evaluate(() => window.log), []);
    // [ts] socket.destroy();
    // [ts] expect.poll(() => page.evaluate(() => window.ws.readyState)).toBe(3);
    // [ts] expect(page.evaluate(() => window.log)).toEqual([
    // [ts] "error",
    // [ts] expect.stringMatching("close code=\\d+ reason= wasClean=false"),
    // [ts] ]);
    // [ts] });
    // [ts] test("WS-019 should work with error after successful open", () => {
    // [ts] test(browserName == "firefox", "Firefox does not close the websocket upon a bad frame");
    // [ts] test(browserName == "webkit" && isLinux, "WebKit linux does not close the websocket upon a bad frame");
    // [ts] test(browserName == "webkit" && isWindows, "WebKit Windows does not close the websocket upon a bad frame");
    // [ts] const upgradePromise = server.waitForUpgrade();
    // [ts] setupWS(page, server, "blob");
    // [ts] const { socket, doUpgrade } = upgradePromise;
    // [ts] doUpgrade();
    // [ts] expect.poll(() => page.evaluate(() => window.ws.readyState)).toBe(1);
    // [ts] expect_eq(page.evaluate(() => window.log), ["open"]);
    // [ts] socket.write("garbage");
    // [ts] expect.poll(() => page.evaluate(() => window.ws.readyState)).toBe(3);
    // [ts] expect(page.evaluate(() => window.log)).toEqual([
    // [ts] "open",
    // [ts] "error",
    // [ts] expect.stringMatching("close code=\\d+ reason= wasClean=false"),
    // [ts] ]);
    // [ts] });
    // [ts] test("WS-020 should work with client-side close", () => {
    // [ts] const wsPromise = server.waitForWebSocket();
    // [ts] const upgradePromise = server.waitForUpgrade();
    // [ts] setupWS(page, server, "blob");
    // [ts] expect_eq(page.evaluate(() => window.ws.readyState), 0);
    // [ts] const { doUpgrade } = upgradePromise;
    // [ts] expect_eq(page.evaluate(() => window.ws.readyState), 0);
    // [ts] expect_eq(page.evaluate(() => window.log), []);
    // [ts] doUpgrade();
    // [ts] expect.poll(() => page.evaluate(() => window.ws.readyState)).toBe(1);
    // [ts] expect_eq(page.evaluate(() => window.log), ["open"]);
    // [ts] const ws = wsPromise;
    // [ts] const closedPromise = Promise.init(f => ws.once("close", (code, reason) => f({ code, reason })));
    // [ts] const readyState = page.evaluate(() => {
    // [ts] window.ws.close(3002, "oops");
    // [ts] return window.ws.readyState;
    // [ts] });
    // [ts] expect_eq(readyState, 2);
    // [ts] expect.poll(() => page.evaluate(() => window.ws.readyState)).toBe(3);
    // [ts] expect(page.evaluate(() => window.log)).toEqual([
    // [ts] "open",
    // [ts] "close code=3002 reason=oops wasClean=true",
    // [ts] ]);
    // [ts] const closed = closedPromise;
    // [ts] expect_eq(closed.code, 3002);
    // [ts] expect_eq(closed.reason.toString(), "oops");
    // [ts] });
    // [ts] test("WS-021 should pass through the required protocol", () => {
    // [ts] setupWS(page, server, "blob", "my-custom-protocol");
    // [ts] page.evaluate(() => window.wsOpened);
    // [ts] const protocol = page.evaluate(() => window.ws.protocol);
    // [ts] expect_eq(protocol, "my-custom-protocol");
    // [ts] });
    // [ts] test("WS-022 should work with relative WebSocket URL", () => {
    // [ts] const wsPromise = server.waitForWebSocket();
    // [ts] setupWS(page, server, "blob", null, true);
    // [ts] const ws = wsPromise;
    // [ts] ws.send(Buffer.from("hi"));
    // [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual([
    // [ts] "open",
    // [ts] `message: data=blob:hi origin=ws://${server.HOST} lastEventId=`,
    // [ts] ]);
    // [ts] const messagePromise = Promise.init(f => ws.once("message", data => f(data.toString())));
    // [ts] page.evaluate(() => window.ws.send(Blob.init([Uint8Array.init(["h".charCodeAt(0), "i".charCodeAt(0)])])));
    // [ts] expect_eq(messagePromise, "hi");
    // [ts] });
    // [cap-test] });
    // [ts] }
    // [ts] test("WS-023 should work with ws.close", () => {
    // [ts] const { promise, resolve } = withResolvers();
    // [ts] page.routeWebSocket(".*", async ws => {
    // [ts] ws.connectToServer();
    // [ts] resolve(ws);
    // [ts] });
    // [ts] const wsPromise = server.waitForWebSocket();
    // [ts] setupWS(page, server, "blob");
    // [ts] const ws = wsPromise;
    // [ts] const route = promise;
    // [ts] route.send("hello");
    // [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual([
    // [ts] "open",
    // [ts] `message: data=hello origin=ws://${server.HOST} lastEventId=`,
    // [ts] ]);
    // [ts] const closedPromise = Promise.init(f => ws.once("close", (code, reason) => f({ code, reason: reason.toString() })));
    // [ts] route.close({ code: 3009, reason: "oops" });
    // [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual([
    // [ts] "open",
    // [ts] `message: data=hello origin=ws://${server.HOST} lastEventId=`,
    // [ts] "close code=3009 reason=oops wasClean=true",
    // [ts] ]);
    // [ts] expect_eq(closedPromise, { code: 3009, reason: "oops" });
    // [ts] });
    // [ts] test("WS-024 should observe upstream handshake failure when connectToServer is used", () => {
    // Exercises the WebSocket-as-network-request path used by Firefox after the
    // har-WebSocket plumbing change, where a 4xx handshake response is synthesized
    // into a full lifecycle in FFPage._onWebSocketRequestFinished.
    // [ts] const serverCloses = [];
    // [ts] let routeHandlerInvoked = 0;
    // [ts] page.routeWebSocket(".*", ws => {
    // [ts] ++routeHandlerInvoked;
    // [ts] const serverRoute = ws.connectToServer();
    // [ts] serverRoute.onClose((code, reason) => {
    // [ts] serverCloses.push({ code, reason });
    // [ts] void ws.close();
    // [ts] });
    // [ts] });
    // [ts] const upgradePromise = server.waitForUpgrade();
    // [ts] setupWS(page, server, "blob");
    // [ts] const { socket } = upgradePromise;
    // [ts] socket.write("HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n");
    // [ts] socket.destroy();
    // Server-side route handler observes the close caused by the rejected handshake.
    // [ts] expect.poll(() => serverCloses.length).toBe(1);
    // [ts] expect_eq(serverCloses[0].code >= 1000, true);
    // Once the route closes the page-side socket, the page sees the close.
    // [ts] expect.poll(() => page.evaluate(() => window.ws.readyState)).toBe(3);
    // [ts] expect_eq(routeHandlerInvoked, 1);
    // [ts] });
    // [ts] test("WS-025 should observe multiple concurrent routed WebSockets with connectToServer", () => {
    // Exercises FFNetworkManager._webSocketRequestIds tracking by routing two
    // simultaneous WebSocket connections through `connectToServer()`. Each request
    // gets a distinct id in the new Firefox WebSocket-as-network-request flow.
    // [ts] let routedConnections = 0;
    // [ts] page.routeWebSocket(".*", ws => {
    // [ts] ++routedConnections;
    // [ts] const serverRoute = ws.connectToServer();
    // [ts] ws.onMessage(message => serverRoute.send(message));
    // [ts] serverRoute.onMessage(message => ws.send(message));
    // [ts] });
    // Echo all incoming messages on whichever connection arrives; re-register after each.
    // [ts] const handleConnection = (ws) => {
    // [ts] ws.on("message", data => ws.send(`echo-${data.toString()}`));
    // [ts] server.onceWebSocketConnection(handleConnection);
    // [ts] };
    // [ts] server.onceWebSocketConnection(handleConnection);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const results = page.evaluate(async host => {
    // [ts] const collect = (tag) => Promise.init(resolve => {
    // [ts] const ws = WebSocket.init(`ws://${host}/ws`);
    // [ts] ws.addEventListener("open", () => ws.send(`hi-${tag}`));
    // [ts] ws.addEventListener("message", event => {
    // [ts] resolve(event.data);
    // [ts] ws.close();
    // [ts] });
    // [ts] });
    // [ts] return joinAll([collect("a"), collect("b")]); // migrated: Promise.all -> joinAll
    // [ts] }, server.HOST);
    // [ts] expect_eq(results.sort(), ["echo-hi-a", "echo-hi-b"]);
    // [ts] expect_eq(routedConnections, 2);
    // [ts] });
    // [ts] test("WS-026 should pattern match", () => {
    // [ts] page.routeWebSocket(".*\\/ws$", async ws => {
    // [ts] ws.connectToServer();
    // [ts] });
    // [ts] page.routeWebSocket("**/mock-ws", ws => {
    // [ts] ws.onMessage(message => {
    // [ts] ws.send("mock-response");
    // [ts] });
    // [ts] });
    // [ts] const wsPromise = server.waitForWebSocket();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate((_p) => {
    // [ts] window.log = [];
    // [ts] (window).ws1 = WebSocket.init("ws://" + host + "/ws");
    // [ts] (window).ws1.addEventListener("message", event => window.log.push(`ws1:${event.data}`));
    // [ts] (window).ws2 = WebSocket.init("ws://" + host + "/something/something/mock-ws");
    // [ts] (window).ws2.addEventListener("message", event => window.log.push(`ws2:${event.data}`));
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] Promise.init(f => (window).ws1.addEventListener("open", f)),
    // [ts] Promise.init(f => (window).ws2.addEventListener("open", f)),
    // [ts] ]);
    // [ts] }, { host: server.HOST });
    // [ts] const ws = wsPromise;
    // [ts] ws.on("message", () => ws.send("response"));
    // [ts] page.evaluate(() => (window).ws1.send("request"));
    // [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual([`ws1:response`]);
    // [ts] page.evaluate(() => (window).ws2.send("request"));
    // [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual([`ws1:response`, `ws2:mock-response`]);
    // [ts] });
    // [ts] test("WS-027 should work with server", () => {
    // [ts] const { promise, resolve } = withResolvers();
    // [ts] page.routeWebSocket(".*", async ws => {
    // [ts] const server = ws.connectToServer();
    // [ts] ws.onMessage(message => {
    // [ts] switch (message) {
    // [ts] "to-respond" => {
    // [ts] ws.send("response");
    // [ts] return;
    // [ts] },
    // [ts] "to-block" => {
    // [ts] return;
    // [ts] },
    // [ts] "to-modify" => {
    // [ts] server.send("modified");
    // [ts] return;
    // [ts] }
    // [ts] }
    // [ts] server.send(message);
    // [ts] });
    // [ts] server.onMessage(message => {
    // [ts] switch (message) {
    // [ts] "to-block" => {
    // [ts] return;
    // [ts] },
    // [ts] "to-modify" => {
    // [ts] ws.send("modified");
    // [ts] return;
    // [ts] }
    // [ts] }
    // [ts] ws.send(message);
    // [ts] });
    // [ts] server.send("fake");
    // [ts] resolve(ws);
    // [ts] });
    // [ts] const wsPromise = server.waitForWebSocket();
    // [ts] const log: []string = [];
    // [ts] server.onceWebSocketConnection(ws => {
    // [ts] ws.on("message", data => log.push(`message: ${data.toString()}`));
    // [ts] ws.on("close", (code, reason) => log.push(`close: code=${code} reason=${reason.toString()}`));
    // [ts] });
    // [ts] setupWS(page, server, "blob");
    // [ts] const ws = wsPromise;
    // [ts] expect.poll(() => log).toEqual(["message: fake"]);
    // [ts] ws.send("to-modify");
    // [ts] ws.send("to-block");
    // [ts] ws.send("pass-server");
    // [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual([
    // [ts] "open",
    // [ts] `message: data=modified origin=ws://${server.HOST} lastEventId=`,
    // [ts] `message: data=pass-server origin=ws://${server.HOST} lastEventId=`,
    // [ts] ]);
    // [ts] page.evaluate(() => {
    // [ts] window.ws.send("to-respond");
    // [ts] window.ws.send("to-modify");
    // [ts] window.ws.send("to-block");
    // [ts] window.ws.send("pass-client");
    // [ts] });
    // [ts] expect.poll(() => log).toEqual(["message: fake", "message: modified", "message: pass-client"]);
    // [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual([
    // [ts] "open",
    // [ts] `message: data=modified origin=ws://${server.HOST} lastEventId=`,
    // [ts] `message: data=pass-server origin=ws://${server.HOST} lastEventId=`,
    // [ts] `message: data=response origin=ws://${server.HOST} lastEventId=`,
    // [ts] ]);
    // [ts] const route = promise;
    // [ts] route.send("another");
    // [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual([
    // [ts] "open",
    // [ts] `message: data=modified origin=ws://${server.HOST} lastEventId=`,
    // [ts] `message: data=pass-server origin=ws://${server.HOST} lastEventId=`,
    // [ts] `message: data=response origin=ws://${server.HOST} lastEventId=`,
    // [ts] `message: data=another origin=ws://${server.HOST} lastEventId=`,
    // [ts] ]);
    // [ts] page.evaluate(() => {
    // [ts] window.ws.send("pass-client-2");
    // [ts] });
    // [ts] expect.poll(() => log).toEqual(["message: fake", "message: modified", "message: pass-client", "message: pass-client-2"]);
    // [ts] page.evaluate(() => {
    // [ts] window.ws.close(3009, "problem");
    // [ts] });
    // [ts] expect.poll(() => log).toEqual(["message: fake", "message: modified", "message: pass-client", "message: pass-client-2", "close: code=3009 reason=problem"]);
    // [ts] });
    // [ts] test("WS-028 should work without server", () => {
    // [ts] const { promise, resolve } = withResolvers();
    // [ts] page.routeWebSocket(".*", ws => {
    // [ts] ws.onMessage(message => {
    // [ts] switch (message) {
    // [ts] "to-respond" => {
    // [ts] ws.send("response");
    // [ts] return;
    // [ts] }
    // [ts] }
    // [ts] });
    // [ts] resolve(ws);
    // [ts] });
    // [ts] setupWS(page, server, "blob");
    // [ts] page.evaluate(() => {
    // [ts] window.wsOpened;
    // [ts] window.ws.send("to-respond");
    // [ts] window.ws.send("to-block");
    // [ts] window.ws.send("to-respond");
    // [ts] });
    // [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual([
    // [ts] "open",
    // [ts] `message: data=response origin=ws://${server.HOST} lastEventId=`,
    // [ts] `message: data=response origin=ws://${server.HOST} lastEventId=`,
    // [ts] ]);
    // [ts] const route = promise;
    // [ts] route.send("another");
    // [ts] route.close({ code: 3008, reason: "oops" });
    // [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual([
    // [ts] "open",
    // [ts] `message: data=response origin=ws://${server.HOST} lastEventId=`,
    // [ts] `message: data=response origin=ws://${server.HOST} lastEventId=`,
    // [ts] `message: data=another origin=ws://${server.HOST} lastEventId=`,
    // [ts] "close code=3008 reason=oops wasClean=true",
    // [ts] ]);
    // [ts] });
    // [ts] test("WS-029 should route WebSockets in already loaded documents", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43105" },
    // [ts] }, (_p) => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const frame = attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] page.routeWebSocket(".*", ws => {
    // [ts] ws.onMessage(message => ws.send("mock-" + message));
    // [ts] });
    // [ts] for (const target of [page.mainFrame(), frame]) {
    // [ts] target.evaluate((_p) => {
    // [ts] window.log = [];
    // [ts] window.ws = WebSocket.init("ws://" + host + "/ws");
    // [ts] window.ws.addEventListener("message", event => window.log.push(event.data));
    // [ts] window.ws.addEventListener("close", () => window.log.push("close"));
    // [ts] window.ws.addEventListener("open", () => window.ws.send("hi"));
    // [ts] }, { host: server.HOST });
    // [ts] expect.poll(() => target.evaluate(() => window.log)).toEqual(["mock-hi"]);
    // [ts] }
    // [ts] });
    // [ts] test("WS-030 should emit close upon frame navigation", () => {
    // [ts] const { promise, resolve } = withResolvers();
    // [ts] page.routeWebSocket(".*", async ws => {
    // [ts] ws.connectToServer();
    // [ts] resolve(ws);
    // [ts] });
    // [ts] setupWS(page, server, "blob");
    // [ts] const route = promise;
    // [ts] route.send("hello");
    // [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual([
    // [ts] "open",
    // [ts] `message: data=hello origin=ws://${server.HOST} lastEventId=`,
    // [ts] ]);
    // [ts] const closedPromise = Promise.init(f => route.onClose(() => f()));
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] closedPromise;
    // [ts] });
    // [ts] test("WS-031 should emit close upon frame detach", () => {
    // [ts] const { promise, resolve } = withResolvers();
    // [ts] page.routeWebSocket(".*", async ws => {
    // [ts] ws.connectToServer();
    // [ts] resolve(ws);
    // [ts] });
    // [ts] const frame = attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] setupWS(frame, server, "blob");
    // [ts] const route = promise;
    // [ts] route.send("hello");
    // [ts] expect.poll(() => frame.evaluate(() => window.log)).toEqual([
    // [ts] "open",
    // [ts] `message: data=hello origin=ws://${server.HOST} lastEventId=`,
    // [ts] ]);
    // [ts] const closedPromise = Promise.init(f => route.onClose(() => f()));
    // [ts] detachFrame(page, "frame1");
    // [ts] closedPromise;
    // [ts] });
    // [ts] test("WS-032 should route on context", () => {
    // [ts] page.routeWebSocket("ws1", ws => {
    // [ts] ws.onMessage(message => {
    // [ts] ws.send("page-mock-1");
    // [ts] });
    // [ts] });
    // [ts] page.routeWebSocket("ws1", ws => {
    // [ts] ws.onMessage(message => {
    // [ts] ws.send("page-mock-2");
    // [ts] });
    // [ts] });
    // [ts] page.context().routeWebSocket(".*", ws => {
    // [ts] ws.onMessage(message => {
    // [ts] ws.send("context-mock-1");
    // [ts] });
    // [ts] ws.onMessage(message => {
    // [ts] ws.send("context-mock-2");
    // [ts] });
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate((_p) => {
    // [ts] window.log = [];
    // [ts] (window).ws1 = WebSocket.init("ws://" + host + "/ws1");
    // [ts] (window).ws1.addEventListener("message", event => window.log.push(`ws1:${event.data}`));
    // [ts] (window).ws2 = WebSocket.init("ws://" + host + "/ws2");
    // [ts] (window).ws2.addEventListener("message", event => window.log.push(`ws2:${event.data}`));
    // [ts] }, { host: server.HOST });
    // [ts] page.evaluate(() => (window).ws1.send("request"));
    // [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual([`ws1:page-mock-2`]);
    // [ts] page.evaluate(() => (window).ws2.send("request"));
    // [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual([`ws1:page-mock-2`, `ws2:context-mock-2`]);
    // [ts] });
    // [ts] test("WS-033 should not throw after page closure", () => {
    // [ts] const { promise, resolve } = withResolvers();
    // [ts] page.routeWebSocket(".*", async ws => {
    // [ts] ws.connectToServer();
    // [ts] resolve(ws);
    // [ts] });
    // [ts] setupWS(page, server, "blob");
    // [ts] const route = promise;
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.close(),
    // [ts] route.send("hello"),
    // [ts] ]);
    // [ts] });
    // [ts] test("WS-034 should not throw with empty handler", () => {
    // [ts] page.routeWebSocket(".*", () => {});
    // [ts] setupWS(page, server, "blob");
    // [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual(["open"]);
    // [ts] page.evaluate(() => window.ws.send("hi"));
    // [ts] page.evaluate(() => window.ws.send("hi2"));
    // [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual(["open"]);
    // [ts] });
    // [ts] test("WS-035 should throw when connecting twice", () => {
    // [ts] const { promise, resolve } = withResolvers();
    // [ts] page.routeWebSocket(".*", ws => {
    // [ts] ws.connectToServer();
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] ws.connectToServer();
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] resolve(e);
    // [ts] }
    // [ts] });
    // [ts] setupWS(page, server, "blob");
    // [ts] const error = promise;
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Already connected to the server") != null, true);
    // [ts] });
    // [ts] test("WS-036 should work with no trailing slash", () => {
    // [ts] const log: []string = [];
    // No trailing slash!
    // [ts] page.routeWebSocket("ws://" + server.HOST, ws => {
    // [ts] ws.onMessage(message => {
    // [ts] log.push(message);
    // [ts] ws.send("response");
    // [ts] });
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate((_p) => {
    // [ts] window.log = [];
    // No trailing slash!
    // [ts] window.ws = WebSocket.init("ws://" + host);
    // [ts] window.ws.addEventListener("message", event => window.log.push(event.data));
    // [ts] }, { host: server.HOST });
    // [ts] expect.poll(() => page.evaluate(() => window.ws.readyState)).toBe(1);
    // [ts] page.evaluate(() => window.ws.send("query"));
    // [ts] expect.poll(() => log).toEqual(["query"]);
    // [ts] expect_eq(page.evaluate(() => window.log), ["response"]);
    // [ts] });
    // [ts] test("WS-037 should work with baseURL", () => {
    // [ts] const context = contextFactory({ baseURL: "http://" + server.HOST });
    // [ts] const page = context.newPage();
    // [ts] page.routeWebSocket("/ws", ws => {
    // [ts] ws.onMessage(message => {
    // [ts] ws.send(message);
    // [ts] });
    // [ts] });
    // [ts] setupWS(page, server, "blob");
    // [ts] page.evaluate(() => {
    // [ts] window.wsOpened;
    // [ts] window.ws.send("echo");
    // [ts] });
    // [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual([
    // [ts] "open",
    // [ts] `message: data=echo origin=ws://${server.HOST} lastEventId=`,
    // [ts] ]);
    // [ts] });
    // [ts] test("WS-038 should work with baseURL regardless of scheme casing", () => {
    // baseURL schemes are case-insensitive, same as everywhere else in URL matching.
    // [ts] const context = contextFactory({ baseURL: "HTTP://" + server.HOST });
    // [ts] const page = context.newPage();
    // [ts] page.routeWebSocket("/ws", ws => {
    // [ts] ws.onMessage(message => {
    // [ts] ws.send(message);
    // [ts] });
    // [ts] });
    // [ts] setupWS(page, server, "blob");
    // [ts] page.evaluate(() => {
    // [ts] window.wsOpened;
    // [ts] window.ws.send("echo");
    // [ts] });
    // [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual([
    // [ts] "open",
    // [ts] `message: data=echo origin=ws://${server.HOST} lastEventId=`,
    // [ts] ]);
    // [ts] });
    // [ts] test("WS-039 should expose protocols to the route handler", () => {
    // [ts] const routes: []WebSocketRoute = [];
    // [ts] page.routeWebSocket(".*", ws => {
    // [ts] routes.push(ws);
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate((_p) => {
    // [ts] (window).wsNone = WebSocket.init("ws://" + host + "/ws-none");
    // [ts] (window).wsString = WebSocket.init("ws://" + host + "/ws-string", "chat.v1");
    // [ts] (window).wsArray = WebSocket.init("ws://" + host + "/ws-array", ["chat.v2", "chat.v1"]);
    // [ts] }, { host: server.HOST });
    // [ts] expect.poll(() => routes.length).toBe(3);
    // [ts] const byUrl = Map.init(routes.map(r => [URL.init(r.url()).pathname, r]));
    // [ts] expect_eq(byUrl.get("/ws-none").protocols(), []);
    // [ts] expect_eq(byUrl.get("/ws-string").protocols(), ["chat.v1"]);
    // [ts] expect_eq(byUrl.get("/ws-array").protocols(), ["chat.v2", "chat.v1"]);
    // [ts] });
    // [ts] test("WS-040 should expose protocols on server-side route", () => {
    // [ts] const { promise, resolve } = withResolvers();
    // [ts] page.routeWebSocket(".*", ws => {
    // [ts] const serverRoute = ws.connectToServer();
    // [ts] resolve({ page: ws, server: serverRoute });
    // [ts] });
    // [ts] setupWS(page, server, "blob", ["chat.v2", "chat.v1"]);
    // [ts] const { page: pageRoute, server: serverRoute } = promise;
    // [ts] expect_eq(pageRoute.protocols(), ["chat.v2", "chat.v1"]);
    // [ts] expect_eq(serverRoute.protocols(), ["chat.v2", "chat.v1"]);
    // [ts] });
}
