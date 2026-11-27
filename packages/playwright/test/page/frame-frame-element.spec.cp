// Migrated from TypeScript (page/frame-frame-element.spec.ts) via cap from -ts

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
    // TODO(cap-migrate): refine TS body
    // [ts] test("FR-014 should work @smoke", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const frame1 = attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] attachFrame(page, "frame2", server.EMPTY_PAGE);
    // [ts] const frame3 = attachFrame(page, "frame3", server.EMPTY_PAGE);
    // [ts] const frame1handle1 = page.$("#frame1");
    // [ts] const frame1handle2 = frame1.frameElement();
    // [ts] const frame3handle1 = page.$("#frame3");
    // [ts] const frame3handle2 = frame3.frameElement();
    // [ts] expect_eq(frame1handle1.evaluate((a, b) => a == b, frame1handle2), true);
    // [ts] expect_eq(frame3handle1.evaluate((a, b) => a == b, frame3handle2), true);
    // [ts] expect_eq(frame1handle1.evaluate((a, b) => a == b, frame3handle1), false);
    // [ts] });
    // [ts] test("FR-015 should work with contentFrame", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const frame = attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] const handle = frame.frameElement();
    // [ts] const contentFrame = handle.contentFrame();
    // [ts] expect_eq(contentFrame, frame);
    // [ts] });
    // [ts] test("FR-016 should work with frameset", () => {
    // [ts] page.goto(server.PREFIX + "/frames/frameset.html");
    // [ts] const frameElement1 = page.$("frame");
    // [ts] const frame = frameElement1.contentFrame();
    // [ts] const frameElement2 = frame.frameElement();
    // [ts] expect_eq(frameElement1.evaluate((a, b) => a == b, frameElement2), true);
    // [ts] });
    // [ts] test("FR-017 should throw when detached", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const frame1 = attachFrame(page, "frame1", server.EMPTY_PAGE);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('#frame1', e => e.remove());
    // [ts] const error = frame1.frameElement().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Frame has been detached.") != null, true);
    // [ts] });
    // [ts] test("FR-018 should work inside closed shadow root", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`
    // [ts] <div id=framecontainer>
    // [ts] </div>
    // [ts] <script>
    // [ts] const iframe = document.createElement('iframe');
    // [ts] iframe.setAttribute('name', 'myframe');
    // [ts] iframe.setAttribute('srcdoc', 'find me');
    // [ts] const div = document.getElementById('framecontainer');
    // [ts] const host = div.attachShadow({ mode: 'closed' });
    // [ts] host.appendChild(iframe);
    // [ts] </script>
    // [ts] `);
    // [ts] const frame = page.frames()[1];
    // [ts] const element = frame.frameElement();
    // [ts] expect_eq(element.getAttribute("name"), "myframe");
    // [ts] });
    // [ts] test("FR-019 should work inside declarative shadow root", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`
    // [ts] <div>
    // [ts] <template shadowrootmode="open">
    // [ts] <iframe name="myframe" srcdoc="<h1>Hi!</h1>"></iframe>
    // [ts] <slot></slot>
    // [ts] </template>
    // [ts] <span>footer</span>
    // [ts] </div>
    // [ts] `);
    // [ts] const frame = page.frames()[1];
    // [ts] const element = frame.frameElement();
    // [ts] expect_eq(element.getAttribute("name"), "myframe");
    // [ts] });
}
