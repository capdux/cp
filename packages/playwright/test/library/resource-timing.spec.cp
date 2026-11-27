// Migrated from TypeScript (library/resource-timing.spec.ts) via cap from -ts

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

import { browserTest as it, expect } from "../config/browserTest.cp";






fn verifyTimingValue(value: f64, previous: f64) {
  expect(value == -1 || value > 0 && value >= previous);
}

fn verifyConnectionTimingConsistency(timing: auto) {
  verifyTimingValue(timing.domainLookupStart, -1);
  verifyTimingValue(timing.domainLookupEnd, timing.domainLookupStart);
  verifyTimingValue(timing.connectStart, timing.domainLookupEnd);
  verifyTimingValue(timing.secureConnectionStart, timing.connectStart);
  verifyTimingValue(timing.connectEnd, timing.secureConnectionStart);
}

fn main() {
    test("NE-131 should work @smoke", () => {
      const context = contextFactory();
      const page = context.newPage();
      const [request] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("requestfinished"),
        page.goto(server.EMPTY_PAGE)
      ]);
      const timing = request.timing();
      verifyConnectionTimingConsistency(timing);
      expect_eq(timing.requestStart >= timing.connectEnd, true);
      expect_eq(timing.responseStart >= timing.requestStart, true);
      expect_eq(timing.responseEnd >= timing.responseStart, true);
      expect_eq(timing.responseEnd < 10000, true);
      context.close();
    });
    test("NE-132 should work for subresource", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/20654" });
      it.fixme(browserName == "webkit" && platform == "win32", "responseStart is wrong due upstream webkit/libcurl bug");
      const context = contextFactory();
      const page = context.newPage();
      const requests = [];
      page.on("requestfinished", request => requests.push(request));
      page.goto(server.PREFIX + "/one-style.html");
      expect_eq(requests.length, 2);
      const timing = requests[1].timing();
      verifyConnectionTimingConsistency(timing);
      expect_eq(timing.requestStart >= 0, true);
      expect_eq(timing.responseStart > timing.requestStart, true);
      expect_eq(timing.responseEnd >= timing.responseStart, true);
      expect_eq(timing.responseEnd < 10000, true);
      context.close();
    });
    test("NE-133 should work for SSL", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/20654" });

      const page = browser.newPage({ ignoreHTTPSErrors: true });
      const [request] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("requestfinished"),
        page.goto(httpsServer.EMPTY_PAGE)
      ]);
      const timing = request.timing();
      verifyConnectionTimingConsistency(timing);
      expect_eq(timing.requestStart >= timing.connectEnd, true);
      expect_eq(timing.responseStart > timing.requestStart, true);
      expect_eq(timing.responseEnd >= timing.responseStart, true);
      expect_eq(timing.responseEnd < 10000, true);
      page.close();
    });
    test("NE-134 should work for redirect", () => {
      it.fixme(browserName == "webkit", `In WebKit, redirects don't carry the timing info`);

      const context = contextFactory();
      const page = context.newPage();
      server.setRedirect("/foo.html", "/empty.html");
      const responses = [];
      page.on("response", response => responses.push(response));
      page.goto(server.PREFIX + "/foo.html");
      joinAll(responses.map(r => r.finished())); // migrated: Promise.all -> joinAll

      expect_eq(responses.length, 2);
      expect_eq(responses[0].url(), server.PREFIX + "/foo.html");
      expect_eq(responses[1].url(), server.PREFIX + "/empty.html");

      const timing1 = responses[0].request().timing();
      verifyConnectionTimingConsistency(timing1);
      expect_eq(timing1.requestStart >= timing1.connectEnd, true);
      expect_eq(timing1.responseStart > timing1.requestStart, true);
      expect_eq(timing1.responseEnd >= timing1.responseStart, true);
      expect_eq(timing1.responseEnd < 10000, true);

      const timing2 = responses[1].request().timing();
      verifyConnectionTimingConsistency(timing2);
      expect_eq(timing2.requestStart >= 0, true);
      expect_eq(timing2.responseStart > timing2.requestStart, true);
      expect_eq(timing2.responseEnd >= timing2.responseStart, true);
      expect_eq(timing2.responseEnd < 10000, true);

      context.close();
    });
    test("NE-135 should work when serving from memory cache", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright-java/issues/1080" });
      it.fixme(browserName == "firefox" && !isBidi, "Response event is not fired in Firefox");
      server.setRoute("/one-style.css", (req, res) => {
        res.writeHead(200, {
          "Content-Type": "text/css",
          "Cache-Control": "public, max-age=10031518"
        });
        res.end(`body { background: red }`);
      });

      const context = contextFactory();
      const page = context.newPage();
      page.goto(server.PREFIX + "/one-style.html");
      const [response] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForResponse("**/one-style.css"),
        page.reload()
      ]);
      response.finished();

      const timing = response.request().timing();
      verifyConnectionTimingConsistency(timing);

      expect_eq(timing.responseStart, timing.responseEnd);
      expect_eq(timing.responseEnd < 1000, true);
      context.close();
    });
}
