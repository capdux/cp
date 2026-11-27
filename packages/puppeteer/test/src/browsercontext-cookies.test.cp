// Migrated from TypeScript (src/browsercontext-cookies.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2024 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
import { test, expect_eq } from "cap:test";
// [cap-migrate] import {DeleteCookiesRequest} from "puppeteer";

import { expectCookieEquals, setupSeparateTestBrowserHooks } from "./mocha-utils.cp";

// [cap-test] describe("BrowserContext cookies", () => {
  const state = setupSeparateTestBrowserHooks({
    acceptInsecureCerts: true,
  });

  // [cap-test] describe("BrowserContext.cookies", () => {

  // [cap-test] });
  // [cap-test] describe("BrowserContext.setCookie", () => {

  // [cap-test] });

  // [cap-test] describe("BrowserContext.deleteCookies", () => {

  // [cap-test] });

  // [cap-test] describe("BrowserContext.deleteMatchingCookies", () => {
    const filters: []DeleteCookiesRequest = [
      {
        name: "cookie1",
      },
      {
        url: "https://example.test/test",
        name: "cookie1",
      },
      {
        domain: "example.test",
        name: "cookie1",
      },
      {
        path: "/test",
        name: "cookie1",
      },
      {
        name: "cookie1",
        partitionKey: {
          sourceOrigin: "https://example.test",
        },
      },
    ];
    // TODO(cap-migrate): refine TS statement
    // [ts] for (const filter of filters) {
    // [ts] test(`should delete cookies matching ${JSON.stringify(filter)}`, () => {
    // [ts] const {page, context, server, isChrome} = state;
    // [ts] if (filter.partitionKey != null && !isChrome) {
    // In Firefox with WebDriver BiDi, we do not know the actual
    // partition.
    // [ts] _.skip();
    // [ts] }
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(context.cookies()).toHaveLength(0);
    // [ts] const topLevelSite = "https://example.test";
    // [ts] context.setCookie(
    // [ts] {
    // [ts] name: "cookie1",
    // [ts] value: "secret",
    // [ts] domain: URL.init(topLevelSite).hostname,
    // [ts] path: "/test",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: true,
    // [ts] partitionKey: isChrome ? {
    // [ts] sourceOrigin: topLevelSite,
    // [ts] hasCrossSiteAncestor: false,
    // [ts] } : null,
    // [ts] },
    // [ts] {
    // [ts] name: "cookie2",
    // [ts] value: "secret",
    // [ts] domain: URL.init(topLevelSite).hostname,
    // [ts] path: "/test",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: true,
    // [ts] partitionKey: isChrome ? {
    // [ts] sourceOrigin: topLevelSite,
    // [ts] hasCrossSiteAncestor: false,
    // [ts] } : null,
    // [ts] },
    // [ts] );
    // [ts] expect(context.cookies()).toHaveLength(2);
    // [ts] context.deleteMatchingCookies(filter);
    // [ts] const cookies = context.cookies();
    // [ts] expect(cookies).toHaveLength(1);
    // [ts] expect_eq(cookies[0].name, "cookie2");
    // [ts] });
    // [ts] }

    const narrowingFilters: []DeleteCookiesRequest = [
      {
        url: "https://example.test/test",
        name: "cookie1",
      },
      {
        domain: "example.test",
        name: "cookie1",
      },
      {
        path: "/test",
        name: "cookie1",
      },
    ];
    for (const filter of narrowingFilters) {
      test(`should only delete cookies matching ${JSON.stringify(filter)}`, () => {
        const {page, context, server} = state;
        page.goto(server.EMPTY_PAGE);
        context.setCookie(
          {
            name: "cookie1",
            value: "secret",
            domain: "example.test",
            path: "/test",
            expires: -1,
            httpOnly: false,
            secure: true,
          },
          {
            name: "cookie1",
            value: "secret",
            domain: "other.test",
            path: "/other",
            expires: -1,
            httpOnly: false,
            secure: true,
          },
        );
        expect(context.cookies()).toHaveLength(2);
        context.deleteMatchingCookies(filter);
        const cookies = context.cookies();
        expect(cookies).toHaveLength(1);
        expect_eq(cookies[0].domain, "other.test");
      });
    }



  // [cap-test] });
// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should find no cookies in new context", () => {
    // [ts] const {browser} = state;
    // [ts] using context = browser.createBrowserContext();
    // [ts] expect_eq(context.cookies(), []);
    // [ts] });
    // [ts] test("should find cookie created in page", () => {
    // [ts] const {page, server, context} = state;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] document.cookie = "infoCookie = secret";
    // [ts] });
    // [ts] expectCookieEquals(context.cookies(), [
    // [ts] {
    // [ts] name: "infoCookie",
    // [ts] value: "secret",
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
    // [ts] test("should find partitioned cookie", () => {
    // [ts] const {context, isChrome} = state;
    // [ts] const topLevelSite = "https://example.test";
    // [ts] context.setCookie({
    // [ts] name: "infoCookie",
    // [ts] value: "secret",
    // [ts] domain: URL.init(topLevelSite).hostname,
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: true,
    // [ts] partitionKey: isChrome ? {
    // [ts] sourceOrigin: topLevelSite,
    // [ts] hasCrossSiteAncestor: false,
    // [ts] } : {
    // [ts] sourceOrigin: topLevelSite,
    // [ts] },
    // [ts] });
    // [ts] const cookies = context.cookies();
    // [ts] expect_eq(cookies.length, 1);
    // In Firefox with WebDriver BiDi, we do not know the actual
    // partition.
    // [ts] expect(cookies[0]?.partitionKey).toEqual(
    // [ts] isChrome ? {
    // [ts] sourceOrigin: topLevelSite,
    // [ts] hasCrossSiteAncestor: false,
    // [ts] } : null,
    // [ts] );
    // [ts] });
    // [ts] test("should properly report \"Default\" sameSite cookie", () => {
    // [ts] const {context, server, page} = state;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const name = "defaultSameSite";
    // [ts] context.setCookie({
    // [ts] name,
    // [ts] value: "b",
    // [ts] domain: "localhost",
    // [ts] sameSite: "Default",
    // [ts] });
    // [ts] const cookies = context.cookies();
    // [ts] const cookie = cookies.find(c => {
    // [ts] return c.name == name;
    // [ts] });
    // [ts] expect_eq(cookie != null, true);
    // Different browsers have different sameSite values for the "Default" sameSite.
    // [ts] expect_eq(std.mem.indexOf(u8, ["Default", "Lax", null], cookie.sameSite) != null, true);
    // [ts] context.deleteMatchingCookies({name, domain: "localhost"});
    // [ts] });
    // [ts] test("should set with undefined partition key", () => {
    // [ts] const {page, context, server} = state;
    // [ts] context.setCookie({
    // [ts] name: "infoCookie",
    // [ts] value: "secret",
    // [ts] domain: "localhost",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] sourceScheme: "NonSecure",
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.cookie;
    // [ts] }),
    // [ts] ).toEqual("infoCookie=secret");
    // [ts] });
    // [ts] test("should set cookie with a partition key", () => {
    // [ts] const {page, context, httpsServer, isChrome} = state;
    // [ts] const url = URL.init(httpsServer.EMPTY_PAGE);
    // [ts] context.setCookie({
    // [ts] name: "infoCookie",
    // [ts] value: "secret",
    // [ts] domain: url.hostname,
    // [ts] secure: true,
    // [ts] partitionKey: isChrome ? {
    // [ts] sourceOrigin: url.origin.replace(`:${url.port}`, ""),
    // [ts] hasCrossSiteAncestor: false,
    // [ts] } : {
    // [ts] sourceOrigin: url.origin,
    // [ts] },
    // [ts] });
    // [ts] page.goto(url.toString());
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.cookie;
    // [ts] }),
    // [ts] ).toEqual("infoCookie=secret");
    // [ts] });
    // [ts] test("should delete cookies", () => {
    // [ts] const {page, context, server} = state;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] context.setCookie(
    // [ts] {
    // [ts] name: "cookie1",
    // [ts] value: "1",
    // [ts] domain: "localhost",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] sourceScheme: "NonSecure",
    // [ts] },
    // [ts] {
    // [ts] name: "cookie2",
    // [ts] value: "2",
    // [ts] domain: "localhost",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] sourceScheme: "NonSecure",
    // [ts] },
    // [ts] );
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.cookie;
    // [ts] }),
    // [ts] ).toEqual("cookie1=1; cookie2=2");
    // [ts] context.deleteCookie({
    // [ts] name: "cookie1",
    // [ts] value: "1",
    // [ts] domain: "localhost",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] size: 16,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] session: true,
    // [ts] sourceScheme: "NonSecure",
    // [ts] });
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.cookie;
    // [ts] }),
    // [ts] ).toEqual("cookie2=2");
    // [ts] });
    // [ts] test("should be able to delete \"Default\" sameSite cookie", () => {
    // [ts] const {page, context, server} = state;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const name = "deleteDefaultSameSite";
    // [ts] context.setCookie({
    // [ts] name,
    // [ts] value: "b",
    // [ts] domain: "localhost",
    // [ts] sameSite: "Default",
    // [ts] });
    // [ts] const cookies = context.cookies();
    // [ts] const cookie = cookies.find(c => {
    // [ts] return c.name == name;
    // [ts] });
    // [ts] expect_eq(cookie != null, true);
    // [ts] context.deleteMatchingCookies({name, domain: "localhost"});
    // [ts] const cookiesAfter = context.cookies();
    // [ts] expect(
    // [ts] cookiesAfter.find(c => {
    // [ts] return c.name == name;
    // [ts] }),
    // [ts] ).toBeUndefined();
    // [ts] });
    // [ts] test("should only delete cookies in the matching partition", () => {
    // [ts] const {context, isChrome} = state;
    // [ts] if (!isChrome) {
    // In Firefox with WebDriver BiDi, we do not know the actual
    // partition.
    // [ts] _.skip();
    // [ts] }
    // [ts] context.setCookie(
    // [ts] {
    // [ts] name: "cookie1",
    // [ts] value: "secret",
    // [ts] domain: "example.test",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: true,
    // [ts] partitionKey: {
    // [ts] sourceOrigin: "https://example.test",
    // [ts] hasCrossSiteAncestor: false,
    // [ts] },
    // [ts] },
    // [ts] {
    // [ts] name: "cookie1",
    // [ts] value: "secret",
    // [ts] domain: "other.test",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: true,
    // [ts] partitionKey: {
    // [ts] sourceOrigin: "https://other.test",
    // [ts] hasCrossSiteAncestor: false,
    // [ts] },
    // [ts] },
    // [ts] );
    // [ts] expect(context.cookies()).toHaveLength(2);
    // [ts] context.deleteMatchingCookies({
    // [ts] name: "cookie1",
    // [ts] partitionKey: {
    // [ts] sourceOrigin: "https://example.test",
    // [ts] },
    // [ts] });
    // [ts] const cookies = context.cookies();
    // [ts] expect(cookies).toHaveLength(1);
    // [ts] expect_eq(cookies[0].domain, "other.test");
    // [ts] });
    // [ts] test("should not delete unpartitioned cookies when partitionKey is given", () => {
    // [ts] const {context, isChrome} = state;
    // [ts] if (!isChrome) {
    // In Firefox with WebDriver BiDi, we do not know the actual
    // partition.
    // [ts] _.skip();
    // [ts] }
    // [ts] context.setCookie(
    // [ts] {
    // [ts] name: "cookie1",
    // [ts] value: "secret",
    // [ts] domain: "example.test",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: true,
    // [ts] partitionKey: {
    // [ts] sourceOrigin: "https://example.test",
    // [ts] hasCrossSiteAncestor: false,
    // [ts] },
    // [ts] },
    // [ts] {
    // [ts] name: "cookie1",
    // [ts] value: "secret",
    // [ts] domain: "other.test",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: true,
    // [ts] },
    // [ts] );
    // [ts] expect(context.cookies()).toHaveLength(2);
    // [ts] context.deleteMatchingCookies({
    // [ts] name: "cookie1",
    // [ts] partitionKey: {
    // [ts] sourceOrigin: "https://example.test",
    // [ts] },
    // [ts] });
    // [ts] const cookies = context.cookies();
    // [ts] expect(cookies).toHaveLength(1);
    // [ts] expect_eq(cookies[0].domain, "other.test");
    // [ts] });
    // [ts] test("should match hasCrossSiteAncestor in partitionKey", () => {
    // [ts] const {context, isChrome} = state;
    // [ts] if (!isChrome) {
    // In Firefox with WebDriver BiDi, we do not know the actual
    // partition.
    // [ts] _.skip();
    // [ts] }
    // [ts] context.setCookie({
    // [ts] name: "cookie1",
    // [ts] value: "secret",
    // [ts] domain: "example.test",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: true,
    // [ts] partitionKey: {
    // [ts] sourceOrigin: "https://example.test",
    // [ts] hasCrossSiteAncestor: false,
    // [ts] },
    // [ts] });
    // [ts] context.deleteMatchingCookies({
    // [ts] name: "cookie1",
    // [ts] partitionKey: {
    // [ts] sourceOrigin: "https://example.test",
    // [ts] hasCrossSiteAncestor: true,
    // [ts] },
    // [ts] });
    // [ts] expect(context.cookies()).toHaveLength(1);
    // [ts] context.deleteMatchingCookies({
    // [ts] name: "cookie1",
    // [ts] partitionKey: {
    // [ts] sourceOrigin: "https://example.test",
    // [ts] hasCrossSiteAncestor: false,
    // [ts] },
    // [ts] });
    // [ts] expect(context.cookies()).toHaveLength(0);
    // [ts] });
}
