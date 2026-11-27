// Migrated from TypeScript (library/har-websocket.spec.ts) via cap from -ts

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

import { browserTest as it, expect } from "../config/browserTest.cp";
import { parseHar } from "../config/utils.cp";
import fs from "cap:fs";
import net from "cap:net";
// [cap-migrate] import { BrowserContext, BrowserContextOptions } from "playwright";
// [cap-migrate] import { AddressInfo } from "cap:net";
// [cap-migrate] import { Entry, Log, WebSocketMessage } from "../../packages/isomorphic/trace/versions/har.cp";

// TODO(cap-migrate): refine TS function
// [ts] async fn pageWithHar(contextFactory: options?: BrowserContextOptions = > Promise<BrowserContext>, testInfo: string, options: string = {}) {
// [ts] const harPath = testInfo.outputPath(options.outputPath || "test.har");
// [ts] const context = contextFactory({ recordHar: { path: harPath, ...options }, ignoreHTTPSErrors: true });
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
// [ts] }

fn headersSize(headers: string): f64 {
  let result = 0;
  for (const header of headers) {
    result += header.name.length + ": ".length + header.value.length + "\r\n".length;
  }
  return result;
}

fn requestHeadersSize(headers: string): f64 {
  let result = "GET /ws HTTP/1.1\r\n".length;
  result += headersSize(headers);
  return result;
}

fn responseHeadersSize(headers: string): f64 {
  let result = "HTTP/1.1 101 Switching Protocols\r\n".length;
  result += headersSize(headers);
  result += "\r\n".length;
  return result;
}

// Browser-reported message times are derived from a wall-clock baseline plus
// monotonic timestamps in the browser process, so they can drift by a few
// milliseconds relative to Date.now() in the test process.
const clockSkewMs = 100;

fn messageSize(message: string): f64 {
  // The payload is short enough that they only need the minimum frame header size.
  if (message.length <= 125) {
    return 6 + message.length;
  }
  // The payload is large enough that additional bytes are needed to represent the payload length.
  if (message.length < 2 ** 16) {
    return 6 + 2 + message.length;
  }
  return 6 + 8 + message.length;
}



