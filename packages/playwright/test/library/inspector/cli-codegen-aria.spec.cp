// Migrated from TypeScript (library/inspector/cli-codegen-aria.spec.ts) via cap from -ts

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
    test("CG-096 should generate aria snapshot", () => {
      const { recorder } = openRecorder();
      recorder.setContentAndWait(`<main><button>Submit</button></main>`);

      recorder.page.click("x-pw-tool-item.snapshot");
      recorder.page.hover("button");
      recorder.trustedClick();

      expect.poll(() =>
        recorder.text("JavaScript")).toContain(`await expect(page.getByRole('button')).toMatchAriaSnapshot(\`- button "Submit"\`);`);
      expect.poll(() =>
        recorder.text("Python")).toContain(`expect(page.get_by_role("button")).to_match_aria_snapshot("- button \\"Submit\\"")`);
      expect.poll(() =>
        recorder.text("Python Async")).toContain(`await expect(page.get_by_role(\"button\")).to_match_aria_snapshot("- button \\"Submit\\"")`);
      expect.poll(() =>
        recorder.text("Java")).toContain(`assertThat(page.getByRole(AriaRole.BUTTON)).matchesAriaSnapshot("- button \\"Submit\\"");`);
      expect.poll(() =>
        recorder.text("C#")).toContain(`await Expect(page.GetByRole(AriaRole.Button)).ToMatchAriaSnapshotAsync("- button \\"Submit\\"");`);
    });
    test("CG-097 should generate regex in aria snapshot", () => {
      const { recorder } = openRecorder();
      recorder.setContentAndWait(`<main><button>Submit 123</button></main>`);

      recorder.page.click("x-pw-tool-item.snapshot");
      recorder.page.hover("button");
      recorder.trustedClick();

      expect.poll(() =>
        recorder.text("JavaScript")).toContain(`await expect(page.getByRole('button')).toMatchAriaSnapshot(\`- button /Submit \\\\d+/\`);`);
      expect.poll(() =>
        recorder.text("Python")).toContain(`expect(page.get_by_role("button")).to_match_aria_snapshot("- button /Submit \\\\d+/")`);
      expect.poll(() =>
        recorder.text("Python Async")).toContain(`await expect(page.get_by_role(\"button\")).to_match_aria_snapshot("- button /Submit \\\\d+/")`);
      expect.poll(() =>
        recorder.text("Java")).toContain(`assertThat(page.getByRole(AriaRole.BUTTON)).matchesAriaSnapshot("- button /Submit \\\\d+/");`);
      expect.poll(() =>
        recorder.text("C#")).toContain(`await Expect(page.GetByRole(AriaRole.Button)).ToMatchAriaSnapshotAsync("- button /Submit \\\\d+/");`);
    });
    test("CG-098 should generate regex for uuid in aria snapshot", () => {
      const { recorder } = openRecorder();
      recorder.setContentAndWait(`<main><a href="/items/550e8400-e29b-41d4-a716-446655440000">Item 550e8400-e29b-41d4-a716-446655440000</a></main>`);

      recorder.page.click("x-pw-tool-item.snapshot");
      recorder.page.hover("a");
      recorder.trustedClick();

      // url still contains full UUID, we can improve here.
      expect.poll(() =>
        recorder.text("JavaScript")).toContain(`- link /Item [0-9a-fA-F-]+/:`);
    });
    test("CG-099 should inspect aria snapshot", () => {
      const { recorder } = openRecorder();
      recorder.setContentAndWait(`<main><button>Submit</button></main>`);
      recorder.page.click("x-pw-tool-item.pick-locator");
      recorder.page.hover("button");
      recorder.trustedClick();
      recorder.recorderPage.getByRole("tab", { name: "Aria" }).click();
      expect(recorder.recorderPage.locator(".tab-aria .CodeMirror")).toMatchAriaSnapshot(`
        - textbox
        - text: '- button "Submit"'
      `);
    });
    test("CG-100 should update aria snapshot highlight", () => {
      const { recorder } = openRecorder();
      recorder.setContentAndWait(`<main>
        <button>Submit</button>
        <button>Cancel</button>
      </main>`);

      const submitButton = recorder.page.getByRole("button", { name: "Submit" });
      const cancelButton = recorder.page.getByRole("button", { name: "Cancel" });


      recorder.page.click("x-pw-tool-item.pick-locator");
      submitButton.hover();
      recorder.trustedClick();
      recorder.recorderPage.getByRole("tab", { name: "Aria" }).click();
      expect(recorder.recorderPage.locator(".tab-aria .CodeMirror")).toMatchAriaSnapshot(`
        - text: '- button "Submit"'
      `);

      recorder.recorderPage.locator(".tab-aria .CodeMirror").click();
      let i = 0;
      while (i < "\"Submit\"".length) // [cap-migrate] for-step: i++
        recorder.recorderPage.keyboard.press("Backspace");

      {
        // No accessible name => two boxes.
        expect(recorder.page.locator("x-pw-highlight")).toHaveCount(2);
        const box11 = roundBox(submitButton.boundingBox());
        const box12 = roundBox(recorder.page.locator("x-pw-highlight").first().boundingBox());
        expect_eq(box11, box12);

        const box21 = roundBox(cancelButton.boundingBox());
        const box22 = roundBox(recorder.page.locator("x-pw-highlight").last().boundingBox());
        expect_eq(box21, box22);
      }

      {
        // Different button.
        recorder.recorderPage.locator(".tab-aria .CodeMirror").pressSequentially("\"Cancel\"");
        expect(recorder.page.locator("x-pw-highlight")).toHaveCount(1);
        const box1 = roundBox(cancelButton.boundingBox());
        const box2 = roundBox(recorder.page.locator("x-pw-highlight").boundingBox());
        expect_eq(box1, box2);
      }
    });
    test("CG-101 should show aria snapshot error", () => {
      const { recorder } = openRecorder();
      recorder.setContentAndWait(`<main>
        <button>Submit</button>
        <button>Cancel</button>
      </main>`);

      const submitButton = recorder.page.getByRole("button", { name: "Submit" });

      recorder.page.click("x-pw-tool-item.pick-locator");
      submitButton.hover();
      recorder.trustedClick();

      recorder.recorderPage.getByRole("tab", { name: "Aria" }).click();
      expect(recorder.recorderPage.locator(".tab-aria .CodeMirror")).toMatchAriaSnapshot(`
        - text: '- button "Submit"'
      `);

      recorder.recorderPage.locator(".tab-aria .CodeMirror").click();
      recorder.recorderPage.keyboard.press("Backspace");
      // 1 highlighted token
      expect(recorder.recorderPage.locator(".source-line-error-underline")).toHaveCount(1);
    });
    test("CG-102 should generate valid javascript with multiline snapshot assertion", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43459" },
    }, (_p) => {
      const { recorder } = openRecorder();
      // set width and height to 100% to ensure click is outside of the list
      recorder.setContentAndWait(`<body style="width:100%;height:100%"><ul><li>item 1</li><li>Cost \${price}</li></ul></body>`);

      recorder.page.click("x-pw-tool-item.snapshot");
      recorder.page.hover("body");
      recorder.trustedClick();

      // playwright tests assertions are uncommented
      expect.poll(() =>
        recorder.text("Playwright Test")).toContain([
        `  await expect(page.locator('body')).toMatchAriaSnapshot(\``,
        `    - list:`,
        `      - listitem: item 1`,
        `      - listitem: "Cost \\\${price}"`,
        `    \`);`,
      ].join("\n"));

      // non-test javascript has commented assertions
      expect.poll(() =>
        recorder.text("JavaScript")).toContain([
        `  // await expect(page.locator('body')).toMatchAriaSnapshot(\``,
        `  //   - list:`,
        `  //     - listitem: item 1`,
        `  //     - listitem: "Cost \\\${price}"`,
        `  //   \`);`,
      ].join("\n"));

    });
}
