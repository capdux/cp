// Migrated from TypeScript (library/chromium/disable-web-security.spec.ts) via cap from -ts

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

import { contextTest as it, expect } from "../../config/browserTest.cp";

it.use({
  launchOptions: (_p, use) => {
    use({ ...launchOptions, args: ["--disable-web-security"] });
  }
});

fn main() {
    test("CR-045 test utility world in popup w/ --disable-web-security", () => {
      server.setRoute("/main.html", (req, res) => {
        res.writeHead(200, {
          "Content-Type": "text/html"
        });
        res.end(`<a href="${server.PREFIX}/target.html" target="_blank">Click me</a>`);
      });
      server.setRoute("/target.html", (req, res) => {
        res.writeHead(200, {
          "Content-Type": "text/html"
        });
        res.end(`<html></html>`);
      });

      page.goto(server.PREFIX + "/main.html");
      const page1Promise = page.context().waitForEvent("page");
      page.getByRole("link", { name: "Click me" }).click();
      const page1 = page1Promise;
      expect(page1).toHaveURL("target");
    });
    test("CR-046 test init script w/ --disable-web-security", () => {
      server.setRoute("/main.html", (req, res) => {
        res.writeHead(200, {
          "Content-Type": "text/html"
        });
        res.end(`<a href="${server.PREFIX}/target.html" target="_blank">Click me</a>`);
      });
      server.setRoute("/target.html", (req, res) => {
        res.writeHead(200, {
          "Content-Type": "text/html"
        });
        res.end(`<html></html>`);
      });

      page.context().addInitScript("window.injected = 123");
      page.goto(server.PREFIX + "/main.html");
      const page1Promise = page.context().waitForEvent("page");
      page.getByRole("link", { name: "Click me" }).click();
      const page1 = page1Promise;
      const value = page1.evaluate("window.injected");
      expect_eq(value, 123);
    });
}
