// Migrated from TypeScript (src/cookies.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
import { test, expect_eq } from "cap:test";

import { expectCookieEquals, getTestState, launch, setupTestBrowserHooks } from "./mocha-utils.cp";
import { attachFrame } from "./utils.cp";

// [cap-test] describe("Cookie specs", () => {
  setupTestBrowserHooks();

  // [cap-test] describe("Page.cookies", () => {
  // [cap-test] });
  // [cap-test] describe("Page.setCookie", () => {
  // [cap-test] });

  // [cap-test] describe("Page.deleteCookie", () => {
  // [cap-test] });
// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should return no cookies in pristine browser context", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expectCookieEquals(page.cookies(), []);
    // [ts] });
    // [ts] test("should get a cookie", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] document.cookie = "username=John Doe";
    // [ts] });
    // [ts] expectCookieEquals(page.cookies(), [
    // [ts] {
    // [ts] name: "username",
    // [ts] value: "John Doe",
    // [ts] domain: "localhost",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] size: 16,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] session: true,
    // [ts] sourceScheme: "NonSecure",
    // [ts] },
    // [ts] ]);
    // [ts] });
    // [ts] test("should properly report httpOnly cookie", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.setRoute("/empty.html", (_req, res) => {
    // [ts] res.setHeader("Set-Cookie", "a=b; HttpOnly; Path=/");
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const cookies = page.cookies();
    // [ts] expect(cookies).toHaveLength(1);
    // [ts] expect_eq(cookies[0].httpOnly, true);
    // [ts] });
    // [ts] test("should properly report \"Strict\" sameSite cookie", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.setRoute("/empty.html", (_req, res) => {
    // [ts] res.setHeader("Set-Cookie", "a=b; SameSite=Strict");
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const cookies = page.cookies();
    // [ts] expect(cookies).toHaveLength(1);
    // [ts] expect_eq(cookies[0].sameSite, "Strict");
    // [ts] });
    // [ts] test("should properly report \"Lax\" sameSite cookie", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.setRoute("/empty.html", (_req, res) => {
    // [ts] res.setHeader("Set-Cookie", "a=b; SameSite=Lax");
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const cookies = page.cookies();
    // [ts] expect(cookies).toHaveLength(1);
    // [ts] expect_eq(cookies[0].sameSite, "Lax");
    // [ts] });
    // [ts] test("should properly report \"Default\" sameSite cookie", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setCookie({
    // [ts] name: "a",
    // [ts] value: "b",
    // [ts] sameSite: "Default",
    // [ts] });
    // [ts] const cookies = page.cookies();
    // [ts] expect(cookies).toHaveLength(1);
    // [ts] expect_eq(cookies[0].name, "a");
    // Different browsers have different sameSite values for the "Default" sameSite.
    // [ts] expect_eq(std.mem.indexOf(u8, ["Default", "Lax", null], cookies[0].sameSite) != null, true);
    // [ts] });
    // [ts] test("should be able to delete \"Default\" sameSite cookie", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setCookie({
    // [ts] name: "a",
    // [ts] value: "b",
    // [ts] sameSite: "Default",
    // [ts] });
    // [ts] const cookies = page.cookies();
    // [ts] expect(
    // [ts] cookies.find(c => {
    // [ts] return c.name == "a";
    // [ts] }),
    // [ts] ).toBeDefined();
    // [ts] page.deleteCookie(...cookies);
    // [ts] expect(page.cookies()).toHaveLength(0);
    // [ts] });
    // [ts] test("should report \"Default\" sameSite cookie when not specified", () => {
    // [ts] const {page, server, defaultBrowserOptions, isFirefox} =
    // [ts] getTestState();
    // [ts] server.setRoute("/empty.html", (_req, res) => {
    // [ts] res.setHeader("Set-Cookie", "a=b");
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const cookies = page.cookies();
    // [ts] expect(cookies).toHaveLength(1);
    // [ts] if (defaultBrowserOptions.protocol == "webDriverBiDi" && isFirefox) {
    // [ts] expect_eq(cookies[0].sameSite, "Default");
    // [ts] }
    // [ts] });
    // [ts] test("should get multiple cookies", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] document.cookie = "username=John Doe";
    // [ts] document.cookie = "password=1234";
    // [ts] });
    // [ts] const cookies = page.cookies();
    // [ts] cookies.sort((a, b) => {
    // [ts] return a.name.localeCompare(b.name);
    // [ts] });
    // [ts] expectCookieEquals(cookies, [
    // [ts] {
    // [ts] name: "password",
    // [ts] value: "1234",
    // [ts] domain: "localhost",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] size: 12,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] session: true,
    // [ts] sourceScheme: "NonSecure",
    // [ts] },
    // [ts] {
    // [ts] name: "username",
    // [ts] value: "John Doe",
    // [ts] domain: "localhost",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] size: 16,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] session: true,
    // [ts] sourceScheme: "NonSecure",
    // [ts] },
    // [ts] ]);
    // [ts] });
    // [ts] test("should get cookies from multiple urls", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setCookie(
    // [ts] {
    // [ts] url: "https://foo.com",
    // [ts] name: "doggo",
    // [ts] value: "woofs",
    // [ts] },
    // [ts] {
    // [ts] url: "https://bar.com",
    // [ts] name: "catto",
    // [ts] value: "purrs",
    // [ts] },
    // [ts] {
    // [ts] url: "https://baz.com",
    // [ts] name: "birdo",
    // [ts] value: "tweets",
    // [ts] },
    // [ts] );
    // [ts] const cookies = page.cookies("https://foo.com", "https://baz.com");
    // [ts] cookies.sort((a, b) => {
    // [ts] return a.name.localeCompare(b.name);
    // [ts] });
    // [ts] expectCookieEquals(cookies, [
    // [ts] {
    // [ts] name: "birdo",
    // [ts] value: "tweets",
    // [ts] domain: "baz.com",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] size: 11,
    // [ts] httpOnly: false,
    // [ts] session: true,
    // [ts] sourceScheme: "Secure",
    // [ts] },
    // [ts] {
    // [ts] name: "doggo",
    // [ts] value: "woofs",
    // [ts] domain: "foo.com",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] size: 10,
    // [ts] httpOnly: false,
    // [ts] session: true,
    // [ts] sourceScheme: "Secure",
    // [ts] },
    // [ts] ]);
    // [ts] });
    // [ts] test("should get cookies from subdomain if the domain field allows it", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setCookie({
    // [ts] url: "https://base_domain.com",
    // [ts] domain: ".base_domain.com",
    // [ts] name: "doggo",
    // [ts] value: "woofs",
    // [ts] });
    // [ts] const cookies = page.cookies("https://sub_domain.base_domain.com");
    // [ts] expect(cookies).toHaveLength(1);
    // [ts] });
    // [ts] test("should get cookies from the domain itself if the domain field allows subdomains", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setCookie({
    // [ts] url: "https://base_domain.com",
    // [ts] domain: ".base_domain.com",
    // [ts] name: "doggo",
    // [ts] value: "woofs",
    // [ts] });
    // [ts] const cookies = page.cookies("https://base_domain.com");
    // [ts] expect(cookies).toHaveLength(1);
    // [ts] });
    // [ts] test("should not get cookies from subdomain if the cookie is for top-level domain", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setCookie({
    // [ts] url: "https://base_domain.com",
    // [ts] domain: "base_domain.com",
    // [ts] name: "doggo",
    // [ts] value: "woofs",
    // [ts] });
    // [ts] const cookies = page.cookies("https://sub_domain.base_domain.com");
    // [ts] expect(cookies).toHaveLength(0);
    // [ts] });
    // [ts] test("should get cookies from nested path", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setCookie({
    // [ts] url: "https://foo.com",
    // [ts] path: "/some_path",
    // [ts] name: "doggo",
    // [ts] value: "woofs",
    // [ts] });
    // [ts] const cookies = page.cookies(
    // [ts] "https://foo.com/some_path/nested_path",
    // [ts] );
    // [ts] expect(cookies).toHaveLength(1);
    // [ts] });
    // [ts] test("should not get cookies from not nested path", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setCookie({
    // [ts] url: "https://foo.com",
    // [ts] path: "/some_path",
    // [ts] name: "doggo",
    // [ts] value: "woofs",
    // [ts] });
    // [ts] const cookies = page.cookies(
    // [ts] "https://foo.com/some_path_looks_like_nested",
    // [ts] );
    // [ts] expect(cookies).toHaveLength(0);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setCookie({
    // [ts] name: "password",
    // [ts] value: "123456",
    // [ts] });
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.cookie;
    // [ts] }),
    // [ts] ).toEqual("password=123456");
    // [ts] });
    // [ts] test("should isolate cookies in browser contexts", () => {
    // [ts] const {page, server, browser} = getTestState();
    // [ts] const anotherContext = browser.createBrowserContext();
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const anotherPage = anotherContext.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] anotherPage.goto(server.EMPTY_PAGE);
    // [ts] page.setCookie({name: "page1cookie", value: "page1value"});
    // [ts] anotherPage.setCookie({name: "page2cookie", value: "page2value"});
    // [ts] const cookies1 = page.cookies();
    // [ts] const cookies2 = anotherPage.cookies();
    // [ts] expect(cookies1).toHaveLength(1);
    // [ts] expect(cookies2).toHaveLength(1);
    // [ts] expect_eq(cookies1[0].name, "page1cookie");
    // [ts] expect_eq(cookies1[0].value, "page1value");
    // [ts] expect_eq(cookies2[0].name, "page2cookie");
    // [ts] expect_eq(cookies2[0].value, "page2value");
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] anotherContext.close();
    // [ts] }
    // [ts] });
    // [ts] test("should set multiple cookies", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setCookie(
    // [ts] {
    // [ts] name: "password",
    // [ts] value: "123456",
    // [ts] },
    // [ts] {
    // [ts] name: "foo",
    // [ts] value: "bar",
    // [ts] },
    // [ts] );
    // [ts] const cookieStrings = page.evaluate(() => {
    // [ts] const cookies = document.cookie.split(";");
    // [ts] return cookies
    // [ts] .map(cookie => {
    // [ts] return cookie.trim();
    // [ts] })
    // [ts] .sort();
    // [ts] });
    // [ts] expect_eq(cookieStrings, ["foo=bar", "password=123456"]);
    // [ts] });
    // [ts] test("should have |expires| set to |-1| for session cookies", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setCookie({
    // [ts] name: "password",
    // [ts] value: "123456",
    // [ts] });
    // [ts] const cookies = page.cookies();
    // [ts] expect_eq(cookies[0].session, true);
    // [ts] expect_eq(cookies[0].expires, -1);
    // [ts] });
    // [ts] test("should set a cookie with a fractional |expires|", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const expires = Math.floor(Date.now() / 1000) + 24 * 60 * 60;
    // [ts] page.setCookie({
    // [ts] name: "expiring",
    // [ts] value: "123456",
    // [ts] expires: expires + 0.5,
    // [ts] });
    // [ts] const cookies = page.cookies();
    // [ts] expect(cookies).toHaveLength(1);
    // [ts] expect_eq(Math.floor(cookies[0].expires), expires);
    // [ts] });
    // [ts] test("should set cookie with reasonable defaults", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setCookie({
    // [ts] name: "password",
    // [ts] value: "123456",
    // [ts] });
    // [ts] const cookies = page.cookies();
    // [ts] expectCookieEquals(
    // [ts] cookies.sort((a, b) => {
    // [ts] return a.name.localeCompare(b.name);
    // [ts] }),
    // [ts] [
    // [ts] {
    // [ts] name: "password",
    // [ts] value: "123456",
    // [ts] domain: "localhost",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] size: 14,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] session: true,
    // [ts] sourceScheme: "NonSecure",
    // [ts] },
    // [ts] ],
    // [ts] );
    // [ts] });
    // [ts] test("should set cookie with all available properties", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setCookie({
    // [ts] name: "password",
    // [ts] value: "123456",
    // [ts] domain: "localhost",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] sourceScheme: "Unset",
    // [ts] });
    // [ts] const cookies = page.cookies();
    // [ts] expectCookieEquals(
    // [ts] cookies.sort((a, b) => {
    // [ts] return a.name.localeCompare(b.name);
    // [ts] }),
    // [ts] [
    // [ts] {
    // [ts] name: "password",
    // [ts] value: "123456",
    // [ts] domain: "localhost",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] size: 14,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] session: true,
    // [ts] sourceScheme: "Unset",
    // [ts] },
    // [ts] ],
    // [ts] );
    // [ts] });
    // [ts] test("should set a cookie with a path", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] page.setCookie({
    // [ts] name: "gridcookie",
    // [ts] value: "GRID",
    // [ts] path: "/grid.html",
    // [ts] });
    // [ts] expectCookieEquals(page.cookies(), [
    // [ts] {
    // [ts] name: "gridcookie",
    // [ts] value: "GRID",
    // [ts] domain: "localhost",
    // [ts] path: "/grid.html",
    // [ts] expires: -1,
    // [ts] size: 14,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] session: true,
    // [ts] sourceScheme: "NonSecure",
    // [ts] },
    // [ts] ]);
    // [ts] expect_eq(page.evaluate("document.cookie"), "gridcookie=GRID");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expectCookieEquals(page.cookies(), []);
    // [ts] expect_eq(page.evaluate("document.cookie"), "");
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] expect_eq(page.evaluate("document.cookie"), "gridcookie=GRID");
    // [ts] });
    // [ts] test("should set a cookie with a partitionKey", () => {
    // [ts] const {page, server, isChrome} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const url = URL.init(page.url());
    // [ts] page.setCookie({
    // [ts] url: url.toString(),
    // [ts] name: "partitionCookie",
    // [ts] value: "partition",
    // [ts] secure: true,
    // [ts] partitionKey: url.origin,
    // [ts] });
    // [ts] expectCookieEquals(page.cookies(), [
    // [ts] {
    // [ts] name: "partitionCookie",
    // [ts] value: "partition",
    // [ts] domain: url.hostname,
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] size: 24,
    // [ts] httpOnly: false,
    // [ts] secure: true,
    // [ts] session: true,
    // [ts] sourceScheme: "Secure",
    // [ts] partitionKey: if (isChrome) url.origin.replace(`:${url.port}`, "") else url.origin,
    // [ts] },
    // [ts] ]);
    // [ts] });
    // [ts] test("should not set a cookie on a blank page", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.goto("about:blank");
    // [ts] let error: Error;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.setCookie({name: "example-cookie", value: "best"});
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error_) {)
    // [ts] error = error_;
    // [ts] }
    // [ts] expect(error.message).toContain(
    // [ts] "At least one of the url and domain needs to be specified",
    // [ts] );
    // [ts] });
    // [ts] test("should not set a cookie with blank page URL", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] let error: Error;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.setCookie(
    // [ts] {name: "example-cookie", value: "best"},
    // [ts] {url: "about:blank", name: "example-cookie-blank", value: "best"},
    // [ts] );
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error_) {)
    // [ts] error = error_;
    // [ts] }
    // [ts] expect(error.message).toEqual(
    // [ts] `Blank page can not have cookie "example-cookie-blank"`,
    // [ts] );
    // [ts] });
    // [ts] test("should not set a cookie on a data URL page", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] page.goto("data:,Hello%2C%20World!");
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.setCookie({name: "example-cookie", value: "best"});
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error_) {)
    // [ts] error = error_;
    // [ts] }
    // [ts] expect(error.message).toContain(
    // [ts] "At least one of the url and domain needs to be specified",
    // [ts] );
    // [ts] });
    // [ts] test("should default to setting secure cookie for HTTPS websites", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const SECURE_URL = "https://example.com";
    // [ts] page.setCookie({
    // [ts] url: SECURE_URL,
    // [ts] name: "foo",
    // [ts] value: "bar",
    // [ts] });
    // [ts] const [cookie] = page.cookies(SECURE_URL);
    // [ts] expect_eq(cookie.secure, true);
    // [ts] });
    // [ts] test("should be able to set insecure cookie for HTTP website", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const HTTP_URL = "http://example.com";
    // [ts] page.setCookie({
    // [ts] url: HTTP_URL,
    // [ts] name: "foo",
    // [ts] value: "bar",
    // [ts] });
    // [ts] const [cookie] = page.cookies(HTTP_URL);
    // [ts] expect_eq(cookie.secure, false);
    // [ts] });
    // [ts] test("should set a cookie on a different domain", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setCookie({
    // [ts] url: "https://www.example.com",
    // [ts] name: "example-cookie",
    // [ts] value: "best",
    // [ts] });
    // [ts] expect_eq(page.evaluate("document.cookie"), "");
    // [ts] expectCookieEquals(page.cookies(), []);
    // [ts] expectCookieEquals(page.cookies("https://www.example.com"), [
    // [ts] {
    // [ts] name: "example-cookie",
    // [ts] value: "best",
    // [ts] domain: "www.example.com",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] size: 18,
    // [ts] httpOnly: false,
    // [ts] session: true,
    // [ts] sourceScheme: "Secure",
    // [ts] },
    // [ts] ]);
    // [ts] });
    // [ts] test("should set cookies from a frame", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] page.setCookie({name: "localhost-cookie", value: "best"});
    // [ts] page.evaluate(src => {
    // [ts] let fulfill: () => void;
    // [ts] const promise = Promise.init(x => {
    // [ts] return (fulfill = x);
    // [ts] });
    // [ts] const iframe = document.createElement("iframe");
    // [ts] document.body.appendChild(iframe);
    // [ts] iframe.onload = fulfill;
    // [ts] iframe.src = src;
    // [ts] return promise;
    // [ts] }, server.CROSS_PROCESS_PREFIX);
    // [ts] page.setCookie({
    // [ts] name: "127-cookie",
    // [ts] value: "worst",
    // [ts] url: server.CROSS_PROCESS_PREFIX,
    // [ts] });
    // [ts] expect(page.evaluate("document.cookie")).toBe(
    // [ts] "localhost-cookie=best",
    // [ts] );
    // [ts] expectCookieEquals(page.cookies(), [
    // [ts] {
    // [ts] name: "localhost-cookie",
    // [ts] value: "best",
    // [ts] domain: "localhost",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] size: 20,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] session: true,
    // [ts] sourceScheme: "NonSecure",
    // [ts] },
    // [ts] ]);
    // [ts] expectCookieEquals(
    // [ts] page.cookies(server.CROSS_PROCESS_PREFIX),
    // [ts] [
    // [ts] {
    // [ts] name: "127-cookie",
    // [ts] value: "worst",
    // [ts] domain: "127.0.0.1",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] size: 15,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] session: true,
    // [ts] sourceScheme: "NonSecure",
    // [ts] },
    // [ts] ],
    // [ts] );
    // [ts] });
    // [ts] test("should set secure same-site cookies from a frame", () => {
    // [ts] const {httpsServer, browser, close} = launch({
    // [ts] acceptInsecureCerts: true,
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const page = browser.newPage();
    // [ts] page.goto(httpsServer.PREFIX + "/grid.html");
    // [ts] const frame = attachFrame(
    // [ts] page,
    // [ts] "frame1",
    // [ts] httpsServer.CROSS_PROCESS_PREFIX,
    // [ts] );
    // [ts] page.setCookie({
    // [ts] name: "127-same-site-cookie",
    // [ts] value: "best",
    // [ts] url: httpsServer.CROSS_PROCESS_PREFIX,
    // [ts] sameSite: "None",
    // [ts] });
    // [ts] expect(frame.evaluate("document.cookie")).toBe(
    // [ts] "127-same-site-cookie=best",
    // [ts] );
    // [ts] expectCookieEquals(
    // [ts] page.cookies(httpsServer.CROSS_PROCESS_PREFIX),
    // [ts] [
    // [ts] {
    // [ts] name: "127-same-site-cookie",
    // [ts] value: "best",
    // [ts] domain: "127.0.0.1",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] size: 24,
    // [ts] httpOnly: false,
    // [ts] sameSite: "None",
    // [ts] secure: true,
    // [ts] session: true,
    // [ts] sourceScheme: "Secure",
    // [ts] },
    // [ts] ],
    // [ts] );
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] close();
    // [ts] }
    // [ts] });
    // [ts] test("should delete cookie", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setCookie(
    // [ts] {
    // [ts] name: "cookie1",
    // [ts] value: "1",
    // [ts] },
    // [ts] {
    // [ts] name: "cookie2",
    // [ts] value: "2",
    // [ts] },
    // [ts] {
    // [ts] name: "cookie3",
    // [ts] value: "3",
    // [ts] },
    // [ts] );
    // [ts] expect(page.evaluate("document.cookie")).toBe(
    // [ts] "cookie1=1; cookie2=2; cookie3=3",
    // [ts] );
    // [ts] page.deleteCookie({name: "cookie2"});
    // [ts] expect(page.evaluate("document.cookie")).toBe(
    // [ts] "cookie1=1; cookie3=3",
    // [ts] );
    // [ts] });
    // [ts] test("should not delete cookie for different domain", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const COOKIE_DESTINATION_URL = "https://example.com";
    // [ts] const COOKIE_NAME = "some_cookie_name";
    // [ts] page.goto(server.EMPTY_PAGE);
    // Set a cookie for the current page.
    // [ts] page.setCookie({
    // [ts] name: COOKIE_NAME,
    // [ts] value: "local page cookie value",
    // [ts] });
    // [ts] expect(page.cookies()).toHaveLength(1);
    // Set a cookie for different domain.
    // [ts] page.setCookie({
    // [ts] url: COOKIE_DESTINATION_URL,
    // [ts] name: COOKIE_NAME,
    // [ts] value: "COOKIE_DESTINATION_URL cookie value",
    // [ts] });
    // [ts] expect(page.cookies(COOKIE_DESTINATION_URL)).toHaveLength(1);
    // [ts] page.deleteCookie({name: COOKIE_NAME});
    // Verify the cookie is deleted for the current page.
    // [ts] expect(page.cookies()).toHaveLength(0);
    // Verify the cookie is not deleted for different domain.
    // [ts] expectCookieEquals(page.cookies(COOKIE_DESTINATION_URL), [
    // [ts] {
    // [ts] name: COOKIE_NAME,
    // [ts] value: "COOKIE_DESTINATION_URL cookie value",
    // [ts] domain: "example.com",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] size: 51,
    // [ts] httpOnly: false,
    // [ts] session: true,
    // [ts] sourceScheme: "Secure",
    // [ts] },
    // [ts] ]);
    // [ts] });
    // [ts] test("should delete cookie for specified URL", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const COOKIE_DESTINATION_URL = "https://example.com";
    // [ts] const COOKIE_NAME = "some_cookie_name";
    // [ts] page.goto(server.EMPTY_PAGE);
    // Set a cookie for the current page.
    // [ts] page.setCookie({
    // [ts] name: COOKIE_NAME,
    // [ts] value: "some_cookie_value",
    // [ts] });
    // [ts] expect(page.cookies()).toHaveLength(1);
    // Set a cookie for specified URL.
    // [ts] page.setCookie({
    // [ts] url: COOKIE_DESTINATION_URL,
    // [ts] name: COOKIE_NAME,
    // [ts] value: "another_cookie_value",
    // [ts] });
    // [ts] expect(page.cookies(COOKIE_DESTINATION_URL)).toHaveLength(1);
    // Delete the cookie for specified URL.
    // [ts] page.deleteCookie({
    // [ts] url: COOKIE_DESTINATION_URL,
    // [ts] name: COOKIE_NAME,
    // [ts] });
    // Verify the cookie is deleted for specified URL.
    // [ts] expect(page.cookies(COOKIE_DESTINATION_URL)).toHaveLength(0);
    // Verify the cookie is not deleted for the current page.
    // [ts] expectCookieEquals(page.cookies(), [
    // [ts] {
    // [ts] name: COOKIE_NAME,
    // [ts] value: "some_cookie_value",
    // [ts] domain: "localhost",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] size: 33,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] session: true,
    // [ts] sourceScheme: "NonSecure",
    // [ts] },
    // [ts] ]);
    // [ts] });
    // [ts] test("should delete cookie for specified URL regardless of the current page", () => {
    // This test verifies the page.deleteCookie method deletes cookies for the custom
    // destination URL, even if it was set from another page. Depending on the cookie
    // partitioning implementation, this test case does not pass, if source origin is in
    // the default cookie partition.
    // [ts] const {page, server} = getTestState();
    // [ts] const COOKIE_DESTINATION_URL = "https://example.com";
    // [ts] const COOKIE_NAME = "some_cookie_name";
    // [ts] const URL_1 = server.EMPTY_PAGE;
    // [ts] const URL_2 = server.CROSS_PROCESS_PREFIX + "/empty.html";
    // [ts] page.goto(URL_1);
    // Set a cookie for the COOKIE_DESTINATION from URL_1.
    // [ts] page.setCookie({
    // [ts] url: COOKIE_DESTINATION_URL,
    // [ts] name: COOKIE_NAME,
    // [ts] value: "Cookie from URL_1",
    // [ts] });
    // [ts] expect(page.cookies(COOKIE_DESTINATION_URL)).toHaveLength(1);
    // [ts] page.goto(URL_2);
    // Set a cookie for the COOKIE_DESTINATION from URL_2.
    // [ts] page.setCookie({
    // [ts] url: COOKIE_DESTINATION_URL,
    // [ts] name: COOKIE_NAME,
    // [ts] value: "Cookie from URL_2",
    // [ts] });
    // [ts] expect(page.cookies(COOKIE_DESTINATION_URL)).toHaveLength(1);
    // Delete the cookie for the COOKIE_DESTINATION from URL_2.
    // [ts] page.deleteCookie({
    // [ts] name: COOKIE_NAME,
    // [ts] url: COOKIE_DESTINATION_URL,
    // [ts] });
    // Expect the cookie for the COOKIE_DESTINATION from URL_2 is deleted.
    // [ts] expect(page.cookies(COOKIE_DESTINATION_URL)).toHaveLength(0);
    // Navigate back to the URL_1.
    // [ts] page.goto(server.EMPTY_PAGE);
    // Expect the cookie for the COOKIE_DESTINATION from URL_1 is deleted.
    // [ts] expect(page.cookies(COOKIE_DESTINATION_URL)).toHaveLength(0);
    // [ts] });
    // [ts] test("should only delete cookie from the default partition if partitionkey is not specified", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const url = URL.init(server.EMPTY_PAGE);
    // [ts] page.goto(url.toString());
    // [ts] page.setCookie({
    // [ts] url: url.toString(),
    // [ts] name: "partitionCookie",
    // [ts] value: "partition",
    // [ts] secure: true,
    // [ts] partitionKey: url.origin,
    // [ts] });
    // [ts] expect(page.cookies()).toHaveLength(1);
    // [ts] page.deleteCookie({
    // [ts] url: url.toString(),
    // [ts] name: "partitionCookie",
    // [ts] });
    // [ts] expect(page.cookies()).toHaveLength(0);
    // [ts] });
    // [ts] test("should delete cookie with partition key if partition key is specified", () => {
    // [ts] const {page, server, isChrome} = getTestState();
    // [ts] const url = URL.init(server.EMPTY_PAGE);
    // [ts] page.goto(url.toString());
    // [ts] const origin = if (isChrome) url.origin.replace(`:${url.port}`, "") else url.origin;
    // [ts] page.setCookie({
    // [ts] url: url.toString(),
    // [ts] name: "partitionCookie",
    // [ts] value: "partition",
    // [ts] secure: true,
    // [ts] partitionKey: origin,
    // [ts] });
    // [ts] expect(page.cookies()).toHaveLength(1);
    // [ts] page.deleteCookie({
    // [ts] url: url.toString(),
    // [ts] name: "partitionCookie",
    // [ts] partitionKey: origin,
    // [ts] });
    // [ts] expect(page.cookies()).toHaveLength(0);
    // [ts] });
    // [ts] test("should delete cookie from the page partition if the page uses the default port", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.respond({
    // [ts] status: 200,
    // [ts] contentType: "text/html",
    // [ts] body: "<!DOCTYPE html>",
    // [ts] });
    // [ts] });
    // [ts] page.goto("https://example.com/");
    // [ts] page.setCookie({
    // [ts] url: "https://example.com/",
    // [ts] name: "partitionCookie",
    // [ts] value: "partition",
    // [ts] secure: true,
    // [ts] partitionKey: "https://example.com",
    // [ts] });
    // [ts] expect(page.cookies()).toHaveLength(1);
    // [ts] page.deleteCookie({name: "partitionCookie"});
    // [ts] expect(page.cookies()).toHaveLength(0);
    // [ts] });
}
