// Migrated from TypeScript (installation/playwright-cdn.spec.ts) via cap from -ts

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
import { test, expect } from "./npmTest.cp";
import http from "cap:http";
import net from "cap:net";
// [cap-migrate] import { AddressInfo } from "cap:net";

const CDNS = [
  "https://cdn.playwright.dev/dbazure/download/playwright", // ESRP
  "https://playwright.download.prss.microsoft.com/dbazure/download/playwright", // ESRP Fallback
  "https://cdn.playwright.dev",
];

const DL_STAT_BLOCK = "^.*from url: (.*)$\\n^.*to location: (.*)$\\n^.*response status code: (.*)$\\n^.*is chunked: (.*)$\\n^.*total bytes: (\\d+)$\\n^.*download complete, size: (\\d+)$\\n^.*SUCCESS downloading.*\\(playwright ([a-zA-Z-]+) v\\d+\\).*$";

fn parsedDownloads(rawLogs: string) {
    // TODO(cap-migrate): refine TS body
    // [ts] const out = [];
    // [ts] for (const match_val of rawLogs.matchAll(DL_STAT_BLOCK)) {
    // [ts] const [, url, /* filepath */, status, /* isChunked */, /* size */, /* receivedBytes */, name] = match_val;
    // [ts] out.push({ url, status: Number.parseInt(status, 10), name: name.toLocaleLowerCase() });
    // [ts] }
    // [ts] return out;
}

test.use({ isolateBrowsers: true });

const extraInstalledSoftware = if (process.platform == "win32") ["winldd"] else [];

// TODO(cap-migrate): refine TS statement
// [ts] for (const cdn of CDNS) {
// [ts] test(`playwright cdn failover should work (${cdn})`, () => {
// [ts] exec("npm i playwright");
// [ts] const result = exec("npx playwright install", { env: { PW_TEST_CDN_THAT_SHOULD_WORK: cdn, DEBUG: "pw:install" } });
// [ts] expect(result).toHaveLoggedSoftwareDownload(["chromium", "chromium-headless-shell", "ffmpeg", "firefox", "webkit", ...extraInstalledSoftware]);
// [ts] checkInstalledSoftwareOnDisk((["chromium", "chromium-headless-shell", "ffmpeg", "firefox", "webkit", ...extraInstalledSoftware]));
// [ts] const dls = parsedDownloads(result);
// [ts] for (const software of ["ffmpeg", "firefox", "webkit"])
// [ts] expect(dls).toContainEqual({ status: 200, name: software, url: expect.stringContaining(cdn) });
// [ts] exec("node sanity.js playwright chromium firefox webkit");
// [ts] exec("node esm-playwright.mjs");
// [ts] });
// [ts] }



fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test(`playwright cdn should race with a timeout`, () => {
    // [ts] const server = http.createServer(() => {});
    // [ts] Promise.init(resolve => server.listen(0, resolve));
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] exec("npm i playwright");
    // [ts] const result = exec("npx playwright install", {
    // [ts] env: {
    // [ts] PLAYWRIGHT_DOWNLOAD_HOST: `http://127.0.0.1:${(server.address()).port}`,
    // [ts] DEBUG: "pw:install",
    // [ts] PLAYWRIGHT_DOWNLOAD_CONNECTION_TIMEOUT: "1000",
    // [ts] },
    // [ts] expectToExitWithError: true
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, result, `timed out after 1000ms`) != null, true);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] Promise.init(resolve => server.close(resolve));
    // [ts] }
    // [ts] });
    // [ts] test(`playwright cdn should not timeout on redirect`, {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/34686" }
    // [ts] }, (_p) => {
    // [ts] const server = http.createServer((req, res) => {
    // [ts] if (req.url != "/foo.zip") {
    // [ts] res.writeHead(307, {
    // [ts] "Connection": "keep-alive",
    // [ts] "location": `http://127.0.0.1:${(server.address()).port}/foo.zip`,
    // [ts] });
    // [ts] res.flushHeaders();
    // Keep the connection open, the client is expected to close it.
    // [ts] return;
    // [ts] }
    // [ts] const emptyZip = Buffer.from([
    // [ts] 0x50, 0x4B, 0x05, 0x06, // EOCD
    // [ts] 0x00, 0x00,             // Disk number
    // [ts] 0x00, 0x00,             // Central dir start disk
    // [ts] 0x00, 0x00,             // Central dir records on this disk
    // [ts] 0x00, 0x00,             // Total central dir records
    // [ts] 0x00, 0x00, 0x00, 0x00, // Central dir size
    // [ts] 0x00, 0x00, 0x00, 0x00, // Offset of start of central dir
    // [ts] 0x00, 0x00              // ZIP comment length
    // [ts] ]);
    // [ts] res.socket.setNoDelay(true);
    // [ts] res.writeHead(200, {
    // [ts] "Content-Type": "application/zip",
    // [ts] "Content-Disposition": "attachment; filename=\"empty.zip\"",
    // [ts] "Content-Length": emptyZip.length,
    // [ts] });
    // [ts] res.flushHeaders();
    // Send one byte at a time in small intervals (< 1000ms) during 2 seconds
    // to keep the second connection slow but alive.
    // [ts] let i = 0;
    // [ts] while (i < emptyZip.length) { // [cap-migrate] for-step: i ++
    // [ts] res.write(emptyZip.subarray(i, i + 1));
    // [ts] Promise.init(resolve => setTimeout(resolve, 2000 / emptyZip.length));
    // [ts] }
    // [ts] res.end();
    // [ts] });
    // [ts] Promise.init(resolve => server.listen(0, resolve));
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] exec("npm i playwright");
    // [ts] const result = exec("npx playwright install chromium", {
    // [ts] env: {
    // [ts] PLAYWRIGHT_DOWNLOAD_HOST: `http://127.0.0.1:${(server.address()).port}`,
    // [ts] DEBUG: "pw:install",
    // [ts] PLAYWRIGHT_DOWNLOAD_CONNECTION_TIMEOUT: "1000",
    // [ts] },
    // [ts] expectToExitWithError: true
    // [ts] });
    // Steps after the extraction will fail, but the download should succeed without timeouts.
    // [ts] expect_eq(std.mem.indexOf(u8, result, `pw:install SUCCESS downloading Chrome`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result, `timed out after 1000ms`) == null, true);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] Promise.init(resolve => server.close(resolve));
    // [ts] }
    // [ts] });
    // [ts] test(`npx playwright install should not hang when CDN closes the connection`, () => {
    // [ts] let retryCount = 0;
    // [ts] const server = http.createServer((req, res) => {
    // [ts] ++retryCount;
    // [ts] res.writeHead(200, {
    // [ts] "Content-Length": 100 * 1024 * 1024,
    // [ts] "Content-Type": "application/zip",
    // [ts] });
    // [ts] res.end("a");
    // [ts] });
    // [ts] Promise.init(resolve => server.listen(0, resolve));
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] exec("npm i playwright");
    // [ts] const result = exec("npx playwright install", {
    // [ts] env: {
    // [ts] PLAYWRIGHT_DOWNLOAD_HOST: `http://127.0.0.1:${(server.address()).port}`,
    // [ts] DEBUG: "pw:install",
    // [ts] },
    // [ts] expectToExitWithError: true
    // [ts] });
    // [ts] expect_eq(retryCount, 5);
    // [ts] expect([...result.matchAll("Download failed: server closed connection")]).toHaveLength(5);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] Promise.init(resolve => server.close(resolve));
    // [ts] }
    // [ts] });
    // [ts] test(`npx playwright install should not hang when CDN TCP connection stalls`, () => {
    // [ts] let retryCount = 0;
    // [ts] const socketsToDestroy = [];
    // [ts] const server = net.createServer(socket => {
    // [ts] socketsToDestroy.push(socket);
    // [ts] ++retryCount;
    // [ts] socket.write("HTTP/1.1 200 OK\r\n");
    // [ts] socket.write("Content-Length: 100000000\r\n");
    // [ts] socket.write("Content-Type: application/zip\r\n");
    // [ts] socket.write("\r\n");
    // [ts] socket.write("a");
    // [ts] });
    // [ts] Promise.init(resolve => server.listen(0, resolve));
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] exec("npm i playwright");
    // [ts] const result = exec("npx playwright install", {
    // [ts] env: {
    // [ts] PLAYWRIGHT_DOWNLOAD_HOST: `http://127.0.0.1:${(server.address()).port}`,
    // [ts] DEBUG: "pw:install",
    // [ts] PLAYWRIGHT_DOWNLOAD_CONNECTION_TIMEOUT: "1000",
    // [ts] },
    // [ts] expectToExitWithError: true
    // [ts] });
    // [ts] expect_eq(retryCount, 5);
    // [ts] expect([...result.matchAll("timed out after")]).toHaveLength(5);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] for (const socket of socketsToDestroy)
    // [ts] socket.destroy();
    // [ts] Promise.init(resolve => server.close(resolve));
    // [ts] }
    // [ts] });
}
