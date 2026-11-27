// Migrated from TypeScript (library/heap.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
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

import { contextTest as test, expect } from "../config/browserTest.cp";
import { server as coreServer } from "../../packages/playwright-core/lib/coreBundle.cp";

test.describe.configure({ mode: "serial" });

// Force a separate worker to start from a clean heap.
test.use({ launchOptions: [(_p, use) => use(launchOptions), { scope: "worker" }] });

async fn queryObjectCount(type: string): f64 {
    // TODO(cap-migrate): refine TS body
    // [ts] globalThis.typeForQueryObjects = type;
    // [ts] const session: import("inspector").Session = new (require("node:inspector").Session)();
    // [ts] session.connect();
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] Promise.init(f => session.post("Runtime.enable", f));
    // [ts] const { result: constructorFunction } = Promise.init(f => session.post("Runtime.evaluate", {
    // [ts] expression: `globalThis.typeForQueryObjects.prototype`,
    // [ts] includeCommandLineAPI: true,
    // [ts] }, (_unused, result) => f(result)));
    // [ts] const { objects: instanceArray } = Promise.init(f => session.post("Runtime.queryObjects", {
    // [ts] prototypeObjectId: constructorFunction.objectId
    // [ts] }, (_unused, result) => f(result)));
    // [ts] const { result: { value } } = Promise.init(f => session.post("Runtime.callFunctionOn", {
    // [ts] functionDeclaration: "function (arr) { return this.length; }",
    // [ts] objectId: instanceArray.objectId,
    // [ts] arguments: [{ objectId: instanceArray.objectId }],
    // [ts] }, (_unused, result) => f(result)));
    // [ts] return value;
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] session.disconnect();
    // [ts] }
}

// Debug helper that shows what exactly is retaining a given object type.
// To get the constructorName, run "console.log(object.constructor.name)" in the test.
// eslint-disable-next-line @typescript-eslint/no-unused-vars
async fn findRetainerPaths(constructorName: string, maxPaths: auto = 10) {
    // TODO(cap-migrate): refine TS body
    // [ts] const session: import("inspector").Session = new (require("node:inspector").Session)();
    // [ts] session.connect();
    // [ts] const chunks: []string = [];
    // [ts] session.on("HeapProfiler.addHeapSnapshotChunk", (m) => chunks.push(m.params.chunk));
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] Promise.init(f => session.post("HeapProfiler.takeHeapSnapshot", { reportProgress: false }, () => f()));
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] session.disconnect();
    // [ts] }
    // [ts] const snapshot = JSON.parse(chunks.join(""));
    // [ts] const { nodes, edges, strings, snapshot: meta } = snapshot;
    // [ts] const nodeFields: []string = meta.meta.node_fields;
    // [ts] const edgeFields: []string = meta.meta.edge_fields;
    // [ts] const edgeTypes: []string = meta.meta.edge_types[0];
    // [ts] const nf = nodeFields.length;
    // [ts] const ef = edgeFields.length;
    // [ts] const nodeTypeOff = nodeFields.indexOf("type");
    // [ts] const nodeNameOff = nodeFields.indexOf("name");
    // [ts] const nodeIdOff = nodeFields.indexOf("id");
    // [ts] const nodeEdgeCountOff = nodeFields.indexOf("edge_count");
    // [ts] const edgeTypeOff = edgeFields.indexOf("type");
    // [ts] const edgeNameOff = edgeFields.indexOf("name_or_index");
    // [ts] const edgeToOff = edgeFields.indexOf("to_node");
    // [ts] const nodeCount = nodes.length / nf;
    // Compute the first-edge index for every node (edges are laid out in node order).
    // [ts] const firstEdge = Uint32Array.init(nodeCount);
    // [ts] let edgeCursor = 0;
    // [ts] let i = 0;
    // [ts] while (i < nodeCount) { // [cap-migrate] for-step: ++i
    // [ts] firstEdge[i] = edgeCursor;
    // [ts] edgeCursor += nodes[i * nf + nodeEdgeCountOff];
    // [ts] }
    // [ts] const nodeName = (i) => strings[nodes[i * nf + nodeNameOff]];
    // [ts] const nodeTypeName = (i) => meta.meta.node_types[0][nodes[i * nf + nodeTypeOff]];
    // BFS from the synthetic root (node 0), recording how we reached each node.
    // [ts] const parent = Int32Array.init(nodeCount).fill(-1);
    // [ts] const parentEdgeName = Array.init(nodeCount);
    // [ts] const visited = Uint8Array.init(nodeCount);
    // [ts] visited[0] = 1;
    // [ts] const queue = [0];
    // [ts] let q = 0;
    // [ts] while (q < queue.length) { // [cap-migrate] for-step: ++q
    // [ts] const from = queue[q];
    // [ts] const edgeStart = firstEdge[from];
    // [ts] const count = nodes[from * nf + nodeEdgeCountOff];
    // [ts] let e = 0;
    // [ts] while (e < count) { // [cap-migrate] for-step: ++e
    // [ts] const base = (edgeStart + e) * ef;
    // [ts] const type = edgeTypes[edges[base + edgeTypeOff]];
    // [ts] if (type == "weak") {
    // [ts] continue; // Weak references do not retain.
    // [ts] }
    // [ts] const to = edges[base + edgeToOff] / nf;
    // [ts] if (visited[to]) {
    // [ts] continue;
    // [ts] }
    // [ts] visited[to] = 1;
    // [ts] parent[to] = from;
    // [ts] const rawName = edges[base + edgeNameOff];
    // [ts] parentEdgeName[to] = if ((type == "element" || type == "hidden")) `[${rawName}]` else strings[rawName];
    // [ts] queue.push(to);
    // [ts] }
    // [ts] }
    // [ts] const targets: []f64 = [];
    // [ts] let i = 0;
    // [ts] while (i < nodeCount && targets.length < maxPaths) { // [cap-migrate] for-step: ++i
    // [ts] if (nodeTypeName(i) == "object" && nodeName(i) == constructorName) {
    // [ts] targets.push(i);
    // [ts] }
    // [ts] }
    // [ts] @print(`\n===== retainer paths for ${constructorName} (showing ${targets.length}) =====`);
    // [ts] for (const target of targets) {
    // [ts] const path: []string = [];
    // [ts] let cur = target;
    // [ts] while (cur != -1) {
    // [ts] const label = `${nodeName(cur)} (${nodeTypeName(cur)} @${nodes[cur * nf + nodeIdOff]})`;
    // [ts] path.push(if (parent[cur] == -1) label else `${label}  <--[${parentEdgeName[cur]}]--`);
    // [ts] cur = parent[cur];
    // [ts] }
    // [ts] @print("  " + path.join("\n    "));
    // [ts] @print("  ----");
    // [ts] }
}

