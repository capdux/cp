// Migrated from TypeScript (page/elementhandle-content-frame.spec.ts) via cap from -ts

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
    // [ts] test("EH-019 should work", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] const elementHandle = page.$("#frame1");
    // [ts] const frame = elementHandle.contentFrame();
    // [ts] expect_eq(frame, page.frames()[1]);
    // [ts] });
    // [ts] test("EH-020 should work for cross-process iframes", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] attachFrame(page, "frame1", server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] const elementHandle = page.$("#frame1");
    // [ts] const frame = elementHandle.contentFrame();
    // [ts] expect_eq(frame, page.frames()[1]);
    // [ts] });
    // [ts] test("EH-021 should work for cross-frame evaluations", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] const frame = page.frames()[1];
    // [ts] const elementHandle = frame.evaluateHandle(() => window.top.document.querySelector("#frame1"));
    // [ts] expect_eq(elementHandle.contentFrame(), frame);
    // [ts] });
    // [ts] test("EH-022 should return null for non-iframes", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] const frame = page.frames()[1];
    // [ts] const elementHandle = frame.evaluateHandle(() => document.body);
    // [ts] expect_eq(elementHandle.contentFrame(), null);
    // [ts] });
    // [ts] test("EH-023 should return null for document.documentElement", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] const frame = page.frames()[1];
    // [ts] const elementHandle = frame.evaluateHandle(() => document.documentElement);
    // [ts] expect_eq(elementHandle.contentFrame(), null);
    // [ts] });
}
