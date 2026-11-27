// Migrated from TypeScript (page/selectors-text.spec.ts) via cap from -ts

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





















fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("SE-028 should work @smoke", () => {
    // [ts] page.setContent(`<div>yo</div><div>ya</div><div>\nye  </div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=ya`, e => e.outerHTML)).toBe('<div>ya</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text="ya"`, e => e.outerHTML)).toBe('<div>ya</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=/^[ay]+$/`, e => e.outerHTML)).toBe('<div>ya</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=/Ya/i`, e => e.outerHTML)).toBe('<div>ya</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=ye`, e => e.outerHTML)).toBe('<div>\nye  </div>');
    // [ts] expect_eq(std.mem.indexOf(u8, page.getByText("ye").evaluate(e => e.outerHTML), ">\nye  </div>") != null, true);
    // [ts] page.setContent(`<div> ye </div><div>ye</div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text="ye"`, e => e.outerHTML)).toBe('<div> ye </div>');
    // [ts] expect_eq(std.mem.indexOf(u8, page.getByText("ye", { exact: true }).first().evaluate(e => e.outerHTML), "> ye </div>") != null, true);
    // [ts] page.setContent(`<div>yo</div><div>"ya</div><div> hello world! </div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text="\\"ya"`, e => e.outerHTML)).toBe('<div>"ya</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=/hello/`, e => e.outerHTML)).toBe('<div> hello world! </div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=/^\\s*heLLo/i`, e => e.outerHTML)).toBe('<div> hello world! </div>');
    // [ts] page.setContent(`<div>yo<div>ya</div>hey<div>hey</div></div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=hey`, e => e.outerHTML)).toBe('<div>hey</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=yo>>text="ya"`, e => e.outerHTML)).toBe('<div>ya</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=yo>> text="ya"`, e => e.outerHTML)).toBe('<div>ya</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=yo >>text='ya'`, e => e.outerHTML)).toBe('<div>ya</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=yo >> text='ya'`, e => e.outerHTML)).toBe('<div>ya</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`'yo'>>"ya"`, e => e.outerHTML)).toBe('<div>ya</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`"yo" >> 'ya'`, e => e.outerHTML)).toBe('<div>ya</div>');
    // [ts] page.setContent(`<div>yo<span id="s1"></span></div><div>yo<span id="s2"></span><span id="s3"></span></div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`text=yo`, es => es.map(e => e.outerHTML).join('\n'))).toBe('<div>yo<span id="s1"></span></div>\n<div>yo<span id="s2"></span><span id="s3"></span></div>');
    // [ts] page.setContent(`<div>'</div><div>"</div><div>\\</div><div>x</div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text='\\''`, e => e.outerHTML)).toBe('<div>\'</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text='"'`, e => e.outerHTML)).toBe('<div>"</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text="\\""`, e => e.outerHTML)).toBe('<div>"</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text="'"`, e => e.outerHTML)).toBe('<div>\'</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text="\\x"`, e => e.outerHTML)).toBe('<div>x</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text='\\x'`, e => e.outerHTML)).toBe('<div>x</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text='\\\\'`, e => e.outerHTML)).toBe('<div>\\</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text="\\\\"`, e => e.outerHTML)).toBe('<div>\\</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text="`, e => e.outerHTML)).toBe('<div>"</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text='`, e => e.outerHTML)).toBe('<div>\'</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`"x"`, e => e.outerHTML)).toBe('<div>x</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`'x'`, e => e.outerHTML)).toBe('<div>x</div>');
    // [ts] let error = page.$(`"`).catch(e => e);
    // [ts] expect(error).toBeInstanceOf(Error);
    // [ts] error = page.$(`'`).catch(e => e);
    // [ts] expect(error).toBeInstanceOf(Error);
    // [ts] page.setContent(`<div> ' </div><div> " </div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text="`, e => e.outerHTML)).toBe('<div> " </div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text='`, e => e.outerHTML)).toBe('<div> \' </div>');
    // [ts] page.setContent(`<div>Hi''&gt;&gt;foo=bar</div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text="Hi''>>foo=bar"`, e => e.outerHTML)).toBe(`<div>Hi''&gt;&gt;foo=bar</div>`);
    // [ts] page.setContent(`<div>Hi'"&gt;&gt;foo=bar</div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text="Hi'\\">>foo=bar"`, e => e.outerHTML)).toBe(`<div>Hi'"&gt;&gt;foo=bar</div>`);
    // [ts] page.setContent(`<div>Hi&gt;&gt;<span></span></div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text="Hi>>">>span`, e => e.outerHTML)).toBe(`<span></span>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=/Hi\\>\\>/ >> span`, e => e.outerHTML)).toBe(`<span></span>`);
    // [ts] page.setContent(`<div>a<br>b</div><div>a</div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=a`, e => e.outerHTML)).toBe('<div>a<br>b</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=b`, e => e.outerHTML)).toBe('<div>a<br>b</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=ab`, e => e.outerHTML)).toBe('<div>a<br>b</div>');
    // [ts] expect_eq(page.$(`text=abc`), null);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`text=a`, els => els.length)).toBe(2);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`text=b`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`text=ab`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`text=abc`, els => els.length)).toBe(0);
    // [ts] page.setContent(`<div></div><span></span>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', div => {
    // [ts-skip] div.appendChild(document.createTextNode('hello'));
    // [ts-skip] div.appendChild(document.createTextNode('world'));
    // [ts-skip] });
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('span', span => {
    // [ts-skip] span.appendChild(document.createTextNode('hello'));
    // [ts-skip] span.appendChild(document.createTextNode('world'));
    // [ts-skip] });
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=lowo`, e => e.outerHTML)).toBe('<div>helloworld</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`text=lowo`, els => els.map(e => e.outerHTML).join(''))).toBe('<div>helloworld</div><span>helloworld</span>');
    // [ts] page.setContent(`<span>Sign&nbsp;in</span><span>Hello\n \nworld</span>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=Sign in`, e => e.outerHTML)).toBe('<span>Sign&nbsp;in</span>');
    // [ts] expect_eq((page.$$(`text=Sign \tin`)).length, 1);
    // [ts] expect_eq((page.$$(`text="Sign in"`)).length, 1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=lo wo`, e => e.outerHTML)).toBe('<span>Hello\n \nworld</span>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text="Hello world"`, e => e.outerHTML)).toBe('<span>Hello\n \nworld</span>');
    // [ts] expect_eq(page.$(`text="lo wo"`), null);
    // [ts] expect_eq((page.$$(`text=lo \nwo`)).length, 1);
    // [ts] expect_eq((page.$$(`text="lo \nwo"`)).length, 0);
    // [ts] page.setContent(`<div>let's<span>hello</span></div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=/let's/i >> span`, e => e.outerHTML)).toBe('<span>hello</span>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=/let\\'s/i >> span`, e => e.outerHTML)).toBe('<span>hello</span>');
    // [ts] });
    // [ts] test("SE-029 should work with :text", () => {
    // [ts] page.setContent(`<div>yo</div><div>ya</div><div>\nHELLO   \n world  </div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`:text("ya")`, e => e.outerHTML)).toBe('<div>ya</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`:text-is("ya")`, e => e.outerHTML)).toBe('<div>ya</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`:text("y")`, e => e.outerHTML)).toBe('<div>yo</div>');
    // [ts] expect_eq(page.$(`:text-is("Y")`), null);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`:text("hello world")`, e => e.outerHTML)).toBe('<div>\nHELLO   \n world  </div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`:text-is("HELLO world")`, e => e.outerHTML)).toBe('<div>\nHELLO   \n world  </div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`:text("lo wo")`, e => e.outerHTML)).toBe('<div>\nHELLO   \n world  </div>');
    // [ts] expect_eq(page.$(`:text-is("lo wo")`), null);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`:text-matches("^[ay]+$")`, e => e.outerHTML)).toBe('<div>ya</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`:text-matches("y", "g")`, e => e.outerHTML)).toBe('<div>yo</div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`:text-matches("Y", "i")`, e => e.outerHTML)).toBe('<div>yo</div>');
    // [ts] expect_eq(page.$(`:text-matches("^y$")`), null);
    // [ts] const error1 = page.$(`:text("foo", "bar")`).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error1.message, `"text" engine expects a single string`) != null, true);
    // [ts] const error2 = page.$(`:text(foo > bar)`).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error2.message, `"text" engine expects a single string`) != null, true);
    // [ts] });
    // [ts] test("SE-030 should support empty string", () => {
    // [ts] page.setContent(`<div></div><div>ya</div><div>\nHELLO   \n world  </div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div:text-is("")`, e => e.outerHTML)).toBe('<div></div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`div:text-is("")`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div:text("")`, e => e.outerHTML)).toBe('<div></div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`div:text("")`, els => els.length)).toBe(3);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div >> text=""`, e => e.outerHTML)).toBe('<div></div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`div >> text=""`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div >> text=/^$/`, e => e.outerHTML)).toBe('<div></div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`div >> text=/^$/`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div:text-matches("")`, e => e.outerHTML)).toBe('<div></div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`div:text-matches("")`, els => els.length)).toBe(3);
    // [ts] });
    // [ts] test("SE-031 should work across nodes", () => {
    // [ts] page.setContent(`<div id=target1>Hello<i>,</i> <span id=target2>world</span><b>!</b></div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`:text("Hello, world!")`, e => e.id)).toBe('target1');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`:text("Hello")`, e => e.id)).toBe('target1');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`:text("world")`, e => e.id)).toBe('target2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`:text("world")`, els => els.length)).toBe(1);
    // [ts] expect_eq(page.$(`:text("hello world")`), null);
    // [ts] expect_eq(page.$(`div:text("world")`), null);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=Hello, world!`, e => e.id)).toBe('target1');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=Hello`, e => e.id)).toBe('target1');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=world`, e => e.id)).toBe('target2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`text=world`, els => els.length)).toBe(1);
    // [ts] expect_eq(page.$(`text=hello world`), null);
    // [ts] expect_eq(page.$(`:text-is("Hello, world!")`), null);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`:text-is("Hello")`, e => e.id)).toBe('target1');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`:text-is("world")`, e => e.id)).toBe('target2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`:text-is("world")`, els => els.length)).toBe(1);
    // [ts] expect_eq(page.$(`text="Hello, world!"`), null);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text="Hello"`, e => e.id)).toBe('target1');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text="world"`, e => e.id)).toBe('target2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`text="world"`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`:text-matches(".*")`, e => e.nodeName)).toBe('I');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`:text-matches("world?")`, e => e.id)).toBe('target2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`:text-matches("world")`, els => els.length)).toBe(1);
    // [ts] expect_eq(page.$(`div:text(".*")`), null);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=/.*/`, e => e.nodeName)).toBe('I');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=/world?/`, e => e.id)).toBe('target2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`text=/world/`, els => els.length)).toBe(1);
    // [ts] });
    // [ts] test("SE-032 text-is() should ignore comments", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/33976" }
    // [ts] }, (_p) => {
    // [ts] page.setContent(`<div id=me>hel<!-- comment -->lo
    // [ts] <!-- comment -->
    // [ts] world</div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`:text-is("hello world")`, e => e.id)).toBe('me');
    // [ts] expect_eq(page.locator("div", { hasText: "hello world" }).getAttribute("id"), "me");
    // [ts] expect_eq(page.getByText("hello world", { exact: true }).getAttribute("id"), "me");
    // [ts] });
    // [ts] test("SE-033 should work with text nodes in quoted mode", () => {
    // [ts] page.setContent(`<div id=target1>Hello<span id=target2>wo  rld  </span>  Hi again  </div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text="Hello"`, e => e.id)).toBe('target1');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text="Hi again"`, e => e.id)).toBe('target1');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text="wo rld"`, e => e.id)).toBe('target2');
    // [ts] expect_eq(page.$(`text="Hellowo rld Hi again"`), null);
    // [ts] expect_eq(page.$(`text="Hellowo"`), null);
    // [ts] expect_eq(page.$(`text="Hellowo rld"`), null);
    // [ts] expect_eq(page.$(`text="wo rld Hi ag"`), null);
    // [ts] expect_eq(page.$(`text="again"`), null);
    // [ts] expect_eq(page.$(`text="hi again"`), null);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=hi again`, e => e.id)).toBe('target1');
    // [ts] });
    // [ts] test("SE-034 should clear caches", () => {
    // [ts] page.setContent(`<div id=target1>text</div><div id=target2>text</div>`);
    // [ts] const div = page.$("#target1");
    // [ts] div.evaluate(div => div.textContent = "text");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=text`, e => e.id)).toBe('target1');
    // [ts] div.evaluate(div => div.textContent = "foo");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=text`, e => e.id)).toBe('target2');
    // [ts] div.evaluate(div => div.textContent = "text");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`:text("text")`, e => e.id)).toBe('target1');
    // [ts] div.evaluate(div => div.textContent = "foo");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`:text("text")`, e => e.id)).toBe('target2');
    // [ts] div.evaluate(div => div.textContent = "text");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`text=text`, els => els.length)).toBe(2);
    // [ts] div.evaluate(div => div.textContent = "foo");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`text=text`, els => els.length)).toBe(1);
    // [ts] div.evaluate(div => div.textContent = "text");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`:text("text")`, els => els.length)).toBe(2);
    // [ts] div.evaluate(div => div.textContent = "foo");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`:text("text")`, els => els.length)).toBe(1);
    // [ts] });
    // [ts] test("SE-035 should work with :has-text", () => {
    // [ts] page.setContent(`
    // [ts] <input id=input2>
    // [ts] <div id=div1>
    // [ts] <span>  Find me  </span>
    // [ts] or
    // [ts] <wrap><span id=span2>maybe me  </span></wrap>
    // [ts] <div><input id=input1></div>
    // [ts] </div>
    // [ts] `);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`:has-text("find me")`, e => e.tagName)).toBe('HTML');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`span:has-text("find me")`, e => e.outerHTML)).toBe('<span>  Find me  </span>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div:has-text("find me")`, e => e.id)).toBe('div1');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div:has-text("find me") input`, e => e.id)).toBe('input1');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`:has-text("find me") input`, e => e.id)).toBe('input2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div:has-text("find me or maybe me")`, e => e.id)).toBe('div1');
    // [ts] expect_eq(page.$(`div:has-text("find noone")`), null);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`:is(div,span):has-text("maybe")`, els => els.map(e => e.id).join(';'))).toBe('div1;span2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div:has-text("find me") :has-text("maybe me")`, e => e.tagName)).toBe('WRAP');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div:has-text("find me") span:has-text("maybe me")`, e => e.id)).toBe('span2');
    // [ts] page.setContent(`<div id=me>hello
    // [ts] wo"r>>ld</div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div:has-text("hello wo\\"r>>ld")`, e => e.id)).toBe('me');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div:has-text("hello\\a wo\\"r>>ld")`, e => e.id)).toBe('me');
    // [ts] expect_eq(page.locator("div", { hasText: "hello\nwo\"r>>ld" }).getAttribute("id"), "me");
    // [ts] const error1 = page.$(`:has-text("foo", "bar")`).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error1.message, `"has-text" engine expects a single string`) != null, true);
    // [ts] const error2 = page.$(`:has-text(foo > bar)`).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error2.message, `"has-text" engine expects a single string`) != null, true);
    // [ts] });
    // [ts] test("SE-036 should work with large DOM", () => {
    // [ts] page.evaluate(() => {
    // [ts] let id = 0;
    // [ts] const next = (tag) => {
    // [ts] const e = document.createElement(tag);
    // [ts] const eid = ++id;
    // [ts] e.textContent = "id" + eid;
    // [ts] e.id = "id" + eid;
    // [ts] return e;
    // [ts] };
    // [ts] const generate = (depth) => {
    // [ts] const div = next("div");
    // [ts] const span1 = next("span");
    // [ts] const span2 = next("span");
    // [ts] div.appendChild(span1);
    // [ts] div.appendChild(span2);
    // [ts] if (depth > 0) {
    // [ts] div.appendChild(generate(depth - 1));
    // [ts] div.appendChild(generate(depth - 1));
    // [ts] }
    // [ts] return div;
    // [ts] };
    // [ts] document.body.appendChild(generate(12));
    // [ts] });
    // [ts] const selectors = [
    // [ts] ":has-text(\"id18\")",
    // [ts] ":has-text(\"id12345\")",
    // [ts] ":has-text(\"id\")",
    // [ts] ":text(\"id18\")",
    // [ts] ":text(\"id12345\")",
    // [ts] ":text(\"id\")",
    // [ts] ":text-matches(\"id12345\", \"i\")",
    // [ts] "text=id18",
    // [ts] "text=id12345",
    // [ts] "text=id",
    // [ts] "#id18",
    // [ts] "#id12345",
    // [ts] "*",
    // [ts] ];
    // [ts] const measure = false;
    // [ts] for (const selector of selectors) {
    // [ts] const time1 = Date.now();
    // [ts] let i = 0;
    // [ts] while (i < (if (measure) 10 else 1)) // [cap-migrate] for-step: i++
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$$eval(selector, els => els.length);
    // [ts] if (measure)
    // [ts] @print(`pw("${selector}"): ` + (Date.now() - time1));
    // [ts] if (measure && !selector.includes("text")) {
    // [ts] const time2 = Date.now();
    // [ts] let i = 0;
    // [ts] while (i < (if (measure) 10 else 1)) // [cap-migrate] for-step: i++
    // [ts] page.evaluate(selector => document.querySelectorAll(selector).length, selector);
    // [ts] @print(`qs("${selector}"): ` + (Date.now() - time2));
    // [ts] }
    // [ts] }
    // [ts] });
    // [ts] test("SE-037 should be case sensitive if quotes are specified", () => {
    // [ts] page.setContent(`<div>yo</div><div>ya</div><div>\nye  </div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=yA`, e => e.outerHTML)).toBe('<div>ya</div>');
    // [ts] expect_eq(page.$(`text="yA"`), null);
    // [ts] expect_eq(page.$(`text= "ya"`), null);
    // [ts] });
    // [ts] test("SE-038 should search for a substring without quotes", () => {
    // [ts] page.setContent(`<div>textwithsubstring</div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=with`, e => e.outerHTML)).toBe('<div>textwithsubstring</div>');
    // [ts] expect_eq(page.$(`text="with"`), null);
    // [ts] });
    // [ts] test("SE-039 should skip head, script and style", () => {
    // [ts] page.setContent(`
    // [ts] <head>
    // [ts] <title>title</title>
    // [ts] <script>var script</script>
    // [ts] <style>.style {}</style>
    // [ts] </head>
    // [ts] <body>
    // [ts] <script>var script</script>
    // [ts] <style>.style {}</style>
    // [ts] <div>title script style</div>
    // [ts] </body>`);
    // [ts] const head = page.$("head");
    // [ts] const title = page.$("title");
    // [ts] const script = page.$("body script");
    // [ts] const style = page.$("body style");
    // [ts] for (const text of ["title", "script", "style"]) {
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=${text}`, e => e.nodeName)).toBe('DIV');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`text=${text}`, els => els.map(e => e.nodeName).join('|'))).toBe('DIV');
    // [ts] for (const root of [head, title, script, style]) {
    // [ts] expect_eq(root.$(`text=${text}`), null);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await root.$$eval(`text=${text}`, els => els.length)).toBe(0);
    // [ts] }
    // [ts] }
    // [ts] });
    // [ts] test("SE-040 should match input[type=button|submit|reset]", () => {
    // [ts] page.setContent(`<input type="submit" value="hello"><input type="button" value="world"><input type="reset" value="clear">`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=hello`, e => e.outerHTML)).toBe('<input type="submit" value="hello">');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=world`, e => e.outerHTML)).toBe('<input type="button" value="world">');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=clear`, e => e.outerHTML)).toBe('<input type="reset" value="clear">');
    // [ts] });
    // [ts] test("SE-041 should work for open shadow roots", () => {
    // [ts] page.goto(server.PREFIX + "/deep-shadow.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=root1`, e => e.textContent)).toBe('Hello from root1');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=root2`, e => e.textContent)).toBe('Hello from root2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=root3`, e => e.textContent)).toBe('Hello from root3');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`#root1 >> text=from root3`, e => e.textContent)).toBe('Hello from root3');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`#target >> text=from root2`, e => e.textContent)).toBe('Hello from root2');
    // [ts] expect_eq(page.$(`text:light=root1`), null);
    // [ts] expect_eq(page.$(`text:light=root2`), null);
    // [ts] expect_eq(page.$(`text:light=root3`), null);
    // [ts] });
    // [ts] test("SE-042 should prioritize light dom over shadow dom in the same parent", () => {
    // [ts] page.evaluate(() => {
    // [ts] const div = document.createElement("div");
    // [ts] document.body.appendChild(div);
    // [ts] div.attachShadow({ mode: "open" });
    // [ts] const shadowSpan = document.createElement("span");
    // [ts] shadowSpan.textContent = "Hello from shadow";
    // [ts] div.shadowRoot.appendChild(shadowSpan);
    // [ts] const lightSpan = document.createElement("span");
    // [ts] lightSpan.textContent = "Hello from light";
    // [ts] div.appendChild(lightSpan);
    // [ts] });
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div >> text=Hello`, e => e.textContent)).toBe('Hello from light');
    // [ts] });
    // [ts] test("SE-043 should waitForSelector with distributed elements", () => {
    // [ts] const promise = page.waitForSelector(`div >> text=Hello`);
    // [ts] page.evaluate(() => {
    // [ts] const div = document.createElement("div");
    // [ts] document.body.appendChild(div);
    // [ts] div.attachShadow({ mode: "open" });
    // [ts] const shadowSpan = document.createElement("span");
    // [ts] shadowSpan.textContent = "Hello from shadow";
    // [ts] div.shadowRoot.appendChild(shadowSpan);
    // [ts] div.shadowRoot.appendChild(document.createElement("slot"));
    // [ts] const lightSpan = document.createElement("span");
    // [ts] lightSpan.textContent = "Hello from light";
    // [ts] div.appendChild(lightSpan);
    // [ts] });
    // [ts] const handle = promise;
    // [ts] expect_eq(handle.textContent(), "Hello from light");
    // [ts] });
    // [ts] test("SE-044 should match root after >>", () => {
    // [ts] page.setContent("<section>test</section>");
    // [ts] const element = page.$("css=section >> text=test");
    // [ts] expect_eq(element, true);
    // [ts] const element2 = page.$("text=test >> text=test");
    // [ts] expect_eq(element2, true);
    // [ts] });
    // [ts] test("SE-045 should match root after >> with *", () => {
    // [ts] page.setContent(`<button> hello world </button> <button> hellow <span> world </span> </button>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('*css=button >> text=hello >> text=world', els => els.length)).toBe(2);
    // [ts] });
    // [ts] test("SE-046 should work with leading and trailing spaces", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/10997" });
    // [ts] page.setContent(`<button> Add widget </button>`);
    // [ts] expect(page.locator("text=Add widget")).toBeVisible();
    // [ts] expect(page.locator("text= Add widget ")).toBeVisible();
    // [ts] });
    // [ts] test("SE-047 should work with unpaired quotes when not at the start", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/12719" });
    // [ts] page.setContent(`
    // [ts] <div>hello"world<span>yay</span></div>
    // [ts] <div>hello'world<span>nay</span></div>
    // [ts] <div>hello\`world<span>oh</span></div>
    // [ts] <div>hello\`world<span>oh2</span></div>
    // [ts] `);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('text=lo" >> span', e => e.outerHTML)).toBe('<span>yay</span>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('  text=lo" >> span', e => e.outerHTML)).toBe('<span>yay</span>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('text  =lo" >> span', e => e.outerHTML)).toBe('<span>yay</span>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('text=  lo" >> span', e => e.outerHTML)).toBe('<span>yay</span>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(' text = lo" >> span', e => e.outerHTML)).toBe('<span>yay</span>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('text=o"wor >> span', e => e.outerHTML)).toBe('<span>yay</span>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=lo'wor >> span`, e => e.outerHTML)).toBe('<span>nay</span>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=o' >> span`, e => e.outerHTML)).toBe('<span>nay</span>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`text=ello\`wor >> span`, e => e.outerHTML)).toBe('<span>oh</span>');
    // [ts] expect(page.locator(`text=ello\`wor`).locator("span").first()).toHaveText("oh");
    // [ts] expect(page.locator(`text=ello\`wor`).locator("span").nth(1)).toHaveText("oh2");
    // [ts] expect_eq(page.$(`text='wor >> span`), null);
    // [ts] expect_eq(page.$(`text=" >> span`), null);
    // [ts] expect_eq(page.$(`text=\` >> span`), null);
    // [ts] });
    // [ts] test("SE-048 should work with paired quotes in the middle of selector", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/16858" });
    // [ts] page.setContent(`<div>pattern "^-?\\d+$"</div>`);
    // [ts] expect(page.locator(`div >> text=pattern "^-?\\d+$`).isVisible());
    // [ts] expect(page.locator(`div >> text=pattern "^-?\\d+$"`).isVisible());
    // Should double escape inside quoted text.
    // [ts] expect(page.locator(`div >> text='pattern "^-?\\\\d+$"'`).isVisible());
    // [ts] expect(page.locator(`div >> text=pattern "^-?\\d+$`)).toBeVisible();
    // [ts] expect(page.locator(`div >> text=pattern "^-?\\d+$"`)).toBeVisible();
    // Should double escape inside quoted text.
    // [ts] expect(page.locator(`div >> text='pattern "^-?\\\\d+$"'`)).toBeVisible();
    // [ts] });
    // [ts] test("SE-049 hasText and internal:text should match full node text in strict mode", () => {
    // [ts] page.setContent(`
    // [ts] <div id=div1>hello<span>world</span></div>
    // [ts] <div id=div2>hello</div>
    // [ts] `);
    // [ts] expect(page.getByText("helloworld", { exact: true })).toHaveId("div1");
    // [ts] expect(page.getByText("hello", { exact: true })).toHaveId("div2");
    // [ts] expect(page.locator("div", { hasText: "^helloworld$" })).toHaveId("div1");
    // [ts] expect(page.locator("div", { hasText: "^hello$" })).toHaveId("div2");
    // [ts] page.setContent(`
    // [ts] <div id=div1><span id=span1>hello</span>world</div>
    // [ts] <div id=div2><span id=span2>hello</span></div>
    // [ts] `);
    // [ts] expect(page.getByText("helloworld", { exact: true })).toHaveId("div1");
    // [ts] expect_eq(page.getByText("hello", { exact: true }).evaluateAll(els => els.map(e => e.id)), ["span1", "span2"]);
    // [ts] expect(page.locator("div", { hasText: "^helloworld$" })).toHaveId("div1");
    // [ts] expect(page.locator("div", { hasText: "^hello$" })).toHaveId("div2");
    // [ts] });
}
