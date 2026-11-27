// Migrated from TypeScript (library/unit/codegen.spec.ts) via cap from -ts

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

import { test, expect_eq } from "cap:test";
import { getAPIRequestCodeGen } from "../../../packages/trace-viewer/src/ui/codegen.cp";

// [cap-test] test.describe("javascript", () => {
  const impl = getAPIRequestCodeGen("javascript");










// [cap-test] });

// [cap-test] test.describe("python", () => {
  const impl = getAPIRequestCodeGen("python");










// [cap-test] });

// [cap-test] test.describe("csharp", () => {
  const impl = getAPIRequestCodeGen("csharp");




// [cap-test] });

// [cap-test] test.describe("java", () => {
  const impl = getAPIRequestCodeGen("java");






// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("CG-164 generatePlaywrightRequestCall", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo?bar=baz",
    // [ts] method: "GET",
    // [ts] headers: [{ name: "User-Agent", value: "Mozilla/5.0" }, { name: "Date", value: "2021-01-01" }],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, "foo")).toEqual(`
    // [ts] page.request.get('http://example.com/foo', {
    // [ts] params: {
    // [ts] bar: 'baz'
    // [ts] },
    // [ts] data: 'foo',
    // [ts] headers: {
    // [ts] 'User-Agent': 'Mozilla/5.0',
    // [ts] Date: '2021-01-01'
    // [ts] }
    // [ts] });`.trim());
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo?bar=baz",
    // [ts] method: "OPTIONS",
    // [ts] headers: [],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, null)).toEqual(`
    // [ts] page.request.fetch('http://example.com/foo', {
    // [ts] method: 'options',
    // [ts] params: {
    // [ts] bar: 'baz'
    // [ts] }
    // [ts] });`.trim());
    // [ts] });
    // [ts] test("CG-165 generatePlaywrightRequestCall with POST method and no body", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo",
    // [ts] method: "POST",
    // [ts] headers: [{ name: "Content-Type", value: "application/json" }],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, null)).toEqual(`
    // [ts] page.request.post('http://example.com/foo', {
    // [ts] headers: {
    // [ts] 'Content-Type': 'application/json'
    // [ts] }
    // [ts] });`.trim());
    // [ts] });
    // [ts] test("CG-166 generatePlaywrightRequestCall with PUT method and JSON body", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo",
    // [ts] method: "PUT",
    // [ts] headers: [{ name: "Content-Type", value: "application/json" }],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, "{\"key\":\"value\"}")).toEqual(`
    // [ts] page.request.put('http://example.com/foo', {
    // [ts] data: '{"key":"value"}',
    // [ts] headers: {
    // [ts] 'Content-Type': 'application/json'
    // [ts] }
    // [ts] });`.trim());
    // [ts] });
    // [ts] test("CG-167 generatePlaywrightRequestCall with PATCH method and form data", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo",
    // [ts] method: "PATCH",
    // [ts] headers: [{ name: "Content-Type", value: "application/x-www-form-urlencoded" }],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, "key=value")).toEqual(`
    // [ts] page.request.patch('http://example.com/foo', {
    // [ts] data: 'key=value',
    // [ts] headers: {
    // [ts] 'Content-Type': 'application/x-www-form-urlencoded'
    // [ts] }
    // [ts] });`.trim());
    // [ts] });
    // [ts] test("CG-168 generatePlaywrightRequestCall with DELETE method and custom header", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo",
    // [ts] method: "DELETE",
    // [ts] headers: [{ name: "Authorization", value: "Bearer token" }],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, null)).toEqual(`
    // [ts] page.request.delete('http://example.com/foo', {
    // [ts] headers: {
    // [ts] Authorization: 'Bearer token'
    // [ts] }
    // [ts] });`.trim());
    // [ts] });
    // [ts] test("CG-169 generatePlaywrightRequestCall with HEAD method", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo",
    // [ts] method: "HEAD",
    // [ts] headers: [],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, null)).toEqual(`
    // [ts] page.request.head('http://example.com/foo');`.trim());
    // [ts] });
    // [ts] test("CG-170 generatePlaywrightRequestCall with complex query parameters", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo?bar=baz&qux=quux",
    // [ts] method: "GET",
    // [ts] headers: [],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, null)).toEqual(`
    // [ts] page.request.get('http://example.com/foo', {
    // [ts] params: {
    // [ts] bar: 'baz',
    // [ts] qux: 'quux'
    // [ts] }
    // [ts] });`.trim());
    // [ts] });
    // [ts] test("CG-171 generatePlaywrightRequestCall with multiple headers", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo",
    // [ts] method: "GET",
    // [ts] headers: [
    // [ts] { name: "User-Agent", value: "Mozilla/5.0" },
    // [ts] { name: "Accept", value: "application/json" },
    // [ts] { name: "Authorization", value: "Bearer token" }
    // [ts] ],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, null)).toEqual(`
    // [ts] page.request.get('http://example.com/foo', {
    // [ts] headers: {
    // [ts] 'User-Agent': 'Mozilla/5.0',
    // [ts] Accept: 'application/json',
    // [ts] Authorization: 'Bearer token'
    // [ts] }
    // [ts] });`.trim());
    // [ts] });
    // [ts] test("CG-172 escape sequences", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo",
    // [ts] method: "GET",
    // [ts] headers: [
    // [ts] { name: "F\\o", value: "B\\r" },
    // [ts] ],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, null)).toEqual(`
    // [ts] page.request.get('http://example.com/foo', {
    // [ts] headers: {
    // [ts] 'F\\\\o': 'B\\\\r'
    // [ts] }
    // [ts] });`.trim());
    // [ts] });
    // [ts] test("CG-173 generatePlaywrightRequestCall", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo?bar=baz",
    // [ts] method: "GET",
    // [ts] headers: [{ name: "User-Agent", value: "Mozilla/5.0" }, { name: "Date", value: "2021-01-01" }],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, "foo")).toEqual(`
    // [ts] page.request.get(
    // [ts] "http://example.com/foo",
    // [ts] params={
    // [ts] "bar": "baz"
    // [ts] },
    // [ts] data="foo",
    // [ts] headers={
    // [ts] "User-Agent": "Mozilla/5.0",
    // [ts] "Date": "2021-01-01"
    // [ts] }
    // [ts] )`.trim());
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo?bar=baz",
    // [ts] method: "OPTIONS",
    // [ts] headers: [],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, null)).toEqual(`
    // [ts] page.request.fetch(
    // [ts] "http://example.com/foo",
    // [ts] method="options",
    // [ts] params={
    // [ts] "bar": "baz"
    // [ts] }
    // [ts] )`.trim());
    // [ts] });
    // [ts] test("CG-174 generatePlaywrightRequestCall with POST method and no body", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo",
    // [ts] method: "POST",
    // [ts] headers: [{ name: "Content-Type", value: "application/json" }],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, null)).toEqual(`
    // [ts] page.request.post(
    // [ts] "http://example.com/foo",
    // [ts] headers={
    // [ts] "Content-Type": "application/json"
    // [ts] }
    // [ts] )`.trim());
    // [ts] });
    // [ts] test("CG-175 generatePlaywrightRequestCall with PUT method and JSON body", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo",
    // [ts] method: "PUT",
    // [ts] headers: [{ name: "Content-Type", value: "application/json" }],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, "{\"key\":\"value\"}")).toEqual(`
    // [ts] page.request.put(
    // [ts] "http://example.com/foo",
    // [ts] data="{\\"key\\":\\"value\\"}",
    // [ts] headers={
    // [ts] "Content-Type": "application/json"
    // [ts] }
    // [ts] )`.trim());
    // [ts] });
    // [ts] test("CG-176 generatePlaywrightRequestCall with PATCH method and form data", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo",
    // [ts] method: "PATCH",
    // [ts] headers: [{ name: "Content-Type", value: "application/x-www-form-urlencoded" }],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, "key=value")).toEqual(`
    // [ts] page.request.patch(
    // [ts] "http://example.com/foo",
    // [ts] data="key=value",
    // [ts] headers={
    // [ts] "Content-Type": "application/x-www-form-urlencoded"
    // [ts] }
    // [ts] )`.trim());
    // [ts] });
    // [ts] test("CG-177 generatePlaywrightRequestCall with DELETE method and custom header", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo",
    // [ts] method: "DELETE",
    // [ts] headers: [{ name: "Authorization", value: "Bearer token" }],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, null)).toEqual(`
    // [ts] page.request.delete(
    // [ts] "http://example.com/foo",
    // [ts] headers={
    // [ts] "Authorization": "Bearer token"
    // [ts] }
    // [ts] )`.trim());
    // [ts] });
    // [ts] test("CG-178 generatePlaywrightRequestCall with HEAD method", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo",
    // [ts] method: "HEAD",
    // [ts] headers: [],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, null)).toEqual(`
    // [ts] page.request.head("http://example.com/foo")`.trim());
    // [ts] });
    // [ts] test("CG-179 generatePlaywrightRequestCall with complex query parameters", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo?bar=baz&qux=quux",
    // [ts] method: "GET",
    // [ts] headers: [],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, null)).toEqual(`
    // [ts] page.request.get(
    // [ts] "http://example.com/foo",
    // [ts] params={
    // [ts] "bar": "baz",
    // [ts] "qux": "quux"
    // [ts] }
    // [ts] )`.trim());
    // [ts] });
    // [ts] test("CG-180 generatePlaywrightRequestCall with multiple headers", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo",
    // [ts] method: "GET",
    // [ts] headers: [
    // [ts] { name: "User-Agent", value: "Mozilla/5.0" },
    // [ts] { name: "Accept", value: "application/json" },
    // [ts] { name: "Authorization", value: "Bearer token" }
    // [ts] ],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, null)).toEqual(`
    // [ts] page.request.get(
    // [ts] "http://example.com/foo",
    // [ts] headers={
    // [ts] "User-Agent": "Mozilla/5.0",
    // [ts] "Accept": "application/json",
    // [ts] "Authorization": "Bearer token"
    // [ts] }
    // [ts] )`.trim());
    // [ts] });
    // [ts] test("CG-181 escape sequences", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo",
    // [ts] method: "GET",
    // [ts] headers: [
    // [ts] { name: "F\\o", value: "B\\r" },
    // [ts] ],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, null)).toEqual(`
    // [ts] page.request.get(
    // [ts] "http://example.com/foo",
    // [ts] headers={
    // [ts] "F\\\\o": "B\\\\r"
    // [ts] }
    // [ts] )`.trim());
    // [ts] });
    // [ts] test("CG-182 generatePlaywrightRequestCall", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo?bar=baz",
    // [ts] method: "GET",
    // [ts] headers: [{ name: "User-Agent", value: "Mozilla/5.0" }, { name: "Date", value: "2021-01-01" }],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, "foo")).toEqual(`
    // [ts] request.GetAsync("http://example.com/foo", new() {
    // [ts] Params = new() {
    // [ts] ["bar"] = "baz"
    // [ts] },
    // [ts] Data = "foo",
    // [ts] Headers = new() {
    // [ts] ["User-Agent"] = "Mozilla/5.0",
    // [ts] ["Date"] = "2021-01-01"
    // [ts] }
    // [ts] });`.trim());
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo?bar=baz",
    // [ts] method: "OPTIONS",
    // [ts] headers: [],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, null)).toEqual(`
    // [ts] request.FetchAsync("http://example.com/foo", new() {
    // [ts] Method = "options",
    // [ts] Params = new() {
    // [ts] ["bar"] = "baz"
    // [ts] }
    // [ts] });`.trim());
    // [ts] });
    // [ts] test("CG-183 generatePlaywrightRequestCall with POST method and no body", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo",
    // [ts] method: "POST",
    // [ts] headers: [{ name: "Content-Type", value: "application/json" }],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, null)).toEqual(`
    // [ts] request.PostAsync("http://example.com/foo", new() {
    // [ts] Headers = new() {
    // [ts] ["Content-Type"] = "application/json"
    // [ts] }
    // [ts] });`.trim());
    // [ts] });
    // [ts] test("CG-184 generatePlaywrightRequestCall with PUT method and JSON body", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo",
    // [ts] method: "PUT",
    // [ts] headers: [{ name: "Content-Type", value: "application/json" }],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, "{\"key\":\"value\"}")).toEqual(`
    // [ts] request.PutAsync("http://example.com/foo", new() {
    // [ts] Data = "{\\"key\\":\\"value\\"}",
    // [ts] Headers = new() {
    // [ts] ["Content-Type"] = "application/json"
    // [ts] }
    // [ts] });`.trim());
    // [ts] });
    // [ts] test("CG-185 escape sequences", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo",
    // [ts] method: "GET",
    // [ts] headers: [
    // [ts] { name: "F\\o", value: "B\\r" },
    // [ts] ],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, null)).toEqual(`
    // [ts] request.GetAsync("http://example.com/foo", new() {
    // [ts] Headers = new() {
    // [ts] ["F\\\\o"] = "B\\\\r"
    // [ts] }
    // [ts] });`.trim());
    // [ts] });
    // [ts] test("CG-186 generatePlaywrightRequestCall", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo?bar=baz",
    // [ts] method: "GET",
    // [ts] headers: [{ name: "User-Agent", value: "Mozilla/5.0" }, { name: "Date", value: "2021-01-01" }],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, "foo")).toEqual(`
    // [ts] request.get("http://example.com/foo", RequestOptions.create()
    // [ts] .setQueryParam("bar", "baz")
    // [ts] .setData("foo")
    // [ts] .setHeader("User-Agent", "Mozilla/5.0")
    // [ts] .setHeader("Date", "2021-01-01")
    // [ts] );`.trim());
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo?bar=baz",
    // [ts] method: "OPTIONS",
    // [ts] headers: [],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, null)).toEqual(`
    // [ts] request.fetch("http://example.com/foo", RequestOptions.create()
    // [ts] .setMethod("options")
    // [ts] .setQueryParam("bar", "baz")
    // [ts] );`.trim());
    // [ts] });
    // [ts] test("CG-187 generatePlaywrightRequestCall with POST method and no body", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo",
    // [ts] method: "POST",
    // [ts] headers: [{ name: "Content-Type", value: "application/json" }],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, null)).toEqual(`
    // [ts] request.post("http://example.com/foo", RequestOptions.create()
    // [ts] .setHeader("Content-Type", "application/json")
    // [ts] );`.trim());
    // [ts] });
    // [ts] test("CG-188 generatePlaywrightRequestCall with PUT method and JSON body", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo",
    // [ts] method: "PUT",
    // [ts] headers: [{ name: "Content-Type", value: "application/json" }],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, "{\"key\":\"value\"}")).toEqual(`
    // [ts] request.put("http://example.com/foo", RequestOptions.create()
    // [ts] .setData("{\\"key\\":\\"value\\"}")
    // [ts] .setHeader("Content-Type", "application/json")
    // [ts] );`.trim());
    // [ts] });
    // [ts] test("CG-189 generatePlaywrightRequestCall with PATCH method and form data", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo",
    // [ts] method: "PATCH",
    // [ts] headers: [{ name: "Content-Type", value: "application/x-www-form-urlencoded" }],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, "key=value")).toEqual(`
    // [ts] request.patch("http://example.com/foo", RequestOptions.create()
    // [ts] .setData("key=value")
    // [ts] .setHeader("Content-Type", "application/x-www-form-urlencoded")
    // [ts] );`.trim());
    // [ts] });
    // [ts] test("CG-190 generatePlaywrightRequestCall with DELETE method and custom header", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo",
    // [ts] method: "DELETE",
    // [ts] headers: [{ name: "Authorization", value: "Bearer token" }],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, null)).toEqual(`
    // [ts] request.delete("http://example.com/foo", RequestOptions.create()
    // [ts] .setHeader("Authorization", "Bearer token")
    // [ts] );`.trim());
    // [ts] });
    // [ts] test("CG-191 escape sequences", () => {
    // [ts] expect(impl.generatePlaywrightRequestCall({
    // [ts] url: "http://example.com/foo",
    // [ts] method: "GET",
    // [ts] headers: [
    // [ts] { name: "F\\o", value: "B\\r" },
    // [ts] ],
    // [ts] httpVersion: "1.1",
    // [ts] cookies: [],
    // [ts] queryString: [],
    // [ts] headersSize: 0,
    // [ts] bodySize: 0,
    // [ts] comment: "",
    // [ts] }, null)).toEqual(`
    // [ts] request.get(\"http://example.com/foo\", RequestOptions.create()
    // [ts] .setHeader(\"F\\\\o\", \"B\\\\r\")
    // [ts] );`.trim());
    // [ts] });
}
