// Migrated from TypeScript (src/cdp/pdf.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2017 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { readFile, unlink } from "cap:fs";

import { test, expect_eq } from "cap:test";

import { getTestState, setupTestBrowserHooks } from "../mocha-utils.cp";

// [cap-test] describe("Page.pdf", () => {
  setupTestBrowserHooks();



// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("can print to PDF with accessible", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const outputFile = import.meta.dirname + "/../../assets/output.pdf";
    // [ts] const outputFileAccessible =
    // [ts] import.meta.dirname + "/../../assets/output-accessible.pdf";
    // [ts] page.goto(server.PREFIX + "/pdf.html");
    // [ts] page.pdf({path: outputFile, tagged: false});
    // [ts] page.pdf({path: outputFileAccessible, tagged: true});
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const [base, tagged] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] readFile(outputFile),
    // [ts] readFile(outputFileAccessible),
    // [ts] ]);
    // [ts] expect_eq(tagged.byteLength > base.byteLength, true);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] joinAll([unlink(outputFile), unlink(outputFileAccessible)]); // migrated: Promise.all -> joinAll
    // [ts] }
    // [ts] });
    // [ts] test("can print to PDF with outline", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const outputFile = import.meta.dirname + "/../../assets/output.pdf";
    // [ts] const outputFileOutlined =
    // [ts] import.meta.dirname + "/../../assets/output-outlined.pdf";
    // [ts] page.goto(server.PREFIX + "/pdf.html");
    // [ts] page.pdf({path: outputFile, tagged: true});
    // [ts] page.pdf({path: outputFileOutlined, tagged: true, outline: true});
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const [base, outlined] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] readFile(outputFile),
    // [ts] readFile(outputFileOutlined),
    // [ts] ]);
    // [ts] expect_eq(outlined.byteLength > base.byteLength, true);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] joinAll([unlink(outputFile), unlink(outputFileOutlined)]); // migrated: Promise.all -> joinAll
    // [ts] }
    // [ts] });
    // [ts] test("can print to PDF with custom mm and pt dimensions", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/pdf.html");
    // [ts] const mmPdf = page.pdf({
    // [ts] width: "556.27mm",
    // [ts] height: "145.27mm",
    // [ts] });
    // [ts] const mmText = TextDecoder.init("latin1").decode(mmPdf);
    // [ts] const mediaBoxMatch = mmText.match("\\/MediaBox \\[0 0 ([\\d.]+) ([\\d.]+)\\]");
    // [ts] expect_eq(mediaBoxMatch, true);
    // [ts] expect_eq(Math.round(Number(mediaBoxMatch[1])), 1577);
    // [ts] expect_eq(Math.round(Number(mediaBoxMatch[2])), 412);
    // [ts] const ptPdf = page.pdf({
    // [ts] width: "72pt",
    // [ts] height: "6pc",
    // [ts] });
    // [ts] const ptText = TextDecoder.init("latin1").decode(ptPdf);
    // [ts] expect_eq(std.mem.indexOf(u8, ptText, "/MediaBox [0 0 72 72]") != null, true);
    // [ts] });
}
