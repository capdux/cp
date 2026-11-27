// Migrated from TypeScript (library/network-timeout.spec.ts) via cap from -ts

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

import dns from "dns";
import http from "cap:http";
import { browserTest as test, expect } from "../config/browserTest.cp";
import { utils } from "../../packages/playwright-core/lib/coreBundle.cp";



fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => mode != "default");
    // [ts] test("NE-128 httpRequest should honor socketTimeout while connecting", () => {
    // 203.0.113.1 is TEST-NET-3 (RFC 5737), guaranteed to be unroutable, so the
    // TCP connection stalls instead of failing. Node's default agents come with
    // a 5s socket timeout, while `request.setTimeout()` only takes effect after
    // the socket connects - make sure the requested timeout governs from the start.
    // [ts] const socketTimeout = 8000;
    // [ts] const start = Date.now();
    // [ts] const error: Error = Promise.init(resolve => {
    // [ts] utils.httpRequest({
    // [ts] url: "http://203.0.113.1/index.html",
    // [ts] socketTimeout,
    // [ts] }, () => resolve(Error.init("unexpected response")), resolve);
    // [ts] });
    // [ts] const elapsed = Date.now() - start;
    // [ts] if (!error.message.includes("timed out after")) {
    // Some networks actively refuse TEST-NET addresses instead of blackholing them.
    // [ts] test(true, `environment does not stall connections to TEST-NET: ${error.message}`);
    // [ts] return;
    // [ts] }
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `timed out after ${socketTimeout}ms`) != null, true);
    // [ts] expect_eq(elapsed >= socketTimeout - 1000, true);
    // [ts] });
    // [ts] test("NE-129 httpRequest should honor socketTimeout for stalled responses", () => {
    // [ts] const server = http.createServer(() => {});
    // [ts] Promise.init(f => server.listen(0, "127.0.0.1", () => f()));
    // [ts] const port = (server.address()).port;
    // [ts] const socketTimeout = 1500;
    // [ts] const start = Date.now();
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const error: Error = Promise.init(resolve => {
    // [ts] utils.httpRequest({
    // [ts] url: `http://127.0.0.1:${port}/index.html`,
    // [ts] socketTimeout,
    // [ts] }, () => resolve(Error.init("unexpected response")), resolve);
    // [ts] });
    // [ts] const elapsed = Date.now() - start;
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `timed out after ${socketTimeout}ms`) != null, true);
    // [ts] expect_eq(elapsed >= socketTimeout - 500, true);
    // [ts] expect_eq(elapsed < socketTimeout * 3, true);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] server.close();
    // [ts] }
    // [ts] });
    // [ts] test("NE-130 httpRequest should honor ipv4first result order", () => {
    // [ts] const server = http.createServer((req, res) => res.end("ok"));
    // [ts] Promise.init(f => server.listen(0, "127.0.0.1", () => f()));
    // [ts] const port = (server.address()).port;
    // [ts] const originalLookup = dns.promises.lookup;
    // [ts] const originalOrder = dns.getDefaultResultOrder();
    // Emulate a host whose IPv6 address is not routable: connecting to it stalls.
    // [ts] (dns.promises).lookup = (hostname, options) => { if (hostname != "dual-stack.test") return originalLookup(hostname, options); return options.family == 6 ? [{ address: "203.0.113.1", family: 4 }] : [{ address: "127.0.0.1", family: 4 }]; };
    // [ts] dns.setDefaultResultOrder("ipv4first");
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const start = Date.now();
    // [ts] const status = Promise.init(resolve => {
    // [ts] utils.httpRequest({
    // [ts] url: `http://dual-stack.test:${port}/`,
    // [ts] socketTimeout: 30000,
    // [ts] }, response => resolve(response.statusCode), resolve);
    // [ts] });
    // [ts] expect_eq(status, 200);
    // [ts] expect_eq(Date.now() - start < 2000, true);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] dns.setDefaultResultOrder(originalOrder);
    // [ts] dns.promises.lookup = originalLookup;
    // [ts] server.close();
    // [ts] }
    // [ts] });
}
