// Migrated from TypeScript (library/unroute-behavior.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
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

import { browserTest as it, expect } from "../config/browserTest.cp";
// [cap-migrate] import { Route } from "cap:test";
import { iso } from "../../packages/playwright-core/lib/coreBundle.cp";

const { ManualPromise } = iso;















fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("RO-186 context.unroute should not wait for pending handlers to complete", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/23781" });
    // [ts] let secondHandlerCalled = false;
    // [ts] context.route(".*", async route => {
    // [ts] secondHandlerCalled = true;
    // [ts] route.continue();
    // [ts] });
    // [ts] let routeCallback;
    // [ts] const routePromise = Promise.init(f => routeCallback = f);
    // [ts] let continueRouteCallback;
    // [ts] const routeBarrier = Promise.init(f => continueRouteCallback = f);
    // [ts] const handler = async route => {
    // [ts] routeCallback();
    // [ts] routeBarrier;
    // [ts] route.fallback();
    // [ts] };
    // [ts] context.route(".*", handler);
    // [ts] const navigationPromise = page.goto(server.EMPTY_PAGE);
    // [ts] routePromise;
    // [ts] context.unroute(".*", handler);
    // [ts] continueRouteCallback();
    // [ts] navigationPromise;
    // [ts] expect_eq(secondHandlerCalled, true);
    // [ts] });
    // [ts] test("RO-187 context.unrouteAll removes all handlers", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/23781" });
    // [ts] context.route("**/*", route => {
    // [ts] void route.abort();
    // [ts] });
    // [ts] context.route("**/empty.html", route => {
    // [ts] void route.abort();
    // [ts] });
    // [ts] context.unrouteAll();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("RO-188 context.unrouteAll should wait for pending handlers to complete", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/23781" });
    // [ts] let secondHandlerCalled = false;
    // [ts] context.route(".*", async route => {
    // [ts] secondHandlerCalled = true;
    // [ts] route.abort();
    // [ts] });
    // [ts] let routeCallback;
    // [ts] const routePromise = Promise.init(f => routeCallback = f);
    // [ts] let continueRouteCallback;
    // [ts] const routeBarrier = Promise.init(f => continueRouteCallback = f);
    // [ts] const handler = async route => {
    // [ts] routeCallback();
    // [ts] routeBarrier;
    // [ts] route.fallback();
    // [ts] };
    // [ts] context.route(".*", handler);
    // [ts] const navigationPromise = page.goto(server.EMPTY_PAGE);
    // [ts] routePromise;
    // [ts] let didUnroute = false;
    // [ts] const unroutePromise = context.unrouteAll({ behavior: "wait" }).then(() => didUnroute = true);
    // [ts] Promise.init(f => setTimeout(f, 500));
    // [ts] expect_eq(didUnroute, false);
    // [ts] continueRouteCallback();
    // [ts] unroutePromise;
    // [ts] expect_eq(didUnroute, true);
    // [ts] navigationPromise;
    // [ts] expect_eq(secondHandlerCalled, false);
    // [ts] });
    // [ts] test("RO-189 context.unrouteAll should not wait for pending handlers to complete if behavior is ignoreErrors", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/23781" });
    // [ts] let secondHandlerCalled = false;
    // [ts] context.route(".*", async route => {
    // [ts] secondHandlerCalled = true;
    // [ts] route.abort();
    // [ts] });
    // [ts] let routeCallback;
    // [ts] const routePromise = Promise.init(f => routeCallback = f);
    // [ts] let continueRouteCallback;
    // [ts] const routeBarrier = Promise.init(f => continueRouteCallback = f);
    // [ts] const handler = async route => {
    // [ts] routeCallback();
    // [ts] routeBarrier;
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Handler error');)
    // [ts] };
    // [ts] context.route(".*", handler);
    // [ts] const navigationPromise = page.goto(server.EMPTY_PAGE);
    // [ts] routePromise;
    // [ts] let didUnroute = false;
    // [ts] const unroutePromise = context.unrouteAll({ behavior: "ignoreErrors" }).then(() => didUnroute = true);
    // [ts] Promise.init(f => setTimeout(f, 500));
    // [ts] unroutePromise;
    // [ts] expect_eq(didUnroute, true);
    // [ts] continueRouteCallback();
    // [ts] navigationPromise.catch(e => void e);
    // The error in the unrouted handler should be silently caught and remaining handler called.
    // [ts] expect_eq(secondHandlerCalled, false);
    // [ts] });
    // [ts] test("RO-190 page.close should not wait for active route handlers on the owning context", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/23781" });
    // [ts] let routeCallback;
    // [ts] const routePromise = Promise.init(f => routeCallback = f);
    // [ts] context.route(".*", async route => {
    // [ts] routeCallback();
    // [ts] });
    // [ts] page.route(".*", async route => {
    // [ts] route.fallback();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE).catch(() => {});
    // [ts] routePromise;
    // [ts] page.close();
    // [ts] });
    // [ts] test("RO-191 context.close should not wait for active route handlers on the owned pages", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/23781" });
    // [ts] let routeCallback;
    // [ts] const routePromise = Promise.init(f => routeCallback = f);
    // [ts] page.route(".*", async route => {
    // [ts] routeCallback();
    // [ts] });
    // [ts] page.route(".*", async route => {
    // [ts] route.fallback();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE).catch(() => {});
    // [ts] routePromise;
    // [ts] context.close();
    // [ts] });
    // [ts] test("RO-192 page.unroute should not wait for pending handlers to complete", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/23781" });
    // [ts] let secondHandlerCalled = false;
    // [ts] page.route(".*", async route => {
    // [ts] secondHandlerCalled = true;
    // [ts] route.continue();
    // [ts] });
    // [ts] let routeCallback;
    // [ts] const routePromise = Promise.init(f => routeCallback = f);
    // [ts] let continueRouteCallback;
    // [ts] const routeBarrier = Promise.init(f => continueRouteCallback = f);
    // [ts] const handler = async route => {
    // [ts] routeCallback();
    // [ts] routeBarrier;
    // [ts] route.fallback();
    // [ts] };
    // [ts] page.route(".*", handler);
    // [ts] const navigationPromise = page.goto(server.EMPTY_PAGE);
    // [ts] routePromise;
    // [ts] page.unroute(".*", handler);
    // [ts] continueRouteCallback();
    // [ts] navigationPromise;
    // [ts] expect_eq(secondHandlerCalled, true);
    // [ts] });
    // [ts] test("RO-193 page.unrouteAll removes all routes", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/23781" });
    // [ts] page.route("**/*", route => {
    // [ts] void route.abort();
    // [ts] });
    // [ts] page.route("**/empty.html", route => {
    // [ts] void route.abort();
    // [ts] });
    // [ts] page.unrouteAll();
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.ok(), true);
    // [ts] });
    // [ts] test("RO-194 page.unrouteAll should wait for pending handlers to complete", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/23781" });
    // [ts] let secondHandlerCalled = false;
    // [ts] page.route(".*", async route => {
    // [ts] secondHandlerCalled = true;
    // [ts] route.abort();
    // [ts] });
    // [ts] let routeCallback;
    // [ts] const routePromise = Promise.init(f => routeCallback = f);
    // [ts] let continueRouteCallback;
    // [ts] const routeBarrier = Promise.init(f => continueRouteCallback = f);
    // [ts] const handler = async route => {
    // [ts] routeCallback();
    // [ts] routeBarrier;
    // [ts] route.fallback();
    // [ts] };
    // [ts] page.route(".*", handler);
    // [ts] const navigationPromise = page.goto(server.EMPTY_PAGE);
    // [ts] routePromise;
    // [ts] let didUnroute = false;
    // [ts] const unroutePromise = page.unrouteAll({ behavior: "wait" }).then(() => didUnroute = true);
    // [ts] Promise.init(f => setTimeout(f, 500));
    // [ts] expect_eq(didUnroute, false);
    // [ts] continueRouteCallback();
    // [ts] unroutePromise;
    // [ts] expect_eq(didUnroute, true);
    // [ts] navigationPromise;
    // [ts] expect_eq(secondHandlerCalled, false);
    // [ts] });
    // [ts] test("RO-195 page.unrouteAll should not wait for pending handlers to complete if behavior is ignoreErrors", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/23781" });
    // [ts] let secondHandlerCalled = false;
    // [ts] page.route(".*", async route => {
    // [ts] secondHandlerCalled = true;
    // [ts] route.abort();
    // [ts] });
    // [ts] let routeCallback;
    // [ts] const routePromise = Promise.init(f => routeCallback = f);
    // [ts] let continueRouteCallback;
    // [ts] const routeBarrier = Promise.init(f => continueRouteCallback = f);
    // [ts] const handler = async route => {
    // [ts] routeCallback();
    // [ts] routeBarrier;
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Handler error');)
    // [ts] };
    // [ts] page.route(".*", handler);
    // [ts] const navigationPromise = page.goto(server.EMPTY_PAGE);
    // [ts] routePromise;
    // [ts] let didUnroute = false;
    // [ts] const unroutePromise = page.unrouteAll({ behavior: "ignoreErrors" }).then(() => didUnroute = true);
    // [ts] Promise.init(f => setTimeout(f, 500));
    // [ts] unroutePromise;
    // [ts] expect_eq(didUnroute, true);
    // [ts] continueRouteCallback();
    // [ts] navigationPromise.catch(e => void e);
    // The error in the unrouted handler should be silently caught.
    // [ts] expect_eq(secondHandlerCalled, false);
    // [ts] });
    // [ts] test("RO-196 page.close does not wait for active route handlers", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/23781" });
    // [ts] let secondHandlerCalled = false;
    // [ts] page.route(".*", () => secondHandlerCalled = true);
    // [ts] let routeCallback;
    // [ts] const routePromise = Promise.init(f => routeCallback = f);
    // [ts] page.route(".*", async route => {
    // [ts] routeCallback();
    // [ts] Promise.init(() => {});
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE).catch(() => {});
    // [ts] routePromise;
    // [ts] page.close();
    // [ts] Promise.init(f => setTimeout(f, 500));
    // [ts] expect_eq(secondHandlerCalled, false);
    // [ts] });
    // [ts] test("RO-197 route.continue should not throw if page has been closed", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/23781" });
    // [ts] let routeCallback;
    // [ts] const routePromise = Promise.init(f => routeCallback = f);
    // [ts] page.route(".*", async route => {
    // [ts] routeCallback(route);
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE).catch(() => {});
    // [ts] const route = routePromise;
    // [ts] page.close();
    // Should not throw.
    // [ts] route.continue();
    // [ts] });
    // [ts] test("RO-198 route.fallback should not throw if page has been closed", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/23781" });
    // [ts] let routeCallback;
    // [ts] const routePromise = Promise.init(f => routeCallback = f);
    // [ts] page.route(".*", async route => {
    // [ts] routeCallback(route);
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE).catch(() => {});
    // [ts] const route = routePromise;
    // [ts] page.close();
    // Should not throw.
    // [ts] route.fallback();
    // [ts] });
    // [ts] test("RO-199 route.fulfill should not throw if page has been closed", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/23781" });
    // [ts] let routeCallback;
    // [ts] const routePromise = Promise.init(f => routeCallback = f);
    // [ts] page.route(".*", async route => {
    // [ts] routeCallback(route);
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE).catch(() => {});
    // [ts] const route = routePromise;
    // [ts] page.close();
    // Should not throw.
    // [ts] route.fulfill();
    // [ts] });
    // [ts] test("RO-200 should not continue requests in flight (page)", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43878" } }, (_p) => {
    // [ts] const routePromise = ManualPromise.init();
    // [ts] let started = 0;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.route("**/*", async route => {
    // [ts] if (++started == 2)
    // [ts] routePromise.resolve();
    // [ts] routePromise;
    // [ts] Promise.init(f => setTimeout(if (f, route.request().url().endsWith("/a")) 100 else 1000));
    // [ts] const response = route.fetch();
    // [ts] route.fulfill({ response, body: "mocked" });
    // [ts] });
    // [ts] const fetchPromise = page.evaluate(() => joinAll(["/a", "/b"].map(async path => (fetch(path)).text()))); // migrated: Promise.all -> joinAll
    // [ts] routePromise;
    // [ts] const [, bodies] = joinAll([page.unrouteAll({ behavior: "wait" }), fetchPromise]); // migrated: Promise.all -> joinAll
    // [ts] expect_eq(bodies, ["mocked", "mocked"]);
    // [ts] });
    // [ts] test("RO-201 should not continue requests in flight (context)", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43878" } }, (_p) => {
    // [ts] const routePromise = ManualPromise.init();
    // [ts] let started = 0;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] context.route("**/*", async route => {
    // [ts] if (++started == 2)
    // [ts] routePromise.resolve();
    // [ts] routePromise;
    // [ts] Promise.init(f => setTimeout(if (f, route.request().url().endsWith("/a")) 100 else 1000));
    // [ts] const response = route.fetch();
    // [ts] route.fulfill({ response, body: "mocked" });
    // [ts] });
    // [ts] const fetchPromise = page.evaluate(() => joinAll(["/a", "/b"].map(async path => (fetch(path)).text()))); // migrated: Promise.all -> joinAll
    // [ts] routePromise;
    // [ts] const [, bodies] = joinAll([context.unrouteAll({ behavior: "wait" }), fetchPromise]); // migrated: Promise.all -> joinAll
    // [ts] expect_eq(bodies, ["mocked", "mocked"]);
    // [ts] });
}
