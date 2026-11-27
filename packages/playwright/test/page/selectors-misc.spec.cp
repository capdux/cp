// Migrated from TypeScript (page/selectors-misc.spec.ts) via cap from -ts

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
    // [ts] test("SE-133 should work for open shadow roots", () => {
    // [ts] page.goto(server.PREFIX + "/deep-shadow.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`id=target`, e => e.textContent)).toBe('Hello from root2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`data-testid=foo`, e => e.textContent)).toBe('Hello from root1');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`data-testid=foo`, els => els.length)).toBe(3);
    // [ts] expect_eq(page.$(`id:light=target`), null);
    // [ts] expect_eq(page.$(`data-testid:light=foo`), null);
    // [ts] expect_eq(page.$$(`data-testid:light=foo`), []);
    // [ts] });
    // [ts] test("SE-134 should click on links in shadow dom", () => {
    // [ts] page.goto(server.PREFIX + "/shadow-dom-link.html");
    // [ts] expect_eq(page.evaluate(() => (window).clickCount), 0);
    // [ts] page.click("#inner-link");
    // [ts] expect_eq(page.evaluate(() => (window).clickCount), 1);
    // [ts] });
    // [ts] test("SE-135 should work with :visible", () => {
    // [ts] page.setContent(`
    // [ts] <section>
    // [ts] <div id=target1></div>
    // [ts] <div id=target2></div>
    // [ts] </section>
    // [ts] `);
    // [ts] expect_eq(page.$("div:visible"), null);
    // [ts] const error = page.waitForSelector(`div:visible`, { timeout: 1000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "1000ms") != null, true);
    // [ts] const promise = page.waitForSelector(`div:visible`, { state: "attached" });
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('#target2', div => div.textContent = 'Now visible');
    // [ts] const element = promise;
    // [ts] expect_eq(element.evaluate(e => e.id), "target2");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div:visible', div => div.id)).toBe('target2');
    // [ts] });
    // [ts] test("SE-136 should work with >> visible=", () => {
    // [ts] page.setContent(`
    // [ts] <section>
    // [ts] <div id=target1></div>
    // [ts] <div id=target2></div>
    // [ts] </section>
    // [ts] `);
    // [ts] expect_eq(page.$("div >> visible=true"), null);
    // [ts] const error = page.waitForSelector(`div >> visible=true`, { timeout: 1000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "1000ms") != null, true);
    // [ts] const promise = page.waitForSelector(`div >> visible=true`, { state: "attached" });
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('#target2', div => div.textContent = 'Now visible');
    // [ts] const element = promise;
    // [ts] expect_eq(element.evaluate(e => e.id), "target2");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div >> visible=true', div => div.id)).toBe('target2');
    // [ts] });
    // [ts] test("SE-137 should work with >> visible=false", () => {
    // [ts] page.setContent(`
    // [ts] <section>
    // [ts] <div id=target1></div>
    // [ts] <div id=target2></div>
    // [ts] </section>
    // [ts] `);
    // [ts] expect(page.locator("div >> visible=false")).toHaveCount(2);
    // [ts] page.locator("#target2").evaluate(div => div.textContent = "Now visible");
    // [ts] expect(page.locator("div >> visible=false")).toHaveCount(1);
    // [ts] });
    // [ts] test("SE-138 should work with :nth-match", () => {
    // [ts] page.setContent(`
    // [ts] <section>
    // [ts] <div id=target1></div>
    // [ts] <div id=target2></div>
    // [ts] </section>
    // [ts] `);
    // [ts] expect_eq(page.$(":nth-match(div, 3)"), null);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(':nth-match(div, 1)', e => e.id)).toBe('target1');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(':nth-match(div, 2)', e => e.id)).toBe('target2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(':nth-match(section > div, 2)', e => e.id)).toBe('target2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(':nth-match(section, div, 2)', e => e.id)).toBe('target1');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(':nth-match(div, section, 3)', e => e.id)).toBe('target2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(':is(:nth-match(div, 1), :nth-match(div, 2))', els => els.length)).toBe(2);
    // [ts] let error;
    // [ts] error = page.$(":nth-match(div, bar, 0)").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `"nth-match" engine expects a one-based index as the last argument`) != null, true);
    // [ts] error = page.$(":nth-match(2)").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `"nth-match" engine expects non-empty selector list and an index argument`) != null, true);
    // [ts] error = page.$(":nth-match(div, bar, foo)").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `"nth-match" engine expects a one-based index as the last argument`) != null, true);
    // [ts] const promise = page.waitForSelector(`:nth-match(div, 3)`, { state: "attached" });
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('section', section => {
    // [ts-skip] const div = document.createElement('div');
    // [ts-skip] div.setAttribute('id', 'target3');
    // [ts-skip] section.appendChild(div);
    // [ts-skip] });
    // [ts] const element = promise;
    // [ts] expect_eq(element.evaluate(e => e.id), "target3");
    // [ts] });
    // [ts] test("SE-139 should work with nth=", () => {
    // [ts] page.setContent(`
    // [ts] <section>
    // [ts] <div id=target1></div>
    // [ts] <div id=target2></div>
    // [ts] </section>
    // [ts] `);
    // [ts] expect_eq(page.$("div >> nth=2"), null);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div >> nth=0', e => e.id)).toBe('target1');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div >> nth=1', e => e.id)).toBe('target2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('section > div >> nth=1', e => e.id)).toBe('target2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('section, div >> nth=1', e => e.id)).toBe('target1');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div, section >> nth=2', e => e.id)).toBe('target2');
    // [ts] const promise = page.waitForSelector(`div >> nth=2`, { state: "attached" });
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('section', section => {
    // [ts-skip] const div = document.createElement('div');
    // [ts-skip] div.setAttribute('id', 'target3');
    // [ts-skip] section.appendChild(div);
    // [ts-skip] });
    // [ts] const element = promise;
    // [ts] expect_eq(element.evaluate(e => e.id), "target3");
    // [ts] page.setContent(`
    // [ts] <div>
    // [ts] <div>
    // [ts] <div>
    // [ts] <span>hi</span>
    // [ts] <span>hello</span>
    // [ts] </div>
    // [ts] </div>
    // [ts] </div>
    // [ts] `);
    // [ts] expect_eq(page.locator("div >> div >> span >> nth=1").textContent(), "hello");
    // [ts] });
    // [ts] test("SE-140 should work with strict mode and chaining", () => {
    // [ts] page.setContent(`
    // [ts] <div>
    // [ts] <div>
    // [ts] <div>
    // [ts] <span>hi</span>
    // [ts] </div>
    // [ts] </div>
    // [ts] </div>
    // [ts] `);
    // [ts] expect_eq(page.locator("div >> div >> span").textContent(), "hi");
    // [ts] });
    // [ts] test("SE-141 should work with layout selectors", () => {
    // [ts] test(trace == "on");
    // [ts] /*
    // [ts] +--+  +--+
    // [ts] | 1|  | 2|
    // [ts] +--+  ++-++
    // [ts] | 3|   | 4|
    // [ts] +-------+  ++-++
    // [ts] |   0   |  | 5|
    // [ts] | +--+  +--+--+
    // [ts] | | 6|  | 7|
    // [ts] | +--+  +--+
    // [ts] |       |
    // [ts] O-------+
    // [ts] +--+
    // [ts] | 8|
    // [ts] +--++--+
    // [ts] | 9|
    // [ts] +--+
    // [ts] */
    // [ts] const boxes = [
    // x, y, width, height
    // [ts] [0, 0, 150, 150],
    // [ts] [100, 200, 50, 50],
    // [ts] [200, 200, 50, 50],
    // [ts] [100, 150, 50, 50],
    // [ts] [201, 150, 50, 50],
    // [ts] [200, 100, 50, 50],
    // [ts] [50, 50, 50, 50],
    // [ts] [150, 50, 50, 50],
    // [ts] [150, -51, 50, 50],
    // [ts] [201, -101, 50, 50],
    // [ts] ];
    // [ts] page.setContent(`<container style="width: 500px; height: 500px; position: relative;"></container>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('container', (container, boxes) => {
    // [ts-skip] for (let i = 0; i < boxes.length; i++) {
    // [ts-skip] const div = document.createElement('div');
    // [ts-skip] div.style.position = 'absolute';
    // [ts-skip] div.style.overflow = 'hidden';
    // [ts-skip] div.style.boxSizing = 'border-box';
    // [ts-skip] div.style.border = '1px solid black';
    // [ts-skip] div.id = 'id' + i;
    // [ts-skip] div.textContent = 'id' + i;
    // [ts-skip] const box = boxes[i];
    // [ts-skip] div.style.left = box[0] + 'px';
    // [ts-skip] // Note that top is a flipped y coordinate.
    // [ts-skip] div.style.top = (250 - box[1] - box[3]) + 'px';
    // [ts-skip] div.style.width = box[2] + 'px';
    // [ts-skip] div.style.height = box[3] + 'px';
    // [ts-skip] container.appendChild(div);
    // [ts-skip] const span = document.createElement('span');
    // [ts-skip] span.textContent = '' + i;
    // [ts-skip] div.appendChild(span);
    // [ts-skip] }
    // [ts-skip] }, boxes);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div:right-of(#id6)', e => e.id)).toBe('id7');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div:right-of(#id1)', e => e.id)).toBe('id2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div:right-of(#id3)', e => e.id)).toBe('id4');
    // [ts] expect_eq(page.$("div:right-of(#id4)"), null);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div:right-of(#id0)', e => e.id)).toBe('id7');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div:right-of(#id8)', e => e.id)).toBe('id9');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('div:right-of(#id3)', els => els.map(e => e.id).join(','))).toBe('id4,id2,id5,id7,id8,id9');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('div:right-of(#id3, 50)', els => els.map(e => e.id).join(','))).toBe('id2,id5,id7,id8');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('div:right-of(#id3, 49)', els => els.map(e => e.id).join(','))).toBe('id7,id8');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div:left-of(#id2)', e => e.id)).toBe('id1');
    // [ts] expect_eq(page.$("div:left-of(#id0)"), null);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div:left-of(#id5)', e => e.id)).toBe('id0');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div:left-of(#id9)', e => e.id)).toBe('id8');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div:left-of(#id4)', e => e.id)).toBe('id3');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('div:left-of(#id5)', els => els.map(e => e.id).join(','))).toBe('id0,id7,id3,id1,id6,id8');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('div:left-of(#id5, 3)', els => els.map(e => e.id).join(','))).toBe('id7,id8');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div:above(#id0)', e => e.id)).toBe('id3');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div:above(#id5)', e => e.id)).toBe('id4');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div:above(#id7)', e => e.id)).toBe('id5');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div:above(#id8)', e => e.id)).toBe('id0');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div:above(#id9)', e => e.id)).toBe('id8');
    // [ts] expect_eq(page.$("div:above(#id2)"), null);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('div:above(#id5)', els => els.map(e => e.id).join(','))).toBe('id4,id2,id3,id1');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('div:above(#id5, 20)', els => els.map(e => e.id).join(','))).toBe('id4,id3');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div:below(#id4)', e => e.id)).toBe('id5');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div:below(#id3)', e => e.id)).toBe('id0');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div:below(#id2)', e => e.id)).toBe('id4');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div:below(#id6)', e => e.id)).toBe('id8');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div:below(#id7)', e => e.id)).toBe('id8');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div:below(#id8)', e => e.id)).toBe('id9');
    // [ts] expect_eq(page.$("div:below(#id9)"), null);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('div:below(#id3)', els => els.map(e => e.id).join(','))).toBe('id0,id5,id6,id7,id8,id9');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('div:below(#id3, 105)', els => els.map(e => e.id).join(','))).toBe('id0,id5,id6,id7');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div:near(#id0)', e => e.id)).toBe('id3');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('div:near(#id7)', els => els.map(e => e.id).join(','))).toBe('id0,id5,id3,id6');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('div:near(#id0)', els => els.map(e => e.id).join(','))).toBe('id3,id6,id7,id8,id1,id5');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('div:near(#id6)', els => els.map(e => e.id).join(','))).toBe('id0,id3,id7');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('div:near(#id6, 10)', els => els.map(e => e.id).join(','))).toBe('id0');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('div:near(#id0, 100)', els => els.map(e => e.id).join(','))).toBe('id3,id6,id7,id8,id1,id5,id4,id2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('div:below(#id5):above(#id8)', els => els.map(e => e.id).join(','))).toBe('id7,id6');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div:below(#id5):above(#id8)', e => e.id)).toBe('id7');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('div:right-of(#id0) + div:above(#id8)', els => els.map(e => e.id).join(','))).toBe('id5,id6,id3');
    // [ts] const error = page.$(":near(50)").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "\"near\" engine expects a selector list and optional maximum distance in pixels") != null, true);
    // [ts] });
    // [ts] test("SE-142 should escape the scope with >>", () => {
    // [ts] page.setContent(`<div><label>Test</label><input id='myinput'></div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`label >> xpath=.. >> input`, e => e.id)).toBe('myinput');
    // [ts] });
    // [ts] test("SE-143 xpath should be relative", () => {
    // [ts] page.setContent(`
    // [ts] <span class="find-me" id=target1>1</span>
    // [ts] <div>
    // [ts] <span class="find-me" id=target2>2</span>
    // [ts] </div>
    // [ts] `);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`//*[@class="find-me"]`, e => e.id)).toBe('target1');
    // [ts] const div = page.$("div");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await div.$eval(`xpath=./*[@class="find-me"]`, e => e.id)).toBe('target2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await div.$eval(`xpath=.//*[@class="find-me"]`, e => e.id)).toBe('target2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await div.$eval(`//*[@class="find-me"]`, e => e.id)).toBe('target2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await div.$eval(`xpath=/*[@class="find-me"]`, e => e.id)).toBe('target2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div >> xpath=./*[@class="find-me"]`, e => e.id)).toBe('target2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div >> xpath=.//*[@class="find-me"]`, e => e.id)).toBe('target2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div >> //*[@class="find-me"]`, e => e.id)).toBe('target2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div >> xpath=/*[@class="find-me"]`, e => e.id)).toBe('target2');
    // [ts] });
    // [ts] test("SE-144 should work with pipe in xpath", () => {
    // [ts] page.setContent(`
    // [ts] <span class="find-me" id=t1>1</span>
    // [ts] <div>
    // [ts] <span class="find-me" id=t2>2</span>
    // [ts] </div>
    // [ts] <div id=t3>3</span>
    // [ts] `);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`//*[@id="t1"]|//*[@id="t3"]`, els => els.length)).toBe(2);
    // [ts] const e1 = page.waitForSelector(`//*[@id="t1"]|//*[@id="t3"]`);
    // [ts] expect_eq(e1, true);
    // [ts] expect_eq(e1.evaluate(e => e.id), "t1");
    // [ts] const e2 = page.waitForSelector(`//*[@id="unknown"]|//*[@id="t2"]`);
    // [ts] expect_eq(e2, true);
    // [ts] expect_eq(e2.evaluate(e => e.id), "t2");
    // [ts] page.click(`//code|//span[@id="t2"]`);
    // [ts] });
    // [ts] test("SE-145 should print original xpath in error", () => {
    // [ts] const error = page.locator(`//*[contains(@Class, 'foo']`).isVisible().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "//*[contains(@Class, \\'foo\\']") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, ".//*[contains(@Class, 'foo']") == null, true);
    // [ts] });
    // [ts] test("SE-146 data-testid on the handle should be relative", () => {
    // [ts] page.setContent(`
    // [ts] <span data-testid="find-me" id=target1>1</span>
    // [ts] <div>
    // [ts] <span data-testid="find-me" id=target2>2</span>
    // [ts] </div>
    // [ts] `);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`data-testid=find-me`, e => e.id)).toBe('target1');
    // [ts] const div = page.$("div");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await div.$eval(`data-testid=find-me`, e => e.id)).toBe('target2');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div >> data-testid=find-me`, e => e.id)).toBe('target2');
    // [ts] });
    // [ts] test("SE-147 should properly determine visibility of display:contents elements", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/11202" });
    // [ts] page.setContent(`
    // [ts] <div>
    // [ts] <p style="display:contents">DISPLAY CONTENTS</p>
    // [ts] </div>`);
    // [ts] page.waitForSelector("\"DISPLAY CONTENTS\"");
    // [ts] page.setContent(`
    // [ts] <div>
    // [ts] <article style="display:contents"><div>DISPLAY CONTENTS</div></article>
    // [ts] </div>`);
    // [ts] page.waitForSelector("article");
    // [ts] page.setContent(`
    // [ts] <div>
    // [ts] <article style="display:contents"><div style="display:contents">DISPLAY CONTENTS</div></article>
    // [ts] </div>`);
    // [ts] page.waitForSelector("article");
    // [ts] page.setContent(`
    // [ts] <div>
    // [ts] <article style="display:contents"><div></div>DISPLAY CONTENTS<span></span></article>
    // [ts] </div>`);
    // [ts] page.waitForSelector("article");
    // [ts] page.setContent(`
    // [ts] <div>
    // [ts] <article style="display:contents"><div></div></article>
    // [ts] </div>`);
    // [ts] page.waitForSelector("article", { state: "hidden" });
    // [ts] });
    // [ts] test("SE-148 should work with internal:has=", () => {
    // [ts] page.goto(server.PREFIX + "/deep-shadow.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`div >> internal:has="#target"`, els => els.length)).toBe(2);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`div >> internal:has="[data-testid=foo]"`, els => els.length)).toBe(3);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`div >> internal:has="[attr*=value]"`, els => els.length)).toBe(2);
    // [ts] page.setContent(`<section><span></span><div></div></section><section><br></section>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`section >> internal:has="span, div"`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`section >> internal:has="span, div"`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`section >> internal:has="br"`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`section >> internal:has="span, br"`, els => els.length)).toBe(2);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`section >> internal:has="span, br, div"`, els => els.length)).toBe(2);
    // [ts] page.setContent(`<div><span>hello</span></div><div><span>world</span></div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`div >> internal:has="text=world"`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div >> internal:has="text=world"`, e => e.outerHTML)).toBe(`<div><span>world</span></div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`div >> internal:has="text=\\"hello\\""`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div >> internal:has="text=\\"hello\\""`, e => e.outerHTML)).toBe(`<div><span>hello</span></div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`div >> internal:has="xpath=./span"`, els => els.length)).toBe(2);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`div >> internal:has="span"`, els => els.length)).toBe(2);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`div >> internal:has="span >> text=wor"`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div >> internal:has="span >> text=wor"`, e => e.outerHTML)).toBe(`<div><span>world</span></div>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div >> internal:has="span >> text=wor" >> span`, e => e.outerHTML)).toBe(`<span>world</span>`);
    // [ts] const error1 = page.$(`div >> internal:has=abc`).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error1.message, "Malformed selector: internal:has=abc") != null, true);
    // [ts] const error2 = page.$(`internal:has="div"`).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error2.message, "\"internal:has\" selector cannot be first") != null, true);
    // [ts] const error3 = page.$(`div >> internal:has=33`).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error3.message, "Malformed selector: internal:has=33") != null, true);
    // [ts] const error4 = page.$(`div >> internal:has="span!"`).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error4.message, "Unexpected token \"!\" while parsing css selector \"span!\"") != null, true);
    // [ts] });
    // [ts] test("SE-149 should work with internal:has-not=", () => {
    // [ts] page.setContent(`<section><span></span><div></div></section><section><br></section>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`section >> internal:has-not="span"`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`section >> internal:has-not="span, div, br"`, els => els.length)).toBe(0);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`section >> internal:has-not="br"`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`section >> internal:has-not="span, div"`, els => els.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`section >> internal:has-not="article"`, els => els.length)).toBe(2);
    // [ts] });
    // [ts] test("SE-150 should work with internal:and=", () => {
    // [ts] page.setContent(`
    // [ts] <div class=foo>hello</div><div class=bar>world</div>
    // [ts] <span class=foo>hello2</span><span class=bar>world2</span>
    // [ts] `);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`div >> internal:and="span"`, els => els.map(e => e.textContent))).toEqual([]);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`div >> internal:and=".foo"`, els => els.map(e => e.textContent))).toEqual(['hello']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`div >> internal:and=".bar"`, els => els.map(e => e.textContent))).toEqual(['world']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`span >> internal:and="span"`, els => els.map(e => e.textContent))).toEqual(['hello2', 'world2']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`.foo >> internal:and="div"`, els => els.map(e => e.textContent))).toEqual(['hello']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`.bar >> internal:and="span"`, els => els.map(e => e.textContent))).toEqual(['world2']);
    // [ts] });
    // [ts] test("SE-151 should work with internal:or=", () => {
    // [ts] page.setContent(`
    // [ts] <div>hello</div>
    // [ts] <span>world</span>
    // [ts] `);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`div >> internal:or="span"`, els => els.map(e => e.textContent))).toEqual(['hello', 'world']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`span >> internal:or="div"`, els => els.map(e => e.textContent))).toEqual(['hello', 'world']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`article >> internal:or="something"`, els => els.length)).toBe(0);
    // [ts] expect_eq(page.locator(`article >> internal:or="div"`).textContent(), "hello");
    // [ts] expect_eq(page.locator(`article >> internal:or="span"`).textContent(), "world");
    // [ts] expect_eq(page.locator(`div >> internal:or="article"`).textContent(), "hello");
    // [ts] expect_eq(page.locator(`span >> internal:or="article"`).textContent(), "world");
    // [ts] });
    // [ts] test("SE-152 should work with internal:chain=", () => {
    // [ts] page.setContent(`
    // [ts] <div>one <span>two</span> <button>three</button> </div>
    // [ts] <span>four</span>
    // [ts] <button>five</button>
    // [ts] `);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`div >> internal:chain="button"`, els => els.map(e => e.textContent))).toEqual(['three']);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`div >> internal:chain="span >> internal:or=\\"button\\""`, els => els.map(e => e.textContent))).toEqual(['two', 'three']);
    // [ts] });
    // [ts] test("SE-153 chaining should work with large DOM @smoke", () => {
    // [ts] page.evaluate(() => {
    // [ts] let last = document.body;
    // [ts] let i = 0;
    // [ts] while (i < 100) { // [cap-migrate] for-step: i++
    // [ts] const e = document.createElement("div");
    // [ts] last.appendChild(e);
    // [ts] last = e;
    // [ts] }
    // [ts] const target = document.createElement("span");
    // [ts] target.textContent = "Found me!";
    // [ts] last.appendChild(target);
    // [ts] });
    // Naive implementation generates C(100, 9) ~= 1.9*10^12 entries.
    // [ts] const selectors = [
    // [ts] "div >> div >> div >> div >> div >> div >> div >> div >> span",
    // [ts] "div div div div div div div div span",
    // [ts] "div div >> div div >> div div >> div div >> span",
    // [ts] ];
    // [ts] const counts = [];
    // [ts] const times = [];
    // [ts] for (const selector of selectors) {
    // [ts] const time = Date.now();
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: counts.push(await page.$$eval(selector, els => els.length));
    // [ts] times.push({ selector, time: Date.now() - time });
    // [ts] }
    // [ts] expect_eq(counts, [1, 1, 1]);
    // Uncomment to see performance results.
    // console.log(times);
    // [ts] });
}
