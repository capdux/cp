// Migrated from TypeScript (src/tracing.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2017 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import fs from "cap:fs";
import path from "cap:path";

import { test, expect_eq } from "cap:test";
// [cap-migrate] import {CdpPage} from "puppeteer-core/internal/cdp/Page.js";
import sinon from "sinon";

import { setupSeparateTestBrowserHooks } from "./mocha-utils.cp";

// [cap-test] describe("Tracing", () => {
  let outputFile: string;
  const state = setupSeparateTestBrowserHooks({});

  // TODO(cap-migrate): refine TS statement
  // [ts] beforeEach(() => {
  // [ts] outputFile = path.join(import.meta.dirname, "trace.json");
  // [ts] });

  afterEach(() => {
    if (fs.existsSync(outputFile)) {
      fs.unlinkSync(outputFile);
    }
  });







// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should output a trace", () => {
    // [ts] const {server, page} = state;
    // [ts] page.tracing.start({screenshots: true, path: outputFile});
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] page.tracing.stop();
    // [ts] expect_eq(fs.existsSync(outputFile), true);
    // [ts] });
    // [ts] test("should run with custom categories if provided", () => {
    // [ts] const {page} = state;
    // [ts] page.tracing.start({
    // [ts] path: outputFile,
    // [ts] categories: ["-*", "disabled-by-default-devtools.timeline.frame"],
    // [ts] });
    // [ts] page.tracing.stop();
    // [ts] const traceJson = JSON.parse(
    // [ts] fs.readFileSync(outputFile, {encoding: "utf8"}),
    // [ts] );
    // [ts] expect(traceJson.traceEvents).not.toContainEqual(
    // [ts] expect.objectContaining({
    // [ts] cat: "toplevel",
    // [ts] }),
    // [ts] );
    // [ts] });
    // [ts] test("should run with default categories", () => {
    // [ts] const {page} = state;
    // [ts] page.tracing.start({
    // [ts] path: outputFile,
    // [ts] });
    // [ts] page.tracing.stop();
    // [ts] const traceJson = JSON.parse(
    // [ts] fs.readFileSync(outputFile, {encoding: "utf8"}),
    // [ts] );
    // [ts] expect(traceJson.traceEvents).toContainEqual(
    // [ts] expect.objectContaining({
    // [ts] cat: "toplevel",
    // [ts] }),
    // [ts] );
    // [ts] });
    // [ts] test("should throw if tracing on two pages", () => {
    // [ts] const {page, browser} = state;
    // [ts] page.tracing.start({path: outputFile});
    // [ts] const newPage = browser.newPage();
    // [ts] let error: Error;
    // [ts] newPage.tracing.start({path: outputFile}).catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] newPage.close();
    // [ts] expect_eq(error, true);
    // [ts] page.tracing.stop();
    // [ts] });
    // [ts] test("should return a typedArray", () => {
    // [ts] const {page, server} = state;
    // [ts] page.tracing.start({screenshots: true, path: outputFile});
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] const trace = (page.tracing.stop());
    // [ts] const buf = fs.readFileSync(outputFile);
    // [ts] expect(trace).toBeInstanceOf(Uint8Array);
    // [ts] expect_eq(Buffer.from(trace).toString(), buf.toString());
    // [ts] });
    // [ts] test("should work without options", () => {
    // [ts] const {page, server} = state;
    // [ts] page.tracing.start();
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] const trace = page.tracing.stop();
    // [ts] expect_eq(trace, true);
    // [ts] });
    // [ts] test("should support bufferSize option", () => {
    // [ts] const {page, server} = state;
    // [ts] const client = (page)._client();
    // [ts] const sendSpy = sinon.spy(client, "send");
    // [ts] page.tracing.start({
    // [ts] path: outputFile,
    // [ts] bufferSize: 10,
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] page.tracing.stop();
    // [ts] const tracingStartCall = sendSpy.getCalls().find(call => {
    // [ts] return call.args[0] == "Tracing.start";
    // [ts] });
    // [ts] expect_eq(tracingStartCall != null, true);
    // [ts] expect(tracingStartCall?.args[1]).toEqual(
    // [ts] expect.objectContaining({
    // [ts] traceConfig: expect.objectContaining({
    // [ts] traceBufferSizeInKb: 10,
    // [ts] }),
    // [ts] }),
    // [ts] );
    // [ts] });
    // [ts] test("should support a typedArray without a path", () => {
    // [ts] const {page, server} = state;
    // [ts] page.tracing.start();
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] const trace = (page.tracing.stop());
    // [ts] expect_eq(Buffer.from(trace).toString().length > 10, true);
    // [ts] });
    // [ts] test("should properly fail if readProtocolStream errors out", () => {
    // [ts] const {page} = state;
    // [ts] page.tracing.start({path: import.meta.dirname});
    // [ts] let error: Error;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.tracing.stop();
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error_) {)
    // [ts] error = error_;
    // [ts] }
    // [ts] expect_eq(error != null, true);
    // [ts] });
    // [ts] test("should allow starting tracing again after a failed start", () => {
    // [ts] const {page, browser} = state;
    // [ts] page.tracing.start({path: outputFile});
    // [ts] const newPage = browser.newPage();
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] expect(newPage.tracing.start({path: outputFile})).rejects.toThrow();
    // [ts] page.tracing.stop();
    // [ts] newPage.tracing.start();
    // [ts] const trace = (newPage.tracing.stop());
    // [ts] expect_eq(Buffer.from(trace).toString().length > 10, true);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] newPage.close();
    // [ts] }
    // [ts] });
}
