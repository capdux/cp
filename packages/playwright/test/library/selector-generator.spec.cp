// Migrated from TypeScript (library/selector-generator.spec.ts) via cap from -ts

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

import { contextTest as it, expect } from "../config/browserTest.cp";
// [cap-migrate] import { Page, Frame } from "playwright";

async fn generate(pageOrFrame: string, target: string, expected: string? = null): string {
  // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: return pageOrFrame.$eval(target, (e, expected) => {
    // [ts-skip] const playwright = (window as any).playwright;
    // [ts-skip] const selector = playwright.selector(e);
    // [ts-skip] const expectedTarget = expected ? playwright.$(expected) : e;
    // [ts-skip] if (playwright.$(selector) === expectedTarget)
      // [ts-skip] return selector;
    // [ts-skip] return 'FAILED: ' + selector;
  // [ts-skip] }, expected);
}

async fn generateNoText(pageOrFrame: string, target: string): string {
  // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: return pageOrFrame.$eval(target, e => (window as any).__injectedScript.generateSelector(e, { noText: true, testIdAttributeName: 'data-testid' }).selector);
}

// TODO(cap-migrate): refine TS statement
// [ts] it.describe("selector generator", () => {
// [ts] test((_p) => mode != "default");
// [ts] it.beforeEach((_p) => {
// Make sure `page`(fixture) is created before enabling recorder, so that
// we properly wait for `extendInjectedScript` call to finish. Otherwise
// if the page is created later, there is a race between ConsoleAPI
// initialization and playwright.selector(e) call in `generate()` function above.
// [ts] (context)._showRecorder({ language: "javascript" });
// [ts] });
// [ts] test("GN-028 should prefer button over inner span", () => {
// [ts] page.setContent(`<button><span>text</span></button>`);
// [ts] expect_eq(generate(page, "span", "button"), "internal:role=button[name=\"text\"i]");
// [ts] });
// [ts] test("GN-029 should prefer role=button over inner span", () => {
// [ts] page.setContent(`<div role=button><span>text</span></div>`);
// [ts] expect_eq(generate(page, "span", "div"), "internal:role=button[name=\"text\"i]");
// [ts] });
// [ts] test("GN-030 should not prefer zero-sized button over inner span", () => {
// [ts] page.setContent(`
// [ts] <button style="width:0;height:0;padding:0;border:0;overflow:visible;">
// [ts] <span style="width:100px;height:100px;">text</span>
// [ts] </button>
// [ts] `);
// [ts] expect_eq(generate(page, "span"), "internal:text=\"text\"i");
// [ts] });
// [ts] test("GN-031 should generate text and normalize whitespace", () => {
// [ts] page.setContent(`<div>Text  some\n\n\n more \t text   </div>`);
// [ts] expect_eq(generate(page, "div"), "internal:text=\"Text some more text\"i");
// [ts] });
// [ts] test("GN-032 should not escape spaces inside named attr selectors", () => {
// [ts] page.setContent(`<input placeholder="Foo b ar"/>`);
// [ts] expect_eq(generate(page, "input"), "internal:role=textbox[name=\"Foo b ar\"i]");
// [ts] });
// [ts] test("GN-033 should generate text for <input type=button>", () => {
// [ts] page.setContent(`<input type=button value="Click me">`);
// [ts] expect_eq(generate(page, "input"), "internal:role=button[name=\"Click me\"i]");
// [ts] });
// [ts] test("GN-034 should trim text", () => {
// [ts] page.setContent(`
// [ts] <div>Text0123456789Text0123456789Text0123456789Text0123456789Text0123456789Text0123456789Text0123456789Text0123456789Text0123456789Text0123456789</div>
// [ts] <div>Text0123456789Text0123456789Text0123456789Text0123456789Text0123456789!Text0123456789Text0123456789Text0123456789Text0123456789Text0123456789</div>
// [ts] `);
// [ts] expect_eq(generate(page, "div"), "internal:text=\"Text0123456789Text0123456789Text0123456789Text0123456789Text0123456789Text012345\"i");
// [ts] });
// [ts] test("GN-035 should try to improve role name", () => {
// [ts] page.setContent(`<div role=button>Issues 23</div>`);
// [ts] expect_eq(generate(page, "div"), "internal:role=button[name=\"Issues\"i]");
// [ts] });
// [ts] test("GN-036 should use description when name is not unique", () => {
// [ts] page.setContent(`
// [ts] <button aria-description="Upload report">Submit</button>
// [ts] <button aria-description="Upload photo">Submit</button>
// [ts] `);
// [ts] expect_eq(generate(page, "button[aria-description=\"Upload report\"]"), "internal:role=button[name=\"Submit\"i][description=\"Upload report\"i]");
// [ts] expect_eq(generate(page, "button[aria-description=\"Upload photo\"]"), "internal:role=button[name=\"Submit\"i][description=\"Upload photo\"i]");
// [ts] });
// [ts] test("GN-037 should not use description when name is unique", () => {
// [ts] page.setContent(`
// [ts] <button aria-description="Some description">Submit</button>
// [ts] <button aria-description="Other description">Cancel</button>
// [ts] `);
// [ts] expect_eq(generate(page, "button[aria-description=\"Some description\"]"), "internal:role=button[name=\"Submit\"i]");
// [ts] });
// [ts] test("GN-038 should use description from aria-describedby", () => {
// [ts] page.setContent(`
// [ts] <span id="desc1">Save form data</span>
// [ts] <span id="desc2">Save as draft</span>
// [ts] <button aria-describedby="desc1">Submit</button>
// [ts] <button aria-describedby="desc2">Submit</button>
// [ts] `);
// [ts] expect_eq(generate(page, "button[aria-describedby=\"desc1\"]"), "internal:role=button[name=\"Submit\"i][description=\"Save form data\"i]");
// [ts] expect_eq(generate(page, "button[aria-describedby=\"desc2\"]"), "internal:role=button[name=\"Submit\"i][description=\"Save as draft\"i]");
// [ts] });
// [ts] test("GN-039 should fall back to nth when name and description are both not unique", () => {
// [ts] page.setContent(`
// [ts] <button aria-description="Same desc">Submit</button>
// [ts] <button aria-description="Same desc">Submit</button>
// [ts] `);
// [ts] expect_eq(generate(page, "button:first-of-type"), "internal:role=button[name=\"Submit\"i] >> nth=0");
// [ts] });
// [ts] test("GN-040 should use consistent exactness for name and description", {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41032" }
// [ts] }, (_p) => {
// The public getByRole API has a single `exact` flag that covers both
// `name` and `description`, so the producer must not emit a candidate
// that mixes substring name with exact description (or vice versa).
// [ts] page.setContent(`
// [ts] <table>
// [ts] <tr title="table row 1">
// [ts] <td>foo</td><td>bar</td><td>baz</td><td>foo</td><td>bar</td><td>baz</td><td>foo</td><td>bar</td><td>baz</td>
// [ts] </tr>
// [ts] <tr title="table row 2">
// [ts] <td>foo</td><td>bar</td><td>baz</td><td>foo</td><td>bar</td><td>baz</td><td>foo</td><td>bar</td><td>baz</td>
// [ts] </tr>
// [ts] </table>
// [ts] `);
// [ts] const selector = generate(page, "tr[title=\"table row 1\"]");
// Both name and description must share the same case-sensitivity flag.
// [ts] expect(selector).not.toMatch("\\[name=\"[^\"]*\"i\\]\\[description=\"[^\"]*\"s\\]");
// [ts] expect(selector).not.toMatch("\\[name=\"[^\"]*\"s\\]\\[description=\"[^\"]*\"i\\]");
// [ts] });
// [ts] test("GN-041 should use description when role has no name", () => {
// [ts] page.setContent(`
// [ts] <div role="alert" aria-description="Error in field A"></div>
// [ts] <div role="alert" aria-description="Error in field B"></div>
// [ts] `);
// [ts] expect_eq(generate(page, "div[aria-description=\"Error in field A\"]"), "internal:role=alert[description=\"Error in field A\"s]");
// [ts] expect_eq(generate(page, "div[aria-description=\"Error in field B\"]"), "internal:role=alert[description=\"Error in field B\"s]");
// [ts] });
// [ts] test("GN-042 should try to improve text", () => {
// [ts] page.setContent(`<div>23 Issues</div>`);
// [ts] expect_eq(generate(page, "div"), "internal:text=\"Issues\"i");
// [ts] });
// [ts] test("GN-043 should try to improve text by shortening", () => {
// [ts] page.setContent(`<div>Longest verbose description of the item</div>`);
// [ts] expect_eq(generate(page, "div"), "internal:text=\"Longest verbose description\"i");
// [ts] });
// [ts] test("GN-044 should try to improve label text by shortening", () => {
// [ts] page.setContent(`<label>Longest verbose description of the item<input></label>`);
// [ts] expect_eq(generate(page, "input"), "internal:role=textbox[name=\"Longest verbose description\"i]");
// [ts] });
// [ts] test("GN-045 should not improve guid text", () => {
// [ts] page.setContent(`<div>91b1b23</div>`);
// [ts] expect_eq(generate(page, "div"), "internal:text=\"91b1b23\"i");
// [ts] });
// [ts] test("GN-046 should not escape text with >>", () => {
// [ts] page.setContent(`<div>text&gt;&gt;text</div>`);
// [ts] expect_eq(generate(page, "div"), "internal:text=\"text>>text\"i");
// [ts] });
// [ts] test("GN-047 should escape text with quote", () => {
// [ts] page.setContent(`<div>text"text</div>`);
// [ts] expect_eq(generate(page, "div"), "internal:text=\"text\\\"text\"i");
// [ts] });
// [ts] test("GN-048 should escape text with slash", () => {
// [ts] page.setContent(`<div>/text</div>`);
// [ts] expect_eq(generate(page, "div"), "internal:text=\"\/text\"i");
// [ts] });
// [ts] test("GN-049 should not use text for select", () => {
// [ts] page.setContent(`
// [ts] <select><option>foo</option></select>
// [ts] <select mark=1><option>bar</option></select>
// [ts] `);
// [ts] expect_eq(generate(page, "[mark=\"1\"]"), "internal:role=combobox >> nth=1");
// [ts] });
// [ts] test("GN-050 should use ordinal for identical nodes", () => {
// [ts] page.setContent(`<div>Text</div><div>Text</div><div mark=1>Text</div><div>Text</div>`);
// [ts] expect_eq(generate(page, "div[mark=\"1\"]"), `internal:text="Text"i >> nth=2`);
// [ts] });
// [ts] test("GN-051 should prefer data-testid", () => {
// [ts] page.setContent(`<div>Text</div><div>Text</div><div data-testid=a>Text</div><div>Text</div>`);
// [ts] expect_eq(generate(page, "[data-testid=\"a\"]"), "internal:testid=[data-testid=\"a\"s]");
// [ts] });
// [ts] test("GN-052 should use data-testid in strict errors", () => {
// [ts] const content = `
// [ts] <div>
// [ts] <div></div>
// [ts] <div>
// [ts] <div></div>
// [ts] <div></div>
// [ts] </div>
// [ts] </div>
// [ts] <div>
// [ts] <div class='foo bar:0' data-custom-id='One'>
// [ts] </div>
// [ts] <div class='foo bar:1' data-custom-id='Two'>
// [ts] </div>
// [ts] </div>
// [ts] `;
// [ts] const checkPage = (page) => {
// [ts] page.setContent(content);
// [ts] const error = page.locator(".foo").hover().catch(e => e);
// [ts] expect_eq(std.mem.indexOf(u8, error.message, "strict mode violation") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, error.message, "<div class=\"foo bar:0") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, error.message, "<div class=\"foo bar:1") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, error.message, `aka getByTestId('One')`) != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, error.message, `aka getByTestId('Two')`) != null, true);
// [ts] };
// [ts] playwright.selectors.setTestIdAttribute("data-custom-id");
// Check page and context that were created before setting the attribute.
// [ts] checkPage(page);
// [ts] const context2 = contextFactory();
// [ts] const page2 = context2.newPage();
// Check page and context that were created after setting the attribute.
// [ts] checkPage(page2);
// [ts] });
// [ts] test("GN-053 should handle first non-unique data-testid", () => {
// [ts] page.setContent(`
// [ts] <div data-testid=a mark=1>
// [ts] Text
// [ts] </div>
// [ts] <div data-testid=a>
// [ts] Text
// [ts] </div>`);
// [ts] expect_eq(generate(page, "div[mark=\"1\"]"), "internal:testid=[data-testid=\"a\"s] >> nth=0");
// [ts] });
// [ts] test("GN-054 should handle second non-unique data-testid", () => {
// [ts] page.setContent(`
// [ts] <div data-testid=a>
// [ts] Text
// [ts] </div>
// [ts] <div data-testid=a mark=1>
// [ts] Text
// [ts] </div>`);
// [ts] expect_eq(generate(page, "div[mark=\"1\"]"), `internal:testid=[data-testid=\"a\"s] >> nth=1`);
// [ts] });
// [ts] test("GN-055 should use readable id", () => {
// [ts] page.setContent(`
// [ts] <div></div>
// [ts] <div id=first-item mark=1></div>
// [ts] `);
// [ts] expect_eq(generate(page, "div[mark=\"1\"]"), "#first-item");
// [ts] });
// [ts] test("GN-056 should not use generated id", () => {
// [ts] page.setContent(`
// [ts] <div></div>
// [ts] <div id=aAbBcCdDeE mark=1></div>
// [ts] `);
// [ts] expect_eq(generate(page, "div[mark=\"1\"]"), `div >> nth=1`);
// [ts] });
// [ts] test("GN-057 should use internal:has-text", () => {
// [ts] page.setContent(`
// [ts] <div>Hello world</div>
// [ts] <a>Hello <span>world</span></a>
// [ts] <a>Goodbye <span>world</span></a>
// [ts] `);
// [ts] expect_eq(generate(page, "a:has-text(\"Hello\")"), `a >> internal:has-text="Hello world"i`);
// [ts] });
// [ts] test("GN-058 should use internal:has-text with regexp", () => {
// [ts] page.setContent(`
// [ts] <span>Hello world</span>
// [ts] <div><div>Hello <span>world</span></div>extra</div>
// [ts] <a>Goodbye <span>world</span></a>
// [ts] `);
// [ts] expect_eq(generate(page, "div div"), `div >> internal:has-text=/^Hello world$/`);
// [ts] });
// [ts] test("GN-059 should use internal:has-text with regexp with a quote", () => {
// [ts] page.setContent(`
// [ts] <span>Hello'world</span>
// [ts] <div><div>Hello'<span>world</span></div>extra</div>
// [ts] <a>Goodbye'<span>world</span></a>
// [ts] `);
// [ts] expect_eq(generate(page, "div div"), `div >> internal:has-text=/^Hello\\'world$/`);
// [ts] });
// [ts] test("GN-060 should chain text after parent", () => {
// [ts] page.setContent(`
// [ts] <div>Hello <span>world</span></div>
// [ts] <b>Hello <span mark=1>world</span></b>
// [ts] `);
// [ts] expect_eq(generate(page, "[mark=\"1\"]"), `b >> internal:text="world"i`);
// [ts] });
// [ts] test("GN-061 should use parent text", () => {
// [ts] page.setContent(`
// [ts] <div>Hello <span>world</span></div>
// [ts] <div>Goodbye <span mark=1>world</span></div>
// [ts] `);
// [ts] expect_eq(generate(page, "[mark=\"1\"]"), `div >> internal:has-text="Goodbye world"i >> span`);
// [ts] });
// [ts] test("GN-062 should separate selectors by >>", () => {
// [ts] page.setContent(`
// [ts] <div>
// [ts] <div>Text</div>
// [ts] </div>
// [ts] <div id="id">
// [ts] <div>Text</div>
// [ts] </div>
// [ts] `);
// [ts] expect_eq(generate(page, "#id > div"), "#id >> internal:text=\"Text\"i");
// [ts] });
// [ts] test("GN-063 should trim long text", () => {
// [ts] page.setContent(`
// [ts] <div>
// [ts] <div>Text that goes on and on and on and on and on and on and on and on and on and on and on and on and on and on and on</div>
// [ts] </div>
// [ts] <div id="id">
// [ts] <div>Text that goes on and on and on and on and on and on and on and on and on and on and on and on and on and on and on</div>
// [ts] <div>Text that goes on and on and on and on and on and on and on and on and X on and on and on and on and on and on and on</div>
// [ts] </div>
// [ts] `);
// [ts] expect_eq(generate(page, "#id > div"), `#id >> internal:text="Text that goes on and on and on and on and on and on and on and on and on and"i`);
// [ts] });
// [ts] test("GN-064 should use nested ordinals", () => {
// [ts] page.setContent(`
// [ts] <div><c></c><c></c><c></c><c></c><c></c><b></b></div>
// [ts] <div>
// [ts] <b>
// [ts] <c>
// [ts] </c>
// [ts] </b>
// [ts] <b>
// [ts] <c mark=1></c>
// [ts] </b>
// [ts] </div>
// [ts] <div><b></b></div>
// [ts] `);
// [ts] expect_eq(generate(page, "c[mark=\"1\"]"), "b:nth-child(2) > c");
// [ts] });
// [ts] test("GN-065 should prefer class to ordinal", () => {
// [ts] page.setContent(`
// [ts] <div><c></c><c></c><c></c><c></c><c></c><b></b></div>
// [ts] <div>
// [ts] <b class="foo">
// [ts] <c>
// [ts] </c>
// [ts] </b>
// [ts] <b>
// [ts] <c mark=1></c>
// [ts] </b>
// [ts] </div>
// [ts] <div><b class="foo"></b></div>
// [ts] `);
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('[mark="1"]', c => c.parentElement.className = 'foo 12.bar.baz[&x]-_?"\'');
// [ts] expect_eq(generate(page, "c[mark=\"1\"]"), `.foo.\\31 2\\.bar\\.baz\\[\\&x\\]-_\\?\\"\\' > c`);
// [ts] });
// [ts] test("GN-066 should properly join child selectors under nested ordinals", () => {
// [ts] page.setContent(`
// [ts] <div><c></c><c></c><c></c><c></c><c></c><b></b></div>
// [ts] <div>
// [ts] <b>
// [ts] <div>
// [ts] <c>
// [ts] </c>
// [ts] </div>
// [ts] </b>
// [ts] <b>
// [ts] <div>
// [ts] <c mark=1></c>
// [ts] </div>
// [ts] </b>
// [ts] </div>
// [ts] <div><b></b></div>
// [ts] `);
// [ts] expect_eq(generate(page, "c[mark=\"1\"]"), "b:nth-child(2) > div > c");
// [ts] });
// [ts] test("GN-067 should not use input[value]", () => {
// [ts] page.setContent(`
// [ts] <input value="one">
// [ts] <input value="two" mark="1">
// [ts] <input value="three">
// [ts] `);
// [ts] expect_eq(generate(page, "input[mark=\"1\"]"), "internal:role=textbox >> nth=1");
// [ts] });
// [ts] it.describe("should prioritize attributes correctly", () => {
// [ts] test("GN-068 role", () => {
// [ts] page.setContent(`<input name="foobar" type="text"/>`);
// [ts] expect_eq(generate(page, "input"), "internal:role=textbox");
// [ts] });
// [ts] test("GN-069 placeholder", () => {
// [ts] page.setContent(`<input placeholder="foobar" type="text"/>`);
// [ts] expect_eq(generate(page, "input"), "internal:role=textbox[name=\"foobar\"i]");
// [ts] });
// [ts] test("GN-070 name", () => {
// [ts] page.setContent(`
// [ts] <input aria-hidden="false" name="foobar" type="date"/>
// [ts] <div role="textbox"/>content</div>
// [ts] `);
// [ts] expect_eq(generate(page, "input"), "input[name=\"foobar\"]");
// [ts] });
// [ts] test("GN-071 type", () => {
// [ts] page.setContent(`
// [ts] <input aria-hidden="false" type="checkbox"/>
// [ts] <div role="checkbox"/>content</div>
// [ts] `);
// [ts] expect_eq(generate(page, "input"), "input[type=\"checkbox\"]");
// [ts] });
// [ts] });
// [ts] test("GN-072 should find text in shadow dom", () => {
// [ts] page.setContent(`<div></div>`);
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', div => {
// [ts-skip] const shadowRoot = div.attachShadow({ mode: 'open' });
// [ts-skip] const span = document.createElement('span');
// [ts-skip] span.textContent = 'Target';
// [ts-skip] shadowRoot.appendChild(span);
// [ts-skip] });
// [ts] expect_eq(generate(page, "span"), "internal:text=\"Target\"i");
// [ts] });
// [ts] test("GN-073 should match in shadow dom", () => {
// [ts] page.setContent(`<div></div>`);
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', div => {
// [ts-skip] const shadowRoot = div.attachShadow({ mode: 'open' });
// [ts-skip] const input = document.createElement('input');
// [ts-skip] shadowRoot.appendChild(input);
// [ts-skip] });
// [ts] expect_eq(generate(page, "input"), "internal:role=textbox");
// [ts] });
// [ts] test("GN-074 should match in deep shadow dom", () => {
// [ts] page.setContent(`<div></div><div></div><div><input></div>`);
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', div1 => {
// [ts-skip] const shadowRoot1 = div1.attachShadow({ mode: 'open' });
// [ts-skip] const input1 = document.createElement('input');
// [ts-skip] shadowRoot1.appendChild(input1);
// [ts-skip] const divExtra3 = document.createElement('div');
// [ts-skip] shadowRoot1.append(divExtra3);
// [ts-skip] const div2 = document.createElement('div');
// [ts-skip] shadowRoot1.append(div2);
// [ts-skip] const shadowRoot2 = div2.attachShadow({ mode: 'open' });
// [ts-skip] const input2 = document.createElement('input');
// [ts-skip] input2.setAttribute('value', 'foo');
// [ts-skip] shadowRoot2.appendChild(input2);
// [ts-skip] });
// [ts] expect_eq(generate(page, "input[value=foo]"), "internal:role=textbox >> nth=2");
// [ts] });
// [ts] test("GN-075 should work in dynamic iframes without navigation", () => {
// [ts] page.setContent(`<div></div>`);
// [ts] const [frame] = joinAll([ // migrated: Promise.all -> joinAll
// [ts] page.waitForEvent("frameattached"),
// [ts] page.evaluate(() => {
// [ts] return Promise.init(f => {
// [ts] const iframe = document.createElement("iframe");
// [ts] iframe.onload = () => {
// [ts] iframe.contentDocument.body.innerHTML = "<div>Target</div>";
// [ts] f();
// [ts] };
// [ts] document.body.appendChild(iframe);
// [ts] });
// [ts] }),
// [ts] ]);
// [ts] expect_eq(generate(frame, "div"), "internal:text=\"Target\"i");
// [ts] });
// [ts] test("GN-076 should use the name attributes for elements that can have it", () => {
// [ts] for (const tagName of ["button", "input", "textarea"]) {
// [ts] page.setContent(`<form><${tagName} name="foo"></${tagName}><${tagName} name="bar"></${tagName}></form>`);
// [ts] expect_eq(generate(page, "[name=bar]"), `${tagName}[name="bar"]`);
// [ts] }
// [ts] page.setContent(`<iframe name="foo"></iframe><iframe name="bar"></iframe>`);
// [ts] expect_eq(generate(page, "[name=bar]"), `iframe[name="bar"]`);
// [ts] page.setContent(`<frameset><frame name="foo"></frame><frame name="bar"></frame></frameset>`);
// [ts] expect_eq(generate(page, "[name=bar]"), `frame[name="bar"]`);
// [ts] });
// [ts] test("GN-077 should work with tricky attributes", () => {
// [ts] page.setContent(`<button id="this:is-my-tricky.id"><span></span></button>`);
// [ts] expect_eq(generate(page, "button"), "[id=\"this:is-my-tricky.id\"]");
// [ts] page.setContent(`<ng:switch><span></span></ng:switch>`);
// [ts] expect_eq(generate(page, "ng\\:switch"), "ng\\:switch");
// [ts] page.setContent(`<button><span></span></button><button></button>`);
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('span', span => span.textContent = `!#'!?"\\:`);
// [ts] expect_eq(generate(page, "button"), `internal:role=button[name="!#'!?\\"\\\\:"i]`);
// [ts] page.setContent(`<div><span></span></div>`);
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', div => div.id = `!#'!?"\\:`);
// [ts] expect_eq(generate(page, "div"), `[id="!#'!?\\"\\\\:"]`);
// [ts] });
// [ts] test("GN-078 should work without CSS.escape", () => {
// [ts] page.setContent(`<button aria-hidden="false"></button><div role="button"></div>`);
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('button', button => {
// [ts-skip] delete window.CSS.escape;
// [ts-skip] button.setAttribute('name', '-tricky\u0001name');
// [ts-skip] });
// [ts] expect_eq(generate(page, "button"), `button[name="-tricky\u0001name"]`);
// [ts] });
// [ts] test("GN-079 should not over-escape for CSS syntax", () => {
// [ts] page.setContent(`<button aria-hidden="false" name="123"></button><div role="button"></div>`);
// [ts] expect_eq(generate(page, "button"), `button[name="123"]`);
// [ts] });
// [ts] test("GN-080 should ignore empty aria-label for candidate consideration", () => {
// [ts] page.setContent(`<button aria-label="" id="buttonId"></button>`);
// [ts] expect_eq(generate(page, "button"), "#buttonId");
// [ts] });
// [ts] test("GN-081 should accept valid aria-label for candidate consideration", () => {
// [ts] page.setContent(`<button aria-label="ariaLabel" id="buttonId"></button>`);
// [ts] expect_eq(generate(page, "button"), "internal:role=button[name=\"ariaLabel\"i]");
// [ts] });
// [ts] test("GN-082 should generate title selector", () => {
// [ts] page.setContent(`<div>
// [ts] <button title="Send to" aria-description="High-Speed Parcel Delivery">Send</button>
// [ts] <button aria-description="High-Speed Parcel Delivery">Send</button>
// [ts] </div>`);
// [ts] expect_eq(generate(page, "button"), "internal:attr=[title=\"Send to\"i]");
// [ts] });
// [ts] test("GN-083 should ignore empty role for candidate consideration", () => {
// [ts] page.setContent(`<button role="" id="buttonId"></button>`);
// [ts] expect_eq(generate(page, "button"), "#buttonId");
// [ts] });
// [ts] test("GN-084 should not accept invalid role for candidate consideration", () => {
// [ts] page.setContent(`<button role="roleDescription" id="buttonId"></button>`);
// [ts] expect_eq(generate(page, "button"), "#buttonId");
// [ts] });
// [ts] test("GN-085 should ignore empty data-test-id for candidate consideration", () => {
// [ts] page.setContent(`<button data-test-id="" id="buttonId"></button>`);
// [ts] expect_eq(generate(page, "button"), "#buttonId");
// [ts] });
// [ts] test("GN-086 should accept valid data-test-id for candidate consideration", () => {
// [ts] page.setContent(`<button data-test-id="testId" id="buttonId"></button>`);
// [ts] expect_eq(generate(page, "button"), "[data-test-id=\"testId\"]");
// [ts] });
// [ts] test("GN-087 should generate label selector", () => {
// [ts] page.setContent(`
// [ts] <label for=target1>Target1</label><input id=target1>
// [ts] <label for=target2>Target2</label><button id=target2>??</button>
// [ts] <label for=target3>Target3</label><select id=target3><option>hey</option></select>
// [ts] <label for=target4>Target4</label><progress id=target4 value=70 max=100>70%</progress>
// [ts] <label for=target5>Target5</label><input id=target5 type=hidden>
// [ts] <label for=target6>Target6</label><div id=target6>text</div>
// [ts] `);
// [ts] expect.soft(generate(page, "#target1")).toBe("internal:role=textbox[name=\"Target1\"i]");
// [ts] expect.soft(generate(page, "#target2")).toBe("internal:role=button[name=\"Target2\"i]");
// [ts] expect.soft(generate(page, "#target3")).toBe("internal:label=\"Target3\"i");
// [ts] expect.soft(generate(page, "#target4")).toBe("internal:role=progressbar[name=\"Target4\"i]");
// [ts] expect.soft(generate(page, "#target5")).toBe("#target5");
// [ts] expect.soft(generate(page, "#target6")).toBe("internal:text=\"text\"i");
// [ts] page.setContent(`<label for=target>Coun"try</label><input id=target>`);
// [ts] expect_eq(generate(page, "input"), "internal:role=textbox[name=\"Coun\\\"try\"i]");
// [ts] });
// [ts] test("GN-088 should prefer role other input[type]", () => {
// [ts] page.setContent(`<input type=checkbox><div data-testid=wrapper><input type=checkbox></div>`);
// [ts] expect_eq(generate(page, "[data-testid=wrapper] > input"), "internal:testid=[data-testid=\"wrapper\"s] >> internal:role=checkbox");
// [ts] });
// [ts] test("GN-089 should generate exact text when necessary", () => {
// [ts] page.setContent(`
// [ts] <span>Text</span>
// [ts] <span>Text and more</span>
// [ts] `);
// [ts] expect_eq(generate(page, "span"), "internal:text=\"Text\"s");
// [ts] });
// [ts] test("GN-090 should generate exact title when necessary", () => {
// [ts] page.setContent(`
// [ts] <span title="Text"></span>
// [ts] <span title="Text and more"></span>
// [ts] `);
// [ts] expect_eq(generate(page, "span"), "internal:attr=[title=\"Text\"s]");
// [ts] });
// [ts] test("GN-091 should generate exact placeholder when necessary", () => {
// [ts] page.setContent(`
// [ts] <input placeholder="Text"></input>
// [ts] <input placeholder="Text and more"></input>
// [ts] `);
// [ts] expect_eq(generate(page, "input"), "internal:role=textbox[name=\"Text\"s]");
// [ts] });
// [ts] test("GN-092 should generate exact role when necessary", () => {
// [ts] page.setContent(`
// [ts] <img alt="Text"></img>
// [ts] <img alt="Text and more"></img>
// [ts] `);
// [ts] expect_eq(generate(page, "img"), "internal:role=img[name=\"Text\"s]");
// [ts] });
// [ts] test("GN-093 should generate exact label when necessary", () => {
// [ts] page.setContent(`
// [ts] <label>Text <input></input></label>
// [ts] <label>Text and more <input></input></label>
// [ts] `);
// [ts] expect_eq(generate(page, "input"), "internal:role=textbox[name=\"Text\"s]");
// [ts] });
// [ts] test("GN-094 should generate relative selector", () => {
// [ts] page.setContent(`
// [ts] <div>
// [ts] <span>Hello</span>
// [ts] <span>World</span>
// [ts] </div>
// [ts] <section>
// [ts] <span>Hello</span>
// [ts] <span>World</span>
// [ts] </section>
// [ts] `);
// [ts] const selectors = page.evaluate(() => {
// [ts] const target = document.querySelector("section > span");
// [ts] const root = document.querySelector("section");
// [ts] const relative = (window).__injectedScript.generateSelectorSimple(target, { root });
// [ts] const absolute = (window).__injectedScript.generateSelectorSimple(target);
// [ts] return { relative, absolute };
// [ts] });
// [ts] expect: (selectors) => {
// [ts] relative: `internal:text="Hello"i`,
// [ts] absolute: `section >> internal:text="Hello"i`,
// [ts] });
// [ts] });
// [ts] test("GN-095 should generate noText: no text engine", () => {
// [ts] page.setContent(`<div>Some text</div>`);
// [ts] expect_eq(generateNoText(page, "div"), `div`);
// [ts] });
// [ts] test("GN-096 should generate noText: no name from content", () => {
// [ts] page.setContent(`<button><span>Send</span></button>`);
// [ts] expect_eq(generateNoText(page, "button"), `internal:role=button`);
// [ts] });
// [ts] test("GN-097 should generate noText: name from aria-label", () => {
// [ts] page.setContent(`<button aria-label="Send message"><span>Send</span></button>`);
// [ts] expect_eq(generateNoText(page, "button"), `internal:role=button[name="Send message"i]`);
// [ts] });
// [ts] test("GN-098 should generate noText: name from external aria-labelledby", () => {
// [ts] page.setContent(`<span id="label">Editor</span><div role="textbox" aria-labelledby="label" contenteditable>Text</div>`);
// [ts] expect_eq(generateNoText(page, "div[role=textbox]"), `internal:role=textbox[name="Editor"i]`);
// [ts] });
// [ts] test("GN-099 should generate noText: ignore aria-labelledby pointing inside the element", () => {
// [ts] page.setContent(`<div role="textbox" aria-labelledby="child" contenteditable><p id="child">Title</p><p>Text</p></div>`);
// [ts] expect_eq(generateNoText(page, "div[role=textbox]"), `internal:role=textbox`);
// [ts] });
// [ts] test("GN-100 should generate noText: contenteditable heading", () => {
// [ts] page.setContent(`<h1 contenteditable>Page title</h1>`);
// [ts] expect_eq(generateNoText(page, "h1"), `internal:role=heading`);
// [ts] });
// [ts] test("GN-101 should generate noText: description from external aria-describedby", () => {
// [ts] page.setContent(`
// [ts] <span id="desc1">First</span><span id="desc2">Second</span>
// [ts] <div role="textbox" aria-label="Editor" aria-describedby="desc1" contenteditable>foo</div>
// [ts] <div role="textbox" aria-label="Editor" aria-describedby="desc2" contenteditable>bar</div>
// [ts] `);
// [ts] expect_eq(generateNoText(page, "div[aria-describedby=desc1]"), `internal:role=textbox[name="Editor"i][description="First"i]`);
// [ts] });
// [ts] test("GN-102 should generate noText: ignore aria-describedby pointing inside the element", () => {
// [ts] page.setContent(`
// [ts] <div role="textbox" aria-label="Editor" aria-describedby="child" contenteditable><p id="child">First</p></div>
// [ts] <div role="textbox" aria-label="Editor" contenteditable>Second</div>
// [ts] `);
// [ts] expect_eq(generateNoText(page, "div[aria-describedby=child]"), `internal:role=textbox[name="Editor"i] >> nth=0`);
// [ts] });
// [ts] test("GN-103 should prefer role with hasText to css with hasText", () => {
// [ts] page.setContent(`
// [ts] <ul>
// [ts] <li>
// [ts] <input aria-label="Toggle Todo" type="checkbox">
// [ts] buy flowers
// [ts] </li>
// [ts] <li>
// [ts] <input aria-label="Toggle Todo" type="checkbox">
// [ts] sell milk
// [ts] </li>
// [ts] </ul>
// [ts] `);
// [ts] expect_eq(generate(page, "input"), `internal:role=listitem >> internal:has-text=\"buy flowers\"i >> internal:label=\"Toggle Todo\"i`);
// [ts] });
// [ts] test("GN-104 should not use icon fonts aria name", () => {
// [ts] page.setContent(`
// [ts] <style>
// [ts] .icon-foo:before {
// [ts] content: "\\F1D4";
// [ts] }
// [ts] .icon-bar:before {
// [ts] content: "\\F0000";
// [ts] }
// [ts] </style>
// [ts] <div>
// [ts] <button><i class="icon-foo"></i></button>
// [ts] <button><i class="icon-bar"></i></button>
// [ts] </div>
// [ts] `);
// [ts] expect.soft(generate(page, "button:first-child")).toBe("internal:role=button >> nth=0");
// [ts] expect.soft(generate(page, "button:last-child")).toBe("internal:role=button >> nth=1");
// [ts] });
// [ts] test("GN-105 should not throw for slotted element when host shadow root has a twin", () => {
// [ts] page.setContent(`
// [ts] <span>Target</span>
// [ts] <div></div><div></div><div></div><div></div><div></div><div></div><div></div>
// [ts] <x-host><x-item class="c" data-text="Target"></x-item></x-host>
// [ts] <script>
// [ts] customElements.define('x-item', class extends HTMLElement {
// [ts] connectedCallback() {
// [ts] this.attachShadow({ mode: 'open' }).innerHTML = '<div class="k"><span>' + (this.dataset.text || '') + '</span></div>';
// [ts] }
// [ts] });
// [ts] customElements.define('x-host', class extends HTMLElement {
// [ts] connectedCallback() {
// [ts] this.attachShadow({ mode: 'open' }).innerHTML = '<x-item class="c"></x-item><slot></slot>';
// [ts] }
// [ts] });
// [ts] </script>
// [ts] `);
// [ts] expect_eq(generate(page, "x-item[data-text] span"), "x-host >> internal:text=\"Target\"i");
// [ts] });
// [ts] });
