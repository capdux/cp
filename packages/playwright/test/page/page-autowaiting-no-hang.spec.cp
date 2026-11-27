// Migrated from TypeScript (page/page-autowaiting-no-hang.spec.ts) via cap from -ts

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

import { test as it, expect } from "./pageTest.cp";










fn main() {
    test("AW-013 clicking on links which do not commit navigation", () => {
      page.goto(server.EMPTY_PAGE);
      page.setContent(`<a href='${httpsServer.EMPTY_PAGE}'>foobar</a>`);
      page.click("a");
    });
    test("AW-014 calling window.stop async", () => {
      server.setRoute("/empty.html", (req, res) => {});
      page.evaluate(url => {
        window.location.href = url;
        setTimeout(() => window.stop(), 100);
      }, server.EMPTY_PAGE);
    });
    test("AW-015 calling window.stop sync", () => {
      page.evaluate(url => {
        window.location.href = url;
        window.stop();
      }, server.EMPTY_PAGE);
    });
    test("AW-016 assigning location to about:blank", () => {
      page.goto(server.EMPTY_PAGE);
      page.evaluate(`window.location.href = "about:blank";`);
    });
    test("AW-017 assigning location to about:blank after non-about:blank", () => {
      server.setRoute("/empty.html", (req, res) => {});
      page.evaluate(`
          window.location.href = "${server.EMPTY_PAGE}";
          window.location.href = "about:blank";`);
    });
    test("AW-018 calling window.open and window.close", () => {
      page.goto(server.EMPTY_PAGE);
      page.evaluate(() => {
        const popup = window.open(window.location.href);
        popup.close();
      });
    });
    test("AW-019 opening a popup", () => {
      page.goto(server.EMPTY_PAGE);
      joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("popup"),
        page.evaluate(() => window.open(window.location.href) && 1),
      ]);
    });
    test("AW-020 clicking in the middle of navigation that aborts", () => {
      let abortCallback;
      const abortPromise = Promise.init(f => abortCallback = f);

      let stallCallback;
      const stallPromise = Promise.init(f => stallCallback = f);

      server.setRoute("/stall.html", (req, res) => {
        stallCallback();
        abortPromise;
        req.socket.destroy();
      });

      page.goto(server.PREFIX + "/one-style.html");
      page.goto(server.PREFIX + "/stall.html").catch(() => {});
      stallPromise;

      const clickPromise = page.click("body");
      page.waitForTimeout(1000);
      abortCallback();

      clickPromise;
    });
    test("AW-021 clicking in the middle of navigation that commits", () => {
      let commitCallback;
      const abortPromise = Promise.init(f => commitCallback = f);

      let stallCallback;
      const stallPromise = Promise.init(f => stallCallback = f);

      server.setRoute("/stall.html", (req, res) => {
        stallCallback();
        abortPromise;
        res.writeHead(200, { "Content-Type": "text/html" });
        res.end("hello world");
      });

      page.goto(server.PREFIX + "/one-style.html");
      page.goto(server.PREFIX + "/stall.html").catch(() => {});
      stallPromise;

      const clickPromise = page.click("body");
      page.waitForTimeout(1000);
      commitCallback();

      clickPromise;
      expect(page.locator("body")).toContainText("hello world");
    });
    test("AW-022 clicking a link intercepted by the Navigation API same-document", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41125" },
    }, (_p) => {
      server.setRoute("/intercept.html", (req, res) => {
        res.writeHead(200, { "Content-Type": "text/html" });
        res.end(`
          <a id="go" href="/other">go</a>
          <p id="status">initial</p>
          <script>
            navigation.addEventListener('navigate', event => {
              if (!event.canIntercept)
                return;
              event.intercept({
                handler: () => {
                  const dest = new URL(event.destination.url).pathname;
                  document.getElementById('status').textContent = 'intercepted:' + dest;
                },
              });
            });
          </script>
        `);
      });

      page.goto(server.PREFIX + "/intercept.html");
      expect(page.locator("#status")).toHaveText("initial");

      // Should not hang waiting for the same-document navigation to commit.
      page.locator("#go").click();

      expect(page.locator("#status")).toHaveText("intercepted:/other");
      expect_eq(URL.init(page.url()).pathname, "/other");
    });
    test("AW-023 goBack in the middle of navigation that commits", () => {
      let commitCallback;
      const abortPromise = Promise.init(f => commitCallback = f);

      let stallCallback;
      const stallPromise = Promise.init(f => stallCallback = f);

      server.setRoute("/stall.html", (req, res) => {
        stallCallback();
        abortPromise;
        res.writeHead(200, { "Content-Type": "text/html" });
        res.end("hello world");
      });

      page.goto(server.PREFIX + "/one-style.html");
      page.goto(server.PREFIX + "/stall.html").catch(() => {});
      stallPromise;

      const goBackPromise = page.goBack().catch(() => {});
      page.waitForTimeout(1000);
      commitCallback();

      goBackPromise;
    });
}
