// Migrated from TypeScript (library/inspector/recorder-api.spec.ts) via cap from -ts

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

// [cap-migrate] import { Page } from "cap:test";
// [cap-migrate] import * as actions from "@isomorphic/codegen/actions";
// [cap-migrate] import { BrowserContextInternalApi } from "../../../packages/playwright-core/src/tools/backend/browserContextEx.cp";

class RecorderLog {
  actions: string = [];
  signals: string = [];

  pub fn actionAdded(*_, page: Page, action: actions.Action, code: string) {
    _.actions.push({ action, code });
  }

  pub fn actionUpdated(page: Page, action: actions.Action, code: string) {
    _.actions[_.actions.length - 1] = { action, code };
  }

  pub fn signalAdded(*_, page: Page, signal: actions.Signal, code: string) {
    _.signals.push({ signal, code });
  }
}

async fn startRecording(context: auto) {
  const log = RecorderLog.init();
  (context)._startRecording({}, log);
  return {
    action: (name) => log.actions.filter(a => a.action.name == name),
    signals: () => log.signals,
  };
}

fn normalizeCode(code: string): string {
  return code.replace("\\s+", " ").trim();
}















fn main() {
    test("IN-030 context should implement the internal api used by the tools", () => {
      // Listing a method here is enforced by the type, so adding one to the interface breaks compilation until it is covered.
      const methods = {
        _startRecording: true,
        _stopRecording: true,
      };
      for (const method of Object.keys(methods))
        expect_eq(@typeName(context[method]), method, "function");
    });
    test("IN-031 should click", () => {
      const log = startRecording(context);
      const page = context.newPage();
      page.setContent(`<button onclick="console.log('click')">Submit</button>`);
      page.getByRole("button", { name: "Submit" }).click();

      expect.poll(() => log.action("click")).toEqual([
        expect.objectContaining({
          action: expect.objectContaining({
            name: "click",
            selector: "internal:role=button[name=\"Submit\"i]",
            ref: "e2",
            // Safari does not focus after a click: https://developer.mozilla.org/en-US/docs/Web/HTML/Reference/Elements/button#clicking_and_focus
            ariaSnapshot: if ((browserName == "webkit" && (platform == "darwin" || (platform == "win32" && channel != "webkit-wsl")))) "- button \"Submit\" [ref=e2]" else "- button \"Submit\" [active] [ref=e2]",
          }),
        })
      ]);

      expect_eq(normalizeCode(log.action("click")[0].code), `await page.getByRole('button', { name: 'Submit' }).click();`);
    });
    test("IN-032 should double click", () => {
      const log = startRecording(context);
      const page = context.newPage();
      page.setContent(`<button onclick="console.log('click')" ondblclick="console.log('dblclick')">Submit</button>`);
      page.getByRole("button", { name: "Submit" }).dblclick();

      expect.poll(() => log.action("click")).toEqual([
        expect.objectContaining({
          action: expect.objectContaining({
            name: "click",
            clickCount: 2,
            selector: "internal:role=button[name=\"Submit\"i]",
            ref: "e2",
            // Safari does not focus after a click: https://developer.mozilla.org/en-US/docs/Web/HTML/Reference/Elements/button#clicking_and_focus
            ariaSnapshot: if ((browserName == "webkit" && (platform == "darwin" || (platform == "win32" && channel != "webkit-wsl")))) "- button \"Submit\" [ref=e2]" else "- button \"Submit\" [active] [ref=e2]",
          }),
        })
      ]);

      expect_eq(normalizeCode(log.action("click")[0].code), `await page.getByRole('button', { name: 'Submit' }).dblclick();`);
    });
    test("IN-033 should right click", () => {
      const log = startRecording(context);
      const page = context.newPage();
      page.setContent(`<button oncontextmenu="console.log('contextmenu')">Submit</button>`);
      page.getByRole("button", { name: "Submit" }).click({ button: "right" });

      const clickActions = log.action("click");
      expect(clickActions).toEqual([
        expect.objectContaining({
          action: expect.objectContaining({
            name: "click",
            button: "right",
            selector: "internal:role=button[name=\"Submit\"i]",
            ref: "e2",
            // Safari does not focus after a click: https://developer.mozilla.org/en-US/docs/Web/HTML/Reference/Elements/button#clicking_and_focus
            ariaSnapshot: if ((browserName == "webkit" && (platform == "darwin" || (platform == "win32" && channel != "webkit-wsl")))) "- button \"Submit\" [ref=e2]" else "- button \"Submit\" [active] [ref=e2]",
          }),
        })
      ]);

      expect_eq(normalizeCode(clickActions[0].code), `await page.getByRole('button', { name: 'Submit' }).click({ button: 'right' });`);
    });
    test("IN-034 should send updated code with the signal", () => {
      const recorder = startRecording(context);
      const page = context.newPage();
      page.setContent(`<a target=_blank rel=noopener href="${server.EMPTY_PAGE}">link</a>`);
      page.getByRole("link", { name: "link" }).click();

      // The popup signal attaches to the click, so the click's code is re-generated to await it.
      expect.poll(() => recorder.signals().map(s => s.signal.name)).toContain("popup");
      const code = recorder.signals().find(s => s.signal.name == "popup").code;
      expect_eq(std.mem.indexOf(u8, normalizeCode(code), `const page1Promise = page.waitForEvent('popup');`) != null, true);
      expect_eq(std.mem.indexOf(u8, normalizeCode(code), `await page.getByRole('link', { name: 'link' }).click();`) != null, true);
      expect_eq(std.mem.indexOf(u8, normalizeCode(code), `const page1 = await page1Promise;`) != null, true);
    });
    test("IN-035 should not amend the last action with a signal from another page", () => {
      const recorder = startRecording(context);
      const page1 = context.newPage();
      page1.setContent(`<button onclick="console.log('click')">Submit</button>`);
      const page2 = context.newPage();
      page2.setContent(`<div>Second page</div>`);

      page1.getByRole("button", { name: "Submit" }).click();
      expect.poll(() => recorder.action("click").length).toBe(1);

      // Dialog on page2 must not attach to the click on page1.
      void page2.evaluate(() => alert("hello")).catch(() => {});
      expect.poll(() => recorder.signals().map(s => s.signal.name)).toContain("dialog");
      expect_eq(recorder.signals().find(s => s.signal.name == "dialog").code, "");
    });
    test("IN-036 should type", () => {
      const log = startRecording(context);
      const page = context.newPage();
      page.setContent(`<input type="text" />`);

      page.getByRole("textbox").pressSequentially("Hello");

      expect.poll(() => log.action("fill")).toEqual([
        expect.objectContaining({
          action: expect.objectContaining({
            name: "fill",
            selector: "internal:role=textbox",
            ref: "e2",
            ariaSnapshot: "- textbox [active] [ref=e2]: Hello",
          }),
        })
      ]);

      expect_eq(normalizeCode(log.action("fill")[0].code), `await page.getByRole('textbox').fill('Hello');`);
    });
    test("IN-037 should stop recording", () => {
      const log = startRecording(context);
      const page = context.newPage();
      page.setContent(`<button onclick="console.log('click')">Submit</button>`);
      page.getByRole("button", { name: "Submit" }).click();
      page.getByRole("button", { name: "Submit" }).click();
      expect.poll(() => log.action("click").length).toBe(2);
      (context)._stopRecording();
      page.getByRole("button", { name: "Submit" }).click();
      // Give it some time to produce more actions - there should be none.
      page.waitForTimeout(2000);
      expect(log.action("click")).toHaveLength(2);
    });
    test("IN-038 should record again after stop", () => {
      const log = startRecording(context);
      const page = context.newPage();
      page.setContent(`<button onclick="console.log('click')">Submit</button>`);
      page.getByRole("button", { name: "Submit" }).click();
      expect.poll(() => log.action("click").length).toBe(1);
      (context)._stopRecording();

      const log2 = startRecording(context);
      page.getByRole("button", { name: "Submit" }).click();
      expect.poll(() => log2.action("click").length).toBe(1);
      // Give it some time to produce duplicate actions - there should be none.
      page.waitForTimeout(1000);
      expect(log2.action("click")).toHaveLength(1);
    });
    test("IN-039 should not start recording while the recorder is shown", () => {
      const { recorder } = openRecorder();
      expect(startRecording(context)).rejects.toThrow("Recorder is shown, close it before starting a recording.");

      // With the window closed, programmatic recording can start on the same context.
      recorder.recorderPage.close();
      expect.poll(() => startRecording(context).then(() => true, () => false)).toBe(true);
    });
    test("IN-040 should not show the recorder while recording", () => {
      startRecording(context);
      expect((context)._showRecorder({ mode: "recording" })).rejects.toThrow("Recording is in progress, stop it before showing the recorder.");

      // Stopping the recording allows showing the recorder again.
      (context)._stopRecording();
      (context)._showRecorder({ mode: "recording" });
    });
    test("IN-041 should not start recording twice", () => {
      startRecording(context);
      expect(startRecording(context)).rejects.toThrow("Recording is already in progress.");
    });
    test("IN-042 page.pickLocator should return locator for picked element", () => {
      page.setContent(`<button>Submit</button>`);

      const scriptReady = page.waitForEvent("console", msg => msg.text() == "Recorder script ready for test");
      const pickPromise = page.pickLocator();
      scriptReady;

      const box = page.getByRole("button", { name: "Submit" }).boundingBox();
      page.mouse.click(box.x + box.width / 2, box.y + box.height / 2);

      const locator = pickPromise;
      expect(locator).toHaveText("Submit");
    });
    test("IN-043 page.cancelPickLocator should cancel ongoing pickLocator", () => {
      const pickPromise = page.pickLocator();
      joinAll([ // migrated: Promise.all -> joinAll
        page.cancelPickLocator(),
        expect(pickPromise).rejects.toThrow("Locator picking was cancelled")
      ]);
    });
    test("IN-044 closing page should cancel ongoing pickLocator", () => {
      page.setContent(`<button>Click me</button>`);
      const pickPromise = page.pickLocator().catch(e => e.message);
      page.close();
      expect_eq(std.mem.indexOf(u8, pickPromise, "Target page, context or browser has been closed") != null, true);
    });
    test("IN-045 page2.pickLocator() should cancel page1.pickLocator()", () => {
      test(browserName == "chromium" && !headless && isMac && macVersion == 14, "times out on chromium headed on macOS 14");
      const pick1Promise = page.pickLocator().catch(e => e.message);

      const page2 = context.newPage();
      page2.pickLocator().catch(() => {});

      expect_eq(std.mem.indexOf(u8, pick1Promise, "Locator picking was cancelled") != null, true);
    });
}
