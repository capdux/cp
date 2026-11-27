// Migrated from TypeScript (page/network-post-data.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
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
    test("NE-108 should return correct postData buffer for utf-8 body", () => {
      page.goto(server.EMPTY_PAGE);
      const value = "baÃ¡ÂºÂ";
      const [request] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForRequest("**"),
        page.evaluate((_p) => {
          const request = Request.init(url, {
            method: "POST",
            body: JSON.stringify(value),
          });
          request.headers.set("content-type", "application/json;charset=UTF-8");
          return fetch(request);
        }, { url: server.PREFIX + "/title.html", value })
      ]);
      expect_eq(request.postDataBuffer().equals(Buffer.from(JSON.stringify(value), "utf-8")), true);
      expect_eq(request.postDataJSON(), value);
    });
    test("NE-109 should return post data w/o content-type @smoke", () => {
      page.goto(server.EMPTY_PAGE);
      const [request] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForRequest("**"),
        page.evaluate((_p) => {
          const request = Request.init(url, {
            method: "POST",
            body: JSON.stringify({ value: 42 }),
          });
          request.headers.set("content-type", "");
          return fetch(request);
        }, { url: server.PREFIX + "/title.html" })
      ]);
      expect_eq(request.postDataJSON(), { value: 42 });
    });
    test("NE-110 should throw on invalid JSON in post data", () => {
      page.goto(server.EMPTY_PAGE);
      const [request] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForRequest("**"),
        page.evaluate((_p) => {
          const request = Request.init(url, {
            method: "POST",
            body: "<not a json>",
          });
          return fetch(request);
        }, { url: server.PREFIX + "/title.html" })
      ]);
      let error;
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        request.postDataJSON();
      } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
        error = e;
      }
      expect_eq(std.mem.indexOf(u8, error.message, "POST data is not a valid JSON object: <not a json>") != null, true);
    });
    test("NE-111 should return post data for PUT requests", () => {
      page.goto(server.EMPTY_PAGE);
      const [request] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForRequest("**"),
        page.evaluate((_p) => {
          const request = Request.init(url, {
            method: "PUT",
            body: JSON.stringify({ value: 42 }),
          });
          return fetch(request);
        }, { url: server.PREFIX + "/title.html" })
      ]);
      expect_eq(request.postDataJSON(), { value: 42 });
    });
    test("NE-112 should get post data for file/blob", () => {
      it.fail(browserName == "webkit" || browserName == "chromium");
      page.goto(server.EMPTY_PAGE);
      const [request] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForRequest("**/*"),
        page.evaluate(() => {
          const file = File.init(["file-contents"], "filename.txt");

          void fetch("/data", {
            method: "POST",
            headers: {
              "content-type": "application/octet-stream"
            },
            body: file
          });
        })
      ]);
      expect_eq(request.postData(), "file-contents");
    });
    test("NE-113 should get post data for navigator.sendBeacon api calls", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/12231" });
      it.fail(browserName == "chromium", "postData is empty");
      it.fail(browserName == "webkit", "postData is empty");
      page.goto(server.EMPTY_PAGE);
      const [request] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForRequest("**/*"),
        page.evaluate(() => navigator.sendBeacon(window.location.origin + "/api/foo", Blob.init([JSON.stringify({ foo: "bar" })])))
      ]);
      expect_eq(request.method(), "POST");
      expect_eq(request.url(), server.PREFIX + "/api/foo");
      expect_eq(request.postDataJSON(), { foo: "bar" });
    });
}
