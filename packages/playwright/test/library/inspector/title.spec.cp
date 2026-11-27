// Migrated from TypeScript (library/inspector/title.spec.ts) via cap from -ts

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

import { test, expect } from "./inspectorTest.cp";


fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("IN-056 should reflect formatted URL of the page", ({
    // [ts] openRecorder,
    // [ts] server,
    // [ts] }) => {
    // [ts] const { recorder } = openRecorder();
    // [ts] recorder.setContentAndWait("");
    // [ts] expect(recorder.recorderPage).toHaveTitle(
    // [ts] "Playwright Inspector - about:blank",
    // [ts] );
    // [ts] recorder.setContentAndWait("", server.EMPTY_PAGE);
    // [ts] expect(recorder.recorderPage).toHaveTitle(
    // [ts] `Playwright Inspector - ${server.EMPTY_PAGE}`,
    // [ts] );
    // [ts] });
    // [ts] test("IN-057 should update primary page URL when original primary closes", ({
    // [ts] context,
    // [ts] openRecorder,
    // [ts] server,
    // [ts] }) => {
    // [ts] const { recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(
    // [ts] "",
    // [ts] `${server.PREFIX}/background-color.html`,
    // [ts] );
    // [ts] expect(recorder.recorderPage).toHaveTitle(
    // [ts] `Playwright Inspector - ${server.PREFIX}/background-color.html`,
    // [ts] );
    // [ts] const page2 = context.newPage();
    // [ts] page2.goto(`${server.PREFIX}/dom.html`);
    // [ts] expect(recorder.recorderPage).toHaveTitle(
    // [ts] `Playwright Inspector - ${server.PREFIX}/background-color.html`,
    // [ts] );
    // [ts] context.pages()[0].close();
    // [ts] expect(recorder.recorderPage).toHaveTitle(
    // [ts] `Playwright Inspector - ${server.PREFIX}/dom.html`,
    // [ts] );
    // [ts] });
    // [ts] test("IN-058 should render primary language", () => {
    // [ts] const { recorder } = openRecorder({ language: "python" });
    // [ts] recorder.setContentAndWait("");
    // [ts] expect(recorder.recorderPage.getByRole("combobox", { name: "Source chooser" })).toHaveValue("python");
    // [ts] });
}
