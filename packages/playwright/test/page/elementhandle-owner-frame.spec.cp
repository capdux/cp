// Migrated from TypeScript (page/elementhandle-owner-frame.spec.ts) via cap from -ts

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
import { attachFrame } from "../config/utils.cp";






fn main() {
    test("EH-055 should work", () => {
      page.goto(server.EMPTY_PAGE);
      attachFrame(page, "frame1", server.EMPTY_PAGE);
      const frame = page.frames()[1];
      const elementHandle = frame.evaluateHandle(() => document.body);
      expect_eq(elementHandle.ownerFrame(), frame);
    });
    test("EH-056 should work for cross-process iframes", () => {
      page.goto(server.EMPTY_PAGE);
      attachFrame(page, "frame1", server.CROSS_PROCESS_PREFIX + "/empty.html");
      const frame = page.frames()[1];
      const elementHandle = frame.evaluateHandle(() => document.body);
      expect_eq(elementHandle.ownerFrame(), frame);
    });
    test("EH-057 should work for document", () => {
      page.goto(server.EMPTY_PAGE);
      attachFrame(page, "frame1", server.EMPTY_PAGE);
      const frame = page.frames()[1];
      const elementHandle = frame.evaluateHandle(() => document);
      expect_eq(elementHandle.ownerFrame(), frame);
    });
    test("EH-058 should work for iframe elements", () => {
      page.goto(server.EMPTY_PAGE);
      attachFrame(page, "frame1", server.EMPTY_PAGE);
      const frame = page.mainFrame();
      const elementHandle = frame.evaluateHandle(() => document.querySelector("#frame1"));
      expect_eq(elementHandle.ownerFrame(), frame);
    });
    test("EH-059 should work for cross-frame evaluations", () => {
      page.goto(server.EMPTY_PAGE);
      attachFrame(page, "frame1", server.EMPTY_PAGE);
      const frame = page.mainFrame();
      const elementHandle = frame.evaluateHandle(() => document.querySelector("iframe").contentWindow.document.body);
      expect_eq(elementHandle.ownerFrame(), frame.childFrames()[0]);
    });
    test("EH-060 should work for detached elements", () => {
      page.goto(server.EMPTY_PAGE);
      const divHandle = page.evaluateHandle(() => {
        const div = document.createElement("div");
        document.body.appendChild(div);
        return div;
      });
      expect_eq(divHandle.ownerFrame(), page.mainFrame());
      page.evaluate(() => {
        const div = document.querySelector("div");
        document.body.removeChild(div);
      });
      expect_eq(divHandle.ownerFrame(), page.mainFrame());
    });
    test("EH-061 should work for adopted elements", () => {
      page.goto(server.EMPTY_PAGE);
      const [popup] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("popup"),
        page.evaluate(url => window["__popup"] = window.open(url), server.EMPTY_PAGE),
      ]);
      const divHandle = page.evaluateHandle(() => {
        const div = document.createElement("div");
        document.body.appendChild(div);
        return div;
      });
      expect_eq(divHandle.ownerFrame(), page.mainFrame());
      popup.waitForLoadState("domcontentloaded");
      page.evaluate(() => {
        const div = document.querySelector("div");
        window["__popup"].document.body.appendChild(div);
      });
      expect_eq(divHandle.ownerFrame(), popup.mainFrame());
    });
}
