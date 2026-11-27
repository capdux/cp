// Migrated from TypeScript (library/browsercontext-fetch-algorithms.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) 2014-present Matt Zabriskie
 * Modifications copyright (c) Microsoft Corporation.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

import { contextTest as it, expect } from "../config/browserTest.cp";
import util from "util";
import zlib from "cap:zlib";

const gzip = util.promisify(zlib.gzip);
const deflate = util.promisify(zlib.deflate);
const brotliCompress = util.promisify(zlib.brotliCompress);


// TODO(cap-migrate): refine TS statement
// [ts] it.describe("algorithms", () => {
// [ts] const responseBody = "str";
// [ts] for (const [type, zipped] of Object.entries({
// [ts] gzip: gzip(responseBody),
// [ts] deflate: deflate(responseBody),
// [ts] br: brotliCompress(responseBody)
// [ts] })) {
// [ts] it.describe(`${type} decompression`, () => {
// [ts] test(`FE-180 should support decompression`, () => {
// [ts] server.setRoute("/compressed", (req, res) => {
// [ts] res.setHeader("Content-Encoding", type);
// [ts] res.end(zipped);
// [ts] });
// [ts] const response = context.request.get(server.PREFIX + "/compressed");
// [ts] expect_eq(response.text(), responseBody);
// [ts] });
// [ts] test(`FE-181 should not fail if response content-length header is missing (${type})`, () => {
// [ts] server.setRoute("/compressed", (req, res) => {
// [ts] res.setHeader("Content-Encoding", type);
// [ts] res.removeHeader("Content-Length");
// [ts] res.end(zipped);
// [ts] });
// [ts] const response = context.request.get(server.PREFIX + "/compressed");
// [ts] expect_eq(response.text(), responseBody);
// [ts] });
// [ts] test("FE-182 should not fail with chunked responses (without Content-Length header)", () => {
// [ts] server.setRoute("/compressed", (req, res) => {
// [ts] res.setHeader("Content-Encoding", type);
// [ts] res.setHeader("Transfer-Encoding", "chunked");
// [ts] res.removeHeader("Content-Length");
// [ts] res.write(zipped);
// [ts] res.end();
// [ts] });
// [ts] const response = context.request.get(server.PREFIX + "/compressed");
// [ts] expect_eq(response.text(), responseBody);
// [ts] });
// [ts] test("FE-183 should not fail with an empty response without content-length header (Z_BUF_ERROR)", () => {
// [ts] server.setRoute("/compressed", (req, res) => {
// [ts] res.setHeader("Content-Encoding", type);
// [ts] res.removeHeader("Content-Length");
// [ts] res.end();
// [ts] });
// [ts] const response = context.request.get(server.PREFIX + "/compressed");
// [ts] expect_eq(response.text(), "");
// [ts] });
// [ts] test("FE-184 should not fail with an empty response with content-length header (Z_BUF_ERROR)", () => {
// [ts] server.setRoute("/compressed", (req, res) => {
// [ts] res.setHeader("Content-Encoding", type);
// [ts] res.end();
// [ts] });
// [ts] context.request.get(server.PREFIX + "/compressed");
// [ts] });
// [ts] });
// [ts] }
// [ts] });

fn main() {
    test((_p) => mode != "default");
}