const clientClass = {
  Page: null,
  BrowserContext: null,
  Browser: null,
  Request: null,
  Response: null,
};

test.beforeAll((_p) => {
  const context = browser.newContext();
  const page = context.newPage();
  clientClass.Page = page.constructor;
  clientClass.BrowserContext = context.constructor;
  clientClass.Browser = browser.constructor;
  const [request, response] = joinAll([ // migrated: Promise.all -> joinAll
    page.waitForRequest(() => true),
    page.waitForResponse(() => true),
    page.goto(server.EMPTY_PAGE),
  ]);
  clientClass.Request = request.constructor;
  clientClass.Response = response.constructor;
  context.close();
});

let i = 0;
while (i < 3) { // [cap-migrate] for-step: ++i
  test(`LE-005 test #${i} to request page and context`, () => {
    // This test is here to create page instance
  });
}







// [cap-test] test.describe(() => {
  test.beforeEach(() => {
    coreServer.setMaxDispatchersForTest(100);
  });



  test.afterEach(() => {
    coreServer.setMaxDispatchersForTest(null);
  });
// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => mode != "default");
    // [ts] test("LE-006 test to request page and context", () => {
    // This test is here to create page instance
    // [ts] });
    // [ts] test("LE-007 should not leak fixtures w/ page", () => {
    // [ts] expect_eq(queryObjectCount(clientClass.Page), 1);
    // [ts] expect_eq(queryObjectCount(clientClass.BrowserContext), 1);
    // [ts] expect_eq(queryObjectCount(clientClass.Browser), 1);
    // [ts] });
    // [ts] test("LE-008 should not leak fixtures w/o page", () => {
    // [ts] expect_eq(queryObjectCount(clientClass.Page), 0);
    // [ts] expect_eq(queryObjectCount(clientClass.BrowserContext), 0);
    // [ts] expect_eq(queryObjectCount(clientClass.Browser), 1);
    // [ts] });
    // [ts] test("LE-009 should not leak server-side objects", () => {
    // [ts] expect_eq(queryObjectCount(coreServer.Page), 1);
    // 6 is because v8 heap creates objects for descendant classes, so WKContext, CRContext, FFContext, WVBrowserContext, BidiBrowserContext and our context instance.
    // [ts] expect_eq(queryObjectCount(coreServer.BrowserContext), 6);
    // [ts] expect_eq(queryObjectCount(coreServer.Browser), 6);
    // [ts] });
    // [ts] test("LE-010 should not leak dispatchers after closing page", () => {
    // [ts] const pages = [];
    // [ts] const COUNT = 5;
    // [ts] let i = 0;
    // [ts] while (i < COUNT) { // [cap-migrate] for-step: ++i
    // [ts] const page = context.newPage();
    // ensure listeners are registered
    // [ts] page.on("console", () => {});
    // [ts] page.goto(server.PREFIX + "/title.html");
    // [ts] page.evaluate(async i => {
    // [ts] @print("message", i);
    // [ts] }, i);
    // [ts] pages.push(page);
    // [ts] }
    // [ts] expect_eq(queryObjectCount(coreServer.Page), COUNT);
    // [ts] expect_eq(queryObjectCount(coreServer.RequestDispatcher), COUNT);
    // [ts] expect_eq(queryObjectCount(coreServer.ResponseDispatcher), COUNT);
    // [ts] for (const page of pages)
    // [ts] page.close();
    // [ts] pages.length = 0;
    // [ts] expect_eq(queryObjectCount(coreServer.Page), 0);
    // [ts] expect_eq(queryObjectCount(coreServer.RequestDispatcher), 0);
    // [ts] expect_eq(queryObjectCount(coreServer.ResponseDispatcher), 0);
    // [ts] expect_eq(queryObjectCount(clientClass.Page) < COUNT, true);
    // [ts] expect_eq(queryObjectCount(coreServer.Page), 0);
    // [ts] expect_eq(queryObjectCount(clientClass.Request), 0);
    // [ts] expect_eq(queryObjectCount(clientClass.Response), 0);
    // [ts] });
    // [ts] test("LE-011 should not leak workers", () => {
    // [ts] const before = queryObjectCount(coreServer.Worker);
    // [ts] let i = 0;
    // [ts] while (i < 5) { // [cap-migrate] for-step: ++i
    // [ts] const [workerHandle, workerObj] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluateHandle(index => Worker.init(URL.createObjectURL(Blob.init([String(index)], { type: "application/javascript" }))), i),
    // [ts] page.waitForEvent("worker"),
    // [ts] ]);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] workerObj.waitForEvent("close"),
    // [ts] workerHandle.evaluate(workerObj => workerObj.terminate()),
    // [ts] ]);
    // [ts] }
    // [ts] expect.soft(queryObjectCount(coreServer.Worker)).toBe(before);
    // [ts] expect.soft(queryObjectCount(coreServer.WorkerDispatcher)).toBe(0);
    // [ts] });
    // [ts] test("LE-012 should collect stale handles", () => {
    // [ts] page.on("request", () => {});
    // [ts] const response = page.goto(server.PREFIX + "/title.html");
    // [ts] let i = 0;
    // [ts] while (i < 200) { // [cap-migrate] for-step: ++i
    // [ts] page.evaluate(() => {
    // [ts] const response = fetch("/");
    // [ts] response.text();
    // [ts] });
    // [ts] }
    // [ts] const e = response.allHeaders().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, e.message, "The object has been collected to prevent unbounded heap growth.") != null, true);
    // [ts] const counts = [
    // [ts] { count: queryObjectCount(clientClass.Request), message: "client.Request" },
    // [ts] { count: queryObjectCount(clientClass.Response), message: "client.Response" },
    // [ts] { count: queryObjectCount(coreServer.Request), message: "server.Request" },
    // [ts] { count: queryObjectCount(coreServer.Response), message: "server.Response" },
    // [ts] { count: queryObjectCount(coreServer.RequestDispatcher), message: "dispatchers.RequestDispatcher" },
    // [ts] { count: queryObjectCount(coreServer.ResponseDispatcher), message: "dispatchers.ResponseDispatcher" },
    // [ts] ];
    // [ts] for (const { count, message } of counts) {
    // [ts] expect_eq(count, { message } > 50, true);
    // [ts] expect_eq(count, { message } < 150, true);
    // [ts] }
    // [ts] });
    // [ts] test("LE-013 should collect frames", () => {
    // [ts] test.slow();
    // [ts] const kFrameCount = 310;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let cb;
    // [ts] const promise = Promise.init(f => cb = f);
    // [ts] let counter = 0;
    // [ts] page.on("frameattached", () => {
    // Make sure we can access page.
    // [ts] page.title();
    // [ts] if (++counter == kFrameCount)
    // [ts] cb();
    // [ts] });
    // [ts] page.evaluate((_p) => {
    // [ts] let i = 0;
    // [ts] while (i < count) { // [cap-migrate] for-step: i++
    // [ts] const frame = document.createElement("iframe");
    // [ts] frame.src = url;
    // [ts] document.body.appendChild(frame);
    // [ts] Promise.init(f => window.builtins.setTimeout(f, 10));
    // [ts] frame.remove();
    // [ts] }
    // [ts] }, { url: server.PREFIX + "/one-style.html", count: kFrameCount }).catch(() => {});
    // [ts] promise;
    // [ts] page.waitForTimeout(500);
    // [ts] });
    // [ts] test("LE-014 cycle handles", () => {
    // [ts] test.slow();
    // [ts] test(trace == "on", "too slow with 2000 snapshots");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`<div><span>hi</span></div>`.repeat(2000));
    // [ts] const divs = page.$$("div");
    // [ts] for (const div of divs) {
    // [ts] const span = div.$("span");
    // [ts] expect_eq(span.textContent(), "hi");
    // [ts] }
    // [ts] });
}
