// Migrated from TypeScript (page/selectors-css.spec.ts) via cap from -ts

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

// [cap-migrate] import { ElementHandle } from "cap:test";
import { test as it, expect } from "./pageTest.cp";


























fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("SE-001 should work with large DOM @smoke", () => {
    // [ts] page.evaluate(() => {
    // [ts] let id = 0;
    // [ts] const next = (tag) => {
    // [ts] const e = document.createElement(tag);
    // [ts] const eid = ++id;
    // [ts] e.textContent = "id" + eid;
    // [ts] e.id = "" + eid;
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
    // [ts] "div div div span",
    // [ts] "div > div div > span",
    // [ts] "div + div div div span + span",
    // [ts] "div ~ div div > span ~ span",
    // [ts] "div > div > div + div > div + div > span ~ span",
    // [ts] "div div div div div div div div div div span",
    // [ts] "div > div > div > div > div > div > div > div > div > div > span",
    // [ts] "div ~ div div ~ div div ~ div div ~ div div ~ div span",
    // [ts] "span",
    // [ts] ];
    // [ts] const measure = false;
    // [ts] for (const selector of selectors) {
    // [ts] const counts1 = [];
    // [ts] const time1 = Date.now();
    // [ts] let i = 0;
    // [ts] while (i < (if (measure) 10 else 1)) // [cap-migrate] for-step: i++
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: counts1.push(await page.$$eval(selector, els => els.length));
    // [ts] if (measure)
    // [ts] @print("pw: " + (Date.now() - time1));
    // [ts] const time2 = Date.now();
    // [ts] const counts2 = [];
    // [ts] let i = 0;
    // [ts] while (i < (if (measure) 10 else 1)) // [cap-migrate] for-step: i++
    // [ts] counts2.push(page.evaluate(selector => document.querySelectorAll(selector).length, selector));
    // [ts] if (measure)
    // [ts] @print("qs: " + (Date.now() - time2));
    // [ts] expect_eq(counts1, counts2);
    // [ts] }
    // [ts] });
    // [ts] test("SE-002 should work for open shadow roots", () => {
    // [ts] page.goto(server.PREFIX + "/deep-shadow.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`css=span`, e => e.textContent)).toBe('Hello from root1');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`css=[attr="value\\ space"]`, e => e.textContent)).toBe('Hello from root3 #2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`css=[attr='value\\ \\space']`, e => e.textContent)).toBe('Hello from root3 #2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`css=div div span`, e => e.textContent)).toBe('Hello from root2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`css=div span + span`, e => e.textContent)).toBe('Hello from root3 #2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`css=span + [attr*="value"]`, e => e.textContent)).toBe('Hello from root3 #2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`css=[data-testid="foo"] + [attr*="value"]`, e => e.textContent)).toBe('Hello from root3 #2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`css=#target`, e => e.textContent)).toBe('Hello from root2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`css=div #target`, e => e.textContent)).toBe('Hello from root2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`css=div div #target`, e => e.textContent)).toBe('Hello from root2');
    // [ts] expect_eq(page.$(`css=div div div #target`), null);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`css=section > div div span`, e => e.textContent)).toBe('Hello from root2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`css=section > div div span:nth-child(2)`, e => e.textContent)).toBe('Hello from root3 #2');
    // [ts] expect_eq(page.$(`css=section div div div div`), null);
    // [ts] const root2 = (page.$(`css=div div`));
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await root2.$eval(`css=#target`, e => e.textContent)).toBe('Hello from root2');
    // [ts] expect_eq(root2.$(`css:light=#target`), null);
    // [ts] const root2Shadow = (root2.evaluateHandle(r => r.shadowRoot));
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await root2Shadow.$eval(`css:light=#target`, e => e.textContent)).toBe('Hello from root2');
    // [ts] const root3 = (page.$$(`css=div div`))[1];
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await root3.$eval(`text=root3`, e => e.textContent)).toBe('Hello from root3');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await root3.$eval(`css=[attr*="value"]`, e => e.textContent)).toBe('Hello from root3 #2');
    // [ts] expect_eq(root3.$(`css:light=[attr*="value"]`), null);
    // [ts] });
    // [ts] test("SE-003 should work with > combinator and spaces", () => {
    // [ts] page.setContent(`<div foo="bar" bar="baz"><span></span></div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div[foo="bar"] > span`, e => e.outerHTML)).toBe(`<span></span>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div[foo="bar"]> span`, e => e.outerHTML)).toBe(`<span></span>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div[foo="bar"] >span`, e => e.outerHTML)).toBe(`<span></span>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div[foo="bar"]>span`, e => e.outerHTML)).toBe(`<span></span>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div[foo="bar"]   >    span`, e => e.outerHTML)).toBe(`<span></span>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div[foo="bar"]>    span`, e => e.outerHTML)).toBe(`<span></span>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div[foo="bar"]     >span`, e => e.outerHTML)).toBe(`<span></span>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div[foo="bar"][bar="baz"] > span`, e => e.outerHTML)).toBe(`<span></span>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div[foo="bar"][bar="baz"]> span`, e => e.outerHTML)).toBe(`<span></span>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div[foo="bar"][bar="baz"] >span`, e => e.outerHTML)).toBe(`<span></span>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div[foo="bar"][bar="baz"]>span`, e => e.outerHTML)).toBe(`<span></span>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div[foo="bar"][bar="baz"]   >    span`, e => e.outerHTML)).toBe(`<span></span>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div[foo="bar"][bar="baz"]>    span`, e => e.outerHTML)).toBe(`<span></span>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div[foo="bar"][bar="baz"]     >span`, e => e.outerHTML)).toBe(`<span></span>`);
    // [ts] });
    // [ts] test("SE-004 should work with comma separated list", () => {
    // [ts] page.goto(server.PREFIX + "/deep-shadow.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span,section #root1`, els => els.length)).toBe(5);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=section #root1, div span`, els => els.length)).toBe(5);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`css=doesnotexist , section #root1`, e => e.id)).toBe('root1');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=doesnotexist ,section #root1`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span,div span`, els => els.length)).toBe(4);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span,div span,div div span`, els => els.length)).toBe(4);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=#target,[attr="value\\ space"]`, els => els.length)).toBe(2);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=#target,[data-testid="foo"],[attr="value\\ space"]`, els => els.length)).toBe(4);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=#target,[data-testid="foo"],[attr="value\\ space"],span`, els => els.length)).toBe(4);
    // [ts] });
    // [ts] test("SE-005 should keep dom order with comma separated list", () => {
    // [ts] page.setContent(`<section><span><div><x></x><y></y></div></span></section>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span,div`, els => els.map(e => e.nodeName).join(','))).toBe('SPAN,DIV');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=div,span`, els => els.map(e => e.nodeName).join(','))).toBe('SPAN,DIV');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span div, div`, els => els.map(e => e.nodeName).join(','))).toBe('DIV');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`*css=section >> css=div,span`, els => els.map(e => e.nodeName).join(','))).toBe('SECTION');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=section >> *css=div >> css=x,y`, els => els.map(e => e.nodeName).join(','))).toBe('DIV');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=section >> *css=div,span >> css=x,y`, els => els.map(e => e.nodeName).join(','))).toBe('SPAN,DIV');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=section >> *css=div,span >> css=y`, els => els.map(e => e.nodeName).join(','))).toBe('SPAN,DIV');
    // [ts] });
    // [ts] test("SE-006 should return multiple captures for the same node", () => {
    // [ts] page.setContent(`<div><div><div><span></span></div></div></div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`*css=div >> span`, els => els.map(e => e.nodeName).join(','))).toBe('DIV,DIV,DIV');
    // [ts] });
    // [ts] test("SE-007 should return multiple captures when going up the hierarchy", () => {
    // [ts] page.setContent(`<section>Hello<ul><li></li><li></li></ul></section>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`*css=li >> ../.. >> text=Hello`, els => els.map(e => e.nodeName).join(','))).toBe('LI,LI');
    // [ts] });
    // [ts] test("SE-008 should work with comma separated list in various positions", () => {
    // [ts] page.setContent(`<section><span><div><x></x><y></y></div></span></section>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span,div >> css=x,y`, els => els.map(e => e.nodeName).join(','))).toBe('X,Y');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span,div >> css=x`, els => els.map(e => e.nodeName).join(','))).toBe('X');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=div >> css=x,y`, els => els.map(e => e.nodeName).join(','))).toBe('X,Y');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=div >> css=x`, els => els.map(e => e.nodeName).join(','))).toBe('X');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=section >> css=div >> css=x`, els => els.map(e => e.nodeName).join(','))).toBe('X');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=section >> css=span >> css=div >> css=y`, els => els.map(e => e.nodeName).join(','))).toBe('Y');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=section >> css=div >> css=x,y`, els => els.map(e => e.nodeName).join(','))).toBe('X,Y');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=section >> css=div,span >> css=x,y`, els => els.map(e => e.nodeName).join(','))).toBe('X,Y');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=section >> css=span >> css=x,y`, els => els.map(e => e.nodeName).join(','))).toBe('X,Y');
    // [ts] });
    // [ts] test("SE-009 should work with comma inside text", () => {
    // [ts] page.setContent(`<span></span><div attr="hello,world!"></div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`css=div[attr="hello,world!"]`, e => e.outerHTML)).toBe('<div attr="hello,world!"></div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`css=[attr="hello,world!"]`, e => e.outerHTML)).toBe('<div attr="hello,world!"></div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`css=div[attr='hello,world!']`, e => e.outerHTML)).toBe('<div attr="hello,world!"></div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`css=[attr='hello,world!']`, e => e.outerHTML)).toBe('<div attr="hello,world!"></div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`css=div[attr="hello,world!"],span`, e => e.outerHTML)).toBe('<span></span>');
    // [ts] });
    // [ts] test("SE-010 should work with attribute selectors", () => {
    // [ts] page.setContent(`<div attr="hello world" attr2="hello-''>>foo=bar[]" attr3="] span"><span></span></div>`);
    // [ts] page.evaluate(() => (window)["div"] = document.querySelector("div"));
    // [ts] const selectors = [
    // [ts] `[attr="hello world"]`,
    // [ts] `[attr = "hello world"]`,
    // [ts] `[attr ~= world]`,
    // [ts] `[attr ^=hello ]`,
    // [ts] `[attr $= world ]`,
    // [ts] `[attr *= "llo wor" ]`,
    // [ts] `[attr2 |= hello]`,
    // [ts] `[attr = "Hello World" i ]`,
    // [ts] `[attr *= "llo WOR"i]`,
    // [ts] `[attr $= woRLD i]`,
    // [ts] `[attr2 = "hello-''>>foo=bar[]"]`,
    // [ts] `[attr2 $="foo=bar[]"]`,
    // [ts] ];
    // [ts] for (const selector of selectors)
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(selector, e => e === (window as any)['div'])).toBe(true);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`[attr*=hello] span`, e => e.parentNode === (window as any)['div'])).toBe(true);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`[attr*=hello] >> span`, e => e.parentNode === (window as any)['div'])).toBe(true);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`[attr3="] span"] >> span`, e => e.parentNode === (window as any)['div'])).toBe(true);
    // [ts] });
    // [ts] test("SE-011 should not match root after >>", () => {
    // [ts] page.setContent("<section><div>test</div></section>");
    // [ts] const element = page.$("css=section >> css=section");
    // [ts] expect_eq(element, null);
    // [ts] });
    // [ts] test("SE-012 should work with numerical id", () => {
    // [ts] page.setContent("<section id=\"123\"></section>");
    // [ts] const element = page.$("#\\31\\32\\33");
    // [ts] expect_eq(element, true);
    // [ts] });
    // [ts] test("SE-013 should work with wrong-case id", () => {
    // [ts] page.setContent("<section id=\"Hello\"></section>");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('#Hello', e => e.tagName)).toBe('SECTION');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('#hello', e => e.tagName)).toBe('SECTION');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('#HELLO', e => e.tagName)).toBe('SECTION');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('#helLO', e => e.tagName)).toBe('SECTION');
    // [ts] });
    // [ts] test("SE-014 should work with *", () => {
    // [ts] page.setContent("<div id=div1></div><div id=div2><span><span></span></span></div>");
    // Includes html, head and body.
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('*', els => els.length)).toBe(7);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('*#div1', els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('*:not(#div1)', els => els.length)).toBe(6);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('*:not(div)', els => els.length)).toBe(5);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('*:not(span)', els => els.length)).toBe(5);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('*:not(*)', els => els.length)).toBe(0);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('*:is(*)', els => els.length)).toBe(7);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('* *', els => els.length)).toBe(6);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('* *:not(span)', els => els.length)).toBe(4);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('div > *', els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('div *', els => els.length)).toBe(2);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('* > *', els => els.length)).toBe(6);
    // [ts] const body = (page.$("body"));
    // Does not include html, head or body.
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await body.$$eval('*', els => els.length)).toBe(4);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await body.$$eval('*#div1', els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await body.$$eval('*:not(#div1)', els => els.length)).toBe(3);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await body.$$eval('*:not(div)', els => els.length)).toBe(2);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await body.$$eval('*:not(span)', els => els.length)).toBe(2);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await body.$$eval('*:not(*)', els => els.length)).toBe(0);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await body.$$eval('*:is(*)', els => els.length)).toBe(4);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await body.$$eval('div > *', els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await body.$$eval('div *', els => els.length)).toBe(2);
    // Selectors v2 matches jquery in the sense that matching starts with the element scope,
    // not the document scope.
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await body.$$eval('* > *', els => els.length)).toBe(2);
    // Adding scope makes querySelectorAll work like jquery.
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await body.$$eval(':scope * > *', els => els.length)).toBe(2);
    // Note that the following two selectors are following jquery logic even
    // with selectors v1. Just running `body.querySelectorAll` returns 4 and 2 respectively.
    // That's probably a bug in v1, but oh well.
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await body.$$eval('* *', els => els.length)).toBe(2);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await body.$$eval('* *:not(span)', els => els.length)).toBe(0);
    // [ts] });
    // [ts] test("SE-015 should work with :nth-child", () => {
    // [ts] page.goto(server.PREFIX + "/deep-shadow.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span:nth-child(odd)`, els => els.length)).toBe(3);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span:nth-child(even)`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span:nth-child(n+1)`, els => els.length)).toBe(4);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span:nth-child(n+2)`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span:nth-child(2n)`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span:nth-child(2n+1)`, els => els.length)).toBe(3);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span:nth-child(-n)`, els => els.length)).toBe(0);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span:nth-child(-n+1)`, els => els.length)).toBe(3);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span:nth-child(-n+2)`, els => els.length)).toBe(4);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span:nth-child(23n+2)`, els => els.length)).toBe(1);
    // [ts] });
    // [ts] test("SE-016 should work with :nth-child(of) notation with nested functions", () => {
    // [ts] test(browserName == "chromium" && browserMajorVersion < 111, "https://caniuse.com/css-nth-child-of");
    // [ts] page.setContent(`
    // [ts] <div>
    // [ts] <span>span1</span>
    // [ts] <span class=foo>span2<dd></dd></span>
    // [ts] <span class=foo>span3<dd class=marker></dd></span>
    // [ts] <span class=foo>span4<dd class=marker></dd></span>
    // [ts] <span class=foo>span5<dd></dd></span>
    // [ts] <span>span6</span>
    // [ts] </div>
    // [ts] `);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span:nth-child(1)`, els => els.map(e => e.textContent))).toEqual(['span1']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span:nth-child(1 of .foo)`, els => els.map(e => e.textContent))).toEqual(['span2']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span:nth-child(1 of .foo:has(dd.marker))`, els => els.map(e => e.textContent))).toEqual(['span3']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span:nth-last-child(1 of .foo:has(dd.marker))`, els => els.map(e => e.textContent))).toEqual(['span4']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span:nth-last-child(1 of .foo)`, els => els.map(e => e.textContent))).toEqual(['span5']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span:nth-last-child(  1  )`, els => els.map(e => e.textContent))).toEqual(['span6']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span:nth-child(1 of .foo:nth-child(3))`, els => els.map(e => e.textContent))).toEqual(['span3']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span:nth-child(1 of .foo:nth-child(6))`, els => els.map(e => e.textContent))).toEqual([]);
    // [ts] });
    // [ts] test("SE-017 should work with :not", () => {
    // [ts] page.goto(server.PREFIX + "/deep-shadow.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=div:not(#root1)`, els => els.length)).toBe(2);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=body :not(span)`, els => els.length)).toBe(4);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=div > :not(span):not(div)`, els => els.length)).toBe(0);
    // [ts] });
    // [ts] test("SE-018 should work with ~", () => {
    // [ts] page.setContent(`
    // [ts] <div id=div1></div>
    // [ts] <div id=div2></div>
    // [ts] <div id=div3></div>
    // [ts] <div id=div4></div>
    // [ts] <div id=div5></div>
    // [ts] <div id=div6></div>
    // [ts] `);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`#div3 >> :scope ~ div`, els => els.map(e => e.id))).toEqual(['div4', 'div5', 'div6']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`#div3 >> :scope ~ *`, els => els.map(e => e.id))).toEqual(['div4', 'div5', 'div6']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`#div3 >> ~ div`, els => els.map(e => e.id))).toEqual(['div4', 'div5', 'div6']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`#div3 >> ~ *`, els => els.map(e => e.id))).toEqual(['div4', 'div5', 'div6']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`#div3 >> #div1 ~ :scope`, els => els.map(e => e.id))).toEqual(['div3']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`#div3 >> #div4 ~ :scope`, els => els.map(e => e.id))).toEqual([]);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=#div1 ~ div ~ #div6`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=#div1 ~ div ~ div`, els => els.length)).toBe(4);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=#div3 ~ div ~ div`, els => els.length)).toBe(2);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=#div4 ~ div ~ div`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=#div5 ~ div ~ div`, els => els.length)).toBe(0);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=#div3 ~ #div2 ~ #div6`, els => els.length)).toBe(0);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=#div3 ~ #div4 ~ #div5`, els => els.length)).toBe(1);
    // [ts] });
    // [ts] test("SE-019 should work with +", () => {
    // [ts] page.setContent(`
    // [ts] <section>
    // [ts] <div id=div1></div>
    // [ts] <div id=div2></div>
    // [ts] <div id=div3></div>
    // [ts] <div id=div4></div>
    // [ts] <div id=div5></div>
    // [ts] <div id=div6></div>
    // [ts] </section>
    // [ts] `);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`#div1 >> :scope+div`, els => els.map(e => e.id))).toEqual(['div2']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`#div1 >> :scope+*`, els => els.map(e => e.id))).toEqual(['div2']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`#div1 >> + div`, els => els.map(e => e.id))).toEqual(['div2']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`#div1 >> + *`, els => els.map(e => e.id))).toEqual(['div2']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`#div3 >> div + :scope`, els => els.map(e => e.id))).toEqual(['div3']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`#div3 >> #div1 + :scope`, els => els.map(e => e.id))).toEqual([]);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=#div1 ~ div + #div6`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=#div1 ~ div + div`, els => els.length)).toBe(4);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=#div3 + div + div`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=#div4 ~ #div5 + div`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=#div5 + div + div`, els => els.length)).toBe(0);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=#div3 ~ #div2 + #div6`, els => els.length)).toBe(0);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=#div3 + #div4 + #div5`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=div + #div1`, els => els.length)).toBe(0);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=section > div + div ~ div`, els => els.length)).toBe(4);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=section > div + #div4 ~ div`, els => els.length)).toBe(2);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=section:has(:scope > div + #div2)`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=section:has(:scope > div + #div1)`, els => els.length)).toBe(0);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`css=div:has(:scope + #div5)`, e => e.id)).toBe('div4');
    // [ts] });
    // [ts] test("SE-020 should work with spaces in :nth-child and :not", () => {
    // [ts] page.goto(server.PREFIX + "/deep-shadow.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span:nth-child(23n +2)`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span:nth-child(23n+ 2)`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span:nth-child( 23n + 2 )`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span:not(#root1 #target)`, els => els.length)).toBe(3);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span:not(:not(#root1 #target))`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span:not(span:not(#root1 #target))`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=div > :not(span)`, els => els.length)).toBe(2);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=body :not(span, div)`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=span, section:not(span, div)`, els => els.length)).toBe(5);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`span:nth-child(23n+ 2) >> xpath=.`, els => els.length)).toBe(1);
    // [ts] });
    // [ts] test("SE-021 should work with :is", () => {
    // [ts] page.goto(server.PREFIX + "/deep-shadow.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=div:is(#root1)`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=div:is(#root1, #target)`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=div:is(span, #target)`, els => els.length)).toBe(0);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=div:is(span, #root1 > *)`, els => els.length)).toBe(2);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=div:is(section div)`, els => els.length)).toBe(3);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=:is(div, span)`, els => els.length)).toBe(7);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=section:is(section) div:is(section div)`, els => els.length)).toBe(3);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=:is(div, span) > *`, els => els.length)).toBe(6);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=#root1:has(:is(#root1))`, els => els.length)).toBe(0);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=#root1:has(:is(:scope, #root1))`, els => els.length)).toBe(1);
    // [ts] });
    // [ts] test("SE-022 should work with :has", () => {
    // [ts] page.goto(server.PREFIX + "/deep-shadow.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=div:has(#target)`, els => els.length)).toBe(2);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=div:has([data-testid=foo])`, els => els.length)).toBe(3);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=div:has([attr*=value])`, els => els.length)).toBe(2);
    // [ts] page.setContent(`<section><span></span><div></div></section><section><br></section>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`section:has(span, div)`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`section:has(span, div)`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`section:has(br)`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`section:has(span, br)`, els => els.length)).toBe(2);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`section:has(span, br, div)`, els => els.length)).toBe(2);
    // [ts] });
    // [ts] test("SE-023 should work with :scope", () => {
    // [ts] page.goto(server.PREFIX + "/deep-shadow.html");
    // 'is' does not change the scope, so it remains 'html'.
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=div:is(:scope#root1)`, els => els.length)).toBe(0);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=div:is(:scope #root1)`, els => els.length)).toBe(1);
    // 'has' does change the scope, so it becomes the 'div' we are querying.
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`css=div:has(:scope > #target)`, els => els.length)).toBe(1);
    // [ts] const handle = page.$(`css=span`);
    // [ts] for (const scope of [page, handle]) {
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await scope.$$eval(`css=:scope`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await scope.$$eval(`css=* :scope`, els => els.length)).toBe(0);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await scope.$$eval(`css=* + :scope`, els => els.length)).toBe(0);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await scope.$$eval(`css=* > :scope`, els => els.length)).toBe(0);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await scope.$$eval(`css=* ~ :scope`, els => els.length)).toBe(0);
    // [ts] }
    // [ts] page.setContent(`<article><div class=target>hello<span></span></div></article>`);
    // 'scope' should allow additional native css modifiers
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div >> :scope.target`, e => e.textContent)).toBe('hello');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div >> :scope:nth-child(1)`, e => e.textContent)).toBe('hello');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div >> :scope.target:has(span)`, e => e.textContent)).toBe('hello');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`html:scope`, e => e.nodeName)).toBe('HTML');
    // [ts] page.setContent(`<section><span id=span1><span id=inner></span></span><span id=span2></span></section>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`#span1 >> span:not(:has(:scope > div))`, els => els.map(e => e.id))).toEqual(['inner']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`#span1 >> #inner,:scope`, els => els.map(e => e.id))).toEqual(['span1', 'inner']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`#span1 >> span,:scope`, els => els.map(e => e.id))).toEqual(['span1', 'inner']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`#span1 >> span:not(:scope)`, els => els.map(e => e.id))).toEqual(['inner']);
    // TODO: the following two do not work. We do not expand the context for the inner :scope,
    // because we should only expand for one clause of :is() that contains :scope, but not the other.
    // expect(await page.$$eval(`#span1 >> span:is(:scope)`, els => els.map(e => e.id))).toEqual(['span1']);
    // expect(await page.$$eval(`#span1 >> span:is(:scope,#inner)`, els => els.map(e => e.id))).toEqual(['span1', 'inner']);
    // [ts] });
    // [ts] test("SE-024 should work with :scope and class", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/17824" });
    // [ts] page.setContent(`<div class="apple"></div>
    // [ts] <div class="apple selected"></div>`);
    // [ts] const apples = page.locator(".apple");
    // [ts] const selectedApples = apples.locator(":scope.selected");
    // [ts] expect(selectedApples).toHaveCount(1);
    // [ts] });
    // [ts] test("SE-025 should absolutize relative selectors", () => {
    // [ts] page.setContent(`<div><span>Hi</span></div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div >> >span', e => e.textContent)).toBe('Hi');
    // [ts] expect_eq(page.locator("div").locator(">span").textContent(), "Hi");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div:has(> span)', e => e.outerHTML)).toBe('<div><span>Hi</span></div>');
    // [ts] expect_eq(page.$("div:has(> div)"), null);
    // [ts] });
    // [ts] test("SE-026 css on the handle should be relative", () => {
    // [ts] page.setContent(`
    // [ts] <span class="find-me" id=target1>1</span>
    // [ts] <div>
    // [ts] <span class="find-me" id=target2>2</span>
    // [ts] </div>
    // [ts] `);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`.find-me`, e => e.id)).toBe('target1');
    // [ts] const div = page.$("div");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await div.$eval(`.find-me`, e => e.id)).toBe('target2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div >> .find-me`, e => e.id)).toBe('target2');
    // [ts] });
    // [ts] test("SE-027 should use light DOM structure for child combinator with slotted content", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/37768" } }, (_p) => {
    // [ts] page.setContent(`
    // [ts] <my-button>
    // [ts] <template shadowrootmode="open">
    // [ts] <button><slot></slot></button>
    // [ts] </template>
    // [ts] <div class="content">Foo</div>
    // [ts] </my-button>
    // [ts] `);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`my-button > div`, e => e.className)).toBe('content');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`my-button > .content`, e => e.textContent)).toBe('Foo');
    // [ts] });
}
