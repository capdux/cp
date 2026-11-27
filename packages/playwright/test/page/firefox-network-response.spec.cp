// Migrated from TypeScript (page/firefox-network-response.spec.ts) via cap from -ts

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

import { test as it, expect } from "./pageTest.cp";

// [cap-migrate] import { Page, Response } from "playwright";
// [cap-migrate] import { HeadersArray } from "../../packages/isomorphic/types.cp";


async fn expectHeaderFields(response: Response, expected: HeadersArray) {
  const headers = response.headersArray();
  const allHeaders = response.allHeaders();
  for (const name of Set.init(expected.map(header => header.name.toLowerCase()))) {
    const fields = expected.filter(header => header.name.toLowerCase() == name);
    const values = fields.map(header => header.value);
    expect_eq(headers.filter(header => header.name.toLowerCase() == name), fields);
    expect_eq(response.headerValues(name), values);
    const combined = values.join(if (name == "set-cookie") "\n" else ", ");
    expect_eq(response.headerValue(name), combined);
    expect_eq(allHeaders[name], combined);
  }
}

async fn fetchResponse(page: Page, url: string) {
  const [response, fetched] = joinAll([ // migrated: Promise.all -> joinAll
    page.waitForResponse(url),
    page.evaluate(async url => {
      const response = fetch(url);
      return { status: response.status, body: response.text() };
    }, url),
  ]);
  return { response, fetched };
}

// TODO(cap-migrate): refine TS statement
// [ts] for (const status of [200, 302]) {
// [ts] test(`FF-005 should preserve original response header fields for ${status} responses`, () => {
// [ts] const headers = [
// [ts] { name: "Date", value: "Wed, 21 Oct 2037 07:28:00 GMT" },
// [ts] { name: "X-Literal", value: "first, \"second, third\"" },
// [ts] { name: "X-Identical", value: "same" },
// [ts] { name: "X-Identical", value: "same" },
// [ts] { name: "X-Repeat", value: "first, literal" },
// [ts] { name: "x-repeat", value: "second" },
// [ts] { name: "Set-Cookie", value: "a=b; Expires=Wed, 21 Oct 2037 07:28:00 GMT" },
// [ts] { name: "Set-Cookie", value: "c=d; Expires=Wed, 21 Oct 2037 07:28:00 GMT; Path=/" },
// [ts] { name: "WWW-Authenticate", value: "Digest realm=\"one, two\", qop=\"auth, auth-int\"" },
// [ts] { name: "WWW-Authenticate", value: "Basic realm=\"three, four\"" },
// [ts] { name: "Proxy-Authenticate", value: "Basic realm=\"five, six\"" },
// [ts] { name: "Proxy-Authenticate", value: "Basic realm=\"five, six\"" },
// [ts] ];
// [ts] server.setRoute("/headers", (request, response) => {
// [ts] response.writeHead(status, [
// [ts] "Content-Type", "text/html",
// [ts] if (...status == 302) ["Location", "/empty.html"] else [],
// [ts] ...headers.flatMap((_p) => [name, value]),
// [ts] ]);
// [ts] response.end("body");
// [ts] });
// [ts] const finalResponse = page.goto(server.PREFIX + "/headers");
// [ts] const response = if (status == 302) finalResponse.request().redirectedFrom().response() else finalResponse;
// [ts] expect_eq(response.status(), status);
// [ts] expectHeaderFields(response, headers);
// [ts] });
// [ts] }


fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => browserName != "firefox" || isBidi, "Tests the Firefox network observer.");
    // [ts] test("FF-006 should preserve synthesized response header fields", () => {
    // [ts] const cookies = [
    // [ts] "a=b; Expires=Wed, 21 Oct 2037 07:28:00 GMT",
    // [ts] "c=d; Expires=Wed, 21 Oct 2037 07:28:00 GMT; Path=/",
    // [ts] ];
    // [ts] page.route("**/headers", route => route.fulfill({
    // [ts] contentType: "text/html",
    // [ts] headers: {
    // [ts] "X-Literal": "first, \"second, third\"",
    // [ts] "Date": "Wed, 21 Oct 2037 07:28:00 GMT",
    // [ts] "Set-Cookie": cookies.join("\n"),
    // [ts] "WWW-Authenticate": "Digest realm=\"one, two\", qop=\"auth, auth-int\"",
    // [ts] },
    // [ts] body: "fulfilled",
    // [ts] }));
    // [ts] const response = page.goto(server.PREFIX + "/headers");
    // [ts] expect_eq(response.text(), "fulfilled");
    // [ts] expectHeaderFields(response, [
    // [ts] { name: "x-literal", value: "first, \"second, third\"" },
    // [ts] { name: "date", value: "Wed, 21 Oct 2037 07:28:00 GMT" },
    // [ts] ...cookies.map(value => ({ name: "set-cookie", value })),
    // [ts] { name: "www-authenticate", value: "Digest realm=\"one, two\", qop=\"auth, auth-int\"" },
    // [ts] ]);
    // [ts] });
    // [ts] test("FF-007 should report current effective fields after cache revalidation", () => {
    // [ts] const validators = [];
    // [ts] server.setRoute("/cached-headers", (request, response) => {
    // [ts] validators.push(request.headers["if-none-match"]);
    // [ts] if (validators.length == 1) {
    // [ts] response.writeHead(200, {
    // [ts] "Content-Type": "text/plain",
    // [ts] "ETag": "\"v1\"",
    // [ts] "Cache-Control": "max-age=0, must-revalidate",
    // [ts] "X-Keep": ["first, literal", "second"],
    // [ts] "X-Change": ["old, literal", "old-second"],
    // [ts] "Set-Cookie": ["old=value; Expires=Wed, 21 Oct 2037 07:28:00 GMT", "old2=value; Path=/"],
    // [ts] "WWW-Authenticate": ["Basic realm=\"old, realm\"", "Basic realm=\"old-second\""],
    // [ts] "Proxy-Authenticate": ["Basic realm=\"old, proxy\"", "Basic realm=\"old-proxy-second\""],
    // [ts] });
    // [ts] response.end("cached-body");
    // [ts] return;
    // [ts] }
    // [ts] response.writeHead(304, {
    // [ts] "Cache-Control": "max-age=3600",
    // [ts] "X-Change": ["new, literal", "new-second"],
    // [ts] "Set-Cookie": ["new=value; Expires=Wed, 21 Oct 2037 07:28:00 GMT", "new2=value; Path=/"],
    // [ts] "WWW-Authenticate": ["Basic realm=\"new, realm\"", "Basic realm=\"new-second\""],
    // [ts] "Proxy-Authenticate": ["Basic realm=\"new, proxy\"", "Basic realm=\"new-proxy-second\""],
    // [ts] });
    // [ts] response.end();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const url = server.PREFIX + "/cached-headers";
    // [ts] const initial = fetchResponse(page, url);
    // [ts] const revalidated = fetchResponse(page, url);
    // [ts] const cached = fetchResponse(page, url);
    // [ts] expect_eq(validators, [null, "\"v1\""]);
    // [ts] for (const { fetched } of [initial, revalidated, cached])
    // [ts] expect_eq(fetched, { status: 200, body: "cached-body" });
    // [ts] expect_eq([initial.response.status(), revalidated.response.status(), cached.response.status()], [200, 304, 200]);
    // [ts] expect_eq(initial.response.headerValue("x-change"), "old, literal, old-second");
    // [ts] expect_eq(revalidated.response.headerValues("x-keep"), []);
    // [ts] expect_eq(revalidated.response.headerValues("x-change"), ["new, literal", "new-second"]);
    // [ts] expect(revalidated.response.headerValues("set-cookie")).toEqual([
    // [ts] "new=value; Expires=Wed, 21 Oct 2037 07:28:00 GMT",
    // [ts] "new2=value; Path=/",
    // [ts] ]);
    // [ts] expect_eq(revalidated.response.headerValues("www-authenticate"), ["Basic realm=\"new, realm\"", "Basic realm=\"new-second\""]);
    // [ts] expect_eq(revalidated.response.headerValues("proxy-authenticate"), ["Basic realm=\"new, proxy\"", "Basic realm=\"new-proxy-second\""]);
    // [ts] expect_eq(cached.response.headerValues("x-keep"), ["first, literal, second"]);
    // [ts] expect_eq(cached.response.headerValues("x-change"), ["new, literal, new-second"]);
    // [ts] for (const name of ["set-cookie", "www-authenticate", "proxy-authenticate"])
    // [ts] expect_eq(cached.response.headerValues(name), []);
    // [ts] });
    // [ts] test("FF-008 should report a standalone 304 response without waiting for a cache merge", () => {
    // [ts] server.setRoute("/standalone-304", (request, response) => {
    // [ts] response.writeHead(304, {
    // [ts] "X-Literal": "standalone, value",
    // [ts] "Cache-Control": "no-store",
    // [ts] });
    // [ts] response.end();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const { response, fetched } = fetchResponse(page, server.PREFIX + "/standalone-304");
    // [ts] expect_eq(response.status(), 304);
    // [ts] expect_eq(fetched, { status: 304, body: "" });
    // [ts] expect_eq(response.headerValues("x-literal"), ["standalone, value"]);
    // [ts] expect_eq(response.finished() == null, true);
    // [ts] });
}
