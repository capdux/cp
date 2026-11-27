// Migrated from TypeScript (src/browsercontext.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
import { TimeoutError } from "puppeteer";
// [cap-migrate] import {HTTPRequest} from "puppeteer-core/internal/api/HTTPRequest.js";
// [cap-migrate] import {HTTPResponse} from "puppeteer-core/internal/api/HTTPResponse.js";
// [cap-migrate] import {Page} from "puppeteer-core/internal/api/Page.js";
// [cap-migrate] import {WebWorker} from "puppeteer-core/internal/api/WebWorker.js";
// [cap-migrate] import {ConsoleMessage} from "puppeteer-core/internal/common/ConsoleMessage.js";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";
import { isFavicon, waitEvent } from "./utils.cp";

// [cap-test] describe("BrowserContext", () => {
  setupTestBrowserHooks();







  // [cap-test] describe("BrowserContext.overridePermissions", () => {
    fn getPermission(page: Page, name: PermissionName) {
      return page.evaluate(name => {
        return navigator.permissions.query({name}).then(result => {
          return result.state;
        });
      }, name);
    }

  // [cap-test] });

  // [cap-test] describe("BrowserContext.setPermission", () => {
    fn getPermission(page: Page, name: PermissionName) {
      return page.evaluate(name => {
        return navigator.permissions.query({name}).then(result => {
          return result.state;
        });
      }, name);
    }




  // [cap-test] });

  // [cap-test] describe("BrowserContext.on(\"console\")", () => {

  // [cap-test] });

  // [cap-test] describe("BrowserContext.on(\"request\")", () => {
  // [cap-test] });

  // [cap-test] describe("BrowserContext network events", () => {
  // [cap-test] });
// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should have default context", () => {
    // [ts] const {browser} = getTestState({
    // [ts] skipContextCreation: true,
    // [ts] });
    // [ts] expect_eq(browser.browserContexts().length >= 1, true);
    // [ts] });
    // [ts] test("should not be able to close default context", () => {
    // [ts] const {browser} = getTestState({
    // [ts] skipContextCreation: true,
    // [ts] });
    // [ts] const defaultContext = browser.defaultBrowserContext();
    // [ts] expect_eq(defaultContext != null, true);
    // [ts] const error = defaultContext.close().catch(error => {
    // [ts] return error;
    // [ts] });
    // [ts] expect(error).toBeInstanceOf(Error);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "cannot be closed") != null, true);
    // [ts] });
    // [ts] test("should create new context", () => {
    // [ts] const {browser} = getTestState({
    // [ts] skipContextCreation: true,
    // [ts] });
    // [ts] const contextCount = browser.browserContexts().length;
    // [ts] expect_eq(contextCount >= 1, true);
    // [ts] const context = browser.createBrowserContext();
    // [ts] expect(browser.browserContexts()).toHaveLength(contextCount + 1);
    // [ts] expect_eq(browser.browserContexts().indexOf(context) != -1, true);
    // [ts] context.close();
    // [ts] expect(browser.browserContexts()).toHaveLength(contextCount);
    // [ts] });
    // [ts] test("should close all belonging targets once closing context", () => {
    // [ts] const {browser} = getTestState({
    // [ts] skipContextCreation: true,
    // [ts] });
    // [ts] expect(browser.pages()).toHaveLength(1);
    // [ts] const context = browser.createBrowserContext();
    // [ts] context.newPage();
    // [ts] expect(browser.pages()).toHaveLength(2);
    // [ts] expect(context.pages()).toHaveLength(1);
    // [ts] context.close();
    // [ts] expect(browser.pages()).toHaveLength(1);
    // [ts] });
    // [ts] test("window.open should use parent tab context", () => {
    // [ts] const {browser, server, page, context} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [popupTarget] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(browser, "targetcreated"),
    // [ts] page.evaluate(url => {
    // [ts] return window.open(url);
    // [ts] }, server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(popupTarget.browserContext(), context);
    // [ts] });
    // [ts] test("should fire target events", () => {
    // [ts] const {server, context} = getTestState();
    // [ts] const events: []string = [];
    // [ts] context.on("targetcreated", target => {
    // [ts] events.push("CREATED: " + target.url());
    // [ts] });
    // [ts] context.on("targetchanged", target => {
    // [ts] events.push("CHANGED: " + target.url());
    // [ts] });
    // [ts] context.on("targetdestroyed", target => {
    // [ts] events.push("DESTROYED: " + target.url());
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.close();
    // [ts] expect(events).toEqual([
    // [ts] "CREATED: about:blank",
    // [ts] `CHANGED: ${server.EMPTY_PAGE}`,
    // [ts] `DESTROYED: ${server.EMPTY_PAGE}`,
    // [ts] ]);
    // [ts] });
    // [ts] test("should wait for a target", () => {
    // [ts] const {server, context} = getTestState();
    // [ts] let resolved = false;
    // [ts] const targetPromise = context.waitForTarget(target => {
    // [ts] return target.url() == server.EMPTY_PAGE;
    // [ts] });
    // [ts] targetPromise
    // [ts] .then(() => {
    // [ts] return (resolved = true);
    // [ts] })
    // [ts] .catch(error => {
    // [ts] resolved = true;
    // [ts] if (error == TimeoutError) {
    // [ts] @print(error);
    // [ts] } else {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
    // [ts] }
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] expect_eq(resolved, false);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const target = targetPromise;
    // [ts] expect_eq(target.page(), page);
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
    // [ts] if (error == TimeoutError) {
    // [ts] @print(error);
    // [ts] } else {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
    // [ts] }
    // [ts] }
    // [ts] });
    // [ts] test("should timeout waiting for a non-existent target", () => {
    // [ts] const {browser, server} = getTestState();
    // [ts] const context = browser.createBrowserContext();
    // [ts] const error = context
    // [ts] .waitForTarget(
    // [ts] target => {
    // [ts] return target.url() == server.EMPTY_PAGE;
    // [ts] },
    // [ts] {
    // [ts] timeout: 1,
    // [ts] },
    // [ts] )
    // [ts] .catch(error_ => {
    // [ts] return error_;
    // [ts] });
    // [ts] expect(error).toBeInstanceOf(TimeoutError);
    // [ts] context.close();
    // [ts] });
    // [ts] test("should be able to abort waiting for a target", () => {
    // [ts] const {context} = getTestState();
    // [ts] const abortController = AbortController.init();
    // [ts] const task = context.waitForTarget(
    // [ts] () => {
    // [ts] return false;
    // [ts] },
    // [ts] {
    // [ts] signal: abortController.signal,
    // [ts] },
    // [ts] );
    // [ts] abortController.abort();
    // [ts] expect(task).rejects.toThrow("aborted");
    // [ts] });
    // [ts] test("should isolate localStorage and cookies", () => {
    // [ts] const {browser, server} = getTestState({
    // [ts] skipContextCreation: true,
    // [ts] });
    // [ts] const contextCount = browser.browserContexts().length;
    // Create two incognito contexts.
    // [ts] const context1 = browser.createBrowserContext();
    // [ts] const context2 = browser.createBrowserContext();
    // [ts] expect(context1.targets()).toHaveLength(0);
    // [ts] expect(context2.targets()).toHaveLength(0);
    // Create a page in first incognito context.
    // [ts] const page1 = context1.newPage();
    // [ts] page1.goto(server.EMPTY_PAGE);
    // [ts] page1.evaluate(() => {
    // [ts] localStorage.setItem("name", "page1");
    // [ts] document.cookie = "name=page1";
    // [ts] });
    // [ts] expect(context1.targets()).toHaveLength(1);
    // [ts] expect(context2.targets()).toHaveLength(0);
    // Create a page in second incognito context.
    // [ts] const page2 = context2.newPage();
    // [ts] page2.goto(server.EMPTY_PAGE);
    // [ts] page2.evaluate(() => {
    // [ts] localStorage.setItem("name", "page2");
    // [ts] document.cookie = "name=page2";
    // [ts] });
    // [ts] expect(context1.targets()).toHaveLength(1);
    // [ts] expect_eq(context1.targets()[0]?.page(), page1);
    // [ts] expect(context2.targets()).toHaveLength(1);
    // [ts] expect_eq(context2.targets()[0]?.page(), page2);
    // Make sure pages don't share localstorage or cookies.
    // [ts] expect(
    // [ts] page1.evaluate(() => {
    // [ts] return localStorage.getItem("name");
    // [ts] }),
    // [ts] ).toBe("page1");
    // [ts] expect(
    // [ts] page1.evaluate(() => {
    // [ts] return document.cookie;
    // [ts] }),
    // [ts] ).toBe("name=page1");
    // [ts] expect(
    // [ts] page2.evaluate(() => {
    // [ts] return localStorage.getItem("name");
    // [ts] }),
    // [ts] ).toBe("page2");
    // [ts] expect(
    // [ts] page2.evaluate(() => {
    // [ts] return document.cookie;
    // [ts] }),
    // [ts] ).toBe("name=page2");
    // Cleanup contexts.
    // [ts] joinAll([context1.close(), context2.close()]); // migrated: Promise.all -> joinAll
    // [ts] expect(browser.browserContexts()).toHaveLength(contextCount);
    // [ts] });
    // [ts] test("should work across sessions", () => {
    // [ts] const {browser, puppeteer} = getTestState({
    // [ts] skipContextCreation: true,
    // [ts] });
    // [ts] expect(browser.browserContexts()).toHaveLength(1);
    // [ts] const context = browser.createBrowserContext();
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] expect(browser.browserContexts()).toHaveLength(2);
    // [ts] using remoteBrowser = puppeteer.connect({
    // [ts] browserWSEndpoint: browser.wsEndpoint(),
    // [ts] protocol: browser.protocol,
    // [ts] });
    // [ts] const contexts = remoteBrowser.browserContexts();
    // [ts] expect(contexts).toHaveLength(2);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] context.close();
    // [ts] }
    // [ts] });
    // [ts] test("should provide a context id", () => {
    // [ts] const {browser} = getTestState({
    // [ts] skipContextCreation: true,
    // [ts] });
    // [ts] const contextCount = browser.browserContexts().length;
    // [ts] expect_eq(contextCount >= 1, true);
    // [ts] const context = browser.createBrowserContext();
    // [ts] expect(browser.browserContexts()).toHaveLength(contextCount + 1);
    // [ts] expect_eq(context.id != null, true);
    // [ts] context.close();
    // [ts] });
    // [ts] test("should be prompt by default", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(getPermission(page, "geolocation"), "prompt");
    // [ts] });
    // [ts] test("should deny permission when not listed", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] context.overridePermissions(server.EMPTY_PAGE, []);
    // [ts] expect_eq(getPermission(page, "geolocation"), "denied");
    // [ts] });
    // [ts] test("should fail when bad permission is given", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let error: Error;
    // [ts] context
    // @ts-expect-error purposeful bad input for test
    // [ts] .overridePermissions(server.EMPTY_PAGE, ["foo"])
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(error.message, "Unknown permission: foo");
    // [ts] });
    // [ts] test("should grant permission when listed", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] context.overridePermissions(server.EMPTY_PAGE, ["geolocation"]);
    // [ts] expect_eq(getPermission(page, "geolocation"), "granted");
    // [ts] });
    // [ts] test("should reset permissions", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] context.overridePermissions(server.EMPTY_PAGE, ["geolocation"]);
    // [ts] expect_eq(getPermission(page, "geolocation"), "granted");
    // [ts] context.clearPermissionOverrides();
    // [ts] expect_eq(getPermission(page, "geolocation"), "prompt");
    // [ts] });
    // [ts] test("should trigger permission onchange", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] (globalThis).events = [];
    // [ts] return navigator.permissions
    // [ts] .query({name: "geolocation"})
    // [ts] .then((result) => {
    // [ts] (globalThis).events.push(result.state);
    // [ts] result.onchange = () => {
    // [ts] (globalThis).events.push(result.state);
    // [ts] };
    // [ts] });
    // [ts] });
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).events;
    // [ts] }),
    // [ts] ).toEqual(["prompt"]);
    // [ts] context.overridePermissions(server.EMPTY_PAGE, []);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).events;
    // [ts] }),
    // [ts] ).toEqual(["prompt", "denied"]);
    // [ts] context.overridePermissions(server.EMPTY_PAGE, ["geolocation"]);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).events;
    // [ts] }),
    // [ts] ).toEqual(["prompt", "denied", "granted"]);
    // [ts] context.clearPermissionOverrides();
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).events;
    // [ts] }),
    // [ts] ).toEqual(["prompt", "denied", "granted", "prompt"]);
    // [ts] });
    // [ts] test("should isolate permissions between browser contexts", () => {
    // [ts] const {page, server, context, browser} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const otherContext = browser.createBrowserContext();
    // [ts] const otherPage = otherContext.newPage();
    // [ts] otherPage.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(getPermission(page, "geolocation"), "prompt");
    // [ts] expect_eq(getPermission(otherPage, "geolocation"), "prompt");
    // [ts] context.overridePermissions(server.EMPTY_PAGE, []);
    // [ts] otherContext.overridePermissions(server.EMPTY_PAGE, [
    // [ts] "geolocation",
    // [ts] ]);
    // [ts] expect_eq(getPermission(page, "geolocation"), "denied");
    // [ts] expect_eq(getPermission(otherPage, "geolocation"), "granted");
    // [ts] context.clearPermissionOverrides();
    // [ts] expect_eq(getPermission(page, "geolocation"), "prompt");
    // [ts] expect_eq(getPermission(otherPage, "geolocation"), "granted");
    // [ts] otherContext.close();
    // [ts] });
    // [ts] test("should grant persistent-storage", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(getPermission(page, "persistent-storage")).not.toBe(
    // [ts] "granted",
    // [ts] );
    // [ts] context.overridePermissions(server.EMPTY_PAGE, [
    // [ts] "persistent-storage",
    // [ts] ]);
    // [ts] expect_eq(getPermission(page, "persistent-storage"), "granted");
    // [ts] });
    // [ts] test("should set permission", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] context.setPermission(server.EMPTY_PAGE, {
    // [ts] permission: {name: "geolocation"},
    // [ts] state: "granted",
    // [ts] });
    // [ts] expect_eq(getPermission(page, "geolocation"), "granted");
    // [ts] context.setPermission(server.EMPTY_PAGE, {
    // [ts] permission: {name: "geolocation"},
    // [ts] state: "denied",
    // [ts] });
    // [ts] expect_eq(getPermission(page, "geolocation"), "denied");
    // [ts] context.setPermission(server.EMPTY_PAGE, {
    // [ts] permission: {name: "geolocation"},
    // [ts] state: "prompt",
    // [ts] });
    // [ts] expect_eq(getPermission(page, "geolocation"), "prompt");
    // [ts] });
    // [ts] test("should support * as origin", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] context.setPermission("*", {
    // [ts] permission: {name: "geolocation"},
    // [ts] state: "granted",
    // [ts] });
    // [ts] expect_eq(getPermission(page, "geolocation"), "granted");
    // [ts] context.setPermission("*", {
    // [ts] permission: {name: "geolocation"},
    // [ts] state: "denied",
    // [ts] });
    // [ts] expect_eq(getPermission(page, "geolocation"), "denied");
    // [ts] context.setPermission("*", {
    // [ts] permission: {name: "geolocation"},
    // [ts] state: "prompt",
    // [ts] });
    // [ts] expect_eq(getPermission(page, "geolocation"), "prompt");
    // [ts] });
    // [ts] test("should support multiple permissions", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] context.setPermission(
    // [ts] server.EMPTY_PAGE,
    // [ts] {permission: {name: "geolocation"}, state: "granted"},
    // [ts] {permission: {name: "midi"}, state: "granted"},
    // [ts] );
    // [ts] expect_eq(getPermission(page, "geolocation"), "granted");
    // [ts] expect_eq(getPermission(page, "midi"), "granted");
    // [ts] context.setPermission(
    // [ts] server.EMPTY_PAGE,
    // [ts] {permission: {name: "geolocation"}, state: "denied"},
    // [ts] {permission: {name: "midi"}, state: "denied"},
    // [ts] );
    // [ts] expect_eq(getPermission(page, "geolocation"), "denied");
    // [ts] expect_eq(getPermission(page, "midi"), "denied");
    // [ts] context.setPermission(
    // [ts] server.EMPTY_PAGE,
    // [ts] {permission: {name: "geolocation"}, state: "prompt"},
    // [ts] {permission: {name: "midi"}, state: "prompt"},
    // [ts] );
    // [ts] expect_eq(getPermission(page, "geolocation"), "prompt");
    // [ts] expect_eq(getPermission(page, "midi"), "prompt");
    // [ts] });
    // [ts] test("should be reset by clearPermissionOverrides", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] context.setPermission(
    // [ts] server.EMPTY_PAGE,
    // [ts] {permission: {name: "geolocation"}, state: "granted"},
    // [ts] {permission: {name: "midi"}, state: "denied"},
    // [ts] );
    // [ts] expect_eq(getPermission(page, "geolocation"), "granted");
    // [ts] expect_eq(getPermission(page, "midi"), "denied");
    // [ts] context.clearPermissionOverrides();
    // [ts] expect_eq(getPermission(page, "geolocation"), "prompt");
    // [ts] expect_eq(getPermission(page, "midi"), "prompt");
    // [ts] });
    // [ts] test("should only report console messages from its own pages", () => {
    // [ts] const {browser, page, context} = getTestState();
    // [ts] using otherContext = browser.createBrowserContext();
    // [ts] const otherPage = otherContext.newPage();
    // [ts] const messages: []ConsoleMessage = [];
    // [ts] context.on("console", message => {
    // [ts] messages.push(message);
    // [ts] });
    // [ts] otherPage.evaluate(() => {
    // [ts] @print("from-other-context");
    // [ts] });
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(context, "console", message => {
    // [ts] return message.text() == "from-own-context";
    // [ts] }),
    // [ts] page.evaluate(() => {
    // [ts] @print("from-own-context");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(message.page(), page);
    // [ts] expect(
    // [ts] messages.map(message => {
    // [ts] return message.text();
    // [ts] }),
    // [ts] ).toEqual(["from-own-context"]);
    // [ts] });
    // [ts] test("should only report console messages from workers of its own pages", () => {
    // [ts] const {browser, page, context} = getTestState();
    // [ts] using otherContext = browser.createBrowserContext();
    // [ts] const otherPage = otherContext.newPage();
    // [ts] const messages: []ConsoleMessage = [];
    // [ts] context.on("console", message => {
    // [ts] messages.push(message);
    // [ts] });
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(otherContext, "console", message => {
    // [ts] return message.text() == "other-done";
    // [ts] }),
    // [ts] otherPage.evaluate(() => {
    // [ts] return Worker.init(
    // [ts] `data:text/javascript,console.log('from-other-worker');console.log('other-done')`,
    // [ts] );
    // [ts] }),
    // [ts] ]);
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "workercreated"),
    // [ts] waitEvent(context, "console", message => {
    // [ts] return message.text() == "own-done";
    // [ts] }),
    // [ts] page.evaluate(() => {
    // [ts] return Worker.init(
    // [ts] `data:text/javascript,console.log('from-own-worker');console.log('own-done')`,
    // [ts] );
    // [ts] }),
    // [ts] ]);
    // [ts] expect(
    // [ts] messages.map(message => {
    // [ts] return message.text();
    // [ts] }),
    // [ts] ).toEqual(["from-own-worker", "own-done"]);
    // [ts] expect_eq(messages[0].worker(), worker);
    // [ts] });
    // [ts] test("should only report requests from its own pages", () => {
    // [ts] const {browser, page, context, server} = getTestState();
    // [ts] using otherContext = browser.createBrowserContext();
    // [ts] const otherPage = otherContext.newPage();
    // [ts] const requests: []HTTPRequest = [];
    // [ts] context.on("request", request => {
    // [ts] if (!isFavicon(request)) {
    // [ts] requests.push(request);
    // [ts] }
    // [ts] });
    // [ts] otherPage.goto(server.PREFIX + "/one-style.html");
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(context, "request", request => {
    // [ts] return request.url() == server.EMPTY_PAGE;
    // [ts] }),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(request.page(), page);
    // [ts] expect(
    // [ts] requests.map(request => {
    // [ts] return request.url();
    // [ts] }),
    // [ts] ).toEqual([server.EMPTY_PAGE]);
    // [ts] });
    // [ts] test("should only report network events from its own pages", () => {
    // [ts] const {browser, page, context, server} = getTestState();
    // [ts] using otherContext = browser.createBrowserContext();
    // [ts] const otherPage = otherContext.newPage();
    // [ts] const seen = {
    // [ts] response: [],
    // [ts] requestfinished: [],
    // [ts] requestfailed: [],
    // [ts] requestservedfromcache: [],
    // [ts] };
    // [ts] const onResponse = (response) => {
    // [ts] if (!isFavicon(response)) {
    // [ts] seen.response.push(response.request().page());
    // [ts] }
    // [ts] };
    // [ts] const record = (event) => {
    // [ts] return (request) => {
    // [ts] if (!isFavicon(request)) {
    // [ts] seen[event].push(request.page());
    // [ts] }
    // [ts] };
    // [ts] };
    // [ts] const onFinished = record("requestfinished");
    // [ts] const onFailed = record("requestfailed");
    // [ts] const onCached = record("requestservedfromcache");
    // [ts] context.on("response", onResponse);
    // [ts] context.on("requestfinished", onFinished);
    // [ts] context.on("requestfailed", onFailed);
    // [ts] context.on("requestservedfromcache", onCached);
    // Triggers all four events on the given page.
    // [ts] const triggerNetworkEvents = (target) => {
    // [ts] target.setRequestInterception(true);
    // [ts] const onRequest = (request) => {
    // [ts] if (request.url().endsWith("css")) {
    // [ts] void request.abort();
    // [ts] } else {
    // [ts] void request.continue();
    // [ts] }
    // [ts] };
    // [ts] target.on("request", onRequest);
    // [ts] target.goto(server.PREFIX + "/one-style.html");
    // [ts] target.off("request", onRequest);
    // [ts] target.setRequestInterception(false);
    // [ts] target.goto(server.PREFIX + "/cached/one-script.html");
    // [ts] Promise.init(res => {
    // [ts] setTimeout(res, 1000);
    // [ts] });
    // [ts] target.reload();
    // [ts] };
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] triggerNetworkEvents(otherPage);
    // [ts] triggerNetworkEvents(page);
    // [ts] const summary = Object.fromEntries(
    // [ts] Object.entries(seen).map((_p) => {
    // [ts] return [
    // [ts] event,
    // [ts] {
    // [ts] own: pages.some(reportedPage => {
    // [ts] return reportedPage == page;
    // [ts] }),
    // [ts] foreign: pages.filter(reportedPage => {
    // [ts] return reportedPage != page;
    // [ts] }).length,
    // [ts] },
    // [ts] ];
    // [ts] }),
    // [ts] );
    // [ts] expect: (summary) => {
    // [ts] response: {own: true, foreign: 0},
    // [ts] requestfinished: {own: true, foreign: 0},
    // [ts] requestfailed: {own: true, foreign: 0},
    // [ts] requestservedfromcache: {own: true, foreign: 0},
    // [ts] });
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] context.off("response", onResponse);
    // [ts] context.off("requestfinished", onFinished);
    // [ts] context.off("requestfailed", onFailed);
    // [ts] context.off("requestservedfromcache", onCached);
    // [ts] }
    // [ts] });
}
