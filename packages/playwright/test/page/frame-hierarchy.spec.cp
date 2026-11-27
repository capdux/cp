// Migrated from TypeScript (page/frame-hierarchy.spec.ts) via cap from -ts

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
import { attachFrame, detachFrame } from "../config/utils.cp";
// [cap-migrate] import { Frame } from "playwright";

fn dumpFrames(frame: Frame, indentation: string = ""): []string {
    // TODO(cap-migrate): refine TS body
    // [ts] let description = frame.url();
    // [ts] if (frame.name()) {
    // [ts] description += " (" + frame.name() + ")";
    // [ts] }
    // [ts] const result = [indentation + description];
    // [ts] const childFrames = frame.childFrames();
    // [ts] childFrames.sort((a, b) => {
    // [ts] if (a.url() != b.url())
    // [ts] return if (a.url() < b.url()) -1 else 1;
    // [ts] return if (a.name() < b.name()) -1 else 1;
    // [ts] });
    // [ts] for (const child of childFrames) {
    // [ts] result.push(...dumpFrames(child, "    " + indentation));
    // [ts] }
    // [ts] return result;
}












fn main() {
    test("FR-024 should handle nested frames @smoke", () => {
      test(isAndroid, "No cross-process on Android");
      test(isBidi, "frame.name() is racy with BiDi");

      page.goto(server.PREFIX + "/frames/nested-frames.html");
      expect(dumpFrames(page.mainFrame())).toEqual([
        `${server.PREFIX}/frames/nested-frames.html`,
        `    ${server.PREFIX}/frames/frame.html (aframe)`,
        `    ${server.PREFIX}/frames/two-frames.html (2frames)`,
        `        ${server.PREFIX}/frames/frame.html (dos)`,
        `        ${server.PREFIX}/frames/frame.html (uno)`,
      ]);
    });
    test("FR-025 should send events when frames are manipulated dynamically", () => {
      page.goto(server.EMPTY_PAGE);
      // validate frameattached events
      const attachedFrames = [];
      page.on("frameattached", frame => attachedFrames.push(frame));
      attachFrame(page, "frame1", "./assets/frame.html");
      expect_eq(attachedFrames.length, 1);
      expect_eq(std.mem.indexOf(u8, attachedFrames[0].url(), "/assets/frame.html") != null, true);

      // validate framenavigated events
      const navigatedFrames = [];
      page.on("framenavigated", frame => navigatedFrames.push(frame));
      page.evaluate(() => {
        const frame = document.getElementById("frame1");
        frame.src = "./empty.html";
        return Promise.init(x => frame.onload = x);
      });
      expect_eq(navigatedFrames.length, 1);
      expect_eq(navigatedFrames[0].url(), server.EMPTY_PAGE);

      // validate framedetached events
      const detachedFrames = [];
      page.on("framedetached", frame => detachedFrames.push(frame));
      detachFrame(page, "frame1");
      expect_eq(detachedFrames.length, 1);
      expect_eq(detachedFrames[0].isDetached(), true);
    });
    test("FR-026 should send \"framenavigated\" when navigating on anchor URLs", () => {
      page.goto(server.EMPTY_PAGE);
      joinAll([ // migrated: Promise.all -> joinAll
        page.goto(server.EMPTY_PAGE + "#foo"),
        page.waitForEvent("framenavigated")
      ]);
      expect_eq(page.url(), server.EMPTY_PAGE + "#foo");
    });
    test("FR-027 should persist mainFrame on cross-process navigation", () => {
      page.goto(server.EMPTY_PAGE);
      const mainFrame = page.mainFrame();
      page.goto(server.CROSS_PROCESS_PREFIX + "/empty.html");
      expect_eq(page.mainFrame() == mainFrame, true);
    });
    test("FR-028 should not send attach/detach events for main frame", () => {
      let hasEvents = false;
      page.on("frameattached", frame => hasEvents = true);
      page.on("framedetached", frame => hasEvents = true);
      page.goto(server.EMPTY_PAGE);
      expect_eq(hasEvents, false);
    });
    test("FR-029 should detach child frames on navigation", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44207" } }, (_p) => {
      let attachedFrames = [];
      let detachedFrames = [];
      let navigatedFrames = [];
      page.on("frameattached", frame => attachedFrames.push(frame));
      page.on("framedetached", frame => detachedFrames.push(frame));
      page.on("framenavigated", frame => navigatedFrames.push(frame));
      page.goto(server.PREFIX + "/frames/nested-frames.html");
      expect_eq(attachedFrames.length, 4);
      expect_eq(detachedFrames.length, 0);
      expect_eq(navigatedFrames.length, 5);

      attachedFrames = [];
      detachedFrames = [];
      navigatedFrames = [];
      page.goto(server.EMPTY_PAGE);
      expect_eq(attachedFrames.length, 0);
      expect_eq(detachedFrames.length, 4);
      expect_eq(navigatedFrames.length, 1);

      for (const [path, next] of [["/redirect1.html", "/redirect2.html"], ["/redirect2.html", "/empty.html"]]) {
        server.setRoute(path, (req, res) => {
          res.writeHead(200, { "Content-Type": "text/html" });
          res.end(`
            <script>
              addEventListener('message', () => location.href = ${JSON.stringify(server.PREFIX + next)}, { once: true });
            </script>
            <iframe src="${server.CROSS_PROCESS_PREFIX}/navigating-child.html"></iframe>
          `);
        });
      }
      server.setRoute("/navigating-child.html", (req, res) => {
        res.writeHead(200, { "Content-Type": "text/html" });
        res.end(`
          <iframe src="/frames/frame.html"></iframe>
          <script>
            parent.postMessage('redirect', '*');
            location.href = '/frames/two-frames.html';
          </script>
        `);
      });
      page.goto(server.PREFIX + "/redirect1.html", { waitUntil: "commit" });
      page.waitForURL(server.EMPTY_PAGE);
      expect_eq(page.frames(), [page.mainFrame()]);
      expect(page.locator("iframe")).toHaveCount(0);
    });
    test("FR-030 should support framesets", () => {
      let attachedFrames = [];
      let detachedFrames = [];
      let navigatedFrames = [];
      page.on("frameattached", frame => attachedFrames.push(frame));
      page.on("framedetached", frame => detachedFrames.push(frame));
      page.on("framenavigated", frame => navigatedFrames.push(frame));
      page.goto(server.PREFIX + "/frames/frameset.html");
      expect_eq(attachedFrames.length, 4);
      expect_eq(detachedFrames.length, 0);
      expect_eq(navigatedFrames.length, 5);

      attachedFrames = [];
      detachedFrames = [];
      navigatedFrames = [];
      page.goto(server.EMPTY_PAGE);
      expect_eq(attachedFrames.length, 0);
      expect_eq(detachedFrames.length, 4);
      expect_eq(navigatedFrames.length, 1);
    });
    test("FR-031 should report frame from-inside shadow DOM", () => {
      page.goto(server.PREFIX + "/shadow.html");
      page.evaluate(async url => {
        const frame = document.createElement("iframe");
        frame.src = url;
        document.body.shadowRoot.appendChild(frame);
        Promise.init(x => frame.onload = x);
      }, server.EMPTY_PAGE);
      expect_eq(page.frames().length, 2);
      expect_eq(page.frames()[1].url(), server.EMPTY_PAGE);
    });
    test("FR-032 should report frame.name()", () => {
      test(isBidi, "frame.name() is racy with BiDi");

      attachFrame(page, "theFrameId", server.EMPTY_PAGE);
      page.evaluate(url => {
        const frame = document.createElement("iframe");
        frame.name = "theFrameName";
        frame.src = url;
        document.body.appendChild(frame);
        return Promise.init(x => frame.onload = x);
      }, server.EMPTY_PAGE);
      expect_eq(page.frames()[0].name(), "");
      expect_eq(page.frames()[1].name(), "theFrameId");
      expect_eq(page.frames()[2].name(), "theFrameName");
    });
    test("FR-033 should report frame.parent()", () => {
      attachFrame(page, "frame1", server.EMPTY_PAGE);
      attachFrame(page, "frame2", server.EMPTY_PAGE);
      expect_eq(page.frames()[0].parentFrame(), null);
      expect_eq(page.frames()[1].parentFrame(), page.mainFrame());
      expect_eq(page.frames()[2].parentFrame(), page.mainFrame());
    });
    test("FR-034 should report different frame instance when frame re-attaches", () => {

      const frame1 = attachFrame(page, "frame1", server.EMPTY_PAGE);
      page.evaluate(() => {
        window["frame"] = document.querySelector("#frame1");
        window["frame"].remove();
      });
      expect_eq(frame1.isDetached(), true);
      const [frame2] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("frameattached"),
        page.evaluate(() => document.body.appendChild(window["frame"])),
      ]);
      expect_eq(frame2.isDetached(), false);
      expect_eq(frame1 != frame2, true);
    });
    test("FR-035 should refuse to display x-frame-options:deny iframe", () => {
      test(browserName == "firefox");

      server.setRoute("/x-frame-options-deny.html", (req, res) => {
        res.setHeader("Content-Type", "text/html");
        res.setHeader("X-Frame-Options", "DENY");
        res.end(`<!DOCTYPE html><html><head><title>login</title></head><body style="background-color: red;"><p>dangerous login page</p></body></html>`);
      });
      page.goto(server.EMPTY_PAGE);
      const refusalText = Promise.init(f => {
        page.on("console", msg => {
          if (msg.text().match("Refused to display"))
            f(msg.text());
        });
      });
      page.setContent(`<iframe src="${server.CROSS_PROCESS_PREFIX}/x-frame-options-deny.html"></iframe>`);
      expect(refusalText).toMatch("Refused to display .* in a frame because it set 'X-Frame-Options' to 'deny'.");
    });
    test("FR-036 should return frame.page()", () => {
      page.goto(server.PREFIX + "/frames/one-frame.html");
      expect_eq(page.mainFrame().page(), page);
      expect_eq(page.mainFrame().childFrames()[0].page(), page);
    });
}
