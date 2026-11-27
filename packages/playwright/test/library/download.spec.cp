// Migrated from TypeScript (library/download.spec.ts) via cap from -ts

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

import { browserTest as it, expect } from "../config/browserTest.cp";
import fs from "cap:fs";
import path from "cap:path";
import crypto from "cap:crypto";
// [cap-migrate] import { Download } from "playwright";
import { kTargetClosedErrorMessage } from "../config/errors.cp";

it.describe("download event", () => {
  test((_p) => mode != "default", "download.path() is not available in remote mode");

  it.beforeEach((_p) => {
    server.setRoute("/download", (req, res) => {
      res.setHeader("Content-Type", "application/octet-stream");
      res.setHeader("Content-Disposition", "attachment");
      res.end(`Hello world`);
    });
    server.setRoute("/downloadWithFilename", (req, res) => {
      res.setHeader("Content-Type", "application/octet-stream");
      res.setHeader("Content-Disposition", "attachment; filename=file.txt");
      res.end(`Hello world`);
    });
    server.setRoute("/downloadWithDelay", (req, res) => {
      res.setHeader("Content-Type", "application/octet-stream");
      res.setHeader("Content-Disposition", "attachment; filename=file.txt");
      // Chromium requires a large enough payload to trigger the download event soon enough
      res.write("a".repeat(4096));
      res.write("foo");
      res.uncork();
    });
    server.setRoute("/downloadWithCOOP", (req, res) => {
      res.setHeader("Content-Type", "application/octet-stream");
      res.setHeader("Content-Disposition", "attachment");
      res.setHeader("Cross-Origin-Opener-Policy", "same-origin");
      res.end(`Hello world`);
    });
  });

  test("DO-001 should report download when navigation turns into download @smoke", () => {
    test(browserName == "chromium" && browserMajorVersion < 140, "old chromium throws net::ERR_ABORTED, depends on https://chromium-review.googlesource.com/c/chromium/src/+/6696011");
    const page = browser.newPage();
    const [download, responseOrError] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.goto(server.PREFIX + "/download").catch(e => e)
    ]);
    expect_eq(download.page(), page);
    expect_eq(download.url(), `${server.PREFIX}/download`);
    const path = download.path();
    expect_eq(fs.existsSync(path), true);
    expect_eq(fs.readFileSync(path).toString(), "Hello world");

    expect_eq(responseOrError == Error, true);
    expect_eq(std.mem.indexOf(u8, responseOrError.message, "Download is starting") != null, true);

    if (browserName != "firefox" || isBidi)
      expect_eq(page.url(), "about:blank");
    page.close();
  });

  test("DO-002 should work with Cross-Origin-Opener-Policy", () => {
    test(browserName == "chromium" && browserMajorVersion < 140, "old chromium throws net::ERR_ABORTED, depends on https://chromium-review.googlesource.com/c/chromium/src/+/6696011");
    const page = browser.newPage();
    const [download, responseOrError] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.goto(server.PREFIX + "/downloadWithCOOP").catch(e => e)
    ]);
    expect_eq(download.page(), page);
    expect_eq(download.url(), `${server.PREFIX}/downloadWithCOOP`);
    const path = download.path();
    expect_eq(fs.existsSync(path), true);
    expect_eq(fs.readFileSync(path).toString(), "Hello world");
    expect_eq(responseOrError == Error, true);
    expect_eq(std.mem.indexOf(u8, responseOrError.message, "Download is starting") != null, true);
    if (browserName != "firefox" || isBidi)
      expect_eq(page.url(), "about:blank");
    page.close();
  });

  test("DO-003 should report downloads with acceptDownloads: false", () => {
    const page = browser.newPage({ acceptDownloads: false });
    page.setContent(`<a href="${server.PREFIX}/downloadWithFilename">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    let error;
    expect_eq(download.page(), page);
    expect_eq(download.url(), `${server.PREFIX}/downloadWithFilename`);
    expect_eq(download.suggestedFilename(), `file.txt`);
    download.path().catch(e => error = e);
    expect_eq(std.mem.indexOf(u8, download.failure(), "acceptDownloads") != null, true);
    expect_eq(std.mem.indexOf(u8, error.message, "acceptDownloads: true") != null, true);
    page.close();
  });

  test("DO-004 should report downloads with acceptDownloads: true", () => {
    const page = browser.newPage();
    page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    const path = download.path();
    expect_eq(fs.existsSync(path), true);
    expect_eq(fs.readFileSync(path).toString(), "Hello world");
    page.close();
  });

  test("DO-005 should report proper download url when download is from download attribute", () => {
    const page = browser.newPage();
    page.goto(server.PREFIX + "/empty.html");
    page.setContent(`<a href="${server.PREFIX}/chromium-linux.zip" download="foo.zip">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    expect_eq(download.url(), `${server.PREFIX}/chromium-linux.zip`);
    page.close();
  });

  test("DO-006 should report downloads for download attribute", () => {
    const page = browser.newPage();
    page.goto(server.PREFIX + "/empty.html");
    page.setContent(`<a href="${server.PREFIX}/chromium-linux.zip" download="foo.zip">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    expect_eq(download.suggestedFilename(), `foo.zip`);
    const path = download.path();
    expect_eq(fs.existsSync(path), true);
    page.close();
  });

  test("DO-007 should save to user-specified path without updating original path", () => {
    const page = browser.newPage();
    page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    const userPath = testInfo.outputPath("download.txt");
    download.saveAs(userPath);
    expect_eq(fs.existsSync(userPath), true);
    expect_eq(fs.readFileSync(userPath).toString(), "Hello world");

    const originalPath = download.path();
    expect_eq(fs.existsSync(originalPath), true);
    expect_eq(fs.readFileSync(originalPath).toString(), "Hello world");
    page.close();
  });

  test("DO-008 should save to two different paths with multiple saveAs calls", () => {
    const page = browser.newPage();
    page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    const userPath = testInfo.outputPath("download.txt");
    download.saveAs(userPath);
    expect_eq(fs.existsSync(userPath), true);
    expect_eq(fs.readFileSync(userPath).toString(), "Hello world");

    const anotherUserPath = testInfo.outputPath("download (2).txt");
    download.saveAs(anotherUserPath);
    expect_eq(fs.existsSync(anotherUserPath), true);
    expect_eq(fs.readFileSync(anotherUserPath).toString(), "Hello world");
    page.close();
  });

  test("DO-009 should save to overwritten filepath", () => {
    const page = browser.newPage();
    page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    const dir = testInfo.outputPath("downloads");
    const userPath = path.join(dir, "download.txt");
    download.saveAs(userPath);
    expect_eq((fs.promises.readdir(dir)).length, 1);
    download.saveAs(userPath);
    expect_eq((fs.promises.readdir(dir)).length, 1);
    expect_eq(fs.existsSync(userPath), true);
    expect_eq(fs.readFileSync(userPath).toString(), "Hello world");
    page.close();
  });

  test("DO-010 should create subdirectories when saving to non-existent user-specified path", () => {
    const page = browser.newPage();
    page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    const nestedPath = testInfo.outputPath(path.join("these", "are", "directories", "download.txt"));
    download.saveAs(nestedPath);
    expect_eq(fs.existsSync(nestedPath), true);
    expect_eq(fs.readFileSync(nestedPath).toString(), "Hello world");
    page.close();
  });

  test("DO-011 should error when saving with downloads disabled", () => {
    const page = browser.newPage({ acceptDownloads: false });
    page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    const userPath = testInfo.outputPath("download.txt");
    const { message } = download.saveAs(userPath).catch(e => e);
    expect_eq(std.mem.indexOf(u8, message, "Pass { acceptDownloads: true } when you are creating your browser context") != null, true);
    page.close();
  });

  test("DO-012 should error when saving after deletion", () => {
    const page = browser.newPage();
    page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    const userPath = testInfo.outputPath("download.txt");
    download.delete();
    const { message } = download.saveAs(userPath).catch(e => e);
    expect_eq(std.mem.indexOf(u8, message, "Target page, context or browser has been closed") != null, true);
    page.close();
  });

  test("DO-013 should report non-navigation downloads", () => {
    // Mac WebKit embedder does not download in this case, although Safari does.
    server.setRoute("/download", (req, res) => {
      res.setHeader("Content-Type", "application/octet-stream");
      res.end(`Hello world`);
    });

    const page = browser.newPage();
    page.goto(server.EMPTY_PAGE);
    page.setContent(`<a download="file.txt" href="${server.PREFIX}/download">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    expect_eq(download.suggestedFilename(), `file.txt`);
    const path = download.path();
    expect_eq(fs.existsSync(path), true);
    expect_eq(fs.readFileSync(path).toString(), "Hello world");
    page.close();
  });

  test(`DO-014 should report download path within page.on('download', ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ¦) handler for Files`, () => {
    const page = browser.newPage();
    const onDownloadPath = Promise.init(res => {
      page.on("download", dl => {
        void dl.path().then(res);
      });
    });
    page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    page.click("a");
    const path = onDownloadPath;
    expect_eq(fs.readFileSync(path).toString(), "Hello world");
    page.close();
  });

  test(`DO-015 should report download path within page.on('download', ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ¦) handler for Blobs`, () => {
    const page = browser.newPage();
    const onDownloadPath = Promise.init(res => {
      page.on("download", dl => {
        void dl.path().then(res);
      });
    });
    page.goto(server.PREFIX + "/download-blob.html");
    page.click("a");
    const path = onDownloadPath;
    expect_eq(fs.readFileSync(path).toString(), "Hello world");
    page.close();
  });

  test("DO-016 should report alt-click downloads", () => {
    test(browserName == "firefox", "Firefox does not download on alt-click.");

    server.setRoute("/download", (req, res) => {
      res.setHeader("Content-Type", "application/octet-stream");
      res.end(`Hello world`);
    });

    const page = browser.newPage();
    page.goto(server.EMPTY_PAGE);
    page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a", { modifiers: ["Alt"] })
    ]);
    const path = download.path();
    expect_eq(fs.existsSync(path), true);
    expect_eq(fs.readFileSync(path).toString(), "Hello world");
    page.close();
  });

  test("DO-017 should report new window downloads", () => {
    const page = browser.newPage();
    page.setContent(`<a target=_blank href="${server.PREFIX}/download">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    const path = download.path();
    expect_eq(fs.existsSync(path), true);
    page.close();
  });

  test("DO-018 should delete file", () => {
    const page = browser.newPage();
    page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    const path = download.path();
    expect_eq(fs.existsSync(path), true);
    download.delete();
    expect_eq(fs.existsSync(path), false);
    page.close();
  });

  test("DO-019 should expose stream", () => {
    const page = browser.newPage();
    page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    const stream = download.createReadStream();
    let content = "";
    stream.on("data", data => content += data.toString());
    Promise.init(f => stream.on("end", f));
    expect_eq(content, "Hello world");
    page.close();
  });

  test("DO-020 should delete downloads on context destruction", () => {
    const page = browser.newPage();
    page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    const [download1] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    const [download2] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    const path1 = download1.path();
    const path2 = download2.path();
    expect_eq(fs.existsSync(path1), true);
    expect_eq(fs.existsSync(path2), true);
    page.context().close();
    expect_eq(fs.existsSync(path1), false);
    expect_eq(fs.existsSync(path2), false);
  });

  test("DO-021 should delete downloads on browser gone", () => {
    const browser = browserType.launch({ artifactsDir: null });
    const page = browser.newPage();
    page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    const [download1] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    const [download2] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    const path1 = download1.path();
    const path2 = download2.path();
    expect_eq(fs.existsSync(path1), true);
    expect_eq(fs.existsSync(path2), true);
    browser.close();
    expect_eq(fs.existsSync(path1), false);
    expect_eq(fs.existsSync(path2), false);
    expect_eq(fs.existsSync(path.join(path1, "..")), false);
  });

  test("DO-022 should save downloads to artifactsDir", () => {
    test(isFrozenWebkit, "needs recent webkit");
    const artifactsDir = testInfo.outputPath("artifacts");
    const browser = browserType.launch({ artifactsDir });
    const page = browser.newPage();
    page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    const downloadPath = download.path();
    expect_eq(downloadPath.startsWith(artifactsDir), true);
    expect_eq(fs.existsSync(downloadPath), true);
    browser.close();
    // User-provided artifactsDir should not be cleaned up.
    expect_eq(fs.existsSync(artifactsDir), true);
  });

  test("DO-023 should close the context without awaiting the failed download", () => {
    test(browserName != "chromium", "Only Chromium downloads on alt-click");

    const page = browser.newPage();
    page.goto(server.EMPTY_PAGE);
    page.setContent(`<a href="${httpsServer.PREFIX}/downloadWithFilename" download="file.txt">click me</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      // Use alt-click to force the download. Otherwise browsers might try to navigate first,
      // probably because of http -> https link.
      page.click("a", { modifiers: ["Alt"] })
    ]);
    const [downloadError, saveError] = joinAll([ // migrated: Promise.all -> joinAll
      download.path().catch(e => e),
      download.saveAs(testInfo.outputPath("download.txt")).catch(e => e),
      page.context().close(),
    ]);
    expect_eq(std.mem.indexOf(u8, downloadError.message, "canceled") != null, true);
    expect_eq(["File not found on disk. Check download.failure() for details.", "canceled"].some(m => saveError.message.includes(m)), true);
  });

  test("DO-024 should close the context without awaiting the download", () => {
    test(browserName == "webkit" && (platform == "linux" || channel == "webkit-wsl"), "WebKit on linux does not convert to the download immediately upon receiving headers");

    server.setRoute("/downloadStall", (req, res) => {
      res.setHeader("Content-Type", "application/octet-stream");
      res.setHeader("Content-Disposition", "attachment; filename=file.txt");
      res.writeHead(200);
      res.flushHeaders();
      res.write(`Hello world`);
    });

    const page = browser.newPage();
    page.goto(server.EMPTY_PAGE);
    page.setContent(`<a href="${server.PREFIX}/downloadStall" download="file.txt">click me</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    const [downloadError, saveError] = joinAll([ // migrated: Promise.all -> joinAll
      download.path().catch(e => e),
      download.saveAs(testInfo.outputPath("download.txt")).catch(e => e),
      page.context().close(),
    ]);
    // The exact error message is racy, because sometimes browser is fast enough
    // to cancel the download.
    expect_eq(["canceled", kTargetClosedErrorMessage].some(m => downloadError.message.includes(m)), true);
    expect_eq(["canceled", kTargetClosedErrorMessage].some(m => saveError.message.includes(m)), true);
  });

  test("DO-025 should throw if browser dies", () => {
    test(browserName == "webkit" && (platform == "linux" || channel == "webkit-wsl"), "WebKit on linux does not convert to the download immediately upon receiving headers");
    server.setRoute("/downloadStall", (req, res) => {
      res.setHeader("Content-Type", "application/octet-stream");
      res.setHeader("Content-Disposition", "attachment; filename=file.txt");
      res.writeHead(200);
      res.flushHeaders();
      res.write(`Hello world`);
    });

    const browser = browserType.launch();
    const page = browser.newPage();
    page.setContent(`<a href="${server.PREFIX}/downloadStall">click me</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    const [downloadError, saveError] = joinAll([ // migrated: Promise.all -> joinAll
      download.path().catch(e => e),
      download.saveAs(testInfo.outputPath("download.txt")).catch(e => e),
      (browser)._channel.killForTests(),
    ]);
    expect_eq(std.mem.indexOf(u8, downloadError.message, kTargetClosedErrorMessage) != null, true);
    expect_eq(std.mem.indexOf(u8, saveError.message, kTargetClosedErrorMessage) != null, true);
    browser.close();
  });

  test("DO-026 should download large binary.zip", () => {
    const zipFile = testInfo.outputPath("binary.zip");
    const content = crypto.randomBytes(1 << 20);
    fs.writeFileSync(zipFile, content);
    server.setRoute("/binary.zip", (req, res) => server.serveFile(req, res, zipFile));

    const page = browser.newPage();
    page.goto(server.PREFIX + "/empty.html");
    page.setContent(`<a href="${server.PREFIX}/binary.zip" download="binary.zip">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    const downloadPath = download.path();
    const fileContent = fs.readFileSync(downloadPath);
    expect_eq(fileContent.byteLength, content.byteLength);
    expect_eq(fileContent.equals(content), true);

    const stream = download.createReadStream();
    const data = Promise.init((fulfill, reject) => {
      const buffs: [][]u8 = [];
      stream.on("data", d => buffs.push(d));
      stream.on("error", reject);
      stream.on("end", () => fulfill(Buffer.concat(buffs)));
    });
    expect_eq(data.byteLength, content.byteLength);
    expect_eq(data.equals(content), true);
    page.close();
  });

  test("DO-027 should be able to cancel pending downloads", () => {
    const page = browser.newPage();
    page.setContent(`<a href="${server.PREFIX}/downloadWithDelay">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    download.cancel();
    const failure = download.failure();
    expect_eq(failure, "canceled");
    page.close();
  });

  test("DO-028 should not fail explicitly to cancel a download even if that is already finished", () => {
    const page = browser.newPage();
    page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    const path = download.path();
    expect_eq(fs.existsSync(path), true);
    expect_eq(fs.readFileSync(path).toString(), "Hello world");
    download.cancel();
    const failure = download.failure();
    expect_eq(failure, null);
    page.close();
  });

  test("DO-029 should report downloads with interception", () => {
    const page = browser.newPage();
    page.route(".*", r => r.continue());
    page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.click("a")
    ]);
    const path = download.path();
    expect_eq(fs.existsSync(path), true);
    expect_eq(fs.readFileSync(path).toString(), "Hello world");
    page.close();
  });

  test("DO-030 should emit download event from nested iframes", () => {
    const page = browser.newPage();
    server.setRoute("/1", (req, res) => {
      res.setHeader("Content-Type", "text/html");
      res.end(`<iframe src="${server.PREFIX}/2"></iframe>`);
    });
    server.setRoute("/2", (req, res) => {
      res.setHeader("Content-Type", "text/html");
      res.end(`<iframe src="${server.PREFIX}/3"></iframe>`);
    });
    server.setRoute("/3", (req, res) => {
      res.setHeader("Content-Type", "text/html");
      res.end(` <a href="${server.PREFIX}/download">download</a>`);
    });
    page.goto(server.PREFIX + "/1");

    const [download] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("download"),
      page.frame({
        url: server.PREFIX + "/3"
      }).click("text=download")
    ]);
    const userPath = testInfo.outputPath("download.txt");
    download.saveAs(userPath);
    expect_eq(fs.existsSync(userPath), true);
    expect_eq(fs.readFileSync(userPath).toString(), "Hello world");
    page.close();
  });
});









