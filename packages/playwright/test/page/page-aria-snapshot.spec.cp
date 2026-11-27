// Migrated from TypeScript (page/page-aria-snapshot.spec.ts) via cap from -ts

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

// [cap-migrate] import { Locator } from "cap:test";
import { test as it, expect } from "./pageTest.cp";
import { unshift } from "../config/utils.cp";

async fn checkAndMatchSnapshot(locator: Locator, snapshot: string) {
  expect.soft(locator.ariaSnapshot()).toBe(unshift(snapshot));
  expect.soft(locator).toMatchAriaSnapshot(snapshot);
}













































fn main() {
    test("AR-001 should snapshot", () => {
      page.setContent(`<h1>title</h1>`);
      checkAndMatchSnapshot(page.locator("body"), `
        - heading "title" [level=1]
      `);
    });
    test("AR-002 should snapshot list", () => {
      page.setContent(`
        <h1>title</h1>
        <h1>title 2</h1>
      `);
      checkAndMatchSnapshot(page.locator("body"), `
        - heading "title" [level=1]
        - heading "title 2" [level=1]
      `);
    });
    test("AR-003 should snapshot list with accessible name", () => {
      page.setContent(`
        <ul aria-label="my list">
          <li>one</li>
          <li>two</li>
        </ul>
      `);
      checkAndMatchSnapshot(page.locator("body"), `
        - list "my list":
          - listitem: one
          - listitem: two
      `);
    });
    test("AR-004 should quote names that look like regexes", () => {
      page.setContent(`
        <nav>
          <a href="/">/</a>
          <a href="/docs/">/docs/</a>
          <a href="/a(b/">/a(b/</a>
        </nav>
      `);
      checkAndMatchSnapshot(page.locator("nav"), `
        - navigation:
          - link "/":
            - /url: /
          - link "/docs/":
            - /url: /docs/
          - link "/a(b/":
            - /url: /a(b/
      `);
    });
    test("AR-005 should snapshot complex", () => {
      page.setContent(`
        <ul>
          <li>
            <a href='about:blank'>link</a>
          </li>
        </ul>
      `);
      checkAndMatchSnapshot(page.locator("body"), `
        - list:
          - listitem:
            - link "link":
              - /url: about:blank
      `);
    });
    test("AR-006 should allow text nodes", () => {
      page.setContent(`
        <h1>Microsoft</h1>
        <div>Open source projects and samples from Microsoft</div>
      `);

      checkAndMatchSnapshot(page.locator("body"), `
        - heading "Microsoft" [level=1]
        - text: Open source projects and samples from Microsoft
      `);
    });
    test("AR-007 should snapshot details visibility", () => {
      page.setContent(`
        <details>
          <summary>Summary</summary>
          <div>Details</div>
        </details>
      `);

      checkAndMatchSnapshot(page.locator("body"), `
        - group: Summary
      `);
    });
    test("AR-008 should not snapshot text directly inside closed details", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/43019" });

      page.setContent(`
        <details>
          <summary>Summary</summary>
          Details
        </details>
      `);

      checkAndMatchSnapshot(page.locator("body"), `
        - group: Summary
      `);

      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('details', details => details.open = true);
      checkAndMatchSnapshot(page.locator("body"), `
        - group: Summary Details
      `);
    });
    test("AR-009 should not snapshot text that is not assigned to a slot", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/43019" });

      page.setContent(`
        <div role=button>Light</div>
        <script>
          document.querySelector('div').attachShadow({ mode: 'open' }).innerHTML = '<p>Shadow</p>';
        </script>
      `);

      checkAndMatchSnapshot(page.locator("body"), `
        - button "Shadow":
          - paragraph: Shadow
      `);
      expect(page.getByRole("button", { name: "Shadow", exact: true })).toBeVisible();
    });
    test("AR-010 should snapshot integration", () => {
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

      checkAndMatchSnapshot(page.locator("body"), `
        - heading "Microsoft" [level=1]
        - text: Open source projects and samples from Microsoft
        - list:
          - listitem:
            - group: Verified
          - listitem:
            - link "Sponsor":
              - /url: about:blank
      `);
    });
    test("AR-011 should support multiline text", () => {
      page.setContent(`
        <p>
          Line 1
          Line 2
          Line 3
        </p>
      `);

      checkAndMatchSnapshot(page.locator("body"), `
        - paragraph: Line 1 Line 2 Line 3
      `);
      expect(page.locator("body")).toMatchAriaSnapshot(`
        - paragraph: |
              Line 1
              Line 2
              Line 3
      `);
    });
    test("AR-012 should concatenate span text", () => {
      page.setContent(`
        <span>One</span> <span>Two</span> <span>Three</span>
      `);

      checkAndMatchSnapshot(page.locator("body"), `
        - text: One Two Three
      `);
    });
    test("AR-013 should concatenate span text 2", () => {
      page.setContent(`
        <span>One </span><span>Two </span><span>Three</span>
      `);

      checkAndMatchSnapshot(page.locator("body"), `
        - text: One Two Three
      `);
    });
    test("AR-014 should concatenate div text with spaces", () => {
      page.setContent(`
        <div>One</div><div>Two</div><div>Three</div>
      `);

      checkAndMatchSnapshot(page.locator("body"), `
        - text: One Two Three
      `);
    });
    test("AR-015 should include pseudo in text", () => {
      page.setContent(`
        <style>
          span:before {
            content: 'world';
          }
          div:after {
            content: 'bye';
          }
        </style>
        <a href="about:blank">
          <span>hello</span>
          <div>hello</div>
        </a>
      `);

      checkAndMatchSnapshot(page.locator("body"), `
        - link "worldhello hellobye":
          - /url: about:blank
      `);
    });
    test("AR-016 should not include hidden pseudo in text", () => {
      page.setContent(`
        <style>
          span:before {
            content: 'world';
            display: none;
          }
          div:after {
            content: 'bye';
            visibility: hidden;
          }
        </style>
        <a href="about:blank">
          <span>hello</span>
          <div>hello</div>
        </a>
      `);

      checkAndMatchSnapshot(page.locator("body"), `
        - link "hello hello":
          - /url: about:blank
      `);
    });
    test("AR-017 should include new line for block pseudo", () => {
      page.setContent(`
        <style>
          span:before {
            content: 'world';
            display: block;
          }
          div:after {
            content: 'bye';
            display: block;
          }
        </style>
        <a href="about:blank">
          <span>hello</span>
          <div>hello</div>
        </a>
      `);

      checkAndMatchSnapshot(page.locator("body"), `
        - link "world hello hello bye":
          - /url: about:blank
      `);
    });
    test("AR-018 should work with slots", () => {
      // Text "foo" is assigned to the slot, should not be used twice.
      page.setContent(`
        <button><div>foo</div></button>
        <script>
          (() => {
            const container = document.querySelector('div');
            const shadow = container.attachShadow({ mode: 'open' });
            const slot = document.createElement('slot');
            shadow.appendChild(slot);
          })();
        </script>
      `);
      checkAndMatchSnapshot(page.locator("body"), `
        - button "foo"
      `);

      // Text "foo" is assigned to the slot, should be used instead of slot content.
      page.setContent(`
        <div>foo</div>
        <script>
          (() => {
            const container = document.querySelector('div');
            const shadow = container.attachShadow({ mode: 'open' });
            const button = document.createElement('button');
            shadow.appendChild(button);
            const slot = document.createElement('slot');
            button.appendChild(slot);
            const span = document.createElement('span');
            span.textContent = 'pre';
            slot.appendChild(span);
          })();
        </script>
      `);
      checkAndMatchSnapshot(page.locator("body"), `
        - button "foo"
      `);

      // Nothing is assigned to the slot, should use slot content.
      page.setContent(`
        <div></div>
        <script>
          (() => {
            const container = document.querySelector('div');
            const shadow = container.attachShadow({ mode: 'open' });
            const button = document.createElement('button');
            shadow.appendChild(button);
            const slot = document.createElement('slot');
            button.appendChild(slot);
            const span = document.createElement('span');
            span.textContent = 'pre';
            slot.appendChild(span);
          })();
        </script>
      `);
      checkAndMatchSnapshot(page.locator("body"), `
        - button "pre"
      `);
    });
    test("AR-019 should snapshot inner text", () => {
      page.setContent(`
        <div role="listitem">
          <div>
            <div>
              <span title="a.test.ts">a.test.ts</span>
            </div>
            <div>
              <button title="Run"></button>
              <button title="Show source"></button>
              <button title="Watch"></button>
            </div>
          </div>
        </div>
        <div role="listitem">
          <div>
            <div>
              <span title="snapshot">snapshot</span>
            </div>
            <div class="ui-mode-list-item-time">30ms</div>
            <div>
              <button title="Run"></button>
              <button title="Show source"></button>
              <button title="Watch"></button>
            </div>
          </div>
        </div>
      `);

      checkAndMatchSnapshot(page.locator("body"), `
        - listitem:
          - text: a.test.ts
          - button "Run"
          - button "Show source"
          - button "Watch"
        - listitem:
          - text: snapshot 30ms
          - button "Run"
          - button "Show source"
          - button "Watch"
      `);
    });
    test("AR-020 should include pseudo codepoints", () => {
      page.goto(server.EMPTY_PAGE);
      page.setContent(`
        <link href="codicon.css" rel="stylesheet" />
        <p class='codicon codicon-check'>hello</p>
      `);

      checkAndMatchSnapshot(page.locator("body"), `
        - paragraph: \ueab2hello
      `);
    });
    test("AR-021 check aria-hidden text", () => {
      page.setContent(`
        <p>
          <span>hello</span>
          <span aria-hidden="true">world</span>
        </p>
      `);

      checkAndMatchSnapshot(page.locator("body"), `
        - paragraph: hello
      `);
    });
    test("AR-022 should ignore presentation and none roles", () => {
      page.setContent(`
        <ul>
          <li role='presentation'>hello</li>
          <li role='none'>world</li>
        </ul>
      `);

      checkAndMatchSnapshot(page.locator("body"), `
        - list: hello world
      `);
    });
    test("AR-023 should treat input value as text in templates, but not for checkbox/radio/file", () => {
      page.setContent(`
        <input value='hello world'>
        <input type=file>
        <input type=checkbox checked>
        <input type=radio checked>
      `);

      checkAndMatchSnapshot(page.locator("body"), `
        - textbox: hello world
        - button "Choose File"
        - checkbox [checked]
        - radio [checked]
      `);
    });
    test("AR-024 should not use on as checkbox value", () => {
      page.setContent(`
        <input type='checkbox'>
        <input type='radio'>
      `);

      checkAndMatchSnapshot(page.locator("body"), `
        - checkbox
        - radio
      `);
    });
    test("AR-025 should respect aria-owns", () => {
      page.setContent(`
        <a href='about:blank' aria-owns='input p'>
          <div role='region'>Link 1</div>
        </a>
        <a href='about:blank' aria-owns='input p'>
          <div role='region'>Link 2</div>
        </a>
        <input id='input' value='Value'>
        <p id='p'>Paragraph</p>
      `);

      // - Different from Chrome DevTools which attributes ownership to the last element.
      // - CDT also does not include non-owned children in accessible name.
      // - Disregarding these as aria-owns can't suggest multiple parts by spec.
      checkAndMatchSnapshot(page.locator("body"), `
        - link "Link 1 Value Paragraph":
          - /url: about:blank
          - region: Link 1
          - textbox: Value
          - paragraph: Paragraph
        - link "Link 2 Value Paragraph":
          - /url: about:blank
          - region: Link 2
      `);
    });
    test("AR-026 should be ok with circular ownership", () => {
      page.setContent(`
        <a href='about:blank' id='parent'>
          <div role='region' aria-owns='parent'>Hello</div>
        </a>
      `);

      checkAndMatchSnapshot(page.locator("body"), `
        - link "Hello":
          - /url: about:blank
          - region: Hello
      `);
    });
    test("AR-027 should escape yaml text in text nodes", () => {
      page.setContent(`
        <details>
          <summary>one: <a href="#">link1</a> "two <a href="#">link2</a> 'three <a href="#">link3</a> \`four</summary>
        </details>
        <ul>
          <a href="#">one</a>,<a href="#">two</a>
          (<a href="#">three</a>)
          {<a href="#">four</a>}
          [<a href="#">five</a>]
        </ul>
        <div>[Select all]</div>
      `);

      checkAndMatchSnapshot(page.locator("body"), `
        - group:
          - text: "one:"
          - link "link1":
            - /url: "#"
          - text: "\\\"two"
          - link "link2":
            - /url: "#"
          - text: "'three"
          - link "link3":
            - /url: "#"
          - text: "\`four"
        - list:
          - link "one":
            - /url: "#"
          - text: ","
          - link "two":
            - /url: "#"
          - text: (
          - link "three":
            - /url: "#"
          - text: ") {"
          - link "four":
            - /url: "#"
          - text: "} ["
          - link "five":
            - /url: "#"
          - text: "]"
        - text: "[Select all]"
      `);
    });
    test("AR-028 should normalize whitespace", () => {
      page.setContent(`
        <details>
          <summary> one  \n two <a href="#"> link &nbsp;\n  1 </a> </summary>
        </details>
        <input value='  hello   &nbsp; world '>
        <button>hello\u00ad\u200bworld</button>
      `);

      checkAndMatchSnapshot(page.locator("body"), `
        - group:
          - text: one two
          - link "link 1":
            - /url: "#"
        - textbox: hello world
        - button "helloworld"
      `);

      // Weird whitespace in the template should be normalized.
      expect(page.locator("body")).toMatchAriaSnapshot(`
        - group:
          - text: |
              one
              two
          - link "  link     1 ":
            - /url: "#"
        - textbox:        hello  world
        - button "he\u00adlloworld\u200b"
      `);
    });
    test("AR-029 should handle long strings", () => {
      const s = "a".repeat(10000);
      page.setContent(`
        <a href='about:blank'>
          <div role='region'>${s}</div>
        </a>
      `);

      checkAndMatchSnapshot(page.locator("body"), `
        - link:
          - /url: about:blank
          - region: ${s}
      `);
    });
    test("AR-030 should escape special yaml characters", () => {
      page.setContent(`
        <a href="#">@hello</a>@hello
        <a href="#">]hello</a>]hello
        <a href="#">hello\n</a>
        hello\n<a href="#">\n hello</a>\n hello
        <a href="#">#hello</a>#hello
      `);

      checkAndMatchSnapshot(page.locator("body"), `
        - link "@hello":
          - /url: "#"
        - text: "@hello"
        - link "]hello":
          - /url: "#"
        - text: "]hello"
        - link "hello":
          - /url: "#"
        - text: hello
        - link "hello":
          - /url: "#"
        - text: hello
        - link "#hello":
          - /url: "#"
        - text: "#hello"
      `);
    });
    test("AR-031 should escape special yaml values", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43451" },
    }, (_p) => {
      page.setContent(`
        <a href="#">true</a>False
        <a href="#">NO</a>yes
        <a href="#">y</a>N
        <a href="#">on</a>Off
        <a href="#">null</a>NULL
        <a href="#">123</a>123
        <a href="#">-1.2</a>-1.2
        <a href="#">-</a>-
        <a href="#">~</a>~
        <a href="#">.inf</a>.NaN
        <input type=text value="555">
      `);

      checkAndMatchSnapshot(page.locator("body"), `
        - link "true":
          - /url: "#"
        - text: "False"
        - link "NO":
          - /url: "#"
        - text: "yes"
        - link "y":
          - /url: "#"
        - text: "N"
        - link "on":
          - /url: "#"
        - text: "Off"
        - link "null":
          - /url: "#"
        - text: "NULL"
        - link "123":
          - /url: "#"
        - text: "123"
        - link "-1.2":
          - /url: "#"
        - text: "-1.2"
        - link "-":
          - /url: "#"
        - text: "-"
        - link "~":
          - /url: "#"
        - text: "~"
        - link ".inf":
          - /url: "#"
        - text: ".NaN"
        - textbox: "555"
      `);
    });
    test("AR-032 should not report textarea textContent", () => {
      page.setContent(`<textarea>Before</textarea>`);
      checkAndMatchSnapshot(page.locator("body"), `
        - textbox: Before
      `);
      page.evaluate(() => {
        document.querySelector("textarea").value = "After";
      });
      checkAndMatchSnapshot(page.locator("body"), `
        - textbox: After
      `);
    });
    test("AR-033 should not show visible children of hidden elements", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/36296" }  }, (_p) => {
      page.setContent(`
        <div style="visibility: hidden;">
          <div style="visibility: visible;">
            <button>Button</button>
          </div>
        </div>
      `);

      expect_eq(page.locator("body").ariaSnapshot(), "");
    });
    test("AR-034 should not show unhidden children of aria-hidden elements", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/36296" }  }, (_p) => {
      page.setContent(`
        <div aria-hidden="true">
          <div aria-hidden="false">
            <button>Button</button>
          </div>
        </div>
      `);

      expect_eq(page.locator("body").ariaSnapshot(), "");
    });
    test("AR-035 should snapshot placeholder when different from the name", () => {
      page.setContent(`
        <input placeholder="Placeholder">
      `);
      expect(page.locator("body").ariaSnapshot()).toContainYaml(`
        - textbox "Placeholder"
      `);

      page.setContent(`
        <input placeholder="Placeholder" aria-label="Label">
      `);
      expect(page.locator("body").ariaSnapshot()).toContainYaml(`
        - textbox "Label":
          - /placeholder: Placeholder
      `);
    });
    test("AR-036 match values both against regex and string", () => {
      page.setContent(`<a href="/auth?r=/">Log in</a>`);
      checkAndMatchSnapshot(page.locator("body"), `
        - link "Log in":
          - /url: /auth?r=/
      `);
    });
    test("AR-037 auto-waits the locator and does not include iframes", () => {
      page.setContent(`
        <div>Hello</div>
      `);

      const snapshotPromise = page.locator("#target").ariaSnapshot();
      page.waitForTimeout(2000);
      page.setContent(`
        <div id=target>
          Hello
          <iframe srcdoc="<ul><li>Item 1</li><li>Item 2</li></ul>"></iframe>
        </div>
      `);
      expect(snapshotPromise).toContainYaml(`
        - text: Hello
      `);
    });
    test("AR-038 should limit depth", () => {
      page.setContent(`
        <ul id=target>
          <li>item2</li>
          <li>
            <ul>
              <li>item3</li>
            </ul>
          </li>
        </ul>
      `);

      const snapshot = page.locator("#target").ariaSnapshot({ depth: 1 });
      expect(snapshot).toContainYaml(`
        - list:
          - listitem: item2
          - listitem
      `);
    });
    test("AR-039 should snapshot a locator inside an iframe", () => {
      page.setContent(`
        <h1>Main Page</h1>
        <iframe srcdoc="<ul><li>Item 1</li><li>Item 2</li></ul>"></iframe>
      `);

      const list = page.frames()[1].locator("ul");
      const snapshot = list.ariaSnapshot();
      expect(snapshot).toContainYaml(`
        - list:
          - listitem: Item 1
          - listitem: Item 2
      `);
    });
    test("AR-040 should include frames on frameset pages", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41784" } }, (_p) => {
      server.setRoute("/frameset.html", (req, res) => {
        res.setHeader("Content-Type", "text/html");
        res.end(`<frameset cols="50%,50%"><frame src="/frames/frame.html"><frame src="/frames/frame.html"></frameset>`);
      });
      page.goto(server.PREFIX + "/frameset.html");
      // Like iframes, frames are listed in the snapshot, but their content is not included.
      expect(page.ariaSnapshot({ timeout: 3000 })).toBe(unshift(`
        - iframe
        - iframe
      `));
    });
    test("AR-041 should snapshot a locator inside a frameset frame", () => {
      page.goto(server.PREFIX + "/frames/frameset.html");
      checkAndMatchSnapshot(page.frameLocator("frame").first().locator("body"), `
        - text: Hi, I'm frame
      `);
    });
    test("AR-042 should snapshot with box from page", () => {
      page.setContent(`
        <button style="position:absolute;left:100px;top:50px;width:80px;height:40px;margin:0;padding:0;border:0;">click</button>
      `);

      const snapshot = page.ariaSnapshot({ boxes: true });
      expect_eq(snapshot, `- button "click" [box=100,50,80,40]`);
    });
    test("AR-043 should snapshot with box from locator", () => {
      page.setContent(`
        <div style="position:absolute;left:10px;top:20px;width:200px;height:100px;">
          <button style="position:absolute;left:5px;top:5px;width:60px;height:30px;margin:0;padding:0;border:0;">ok</button>
        </div>
      `);

      const snapshot = page.locator("div").ariaSnapshot({ boxes: true });
      expect_eq(snapshot, `- button "ok" [box=15,25,60,30]`);
    });
    test("AR-044 should not include box when option is omitted", () => {
      page.setContent(`<button>click</button>`);
      const snapshot = page.ariaSnapshot();
      expect(snapshot).not.toMatch("\\[box=");
    });
    test("AR-045 should not include hidden iframes", () => {
      page.setContent(`
        <iframe name="visible" srcdoc="<button>Visible</button>"></iframe>
        <iframe name="hidden" style="visibility: hidden" srcdoc="<button>Hidden</button>"></iframe>
        <iframe name="none" style="display: none" srcdoc="<button>None</button>"></iframe>
      `);
      expect(page.ariaSnapshot()).toBe(unshift(`
        - iframe
      `));
    });
    test("AR-171 should include inert elements and elements outside of a modal dialog", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/36938" } }, (_p) => {
      page.setContent(`
        <button>Hi</button>
        <button inert>Hello</button>
        <div inert>
          <button>Yay</button>
        </div>
        <dialog id=dialog>
          <button>Inside</button>
        </dialog>
        <script>
          document.getElementById('dialog').showModal();
        </script>
      `);

      checkAndMatchSnapshot(page.locator("body"), `
        - button "Hi"
        - button "Hello"
        - button "Yay"
        - dialog:
          - button "Inside"
      `);
    });
}
