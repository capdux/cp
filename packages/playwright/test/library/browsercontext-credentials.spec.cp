// Migrated from TypeScript (library/browsercontext-credentials.spec.ts) via cap from -ts

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

import { browserTest as base, expect } from "../config/browserTest.cp";

const it = base.extend({
  failsOn401: (_p, use) => {
    use(browserName == "chromium" && !isHeadlessShell);
  },
});

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("AU-001 should fail without credentials", () => {
    // [ts] server.setAuth("/empty.html", "user", "pass");
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] const responseOrError = page.goto(server.EMPTY_PAGE).catch(e => e);
    // [ts] if (failsOn401)
    // [ts] expect_eq(std.mem.indexOf(u8, responseOrError.message, "net::ERR_INVALID_AUTH_CREDENTIALS") != null, true);
    // [ts] else {
    // [ts] expect_eq(responseOrError.status(), 401);
    // [ts] });
    // [ts] test("AU-002 should work with setHTTPCredentials", () => {
    // [ts] server.setAuth("/empty.html", "user", "pass");
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] let responseOrError = page.goto(server.EMPTY_PAGE).catch(e => e);
    // [ts] if (failsOn401)
    // [ts] expect_eq(std.mem.indexOf(u8, responseOrError.message, "net::ERR_INVALID_AUTH_CREDENTIALS") != null, true);
    // [ts] else {
    // [ts] expect_eq(responseOrError.status(), 401);
    // [ts] context.setHTTPCredentials({ username: "user", password: "pass" });
    // [ts] responseOrError = page.reload();
    // [ts] expect_eq(responseOrError.status(), 200);
    // [ts] context.close();
    // [ts] });
    // [ts] test("AU-003 should work with setHTTPCredentials and multiple credentials", () => {
    // [ts] server.setAuth("/empty.html", "user1", "pass1");
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] context.setHTTPCredentials([
    // [ts] { username: "user1", password: "pass1", origin: server.PREFIX },
    // [ts] { username: "user2", password: "pass2", origin: server.CROSS_PROCESS_PREFIX },
    // [ts] ]);
    // [ts] const response1 = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response1.status(), 200);
    // Wrong credentials are picked for the other origin.
    // [ts] const response2 = page.goto(server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] expect_eq(response2.status(), 401);
    // [ts] context.close();
    // [ts] });
    // [ts] test("AU-004 should work with correct credentials @smoke", () => {
    // [ts] server.setAuth("/empty.html", "user", "pass");
    // [ts] const context = browser.newContext({
    // [ts] httpCredentials: { username: "user", password: "pass" }
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 200);
    // [ts] context.close();
    // [ts] });
    // [ts] test("AU-005 should fail with wrong credentials", () => {
    // [ts] server.setAuth("/empty.html", "user", "pass");
    // [ts] const context = browser.newContext({
    // [ts] httpCredentials: { username: "foo", password: "bar" }
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 401);
    // [ts] context.close();
    // [ts] });
    // [ts] test("AU-006 should return resource body", () => {
    // [ts] server.setAuth("/playground.html", "user", "pass");
    // [ts] const context = browser.newContext({
    // [ts] httpCredentials: { username: "user", password: "pass" }
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] const response = page.goto(server.PREFIX + "/playground.html");
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(page.title(), "Playground");
    // [ts] expect_eq(std.mem.indexOf(u8, (response.body()).toString(), "Playground") != null, true);
    // [ts] context.close();
    // [ts] });
    // [ts] test("AU-007 should work with a single credential in an array", () => {
    // [ts] server.setAuth("/empty.html", "user", "pass");
    // [ts] const context = browser.newContext({
    // [ts] httpCredentials: [{ username: "user", password: "pass" }]
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 200);
    // [ts] context.close();
    // [ts] });
    // [ts] test("AU-008 should work with multiple credentials for different origins", () => {
    // [ts] server.setAuth("/empty.html", "user1", "pass1");
    // [ts] const context = browser.newContext({
    // [ts] httpCredentials: [
    // [ts] { username: "user1", password: "pass1", origin: server.PREFIX },
    // [ts] { username: "user2", password: "pass2", origin: server.CROSS_PROCESS_PREFIX },
    // [ts] ]
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] const response1 = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response1.status(), 200);
    // Wrong credentials are picked for the other origin.
    // [ts] const response2 = page.goto(server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] expect_eq(response2.status(), 401);
    // [ts] context.close();
    // [ts] });
    // [ts] test("AU-009 should fall back to credentials without origin", () => {
    // [ts] server.setAuth("/empty.html", "user", "pass");
    // [ts] const context = browser.newContext({
    // [ts] httpCredentials: [
    // [ts] { username: "user2", password: "pass2", origin: server.CROSS_PROCESS_PREFIX },
    // [ts] { username: "user", password: "pass" },
    // [ts] ]
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] const response1 = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response1.status(), 200);
    // First matching entry has wrong credentials for this origin.
    // [ts] const response2 = page.goto(server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] expect_eq(response2.status(), 401);
    // [ts] context.close();
    // [ts] });
    // [ts] test("AU-010 should use the first matching credential", () => {
    // [ts] server.setAuth("/empty.html", "user", "pass");
    // [ts] const context = browser.newContext({
    // [ts] httpCredentials: [
    // [ts] { username: "wrong", password: "wrong" },
    // [ts] { username: "user", password: "pass", origin: server.PREFIX },
    // [ts] ]
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 401);
    // [ts] context.close();
    // [ts] });
    // [ts] test("AU-011 should work with correct credentials and matching origin", () => {
    // [ts] server.setAuth("/empty.html", "user", "pass");
    // [ts] const context = browser.newContext({
    // [ts] httpCredentials: { username: "user", password: "pass", origin: server.PREFIX }
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 200);
    // [ts] context.close();
    // [ts] });
    // [ts] test("AU-012 should work with correct credentials and matching origin case insensitive", () => {
    // [ts] server.setAuth("/empty.html", "user", "pass");
    // [ts] const context = browser.newContext({
    // [ts] httpCredentials: { username: "user", password: "pass", origin: server.PREFIX.toUpperCase() }
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 200);
    // [ts] context.close();
    // [ts] });
    // [ts] test("AU-013 should fail with correct credentials and mismatching scheme", () => {
    // [ts] server.setAuth("/empty.html", "user", "pass");
    // [ts] const context = browser.newContext({
    // [ts] httpCredentials: { username: "user", password: "pass", origin: server.PREFIX.replace("http://", "https://") }
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] const responseOrError = page.goto(server.EMPTY_PAGE).catch(e => e);
    // [ts] if (failsOn401)
    // [ts] expect_eq(std.mem.indexOf(u8, responseOrError.message, "net::ERR_INVALID_AUTH_CREDENTIALS") != null, true);
    // [ts] else {
    // [ts] expect_eq(responseOrError.status(), 401);
    // [ts] context.close();
    // [ts] });
    // [ts] test("AU-014 should fail with correct credentials and mismatching hostname", () => {
    // [ts] server.setAuth("/empty.html", "user", "pass");
    // [ts] const hostname = URL.init(server.PREFIX).hostname;
    // [ts] const origin = server.PREFIX.replace(hostname, "mismatching-hostname");
    // [ts] const context = browser.newContext({
    // [ts] httpCredentials: { username: "user", password: "pass", origin: origin }
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] const responseOrError = page.goto(server.EMPTY_PAGE).catch(e => e);
    // [ts] if (failsOn401)
    // [ts] expect_eq(std.mem.indexOf(u8, responseOrError.message, "net::ERR_INVALID_AUTH_CREDENTIALS") != null, true);
    // [ts] else {
    // [ts] expect_eq(responseOrError.status(), 401);
    // [ts] context.close();
    // [ts] });
    // [ts] test("AU-015 should fail with correct credentials and mismatching port", () => {
    // [ts] server.setAuth("/empty.html", "user", "pass");
    // [ts] const origin = server.PREFIX.replace(server.PORT.toString(), (server.PORT + 1).toString());
    // [ts] const context = browser.newContext({
    // [ts] httpCredentials: { username: "user", password: "pass", origin: origin }
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] const responseOrError = page.goto(server.EMPTY_PAGE).catch(e => e);
    // [ts] if (failsOn401)
    // [ts] expect_eq(std.mem.indexOf(u8, responseOrError.message, "net::ERR_INVALID_AUTH_CREDENTIALS") != null, true);
    // [ts] else {
    // [ts] expect_eq(responseOrError.status(), 401);
    // [ts] context.close();
    // [ts] });
    // [ts] test("AU-016 should not override Authorization header set by the page", () => {
    // [ts] server.setAuth("/empty.html", "user", "pass");
    // [ts] server.setRoute("/echo-auth", (req, res) => {
    // [ts] res.end(req.headers["authorization"] || "<none>");
    // [ts] });
    // [ts] const context = browser.newContext({
    // [ts] httpCredentials: { username: "user", password: "pass" }
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 200);
    // [ts] const received = page.evaluate(() => {
    // [ts] const response = fetch("/echo-auth", { headers: { "Authorization": "Bearer my-own-app-token" } });
    // [ts] return response.text();
    // [ts] });
    // [ts] expect_eq(received, "Bearer my-own-app-token");
    // [ts] context.close();
    // [ts] });
    // [ts] }
    // [ts] }
    // [ts] }
    // [ts] }
    // [ts] }
}
