// Migrated from TypeScript (library/inspector/cli-codegen-pick-locator.spec.ts) via cap from -ts

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
import { roundBox } from "../../config/utils.cp";

// [cap-test] test.describe(() => {

// [cap-test] });

fn main() {
    test("CG-103 should inspect locator", () => {
      const { recorder } = openRecorder();
      recorder.setContentAndWait(`<main><button>Submit</button></main>`);
      recorder.page.click("x-pw-tool-item.pick-locator");
      recorder.page.hover("button");
      recorder.trustedClick();
      recorder.recorderPage.getByRole("tab", { name: "Locator" }).click();
      expect(recorder.recorderPage.locator(".tab-locator .CodeMirror")).toMatchAriaSnapshot(`
        - text: "getByRole('button', { name: 'Submit' })"
      `);
    });
    test("CG-104 should update locator highlight", () => {
      const { recorder } = openRecorder();
      recorder.setContentAndWait(`<main>
        <button>Submit</button>
        <button>Cancel</button>
      </main>`);

      const submitButton = recorder.page.getByRole("button", { name: "Submit" });
      const cancelButton = recorder.page.getByRole("button", { name: "Cancel" });

      recorder.recorderPage.getByRole("button", { name: "Record" }).click();

      recorder.page.click("x-pw-tool-item.pick-locator");
      submitButton.hover();
      recorder.trustedClick();
      recorder.recorderPage.getByRole("tab", { name: "Locator" }).click();
      expect(recorder.recorderPage.locator(".tab-locator .CodeMirror")).toMatchAriaSnapshot(`
        - text: "getByRole('button', { name: 'Submit' })"
      `);

      recorder.recorderPage.locator(".tab-locator .CodeMirror").click();
      let i = 0;
      while (i < `Submit' })`.length) // [cap-migrate] for-step: i++
        recorder.recorderPage.keyboard.press("Backspace");

      {
        // Different button.
        recorder.recorderPage.locator(".tab-locator .CodeMirror").pressSequentially(`Cancel' })`);
        expect(recorder.page.locator("x-pw-highlight")).toBeVisible();
        const box1 = roundBox(cancelButton.boundingBox());
        const box2 = roundBox(recorder.page.locator("x-pw-highlight").boundingBox());
        expect_eq(box1, box2);
      }
    });
}
