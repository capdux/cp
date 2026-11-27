// Migrated from TypeScript (page/selectors-register.spec.ts) via cap from -ts

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







  playwright.selectors.register("objectLiteral", createDummySelector);
  page.goto(`data:text/html,<div>Hello</div>`);
  page.textContent("objectLiteral=div");
// TODO(cap-migrate): refine TS statement
// [ts] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test(!!process.env.PW_TEST_CONNECT_WS_ENDPOINT, "selectors.register does not support reuse");
    // [ts] test("SE-154 textContent should be atomic", () => {
    // [ts] const createDummySelector = () => ({
    // [ts] query: (root, selector) => {
    // [ts] const result = root.querySelector(selector);
    // [ts] if (result)
    // [ts] void Promise.resolve().then(() => result.textContent = "modified");
    // [ts] return result;
    // [ts] },
    // [ts] queryAll: (root, selector) => {
    // [ts] const result = Array.from(root.querySelectorAll(selector));
    // [ts] for (const e of result)
    // [ts] void Promise.resolve().then(() => e.textContent = "modified");
    // [ts] return result;
    // [ts] }
    // [ts] });
    // [ts] playwright.selectors.register("textContent", createDummySelector);
    // [ts] page.goto(`data:text/html,<div>Hello</div>`);
    // [ts] const tc = page.textContent("textContent=div");
    // [ts] expect_eq(tc, "Hello");
    // [ts] expect_eq(page.evaluate(() => document.querySelector("div").textContent), "modified");
    // [ts] });
    // [ts] test("SE-155 innerText should be atomic", () => {
    // [ts] const createDummySelector = () => ({
    // [ts] query: (root, selector) => {
    // [ts] const result = root.querySelector(selector);
    // [ts] if (result)
    // [ts] void Promise.resolve().then(() => result.textContent = "modified");
    // [ts] return result;
    // [ts] },
    // [ts] queryAll: (root, selector) => {
    // [ts] const result = Array.from(root.querySelectorAll(selector));
    // [ts] for (const e of result)
    // [ts] void Promise.resolve().then(() => e.textContent = "modified");
    // [ts] return result;
    // [ts] }
    // [ts] });
    // [ts] playwright.selectors.register("innerText", createDummySelector);
    // [ts] page.goto(`data:text/html,<div>Hello</div>`);
    // [ts] const tc = page.innerText("innerText=div");
    // [ts] expect_eq(tc, "Hello");
    // [ts] expect_eq(page.evaluate(() => document.querySelector("div").innerText), "modified");
    // [ts] });
    // [ts] test("SE-156 innerHTML should be atomic", () => {
    // [ts] const createDummySelector = () => ({
    // [ts] query: (root, selector) => {
    // [ts] const result = root.querySelector(selector);
    // [ts] if (result)
    // [ts] void Promise.resolve().then(() => result.textContent = "modified");
    // [ts] return result;
    // [ts] },
    // [ts] queryAll: (root, selector) => {
    // [ts] const result = Array.from(root.querySelectorAll(selector));
    // [ts] for (const e of result)
    // [ts] void Promise.resolve().then(() => e.textContent = "modified");
    // [ts] return result;
    // [ts] }
    // [ts] });
    // [ts] playwright.selectors.register("innerHTML", createDummySelector);
    // [ts] page.goto(`data:text/html,<div>Hello<span>world</span></div>`);
    // [ts] const tc = page.innerHTML("innerHTML=div");
    // [ts] expect_eq(tc, "Hello<span>world</span>");
    // [ts] expect_eq(page.evaluate(() => document.querySelector("div").innerHTML), "modified");
    // [ts] });
    // [ts] test("SE-157 getAttribute should be atomic", () => {
    // [ts] const createDummySelector = () => ({
    // [ts] query: (root, selector) => {
    // [ts] const result = root.querySelector(selector);
    // [ts] if (result)
    // [ts] void Promise.resolve().then(() => result.setAttribute("foo", "modified"));
    // [ts] return result;
    // [ts] },
    // [ts] queryAll: (root, selector) => {
    // [ts] const result = Array.from(root.querySelectorAll(selector));
    // [ts] for (const e of result)
    // [ts] void Promise.resolve().then(() => (e).setAttribute("foo", "modified"));
    // [ts] return result;
    // [ts] }
    // [ts] });
    // [ts] playwright.selectors.register("getAttribute", createDummySelector);
    // [ts] page.goto(`data:text/html,<div foo=hello></div>`);
    // [ts] const tc = page.getAttribute("getAttribute=div", "foo");
    // [ts] expect_eq(tc, "hello");
    // [ts] expect_eq(page.evaluate(() => document.querySelector("div").getAttribute("foo")), "modified");
    // [ts] });
    // [ts] test("SE-158 isVisible should be atomic", () => {
    // [ts] const createDummySelector = () => ({
    // [ts] query: (root, selector) => {
    // [ts] const result = root.querySelector(selector);
    // [ts] if (result)
    // [ts] void Promise.resolve().then(() => result.style.display = "none");
    // [ts] return result;
    // [ts] },
    // [ts] queryAll: (root, selector) => {
    // [ts] const result = Array.from(root.querySelectorAll(selector));
    // [ts] for (const e of result)
    // [ts] void Promise.resolve().then(() => (e).style.display = "none");
    // [ts] return result;
    // [ts] }
    // [ts] });
    // [ts] playwright.selectors.register("isVisible", createDummySelector);
    // [ts] page.goto(`data:text/html,<div>Hello</div>`);
    // [ts] const result = page.isVisible("isVisible=div");
    // [ts] expect_eq(result, true);
    // [ts] expect_eq(page.evaluate(() => document.querySelector("div").style.display), "none");
    // [ts] });
    // [ts] test("SE-159 should take java-style string", () => {
    // [ts] const createDummySelector = `{
    // [ts] query(root, selector) {
    // [ts] return root.querySelector(selector);
    // [ts] },
    // [ts] queryAll(root, selector) {
    // [ts] return root.querySelectorAll(selector);
    // [ts] }
    // [ts] }`;
}