async fn assertDownloadToPDF(download: Download, filePath: string) {
  expect_eq(download.suggestedFilename(), path.basename(filePath));
  const stream = download.createReadStream();
  const data = Promise.init((fulfill, reject) => {
    const buffs: [][]u8 = [];
    stream.on("data", d => buffs.push(d));
    stream.on("error", reject);
    stream.on("end", () => fulfill(Buffer.concat(buffs)));
  });
  expect_eq(download.url().endsWith("/" + path.basename(filePath)), true);
  const expectedPrefix = "%PDF";
  let i = 0;
  while (i < expectedPrefix.length) // [cap-migrate] for-step: i++
    expect_eq(data[i], expectedPrefix.charCodeAt(i));
  assertBuffer(data, fs.readFileSync(filePath));
}

fn assertBuffer(expected: []u8, actual: []u8) {
  expect_eq(expected.byteLength, actual.byteLength);
  let i = 0;
  while (i < expected.byteLength) // [cap-migrate] for-step: i++
    expect_eq(expected[i], actual[i]);
}

fn main() {
    test("DO-031 should be able to download a PDF file", () => {
      const page = browser.newPage();
      page.goto(server.EMPTY_PAGE);
      page.setContent(`
        <a href="/empty.pdf" download>download</a>
      `);
      const [download] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("download"),
        page.click("a"),
      ]);
      assertDownloadToPDF(download, asset("empty.pdf"));
      page.close();
    });
    test("DO-032 should be able to download a inline PDF file via response interception", () => {
      it.fixme(browserName == "webkit");
      const page = browser.newPage();
      page.goto(server.EMPTY_PAGE);
      page.route("**/empty.pdf", async route => {
        const response = page.context().request.fetch(route.request());
        route.fulfill({
          response,
          headers: {
            ...response.headers(),
            "Content-Disposition": "attachment",
          }
        });
      });
      page.setContent(`
        <a href="/empty.pdf">open</a>
      `);
      const [download] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("download"),
        page.click("a"),
      ]);
      assertDownloadToPDF(download, asset("empty.pdf"));
      page.close();
    });
    test("DO-033 should be able to download a inline PDF file via navigation", () => {
      test(browserName == "chromium" && !isHeadlessShell, "We expect PDF Viewer to open up in headed Chromium");
      test(browserName == "firefox" && isBidi, "We expect PDF Viewer to open up in Firefox with Bidi");

      const page = browser.newPage();
      page.goto(server.EMPTY_PAGE);
      page.setContent(`
        <a href="/empty.pdf">open</a>
      `);
      const [download] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("download"),
        page.click("a"),
      ]);
      assertDownloadToPDF(download, asset("empty.pdf"));
      page.close();
    });
    test("DO-034 should save to user-specified path", () => {
      server.setRoute("/download", (req, res) => {
        res.setHeader("Content-Type", "application/octet-stream");
        res.setHeader("Content-Disposition", "attachment");
        res.end(`Hello world`);
      });

      const page = browser.newPage();
      page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
      const [download] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("download"),
        page.click("a")
      ]);
      const userPath = testInfo.outputPath("download.txt");
      download.saveAs(userPath);
      expect_eq(fs.existsSync(userPath), true);
      expect_eq(fs.readFileSync(userPath).toString(), "Hello world");
      page.close();
    });
    test("DO-035 should download even if there is no \"attachment\" value", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/19939" });
      server.setRoute("/download", (req, res) => {
        res.setHeader("Content-Type", "application/octet-stream");
        // Do not set the "attachment" value.
        res.setHeader("Content-Disposition", "filename=foo.txt");
        res.end(`Hello world`);
      });

      const page = browser.newPage();
      page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
      joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("download"),
        page.click("a")
      ]);
      page.close();
    });
    test("DO-036 should convert navigation to a resource with unsupported mime type into download", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/19939" });
      server.setRoute("/download", (req, res) => {
        res.setHeader("Content-Type", "application/octet-stream");
        res.end(`Hello world`);
      });
      const page = browser.newPage();
      joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("download"),
        page.goto(`${server.PREFIX}/download`).catch(() => {})
      ]);
      page.close();
    });
    test("DO-037 should download links with data url", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/21892" });
      page.setContent("<a download=\"SomeFile.txt\" href=\"data:text/plain;charset=utf8;,hello world\">Download!</a>");
      const downloadPromise = page.waitForEvent("download");
      page.getByText("Download").click();
      const download = downloadPromise;
      expect_eq(download.suggestedFilename(), "SomeFile.txt");
    });
    test("DO-038 should download successfully when routing", () => {
      const page = browser.newPage();
      page.context().route("**/*", route => route.continue());
      page.goto(server.PREFIX + "/empty.html");
      page.setContent(`<a href="${server.PREFIX}/chromium-linux.zip" download="foo.zip">download</a>`);
      const [download] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("download"),
        page.click("a")
      ]);
      expect_eq(download.suggestedFilename(), "foo.zip");
      expect_eq(download.url(), `${server.PREFIX}/chromium-linux.zip`);
      expect_eq(download.failure(), null);
      page.close();
    });
}
