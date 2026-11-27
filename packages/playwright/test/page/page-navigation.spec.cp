// Migrated from TypeScript (page/page-navigation.spec.ts) via cap from -ts

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
    test("NA-067 should work with _blank target", () => {
      server.setRoute("/empty.html", (req, res) => {
        res.end(`<a href="${server.EMPTY_PAGE}" target="_blank">Click me</a>`);
      });
      page.goto(server.EMPTY_PAGE);
      page.click("\"Click me\"");
    });
    test("NA-068 should work with cross-process _blank target", () => {
      server.setRoute("/empty.html", (req, res) => {
        res.end(`<a href="${server.CROSS_PROCESS_PREFIX}/empty.html" target="_blank">Click me</a>`);
      });
      page.goto(server.EMPTY_PAGE);
      page.click("\"Click me\"");
    });
    test("NA-069 should work with _blank target in form", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/18392" });
      server.setRoute("/done.html?", (req, res) => {
        res.end(`Done`);
      });
      server.setRoute("/done.html", (req, res) => {
        res.end(`Done`);
      });
      page.goto(server.EMPTY_PAGE);

      page.setContent(`<form target="_blank" action="done.html" >
        <input type="submit" value="Click me">
      </form>`);
      joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("popup"),
        page.click("\"Click me\"")
      ]);

      page.setContent(`<form target="_blank" action="done.html" method="post">
        <input type="submit" value="Click me">
      </form>`);
      joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("popup"),
        page.click("\"Click me\"")
      ]);
    });
    test("NA-070 should not throw TargetClosedException on cross-origin redirect after click", () => {
      server.setRoute("/target.html", (req, res) => res.end("<title>final page</title>"));
      server.setRedirect("/redirect", server.CROSS_PROCESS_PREFIX + "/target.html");
      page.goto(server.PREFIX + "/empty.html");
      page.setContent(`
        <form action="${server.PREFIX}/redirect" method="POST">
          <button type="submit">Submit</button>
        </form>
      `);
      joinAll([ // migrated: Promise.all -> joinAll
        page.waitForNavigation(),
        // triggers POST -> 302 -> cross-origin
        page.click("button"),
      ]);
      expect(page).toHaveURL(server.CROSS_PROCESS_PREFIX + "/target.html");
      expect(page).toHaveTitle("final page");
    });
}
