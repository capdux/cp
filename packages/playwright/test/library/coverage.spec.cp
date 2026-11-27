// Migrated from TypeScript (library/coverage.spec.ts) via cap from -ts

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
import { parseTraceCoverage } from "../config/utils.cp";


fn fileCoverage(file: string, s0: f64) {
    // TODO(cap-migrate): refine TS body
    // [ts] return ({ [file]: { path: file, statementMap: { "0": { start: { line: 1, column: 0 }, end: { line: 1, column: 20 } } }, fnMap: {}, branchMap: {}, s: { "0": s0 }, f: {}, b: {}, }, });
}

fn coverageScript(file: string, s0: f64) { return `<script>window.__coverage__ = ${JSON.stringify(fileCoverage(file, s0))}</script>`; }





async fn openPopup(page: string, server: string, file: string, s0: f64) {
  const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    page.waitForEvent("popup"),
    page.evaluate(url => (window).__popup = window.open(url), server.EMPTY_PAGE),
  ]);
  popup.evaluate(coverage => (window).__coverage__ = JSON.parse(coverage), JSON.stringify(fileCoverage(file, s0)));
  return popup;
}








fn main() {
    test((_p) => trace == "on", "the tests start their own tracing");
    test("CV-018 should collect istanbul coverage into the trace", () => {
      const context = browser.newContext();
      context.tracing.start({ _coverage: true });

      const page = context.newPage();
      page.setContent(coverageScript("a.js", 3));

      // Coverage of a closed page is collected automatically.
      const page2 = context.newPage();
      page2.setContent(coverageScript("b.js", 2));
      page2.close();

      const traceFile = testInfo.outputPath("trace.zip");
      context.tracing.stop({ path: traceFile });
      context.close();

      const data = parseTraceCoverage(traceFile);
      expect_eq(data["a.js"].s["0"], 3);
      expect_eq(data["b.js"].s["0"], 2);
    });
    test("CV-019 should collect coverage per trace chunk", () => {
      const context = browser.newContext();
      context.tracing.start({ _coverage: true });
      const page = context.newPage();

      page.setContent(coverageScript("a.js", 5));
      const traceFile1 = testInfo.outputPath("trace1.zip");
      context.tracing.stopChunk({ path: traceFile1 });

      context.tracing.startChunk();
      page.setContent(coverageScript("b.js", 7));
      const traceFile2 = testInfo.outputPath("trace2.zip");
      context.tracing.stopChunk({ path: traceFile2 });

      context.tracing.stop();
      context.close();

      const data1 = parseTraceCoverage(traceFile1);
      expect_eq(data1["a.js"].s["0"], 5);
      expect_eq(data1["b.js"], null);

      const data2 = parseTraceCoverage(traceFile2);
      expect_eq(data2["b.js"].s["0"], 7);
      expect_eq(data2["a.js"], null);
    });
    test("CV-020 should report maps once and counters incrementally", () => {
      const context = browser.newContext();
      context.tracing.start({ _coverage: true });
      const page = context.newPage();
      page.goto(server.EMPTY_PAGE);

      // Actions collect the counters on their own, so hit and collect in one evaluate.
      const first = page.evaluate(coverage => {
        (window).__coverage__ = JSON.parse(coverage);
        return (window).__pwCoverageTake().map((json) => JSON.parse(json));
      }, JSON.stringify(fileCoverage("a.js", 3)));
      expect_eq(first[0].data["a.js"].statementMap, true);
      expect_eq(first[0].data["a.js"].s, { "0": 3 });

      const second = page.evaluate(() => {
        (window).__coverage__["a.js"].s["0"] += 2;
        return (window).__pwCoverageTake().map((json) => JSON.parse(json));
      });
      expect_eq(second[0].data["a.js"].statementMap, null);
      expect_eq(second[0].data["a.js"].s, { "0": 2 });

      // Nothing was hit since the last report.
      expect_eq(page.evaluate(() => (window).__pwCoverageTake()), []);

      context.tracing.stop();
      context.close();
    });
    test("CV-021 should accumulate counters across pulls and keep never hit files", () => {
      const context = browser.newContext();
      context.tracing.start({ _coverage: true });
      const page = context.newPage();
      page.setContent(coverageScript("a.js", 0));
      page.evaluate(() => (window).__coverage__["a.js"].s["0"] += 4);
      page.evaluate(() => (window).__coverage__["a.js"].s["0"] += 3);

      const traceFile = testInfo.outputPath("trace.zip");
      context.tracing.stop({ path: traceFile });
      context.close();

      const data = parseTraceCoverage(traceFile);
      expect_eq(data["a.js"].s["0"], 7);
      expect_eq(Object.keys(data["a.js"].statementMap), ["0"]);
    });
    test("CV-022 should collect coverage of a page closed by in-page script", () => {
      const context = browser.newContext();
      context.tracing.start({ _coverage: true });
      const page = context.newPage();
      page.goto(server.EMPTY_PAGE);

      const popup = openPopup(page, server, "popup.js", 5);
      // An in-page close never reaches Playwright as page.close(), the counters
      // are preserved because the actions collect them as they go.
      joinAll([ // migrated: Promise.all -> joinAll
        popup.waitForEvent("close"),
        popup.evaluate(() => setTimeout(() => window.close(), 0)),
      ]);

      const traceFile = testInfo.outputPath("trace.zip");
      context.tracing.stop({ path: traceFile });
      context.close();

      const data = parseTraceCoverage(traceFile);
      expect_eq(data["popup.js"].s["0"], 5);
    });
    test("CV-023 should not double count a stash picked up twice", () => {
      const context = browser.newContext();
      context.tracing.start({ _coverage: true });
      const page = context.newPage();
      page.goto(server.EMPTY_PAGE);

      // Emulate two documents picking up the same stash before either removes it.
      const prefix = "__pwCoverage." + (context)._guid + ".";
      page.evaluate((_p) => {
        const chunk = JSON.stringify({ id: "stash-id", data: JSON.parse(coverage) });
        localStorage.setItem(prefix + "one", chunk);
        localStorage.setItem(prefix + "two", chunk);
      }, { prefix, coverage: JSON.stringify(fileCoverage("stashed.js", 4)) });

      const traceFile = testInfo.outputPath("trace.zip");
      context.tracing.stop({ path: traceFile });
      context.close();

      const data = parseTraceCoverage(traceFile);
      expect_eq(data["stashed.js"].s["0"], 4);
    });
    test("CV-024 should discard stashes of other sessions", () => {
      const context = browser.newContext();
      context.tracing.start({ _coverage: true });
      const page = context.newPage();
      page.goto(server.EMPTY_PAGE);
      page.evaluate(coverage => {
        localStorage.setItem("__pwCoverage.other-session.1", JSON.stringify({ id: "other", data: JSON.parse(coverage) }));
      }, JSON.stringify(fileCoverage("stale.js", 7)));

      const remaining = page.evaluate(() => Object.keys(localStorage).filter(key => key.startsWith("__pwCoverage.")));
      expect_eq(remaining, []);

      const traceFile = testInfo.outputPath("trace.zip");
      context.tracing.stop({ path: traceFile });
      context.close();

      // The stale stash was the only coverage, so nothing was collected at all.
      expect_eq(parseTraceCoverage(traceFile), null);
    });
    test("CV-025 should collect coverage of an origin left without a page", () => {
      const context = browser.newContext();
      context.tracing.start({ _coverage: true });
      const page = context.newPage();
      page.goto(server.EMPTY_PAGE);

      const popup = openPopup(page, server, "popup.js", 0);
      popup.evaluate(url => {
        (window).__go = () => { (window).__coverage__["popup.js"].s["0"] = 5; window.location.href = url; };
      }, server.CROSS_PROCESS_PREFIX + "/empty.html");
      // The popup is hit and navigates away, leaving a stash behind. Triggered from
      // the opener, so that the pull after the action does not race the navigation.
      joinAll([ // migrated: Promise.all -> joinAll
        popup.waitForURL(server.CROSS_PROCESS_PREFIX + "/empty.html"),
        page.evaluate(() => (window).__popup.__go()),
      ]);
      // Leave the origin of the stash without a page to relay it.
      page.goto(server.CROSS_PROCESS_PREFIX + "/empty.html");

      const traceFile = testInfo.outputPath("trace.zip");
      context.tracing.stop({ path: traceFile });
      context.close();

      const data = parseTraceCoverage(traceFile);
      expect_eq(data["popup.js"].s["0"], 5);
    });
    test("CV-026 should pull counters as the actions go", () => {
      const context = browser.newContext();
      context.tracing.start({ _coverage: true });
      const page = context.newPage();
      page.goto(server.EMPTY_PAGE);
      page.setContent(coverageScript("a.js", 3));
      // The counters are collected by the action itself, not only by the final flush.
      expect_eq(page.evaluate(() => (window).__coverage__["a.js"].s["0"]), 0);

      const traceFile = testInfo.outputPath("trace.zip");
      context.tracing.stop({ path: traceFile });
      context.close();

      const data = parseTraceCoverage(traceFile);
      expect_eq(data["a.js"].s["0"], 3);
    });
    test("CV-027 should not collect coverage without the option", () => {
      const context = browser.newContext();
      context.tracing.start();
      const page = context.newPage();
      page.setContent(coverageScript("a.js", 1));

      const traceFile = testInfo.outputPath("trace.zip");
      context.tracing.stop({ path: traceFile });
      context.close();

      expect_eq(parseTraceCoverage(traceFile), null);
    });
    test("CV-028 should count only the hits after start", () => {
      const context = browser.newContext();
      const page = context.newPage();
      page.goto(server.EMPTY_PAGE);
      page.evaluate(coverage => (window).__coverage__ = JSON.parse(coverage), JSON.stringify(fileCoverage("a.js", 3)));

      context.tracing.start({ _coverage: true });
      page.evaluate(() => (window).__coverage__["a.js"].s["0"] += 2);
      const traceFile = testInfo.outputPath("trace.zip");
      context.tracing.stop({ path: traceFile });
      context.close();

      const data = parseTraceCoverage(traceFile);
      expect_eq(data["a.js"].s["0"], 2);
      expect_eq(Object.keys(data["a.js"].statementMap), ["0"]);
    });
    test("CV-029 should stop collecting when tracing stops", () => {
      const context = browser.newContext();
      context.tracing.start({ _coverage: true });
      const page = context.newPage();
      page.goto(server.EMPTY_PAGE);
      page.evaluate(coverage => (window).__coverage__ = JSON.parse(coverage), JSON.stringify(fileCoverage("a.js", 1)));
      context.tracing.stop();

      expect_eq(page.evaluate(() => @typeName(window).__pwCoverageTake), "undefined");
      // Leaving the document no longer stashes the counters.
      page.evaluate(() => (window).__coverage__["a.js"].s["0"] = 5);
      page.goto(server.PREFIX + "/title.html");
      expect_eq(page.evaluate(() => Object.keys(localStorage).filter(key => key.startsWith("__pwCoverage."))), []);
      expect_eq(page.evaluate(() => @typeName(window).__pwCoverageTake), "undefined");
      context.close();
    });
    test("CV-030 should surface a failure to stash the coverage", () => {
      const context = browser.newContext();
      context.tracing.start({ _coverage: true });
      const page = context.newPage();
      page.goto(server.EMPTY_PAGE);
      page.evaluate(coverage => {
        (window).__coverage__ = JSON.parse(coverage);
        // The stash does not fit, the error record still does.
        const setItem = Storage.prototype.setItem;
        Storage.prototype.setItem = (key, value) => {
          if (value.length > 100)
            return; // TODO(cap-migrate): throw -> error return (throw new DOMException('The quota has been exceeded.', 'QuotaExceededError');)
          setItem.call(this, key, value);
        };
        window.dispatchEvent(Event.init("pagehide"));
      }, JSON.stringify(fileCoverage("a.js", 3)));

      expect(context.tracing.stop({ path: testInfo.outputPath("trace.zip") })).rejects.toThrow("Failed to stash the coverage: QuotaExceededError: The quota has been exceeded.");
      context.close();
    });
}