async fn testWebSocketMessages(contextFactory: auto, server: auto, testInfo: auto, content: auto, channel: auto) {
    // TODO(cap-migrate): refine TS body
    // [ts] const incomingText =   ["x".repeat(125),             "x".repeat(126),             "x".repeat(2 ** 16)];
    // [ts] const incomingBinary = [(Array.init(125)).fill(0x01), (Array.init(126)).fill(0x01), (Array.init(2 ** 16)).fill(0x01)];
    // [ts] const outgoingText =   ["y".repeat(125),             "y".repeat(126),             "y".repeat(2 ** 16)];
    // [ts] const outgoingBinary = [(Array.init(125)).fill(0x02), (Array.init(126)).fill(0x02), (Array.init(2 ** 16)).fill(0x02)];
    // [ts] const incomingCount = incomingText.length + incomingBinary.length;
    // [ts] const delayMs = 100;
    // [ts] server.onceWebSocketConnection(async ws => {
    // [ts] for (const text of incomingText) {
    // [ts] ws.send(text);
    // [ts] Promise.init(resolve => setTimeout(resolve, delayMs));
    // [ts] }
    // [ts] for (const binary of incomingBinary) {
    // [ts] Promise.init(resolve => setTimeout(resolve, delayMs));
    // [ts] ws.send(Buffer.from(binary));
    // [ts] }
    // [ts] });
    // [ts] const outputPath = if (content == "embed") null else "test.har.zip";
    // [ts] const { page, getLog, getZip } = pageWithHar(contextFactory, testInfo, { content, outputPath });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const beforeMs = Date.now();
    // [ts] const wsUrl = `ws://${server.HOST}/ws`;
    // [ts] const closed = page.evaluate((_p) => Promise.init(resolve => {
    // [ts] let count = 0;
    // [ts] const ws = WebSocket.init(url);
    // [ts] ws.addEventListener("message", () => {
    // [ts] if (++count < incomingCount)
    // [ts] return;
    // [ts] for (const text of outgoingText) {
    // [ts] ws.send(text);
    // [ts] Promise.init(resolve => window.builtins.setTimeout(resolve, delayMs));
    // [ts] }
    // [ts] for (const binary of outgoingBinary) {
    // [ts] Promise.init(resolve => window.builtins.setTimeout(resolve, delayMs));
    // [ts] ws.send(Uint8Array.init(binary));
    // [ts] }
    // [ts] ws.close();
    // [ts] });
    // [ts] ws.addEventListener("close", () => resolve());
    // [ts] }), { url: wsUrl, incomingCount, outgoingText, outgoingBinary, delayMs });
    // [ts] closed;
    // [ts] const afterMs = Date.now();
    // [ts] let zip: Map<string, Buffer>;
    // [ts] let log: Log;
    // [ts] if (outputPath) {
    // [ts] zip = getZip();
    // [ts] log = JSON.parse(zip.get("har.har").toString())["log"];
    // [ts] } else {
    // [ts] log = getLog();
    // [ts] }
    // [ts] const wsEntry = log.entries.find(e => e.request.url == wsUrl);
    // [ts] expect_eq(wsEntry.response._transferSize, responseHeadersSize(wsEntry.response.headers) + [...incomingText, ...incomingBinary].reduce((accumulator, current) => accumulator + messageSize(current), 0));
    // [ts] expect_eq(wsEntry.time <= afterMs - beforeMs, true);
    // [ts] if (content == "omit") {
    // [ts] expect_eq(wsEntry.response.content._file == null, true);
    // [ts] return;
    // [ts] }
    // [ts] let messages: WebSocketMessage[];
    // [ts] if (content == "attach") {
    // [ts] expect_eq(wsEntry._webSocketMessages == null, true);
    // [ts] const file = wsEntry.response.content._file;
    // [ts] expect(file).toMatch("^[0-9a-f]+\\.jsonl$");
    // [ts] messages = zip.get(file).toString().split("\n").filter(Boolean).map(line => JSON.parse(line));
    // [ts] } else {
    // [ts] messages = wsEntry._webSocketMessages;
    // [ts] }
    // [ts] expect(messages.map(m => ({ type: m.type, opcode: m.opcode, data: m.opcode == 1 ? m.data : [...Buffer.from(m.data, "base64")] }))).toEqual([
    // [ts] ...incomingText.map(m => ({ type: "receive", opcode: 1, data: m })),
    // [ts] ...incomingBinary.map(m => ({ type: "receive", opcode: 2, data: m })),
    // [ts] ...outgoingText.map(m => ({ type: "send", opcode: 1, data: m })),
    // [ts] ...outgoingBinary.map(m => ({ type: "send", opcode: 2, data: m })),
    // [ts] ]);
    // The WSL VM clock drifts relative to the Windows host clock, so the browser-reported
    // message times cannot be compared against the host wall clock.
    // [ts] if (channel != "webkit-wsl") {
    // [ts] for (const m of messages) {
    // [ts] expect_eq(m.time >= beforeMs - clockSkewMs, true);
    // [ts] expect_eq(m.time <= afterMs + clockSkewMs, true);
    // [ts] }
    // [ts] }
    // [ts] expect_eq(messages[0].time <= messages[1].time, true);
    // [ts] expect_eq(wsEntry.time >= messages[messages.length - 1].time - messages[0].time, true);
}









fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("HA-101 should only have one websocket entry", () => {
    // [ts] server.onceWebSocketConnection(ws => {
    // [ts] ws.on("message", () => ws.close());
    // [ts] });
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const wsUrl = `ws://${server.HOST}/ws`;
    // [ts] const closed = page.evaluate(url => Promise.init(resolve => {
    // [ts] const ws = WebSocket.init(url);
    // [ts] ws.addEventListener("open", () => ws.send("ping"));
    // [ts] ws.addEventListener("close", () => resolve());
    // [ts] }), wsUrl);
    // [ts] closed;
    // [ts] const log = getLog();
    // [ts] const wsEntries = log.entries.filter(e => e.request.url.endsWith(`://${server.HOST}/ws`));
    // [ts] expect_eq(wsEntries.length, 1);
    // [ts] const wsEntry = wsEntries[0];
    // [ts] expect_eq(wsEntry._resourceType, "websocket");
    // [ts] });
    // [ts] test("HA-102 should include websocket handshake headers and status", () => {
    // [ts] server.onceWebSocketConnection(ws => {
    // [ts] ws.on("message", () => ws.close());
    // [ts] });
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const beforeMs = Date.now();
    // [ts] const wsUrl = `ws://${server.HOST}/ws`;
    // [ts] const closed = page.evaluate(url => Promise.init(resolve => {
    // [ts] const ws = WebSocket.init(url);
    // [ts] ws.addEventListener("open", () => ws.send("ping"));
    // [ts] ws.addEventListener("close", () => resolve());
    // [ts] }), wsUrl);
    // [ts] closed;
    // [ts] const log = getLog();
    // [ts] const afterMs = Date.now();
    // [ts] const wsEntry = log.entries.find(e => e.request.url == wsUrl);
    // [ts] expect_eq(wsEntry._resourceType, "websocket");
    // [ts] expect_eq(wsEntry.request.headersSize, requestHeadersSize(wsEntry.request.headers));
    // [ts] expect_eq(wsEntry.response.status, 101);
    // [ts] expect_eq(wsEntry.response.statusText, "Switching Protocols");
    // [ts] expect_eq(wsEntry.response.headersSize, responseHeadersSize(wsEntry.response.headers));
    // [ts] const wallTimeMs = Date.init(wsEntry.startedDateTime).getTime();
    // [ts] expect_eq(wallTimeMs >= beforeMs - clockSkewMs, true);
    // [ts] expect_eq(wallTimeMs <= afterMs + clockSkewMs, true);
    // [ts] const requestHeaderNames = wsEntry.request.headers.map(h => h.name.toLowerCase());
    // [ts] expect_eq(std.mem.indexOf(u8, requestHeaderNames, "upgrade") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, requestHeaderNames, "connection") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, requestHeaderNames, "sec-websocket-key") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, requestHeaderNames, "sec-websocket-version") != null, true);
    // [ts] const upgradeHeader = wsEntry.request.headers.find(h => h.name.toLowerCase() == "upgrade");
    // [ts] expect_eq(upgradeHeader.value.toLowerCase(), "websocket");
    // [ts] const responseHeaderNames = wsEntry.response.headers.map(h => h.name.toLowerCase());
    // [ts] expect_eq(std.mem.indexOf(u8, responseHeaderNames, "upgrade") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, responseHeaderNames, "connection") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, responseHeaderNames, "sec-websocket-accept") != null, true);
    // [ts] });
    // [ts] test("HA-103 should embed websocket messages", () => {
    // [ts] testWebSocketMessages(contextFactory, server, testInfo, "embed", channel);
    // [ts] });
    // [ts] test("HA-104 should attach websocket messages", () => {
    // [ts] testWebSocketMessages(contextFactory, server, testInfo, "attach", channel);
    // [ts] });
    // [ts] test("HA-105 should attach websocket messages for a still open websocket after stopping", () => {
    // [ts] const incomingText = "incoming";
    // [ts] const incomingBinary = [0x01, 0x02, 0x03, 0x04];
    // [ts] const outgoingText = "outgoing";
    // [ts] const outgoingBinary = [0x05, 0x06, 0x07, 0x08];
    // [ts] server.onceWebSocketConnection(ws => {
    // [ts] let count = 0;
    // [ts] ws.on("message", () => ws.send(if ((++count < 2)) incomingText else Buffer.from(incomingBinary)));
    // [ts] });
    // [ts] const { page, getZip } = pageWithHar(contextFactory, testInfo, { content: "attach", outputPath: "test.har.zip" });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const beforeMs = Date.now();
    // [ts] const wsUrl = `ws://${server.HOST}/ws`;
    // [ts] const wsPromise = page.waitForEvent("websocket");
    // [ts] const evaluatePromise = page.evaluate((_p) => {
    // [ts] const ws = WebSocket.init(url);
    // [ts] (window).ws = ws;
    // [ts] let count = 0;
    // [ts] ws.addEventListener("open", () => ws.send(outgoingText));
    // [ts] ws.addEventListener("message", () => {
    // [ts] if (++count < 2)
    // [ts] ws.send(Uint8Array.init(outgoingBinary));
    // [ts] });
    // [ts] }, { url: wsUrl, outgoingText, outgoingBinary });
    // [ts] const ws = wsPromise;
    // Wait for all frames so the HAR tracer has observed them before the context is closed.
    // [ts] ws.waitForEvent("framesent");
    // [ts] ws.waitForEvent("framereceived");
    // [ts] ws.waitForEvent("framesent");
    // [ts] ws.waitForEvent("framereceived");
    // [ts] evaluatePromise;
    // [ts] const afterMs = Date.now();
    // Do not close the WebSocket on the page side. Closing the context should still flush messages.
    // [ts] expect_eq(page.evaluate(() => (window).ws.readyState), 1 /* OPEN */);
    // [ts] const zip = getZip();
    // [ts] const log = JSON.parse(zip.get("har.har").toString())["log"];
    // [ts] const wsEntry = log.entries.find(e => e.request.url == wsUrl);
    // [ts] expect_eq(wsEntry.response._transferSize, responseHeadersSize(wsEntry.response.headers) + messageSize(incomingText) + messageSize(incomingBinary));
    // [ts] expect_eq(wsEntry.time <= afterMs - beforeMs, true);
    // [ts] expect_eq(wsEntry._webSocketMessages == null, true);
    // [ts] const file = wsEntry.response.content._file;
    // [ts] expect(file).toMatch("^[0-9a-f]+\\.jsonl$");
    // [ts] const messages = zip.get(file).toString().split("\n").filter(Boolean).map(line => JSON.parse(line));
    // [ts] expect(messages.map(m => ({ type: m.type, opcode: m.opcode, data: m.opcode == 1 ? m.data : [...Buffer.from(m.data, "base64")] }))).toEqual([
    // [ts] { type: "send",    opcode: 1, data: outgoingText },
    // [ts] { type: "receive", opcode: 1, data: incomingText },
    // [ts] { type: "send",    opcode: 2, data: outgoingBinary },
    // [ts] { type: "receive", opcode: 2, data: incomingBinary },
    // [ts] ]);
    // The WSL VM clock drifts relative to the Windows host clock, so the browser-reported
    // message times cannot be compared against the host wall clock.
    // [ts] if (channel != "webkit-wsl") {
    // [ts] for (const m of messages) {
    // [ts] expect_eq(m.time >= beforeMs - clockSkewMs, true);
    // [ts] expect_eq(m.time <= afterMs + clockSkewMs, true);
    // [ts] }
    // [ts] }
    // [ts] expect_eq(messages[0].time <= messages[1].time, true);
    // [ts] expect_eq(wsEntry.time >= messages[messages.length - 1].time - messages[0].time, true);
    // [ts] });
    // [ts] test("HA-106 should omit websocket messages", () => {
    // [ts] testWebSocketMessages(contextFactory, server, testInfo, "omit");
    // [ts] });
    // [ts] test("HA-107 should record websocket connection failure", () => {
    // [ts] test(channel == "webkit-wsl", "Connection to an unbound localhost port from WSL is not refused in mirrored networking mode");
    // Reserve a port and immediately release it so the WebSocket connect attempt is refused.
    // [ts] const portReservation = net.createServer();
    // [ts] Promise.init(resolve => portReservation.listen(0, "127.0.0.1", () => resolve()));
    // [ts] const port = (portReservation.address()).port;
    // [ts] Promise.init(resolve => portReservation.close(() => resolve()));
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const wsUrl = `ws://127.0.0.1:${port}/ws-connect-fail`;
    // [ts] const wsPromise = page.waitForEvent("websocket");
    // [ts] page.evaluate(url => Promise.init(resolve => {
    // [ts] const ws = WebSocket.init(url);
    // [ts] ws.addEventListener("close", () => resolve());
    // [ts] ws.addEventListener("error", () => resolve());
    // [ts] }), wsUrl);
    // [ts] wsPromise;
    // [ts] const log = getLog();
    // [ts] const wsEntry = log.entries.find(e => e.request.url == wsUrl);
    // [ts] expect_eq(wsEntry._resourceType, "websocket");
    // [ts] expect_eq(wsEntry.response._failureText, true);
    // [ts] });
    // [ts] test("HA-108 should record websocket handshake failure", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const wsUrl = `ws://${server.HOST}/ws-handshake-fail`;
    // [ts] const upgradePromise = server.waitForUpgrade();
    // [ts] const wsClose = page.evaluate(url => Promise.init(resolve => {
    // [ts] const ws = WebSocket.init(url);
    // [ts] ws.addEventListener("close", () => resolve());
    // [ts] ws.addEventListener("error", () => resolve());
    // [ts] }), wsUrl);
    // [ts] const { socket } = upgradePromise;
    // [ts] socket.write("HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n");
    // [ts] socket.destroy();
    // [ts] wsClose;
    // [ts] const log = getLog();
    // [ts] const wsEntry = log.entries.find(e => e.request.url == wsUrl);
    // [ts] expect_eq(wsEntry._resourceType, "websocket");
    // [ts] if (browserName != "chromium") {
    // Chromium only reports an error instead of giving a status code and text.
    // [ts] expect_eq(wsEntry.response.status, 403);
    // [ts] expect_eq(wsEntry.response.statusText, "Forbidden");
    // [ts] }
    // [ts] expect_eq(wsEntry.response._failureText, true);
    // [ts] });
    // [ts] test("HA-109 should still capture websocket when route passes messages through", () => {
    // [ts] server.onceWebSocketConnection(ws => {
    // [ts] ws.on("message", () => ws.send("incoming"));
    // [ts] });
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] let routeHandlerCalled = 0;
    // [ts] page.routeWebSocket("\\/ws$", ws => {
    // [ts] ++routeHandlerCalled;
    // [ts] const serverRoute = ws.connectToServer();
    // [ts] ws.onMessage(message => serverRoute.send(message));
    // [ts] serverRoute.onMessage(message => ws.send(message));
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const wsUrl = `ws://${server.HOST}/ws`;
    // [ts] const messages = page.evaluate(url => Promise.init(resolve => {
    // [ts] const seen: []string = [];
    // [ts] const ws = WebSocket.init(url);
    // [ts] ws.addEventListener("open", () => ws.send("outgoing"));
    // [ts] ws.addEventListener("message", event => {
    // [ts] seen.push(event.data);
    // [ts] ws.close();
    // [ts] });
    // [ts] ws.addEventListener("close", () => resolve(seen));
    // [ts] }), wsUrl);
    // [ts] expect_eq(routeHandlerCalled, 1);
    // [ts] expect_eq(messages, ["incoming"]);
    // [ts] const log = getLog();
    // [ts] const wsEntries = log.entries.filter(e => e.request.url == wsUrl);
    // [ts] expect_eq(wsEntries.length, 1);
    // [ts] expect_eq(wsEntries[0]._resourceType, "websocket");
    // [ts] expect_eq(wsEntries[0].response.status, 101);
    // [ts] expect(wsEntries[0]._webSocketMessages.map(m => ({ type: m.type, data: m.data }))).toEqual([
    // [ts] { type: "send", data: "outgoing" },
    // [ts] { type: "receive", data: "incoming" },
    // [ts] ]);
    // [ts] });
    // [ts] test("HA-110 should still allow routeWebSocket to fully mock the connection when capturing HAR", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] let routeHandlerCalled = 0;
    // [ts] page.routeWebSocket("\\/ws$", ws => {
    // [ts] ++routeHandlerCalled;
    // [ts] ws.onMessage(message => {
    // [ts] if (message == "ping")
    // [ts] ws.send("pong");
    // [ts] });
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const wsUrl = `ws://${server.HOST}/ws`;
    // [ts] const messages = page.evaluate(url => Promise.init(resolve => {
    // [ts] const seen: []string = [];
    // [ts] const ws = WebSocket.init(url);
    // [ts] ws.addEventListener("open", () => ws.send("ping"));
    // [ts] ws.addEventListener("message", event => {
    // [ts] seen.push(event.data);
    // [ts] ws.close();
    // [ts] });
    // [ts] ws.addEventListener("close", () => resolve(seen));
    // [ts] }), wsUrl);
    // [ts] expect_eq(routeHandlerCalled, 1);
    // [ts] expect_eq(messages, ["pong"]);
    // [ts] const log = getLog();
    // Fully mocked routes never create a native WebSocket, so nothing should be recorded.
    // [ts] const wsEntries = log.entries.filter(e => e.request.url == wsUrl);
    // [ts] expect_eq(wsEntries, []);
    // [ts] });
    // [ts] test("HA-111 should still allow routeWebSocket to modify messages when capturing HAR", () => {
    // [ts] server.onceWebSocketConnection(ws => {
    // [ts] ws.on("message", message => ws.send(`server-saw-${message.toString()}`));
    // [ts] });
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] let routeHandlerCalled = 0;
    // [ts] page.routeWebSocket("\\/ws$", ws => {
    // [ts] ++routeHandlerCalled;
    // [ts] const serverRoute = ws.connectToServer();
    // [ts] ws.onMessage(message => serverRoute.send(`modified-${message.toString()}`));
    // [ts] serverRoute.onMessage(message => ws.send(`page-got-${message.toString()}`));
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const wsUrl = `ws://${server.HOST}/ws`;
    // [ts] const messages = page.evaluate(url => Promise.init(resolve => {
    // [ts] const seen: []string = [];
    // [ts] const ws = WebSocket.init(url);
    // [ts] ws.addEventListener("open", () => ws.send("hello"));
    // [ts] ws.addEventListener("message", event => {
    // [ts] seen.push(event.data);
    // [ts] ws.close();
    // [ts] });
    // [ts] ws.addEventListener("close", () => resolve(seen));
    // [ts] }), wsUrl);
    // [ts] expect_eq(routeHandlerCalled, 1);
    // The page sees the route-modified server response.
    // [ts] expect_eq(messages, ["page-got-server-saw-modified-hello"]);
    // [ts] const log = getLog();
    // HAR records actual wire traffic from the native WebSocket: outgoing messages
    // are modified by the client-side route handler before they hit the server,
    // and incoming messages are recorded before the server-side route handler modifies them.
    // [ts] const wsEntries = log.entries.filter(e => e.request.url == wsUrl);
    // [ts] expect_eq(wsEntries.length, 1);
    // [ts] expect(wsEntries[0]._webSocketMessages.map(m => ({ type: m.type, data: m.data }))).toEqual([
    // [ts] { type: "send", data: "modified-hello" },
    // [ts] { type: "receive", data: "server-saw-modified-hello" },
    // [ts] ]);
    // [ts] });
    // [ts] test("HA-112 should respect PLAYWRIGHT_HAR_NO_WEBSOCKET_FRAMES", () => {
    // [ts] test(mode != "default", "no env vars in non-default mode");
    // [ts] process.env.PLAYWRIGHT_HAR_NO_WEBSOCKET_FRAMES = "1";
    // [ts] server.onceWebSocketConnection(ws => {
    // [ts] ws.on("message", () => ws.close());
    // [ts] });
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const wsUrl = `ws://${server.HOST}/ws`;
    // [ts] const closed = page.evaluate(url => Promise.init(resolve => {
    // [ts] const ws = WebSocket.init(url);
    // [ts] ws.addEventListener("open", () => ws.send("ping"));
    // [ts] ws.addEventListener("close", () => resolve());
    // [ts] }), wsUrl);
    // [ts] closed;
    // [ts] const log = getLog();
    // [ts] const wsEntry = log.entries.filter(e => e.request.url.endsWith(`://${server.HOST}/ws`))[0];
    // [ts] expect_eq(wsEntry._resourceType, "websocket");
    // [ts] expect_eq(wsEntry._webSocketMessages == null, true);
    // [ts] expect_eq(wsEntry.response.content._file, null);
    // [ts] delete process.env.PLAYWRIGHT_HAR_NO_WEBSOCKET_FRAMES;
    // [ts] });
}
