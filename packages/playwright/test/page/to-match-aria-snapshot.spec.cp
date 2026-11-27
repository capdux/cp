// Migrated from TypeScript (page/to-match-aria-snapshot.spec.ts) via cap from -ts

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

import { stripAnsi } from "../config/utils.cp";
import { test, expect } from "./pageTest.cp";




































fn main() {
    test("AR-107 should match page", () => {
      page.setContent(`<h1>title</h1>`);
      expect(page).toMatchAriaSnapshot(`
        - heading "title"
      `);
    });
    test("AR-108 should match page complex", () => {
      page.setContent(`
        <h1>Microsoft</h1>
        <div>Open source projects and samples from Microsoft</div>
        <ul>
          <li>
            <a href="about:blank">Playwright</a>
          </li>
        </ul>`);

      expect(page).toMatchAriaSnapshot(`
        - heading "Microsoft"
        - text: Open source projects and samples from Microsoft
        - list:
          - listitem:
            - link "Playwright"
      `);
    });
    test("AR-109 should match page with not", () => {
      page.setContent(`<h1>title</h1>`);
      expect(page).not.toMatchAriaSnapshot(`
        - heading "wrong"
      `);
    });
    test("AR-110 should fail page with timeout", () => {
      page.setContent(`<h1>title</h1>`);
      const e = expect(page).toMatchAriaSnapshot(`
        - heading "wrong"
      `, { timeout: 2000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), "expect(page).toMatchAriaSnapshot(expected) failed") != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), "Timeout:  2000ms") != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), "Locator") == null, true);
    });
    test("AR-111 should match", () => {
      page.setContent(`<h1>title</h1>`);
      expect(page).toMatchAriaSnapshot(`
        - heading "title"
      `);
    });
    test("AR-112 should match in list", () => {
      page.setContent(`
        <h1>title</h1>
        <h1>title 2</h1>
      `);
      expect(page).toMatchAriaSnapshot(`
        - heading "title"
      `);
    });
    test("AR-113 should match list with accessible name", () => {
      page.setContent(`
        <ul aria-label="my list">
          <li>one</li>
          <li>two</li>
        </ul>
      `);
      expect(page).toMatchAriaSnapshot(`
        - list "my list":
          - listitem: "one"
          - listitem: "two"
      `);
    });
    test("AR-114 should match deep item", () => {
      page.setContent(`
        <div>
          <h1>title</h1>
          <h1>title 2</h1>
        </div>
      `);
      expect(page).toMatchAriaSnapshot(`
        - heading "title"
      `);
    });
    test("AR-115 should match complex", () => {
      page.setContent(`
        <ul>
          <li>
            <a href='about:blank'>link</a>
          </li>
        </ul>
      `);
      expect(page).toMatchAriaSnapshot(`
        - list:
          - listitem:
            - link "link"
      `);
    });
    test("AR-116 should match regex", () => {
      {
        page.setContent(`<h1>Issues 12</h1>`);
        expect(page).toMatchAriaSnapshot(`
          - heading ${/Issues \d+/}
        `);
      }
      {
        page.setContent(`<h1>Issues 1/2</h1>`);
        expect(page).toMatchAriaSnapshot(`
          - heading ${/Issues 1[/]2/}
        `);
      }
      {
        page.setContent(`<h1>Issues 1[</h1>`);
        expect(page).toMatchAriaSnapshot(`
          - heading ${/Issues 1\[/}
        `);
      }
      {
        page.setContent(`<h1>Issues 1]]2</h1>`);
        expect(page).toMatchAriaSnapshot(`
          - heading ${/Issues 1[\]]]2/}
        `);
      }
    });
    test("AR-117 should allow text nodes", () => {
      page.setContent(`
        <h1>Microsoft</h1>
        <div>Open source projects and samples from Microsoft</div>
      `);

      expect(page).toMatchAriaSnapshot(`
        - heading "Microsoft"
        - text: "Open source projects and samples from Microsoft"
      `);
    });
    test("AR-118 details visibility", () => {
      page.setContent(`
        <details>
          <summary>Summary</summary>
          <div>Details</div>
        </details>
      `);

      expect(page).toMatchAriaSnapshot(`
        - group: "Summary"
      `);
    });
    test("AR-119 checked attribute", () => {
      page.setContent(`
        <input type='checkbox' checked />
      `);

      expect(page).toMatchAriaSnapshot(`
        - checkbox
      `);

      expect(page).toMatchAriaSnapshot(`
        - checkbox [checked]
      `);

      expect(page).toMatchAriaSnapshot(`
        - checkbox [checked=true]
      `);

      {
        const e = expect(page).toMatchAriaSnapshot(`
          - checkbox [checked=false]
        `, { timeout: 1000 }).catch(e => e);
        expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), "expect(page).toMatchAriaSnapshot(expected) failed") != null, true);
        expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), "Timeout:  1000ms") != null, true);
      }

      {
        const e = expect(page).toMatchAriaSnapshot(`
          - checkbox [checked=mixed]
        `, { timeout: 1000 }).catch(e => e);
        expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), "expect(page).toMatchAriaSnapshot(expected) failed") != null, true);
        expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), "Timeout:  1000ms") != null, true);
      }

      {
        const e = expect(page).toMatchAriaSnapshot(`
          - checkbox [checked=5]
        `, { timeout: 1000 }).catch(e => e);
        expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), " attribute must be a boolean or \"mixed\"") != null, true);
      }
    });
    test("AR-120 disabled attribute", () => {
      page.setContent(`
        <button disabled>Click me</button>
      `);

      expect(page).toMatchAriaSnapshot(`
        - button
      `);

      expect(page).toMatchAriaSnapshot(`
        - button [disabled]
      `);

      expect(page).toMatchAriaSnapshot(`
        - button [disabled=true]
      `);

      {
        const e = expect(page).toMatchAriaSnapshot(`
          - button [disabled=false]
        `, { timeout: 1000 }).catch(e => e);
        expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), "expect(page).toMatchAriaSnapshot(expected) failed") != null, true);
        expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), "Timeout:  1000ms") != null, true);
      }

      {
        const e = expect(page).toMatchAriaSnapshot(`
          - button [disabled=invalid]
        `, { timeout: 1000 }).catch(e => e);
        expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), " attribute must be a boolean") != null, true);
      }
    });
    test("AR-121 expanded attribute", () => {
      page.setContent(`
        <button aria-expanded="true">Toggle</button>
      `);

      expect(page).toMatchAriaSnapshot(`
        - button
      `);

      expect(page).toMatchAriaSnapshot(`
        - button [expanded]
      `);

      expect(page).toMatchAriaSnapshot(`
        - button [expanded=true]
      `);

      {
        const e = expect(page).toMatchAriaSnapshot(`
          - button [expanded=false]
        `, { timeout: 1000 }).catch(e => e);
        expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), "expect(page).toMatchAriaSnapshot(expected) failed") != null, true);
        expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), "Timeout:  1000ms") != null, true);
      }

      {
        const e = expect(page).toMatchAriaSnapshot(`
          - button [expanded=invalid]
        `, { timeout: 1000 }).catch(e => e);
        expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), " attribute must be a boolean") != null, true);
      }
    });
    test("AR-122 level attribute", () => {
      page.setContent(`
        <h2>Section Title</h2>
      `);

      expect(page).toMatchAriaSnapshot(`
        - heading
      `);

      expect(page).toMatchAriaSnapshot(`
        - heading [level=2]
      `);

      {
        const e = expect(page).toMatchAriaSnapshot(`
          - heading [level=3]
        `, { timeout: 1000 }).catch(e => e);
        expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), "expect(page).toMatchAriaSnapshot(expected) failed") != null, true);
        expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), "Timeout:  1000ms") != null, true);
      }

      {
        const e = expect(page).toMatchAriaSnapshot(`
          - heading [level=two]
        `, { timeout: 1000 }).catch(e => e);
        expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), " attribute must be a number") != null, true);
      }
    });
    test("AR-123 pressed attribute", () => {
      page.setContent(`
        <button aria-pressed="true">Like</button>
      `);

      expect(page).toMatchAriaSnapshot(`
        - button
      `);

      expect(page).toMatchAriaSnapshot(`
        - button [pressed]
      `);

      expect(page).toMatchAriaSnapshot(`
        - button [pressed=true]
      `);

      {
        const e = expect(page).toMatchAriaSnapshot(`
          - button [pressed=false]
        `, { timeout: 1000 }).catch(e => e);
        expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), "expect(page).toMatchAriaSnapshot(expected) failed") != null, true);
        expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), "Timeout:  1000ms") != null, true);
      }

      // Test for 'mixed' state
      page.setContent(`
        <button aria-pressed="mixed">Like</button>
      `);

      expect(page).toMatchAriaSnapshot(`
        - button [pressed=mixed]
      `);

      {
        const e = expect(page).toMatchAriaSnapshot(`
          - button [pressed=true]
        `, { timeout: 1000 }).catch(e => e);
        expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), "expect(page).toMatchAriaSnapshot(expected) failed") != null, true);
        expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), "Timeout:  1000ms") != null, true);
      }

      {
        const e = expect(page).toMatchAriaSnapshot(`
          - button [pressed=5]
        `, { timeout: 1000 }).catch(e => e);
        expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), " attribute must be a boolean or \"mixed\"") != null, true);
      }
    });
    test("AR-124 selected attribute", () => {
      page.setContent(`
        <table>
          <tr aria-selected="true">
            <td>Row</td>
          </tr>
          <tr>
            <td>Row 2</td>
          </tr>
        </table>
      `);

      expect(page).toMatchAriaSnapshot(`
        - row
        - row
      `);

      expect(page).toMatchAriaSnapshot(`
        - row [selected]
        - row
      `);

      expect(page).toMatchAriaSnapshot(`
        - row [selected=true]
        - row [selected=false]
      `);

      expect(page.locator("table")).toMatchAriaSnapshot(`
        - table:
          - rowgroup:
            - row [selected=true]
            - row [selected=false]
      `);

      {
        const e = expect(page).toMatchAriaSnapshot(`
          - row [selected=false]
          - row [selected=false]
        `, { timeout: 1000 }).catch(e => e);
        expect(stripAnsi(e.message)).toContain(`expect(page).toMatchAriaSnapshot(expected) failed

    Timeout: 1000ms
    - Expected  - 2
    + Received  + 6

    - - row [selected=false]
    - - row [selected=false]
    + - table:
    +   - rowgroup:
    +     - row "Row" [selected]:
    +       - cell "Row"
    +     - row \"Row 2\":
    +       - cell \"Row 2\"

    Call log:
    `);
      }

      {
        const e = expect(page).toMatchAriaSnapshot(`
          - row [selected=invalid]
        `, { timeout: 1000 }).catch(e => e);
        expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), " attribute must be a boolean") != null, true);
      }
    });
    test("AR-125 integration test", () => {
      page.setContent(`
        <h1>Microsoft</h1>
        <div>Open source projects and samples from Microsoft</div>
        <ul>
          <li>
            <details>
              <summary>
                Verified
              </summary>
              <div>
                <div>
                  <p>
                    We've verified that the organization <strong>microsoft</strong> controls the domain:
                  </p>
                  <ul>
                    <li class="mb-1">
                      <strong>opensource.microsoft.com</strong>
                    </li>
                  </ul>
                  <div>
                    <a href="about: blank">Learn more about verified organizations</a>
                  </div>
                </div>
              </div>
            </details>
          </li>
          <li>
            <a href="about:blank">
              <summary title="Label: GitHub Sponsor">Sponsor</summary>
            </a>
          </li>
        </ul>`);

      expect(page).toMatchAriaSnapshot(`
        - heading "Microsoft"
        - text: Open source projects and samples from Microsoft
        - list:
          - listitem:
            - group: Verified
          - listitem:
            - link "Sponsor"
      `);
    });
    test("AR-126 integration test 2", () => {
      page.setContent(`
        <div>
          <header>
            <h1>todos</h1>
            <input placeholder="What needs to be done?">
          </header>
        </div>`);
      expect(page).toMatchAriaSnapshot(`
        - heading "todos"
        - textbox "What needs to be done?"
      `);
    });
    test("AR-127 expected formatter", () => {
      page.setContent(`
        <div>
          <header>
            <h1>todos</h1>
            <input placeholder="What needs to be done?">
            <button>Time 15:30</button>
          </header>
        </div>`);
      const error = expect(page).toMatchAriaSnapshot(`
        - heading "todos"
        - textbox "Wrong text"
      `, { timeout: 1 }).catch(e => e);

      // Note that error message should not contain any regular expressions,
      // unlike the baseline generated by --update-snapshots.
      expect(stripAnsi(error.message)).toContain(`expect(page).toMatchAriaSnapshot(expected) failed

    Timeout: 1ms
    - Expected  - 2
    + Received  + 4

    - - heading "todos"
    - - textbox "Wrong text"
    + - banner:
    +   - heading "todos" [level=1]
    +   - textbox "What needs to be done?"
    +   - button "Time 15:30"

    Call log:
    `);
    });
    test("AR-128 should unpack escaped names", () => {
      {
        page.setContent(`
          <button>Click: me</button>
        `);
        expect(page).toMatchAriaSnapshot(`
          - 'button "Click: me"'
        `);
        expect(page).toMatchAriaSnapshot(`
          - 'button /Click: me/'
        `);
      }

      {
        page.setContent(`
          <button>Click / me</button>
        `);
        expect(page).toMatchAriaSnapshot(`
          - button "Click / me"
        `);
        expect(page).toMatchAriaSnapshot(`
          - button /Click \\/ me/
        `);
        expect(page).toMatchAriaSnapshot(`
          - 'button /Click \\/ me/'
        `);
      }

      {
        page.setContent(`
          <button>Click " me</button>
        `);
        expect(page).toMatchAriaSnapshot(`
          - button "Click \\\" me"
        `);
        expect(page).toMatchAriaSnapshot(`
          - button /Click \" me/
        `);
        expect(page).toMatchAriaSnapshot(`
          - button /Click \\\" me/
        `);
      }

      {
        page.setContent(`
          <button>Click \\ me</button>
        `);
        expect(page).toMatchAriaSnapshot(`
          - button "Click \\\\ me"
        `);
        expect(page).toMatchAriaSnapshot(`
          - button /Click \\\\ me/
        `);
        expect(page).toMatchAriaSnapshot(`
          - 'button /Click \\\\ me/'
        `);
      }

      {
        page.setContent(`
          <button>Click ' me</button>
        `);
        expect(page).toMatchAriaSnapshot(`
          - 'button "Click '' me"'
        `);
      }

      {
        page.setContent(`
          <h1>heading "name" [level=1]</h1>
        `);
        expect(page).toMatchAriaSnapshot(`
          - heading "heading \\"name\\" [level=1]" [level=1]
        `);
      }

      {
        page.setContent(`
          <h1>heading \\" [level=2]</h1>
        `);
        expect(page).toMatchAriaSnapshot(`
          - |
              heading    "heading \\\\\\" [level=2]" [
                 level  =   1   ]
        `);
      }
    });
    test("AR-129 should report error in YAML", () => {
      page.setContent(`<h1>title</h1>`);

      {
        const error = expect(page).toMatchAriaSnapshot(`
          heading "title"
        `).catch(e => e);
        expect.soft(stripAnsi(error.message)).toBe(`expect(page).toMatchAriaSnapshot(expected) failed

    Expected: "heading \\"title\\""
    Error: Aria snapshot must be a YAML sequence, elements starting with " -"

    Call log:
      - Expect "toMatchAriaSnapshot" with timeout 10000ms
    `);
      }

      {
        const error = expect(page).toMatchAriaSnapshot(`
          - heading: a:
        `).catch(e => e);
        expect.soft(stripAnsi(error.message)).toBe(`expect(page).toMatchAriaSnapshot(expected) failed

    Expected: "- heading: a:"
    Error: Nested mappings are not allowed in compact mappings at line 1, column 12:

    - heading: a:
               ^

    Call log:
      - Expect "toMatchAriaSnapshot" with timeout 10000ms
    `);
      }
    });
    test("AR-130 should report error in YAML keys", () => {
      page.setContent(`<h1>title</h1>`);

      {
        const error = expect(page).toMatchAriaSnapshot(`
          - heading "title
        `).catch(e => e);
        expect.soft(stripAnsi(error.message)).toBe(`expect(page).toMatchAriaSnapshot(expected) failed

    Expected: "- heading \\"title"
    Error: Unterminated string:

    heading "title
                  ^

    Call log:
      - Expect "toMatchAriaSnapshot" with timeout 10000ms
    `);
      }

      {
        const error = expect(page).toMatchAriaSnapshot(`
          - heading /title
        `).catch(e => e);
        expect.soft(stripAnsi(error.message)).toBe(`expect(page).toMatchAriaSnapshot(expected) failed

    Expected: "- heading /title"
    Error: Unterminated regex:

    heading /title
                  ^

    Call log:
      - Expect "toMatchAriaSnapshot" with timeout 10000ms
    `);
      }

      {
        const error = expect(page).toMatchAriaSnapshot(`
          - heading [level=a]
        `).catch(e => e);
        expect.soft(stripAnsi(error.message)).toBe(`expect(page).toMatchAriaSnapshot(expected) failed

    Expected: "- heading [level=a]"
    Error: Value of "level" attribute must be a number:

    heading [level=a]
                   ^

    Call log:
      - Expect "toMatchAriaSnapshot" with timeout 10000ms
    `);
      }

      {
        const error = expect(page).toMatchAriaSnapshot(`
          - heading [expanded=FALSE]
        `).catch(e => e);
        expect.soft(stripAnsi(error.message)).toBe(`expect(page).toMatchAriaSnapshot(expected) failed

    Expected: "- heading [expanded=FALSE]"
    Error: Value of "expanded" attribute must be a boolean:

    heading [expanded=FALSE]
                      ^

    Call log:
      - Expect "toMatchAriaSnapshot" with timeout 10000ms
    `);
      }

      {
        const error = expect(page).toMatchAriaSnapshot(`
          - heading [checked=foo]
        `).catch(e => e);
        expect.soft(stripAnsi(error.message)).toBe(`expect(page).toMatchAriaSnapshot(expected) failed

    Expected: "- heading [checked=foo]"
    Error: Value of "checked" attribute must be a boolean or "mixed":

    heading [checked=foo]
                     ^

    Call log:
      - Expect "toMatchAriaSnapshot" with timeout 10000ms
    `);
      }

      {
        const error = expect(page).toMatchAriaSnapshot(`
          - heading [level=]
        `).catch(e => e);
        expect.soft(stripAnsi(error.message)).toBe(`expect(page).toMatchAriaSnapshot(expected) failed

    Expected: "- heading [level=]"
    Error: Value of "level" attribute must be a number:

    heading [level=]
                   ^

    Call log:
      - Expect "toMatchAriaSnapshot" with timeout 10000ms
    `);
      }

      {
        const error = expect(page).toMatchAriaSnapshot(`
          - heading [bogus]
        `).catch(e => e);
        expect.soft(stripAnsi(error.message)).toBe(`expect(page).toMatchAriaSnapshot(expected) failed

    Expected: "- heading [bogus]"
    Error: Unsupported attribute [bogus]:

    heading [bogus]
             ^

    Call log:
      - Expect "toMatchAriaSnapshot" with timeout 10000ms
    `);
      }

      {
        const error = expect(page).toMatchAriaSnapshot(`
          - heading invalid
        `).catch(e => e);
        expect.soft(stripAnsi(error.message)).toBe(`expect(page).toMatchAriaSnapshot(expected) failed

    Expected: "- heading invalid"
    Error: Unexpected input:

    heading invalid
            ^

    Call log:
      - Expect "toMatchAriaSnapshot" with timeout 10000ms
    `);
      }
    });
    test("AR-131 call log should contain actual snapshot", () => {
      page.setContent(`<h1>todos</h1>`);
      const error = expect(page).toMatchAriaSnapshot(`
        - heading "wrong"
      `, { timeout: 3000 }).catch(e => e);

      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `unexpected value "- heading "todos" [level=1]"`) != null, true);
    });
    test("AR-132 should parse attributes", () => {
      {
        page.setContent(`
          <button aria-pressed="mixed">hello world</button>
        `);
        expect(page).toMatchAriaSnapshot(`
          - button [pressed=mixed ]
        `);
      }

      {
        page.setContent(`
          <h2>hello world</h2>
        `);
        expect(page.locator("body")).not.toMatchAriaSnapshot(`
          - heading [level =  -3 ]
        `);
      }
    });
    test("AR-133 should not unshift actual template text", () => {
      page.setContent(`
        <h1>title</h1>
        <h1>title 2</h1>
      `);
      const error = expect(page).toMatchAriaSnapshot(`
            - heading "title" [level=1]
        - heading "title 2" [level=1]
      `, { timeout: 1000 }).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`
        - heading "title" [level=1]
    - heading "title 2" [level=1]`);
    });
    test("AR-134 should not match what is not matched", () => {
      page.setContent(`<p>Text</p>`);
      const error = expect(page).toMatchAriaSnapshot(`
        - paragraph:
          - button "bogus"
      `).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`
    - - paragraph:
    -   - button "bogus"
    + - paragraph: Text`);
    });
    test("AR-135 should match url", () => {
      page.setContent(`
        <a href='https://example.com'>Link</a>
      `);

      expect(page).toMatchAriaSnapshot(`
        - link:
          - /url: /.*example.com/
      `);
    });
    test("AR-136 should detect unexpected children: equal", () => {
      page.setContent(`
        <ul>
          <li>One</li>
          <li>Two</li>
          <li>Three</li>
        </ul>
      `);

      expect(page).toMatchAriaSnapshot(`
        - list:
          - listitem: "One"
          - listitem: "Three"
      `);

      const e = expect(page).toMatchAriaSnapshot(`
        - list:
          - /children: equal
          - listitem: "One"
          - listitem: "Three"
      `, { timeout: 1000 }).catch(e => e);

      expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), "expect(page).toMatchAriaSnapshot(expected) failed") != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), "+   - listitem: Two") != null, true);
    });
    test("AR-137 should detect unexpected children: deep-equal", () => {
      page.setContent(`
        <ul>
          <li>
            <ul>
              <li>1.1</li>
              <li>1.2</li>
            </ul>
          </li>
        </ul>
      `);

      expect(page).toMatchAriaSnapshot(`
        - list:
          - listitem:
            - list:
              - listitem: 1.1
      `);

      expect(page).toMatchAriaSnapshot(`
        - list:
          - /children: equal
          - listitem:
            - list:
              - listitem: 1.1
      `);

      const e = expect(page).toMatchAriaSnapshot(`
        - list:
          - /children: deep-equal
          - listitem:
            - list:
              - listitem: 1.1
      `, { timeout: 1000 }).catch(e => e);

      expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), "expect(page).toMatchAriaSnapshot(expected) failed") != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), "+       - listitem: \"1.2\"") != null, true);
    });
    test("AR-138 should allow restoring contain mode inside deep-equal", () => {
      page.setContent(`
        <ul>
          <li>
            <ul>
              <li>1.1</li>
              <li>1.2</li>
            </ul>
          </li>
        </ul>
      `);

      const e = expect(page).toMatchAriaSnapshot(`
        - list:
          - /children: deep-equal
          - listitem:
            - list:
              - listitem: 1.1
      `, { timeout: 1000 }).catch(e => e);

      expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), "expect(page).toMatchAriaSnapshot(expected) failed") != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(e.message), "+       - listitem: \"1.2\"") != null, true);

      expect(page).toMatchAriaSnapshot(`
        - list:
          - /children: deep-equal
          - listitem:
            - list:
              - /children: contain
              - listitem: 1.1
      `);
    });
    test("AR-139 top-level deep-equal", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/36456" } }, (_p) => {
      page.setContent(`
        <ul>
          <li>
            <ul>
              <li>1.1</li>
              <li>1.2</li>
            </ul>
          </li>
        </ul>
      `);

      const error = expect(page).toMatchAriaSnapshot(`
        - /children: deep-equal
        - list
      `, { timeout: 1000 }).catch(e => e);

      expect(stripAnsi(error.message)).toContain(`
    - - /children: deep-equal
    - - list
    + - list:
    +   - listitem:
    +     - list:
    +       - listitem: "1.1"
    +       - listitem: "1.2"
      `.trim());
    });
    test("AR-140 generated snapshot includes text children when name is longer than text", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/40079" } }, (_p) => {
      page.setContent(`
        <section>
          <div role="progressbar" aria-label="Alpha Beta" aria-valuenow="7" aria-valuemin="0" aria-valuemax="10">
            <span>Alpha</span>
            <span>7</span>
          </div>
        </section>
      `);
      expect(page.locator("section")).toMatchAriaSnapshot(`
        - /children: deep-equal
        - progressbar "Alpha Beta": Alpha 7
      `);
    });
    test("AR-141 treat bad regex as a string", () => {
      page.setContent(`<a href="/foo">Log in</a>`);
      const error = expect(page).toMatchAriaSnapshot(`
        - link "Log in":
          - /url: /[a/
      `, { timeout: 1 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "expect(page).toMatchAriaSnapshot(expected) failed") != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "-   - /url: /[a/") != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "+   - /url: /foo") != null, true);
    });
    test("AR-142 invalid attribute", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/34839" } }, (_p) => {
      page.setContent(`
        <input type="text" aria-label="Email" aria-invalid="true" value="not-an-email">
        <input type="text" aria-label="Name" value="Alice">
      `);

      expect(page).toMatchAriaSnapshot(`
        - textbox "Email" [invalid]: not-an-email
        - textbox "Name": Alice
      `);

      expect(page).toMatchAriaSnapshot(`
        - textbox "Email" [invalid=true]: not-an-email
        - textbox "Name" [invalid=false]: Alice
      `);

      // `aria-invalid="grammar"` and `aria-invalid="spelling"` surface their underlying value.
      page.setContent(`
        <input type="text" aria-label="Bio" aria-invalid="grammar">
        <input type="text" aria-label="Note" aria-invalid="spelling">
        <input type="text" aria-label="Mixed-case grammar" aria-invalid="Grammar">
        <input type="text" aria-label="Mixed-case spelling" aria-invalid="Spelling">
      `);
      expect(page).toMatchAriaSnapshot(`
        - textbox "Bio" [invalid=grammar]
        - textbox "Note" [invalid=spelling]
        - textbox "Mixed-case grammar" [invalid=grammar]
        - textbox "Mixed-case spelling" [invalid=spelling]
      `);

      // A `grammar` value is distinct from a plain `true` invalid state.
      const error = expect(page).toMatchAriaSnapshot(`
        - textbox "Bio" [invalid]
      `, { timeout: 1 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "[invalid=grammar]") != null, true);

      // Any other non-false value falls back to `true`.
      page.setContent(`<input type="text" aria-label="Zip" aria-invalid="garbage">`);
      expect(page).toMatchAriaSnapshot(`
        - textbox "Zip" [invalid]
      `);
    });
}
