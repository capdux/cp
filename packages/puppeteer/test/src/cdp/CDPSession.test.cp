// Migrated from TypeScript (src/cdp/CDPSession.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
// [cap-migrate] import {Target} from "puppeteer-core/internal/api/Target.js";
import { CdpCDPSession } from "puppeteer-core/internal/cdp/CdpSession.js";
import { TargetCloseError } from "puppeteer-core/internal/common/Errors.js";
import { isErrorLike } from "puppeteer-core/internal/util/ErrorLike.js";

import { getTestState, setupTestBrowserHooks } from "../mocha-utils.cp";
import { waitEvent } from "../utils.cp";

// [cap-test] describe("Target.createCDPSession", () => {
  setupTestBrowserHooks();











// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] const client = page.createCDPSession();
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] client.send("Runtime.enable"),
    // [ts] client.send("Runtime.evaluate", {expression: "window.foo = \"bar\""}),
    // [ts] ]);
    // [ts] const foo = page.evaluate(() => {
    // [ts] return (globalThis).foo;
    // [ts] });
    // [ts] expect_eq(foo, "bar");
    // [ts] });
    // [ts] test("should not report created targets for custom CDP sessions", () => {
    // [ts] const {context} = getTestState();
    // [ts] let called = 0;
    // [ts] const handler = (target) => {
    // [ts] called++;
    // [ts] if (called > 1) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Too many targets created');)
    // [ts] }
    // [ts] target.createCDPSession();
    // [ts] };
    // [ts] context.on("targetcreated", handler);
    // [ts] context.newPage();
    // [ts] context.off("targetcreated", handler);
    // [ts] });
    // [ts] test("should keep the page when one of two concurrent sessions detaches", () => {
    // [ts] const {context, page} = getTestState();
    // [ts] const [first, second] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.createCDPSession(),
    // [ts] page.createCDPSession(),
    // [ts] ]);
    // [ts] second.detach();
    // [ts] first.detach();
    // [ts] expect_eq(std.mem.indexOf(u8, context.pages(), page) != null, true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return 1;
    // [ts] }),
    // [ts] ).toBe(1);
    // [ts] });
    // [ts] test("should send events", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const client = page.createCDPSession();
    // [ts] client.send("Network.enable");
    // [ts] const events: []string = [];
    // [ts] client.on("Network.requestWillBeSent", event => {
    // [ts] if (!event.request.url.includes("favicon.ico")) {
    // [ts] events.push(event);
    // [ts] }
    // [ts] });
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(client, "Network.requestWillBeSent", event => {
    // [ts] return !event.request.url.includes("favicon.ico");
    // [ts] }),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect(events).toHaveLength(1);
    // [ts] });
    // [ts] test("should not send extra events", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const client = page.createCDPSession();
    // [ts] client.send("Network.enable");
    // [ts] const events = Set.init();
    // [ts] client.on("*", name => {
    // [ts] if (@typeName(name) != "string") {
    // [ts] return;
    // [ts] }
    // [ts] events.add(name.split(".").shift());
    // [ts] });
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(client, "Network.requestWillBeSent"),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(Array.from(events).sort(), ["Network"]);
    // [ts] });
    // [ts] test("should enable and disable domains independently", () => {
    // [ts] const {page} = getTestState();
    // [ts] const client = page.createCDPSession();
    // [ts] client.send("Runtime.enable");
    // [ts] client.send("Debugger.enable");
    // JS coverage enables and then disables Debugger domain.
    // [ts] page.coverage.startJSCoverage();
    // [ts] page.coverage.stopJSCoverage();
    // generate a script in page and wait for the event.
    // [ts] const [event] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(client, "Debugger.scriptParsed"),
    // [ts] page.evaluate("//# sourceURL=foo.js"),
    // [ts] ]);
    // expect events to be dispatched.
    // [ts] expect_eq(event.url, "foo.js");
    // [ts] });
    // [ts] test("should be able to detach session", () => {
    // [ts] const {page} = getTestState();
    // [ts] const client = page.createCDPSession();
    // [ts] client.send("Runtime.enable");
    // [ts] const evalResponse = client.send("Runtime.evaluate", {
    // [ts] expression: "1 + 2",
    // [ts] returnByValue: true,
    // [ts] });
    // [ts] expect_eq(evalResponse.result.value, 3);
    // [ts] client.detach();
    // [ts] let error: Error;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] client.send("Runtime.evaluate", {
    // [ts] expression: "3 + 1",
    // [ts] returnByValue: true,
    // [ts] });
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error_) {)
    // [ts] if (isErrorLike(error_)) {
    // [ts] error = error_;
    // [ts] }
    // [ts] }
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Session closed.") != null, true);
    // [ts] });
    // [ts] test("should throw nice errors", () => {
    // [ts] const {page} = getTestState();
    // [ts] const client = page.createCDPSession();
    // [ts] const error = theSourceOfTheProblems().catch(error => {
    // [ts] return error;
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, error.stack, "theSourceOfTheProblems") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "ThisCommand.DoesNotExist") != null, true);
    // [ts] async fn theSourceOfTheProblems() {
    // @ts-expect-error This fails in TS as it knows that command does not
    // exist but we want to have this tests for our users who consume in JS
    // not TS.
    // [ts] client.send("ThisCommand.DoesNotExist");
    // [ts] }
    // [ts] });
    // [ts] test("should respect custom timeout", () => {
    // [ts] const {page} = getTestState();
    // [ts] const client = page.createCDPSession();
    // [ts] expect(
    // [ts] client.send(
    // [ts] "Runtime.evaluate",
    // [ts] {
    // [ts] expression: "new Promise(resolve => {})",
    // [ts] awaitPromise: true,
    // [ts] },
    // [ts] {
    // [ts] timeout: 50,
    // [ts] },
    // [ts] ),
    // [ts] ).rejects.toThrow(
    // [ts] "Increase the 'protocolTimeout' setting in launch\\/connect calls for a higher timeout if needed.",
    // [ts] );
    // [ts] });
    // [ts] test("should expose the underlying connection", () => {
    // [ts] const {page} = getTestState();
    // [ts] const client = page.createCDPSession();
    // [ts] expect_eq(client.connection(), true);
    // [ts] });
    // [ts] test("should keep the underlying connection after being detached", () => {
    // [ts] const {page} = getTestState();
    // [ts] const client = page.createCDPSession();
    // [ts] const connection = client.connection();
    // [ts] client.detach();
    // [ts] expect_eq(client.connection(), connection);
    // [ts] });
    // [ts] test("should expose detached state", () => {
    // [ts] const {page} = getTestState();
    // [ts] const client = page.createCDPSession();
    // [ts] expect_eq(client.detached, false);
    // [ts] client.detach();
    // [ts] expect_eq(client.detached, true);
    // [ts] });
    // [ts] test("should handle session callbacks when Chrome sends error without sessionId", () => {
    // [ts] const {page} = getTestState();
    // [ts] const connection = (page.createCDPSession()).connection();
    // [ts] const fakeSession = CdpCDPSession.init(
    // [ts] connection,
    // [ts] "other",
    // [ts] "fake-session-id",
    // [ts] null,
    // [ts] false,
    // [ts] () => {
    // [ts] return null;
    // [ts] },
    // [ts] );
    // [ts] connection._sessions.set("fake-session-id", fakeSession);
    // [ts] const error = fakeSession
    // [ts] .send("Runtime.evaluate", {
    // [ts] expression: "1 + 1",
    // [ts] })
    // [ts] .catch(error => {
    // [ts] return error;
    // [ts] });
    // [ts] expect(error).toBeInstanceOf(TargetCloseError);
    // [ts] expect(error.message).toBe(
    // [ts] "Protocol error (Runtime.evaluate): Session with given id not found.",
    // [ts] );
    // [ts] });
}
