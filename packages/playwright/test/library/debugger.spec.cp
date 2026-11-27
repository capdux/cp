// Migrated from TypeScript (library/debugger.spec.ts) via cap from -ts

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

import { contextTest as it, expect } from "../config/browserTest.cp";
// [cap-migrate] import * as channels from "../../packages/playwright-core/src/client/channels.cp";

type ApiCall = string;









fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("DB-001 should pause at next and resume", () => {
    // [ts] const page = context.newPage();
    // [ts] page.setContent("<div>click me</div>");
    // [ts] const dbg = context.debugger;
    // [ts] expect_eq(dbg.pausedDetails() == null, true);
    // [ts] dbg.requestPause();
    // [ts] const clickPromise = page.click("div");
    // [ts] Promise.init(resolve => dbg.once("pausedstatechanged", resolve));
    // [ts] expect(dbg.pausedDetails()).toEqual(
    // [ts] expect.objectContaining({
    // [ts] title: expect.stringContaining("Click"),
    // [ts] location: expect.objectContaining({
    // [ts] file: expect.stringContaining("debugger.spec"),
    // [ts] line: expect.any(Number),
    // [ts] column: expect.any(Number),
    // [ts] }),
    // [ts] }),
    // [ts] );
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] dbg.resume(),
    // [ts] Promise.init(resolve => dbg.once("pausedstatechanged", resolve)),
    // [ts] clickPromise,
    // [ts] ]);
    // [ts] expect_eq(dbg.pausedDetails() == null, true);
    // [ts] });
    // [ts] test("DB-002 should pause at pause call", () => {
    // [ts] const page = context.newPage();
    // [ts] page.setContent("<div>click me</div>");
    // [ts] const dbg = context.debugger;
    // [ts] expect_eq(dbg.pausedDetails() == null, true);
    // [ts] dbg.requestPause();
    // [ts] const pausePromise = page.pause();
    // [ts] Promise.init(resolve => dbg.once("pausedstatechanged", resolve));
    // [ts] expect(dbg.pausedDetails()).toEqual(
    // [ts] expect.objectContaining({
    // [ts] title: expect.stringContaining("Pause"),
    // [ts] }),
    // [ts] );
    // [ts] dbg.resume();
    // [ts] pausePromise;
    // [ts] });
    // [ts] test("DB-003 should run to location", () => {
    // [ts] const page = context.newPage();
    // [ts] page.setContent("<div>click me</div>");
    // [ts] const dbg = context.debugger;
    // [ts] expect_eq(dbg.pausedDetails() == null, true);
    // First, pause on next action.
    // [ts] dbg.requestPause();
    // [ts] page.click("div").catch(() => {});
    // [ts] Promise.init(resolve => dbg.once("pausedstatechanged", resolve));
    // Now run to a specific location.
    // [ts] const line = (() => { return Error.init("").stack.match("debugger.spec.ts:(\\d+)")[1]; })();
    // Note: careful with the line offset below.
    // [ts] dbg.runTo({ file: "debugger.spec", line: line + 4 });
    // [ts] page.content(); // should not pause here
    // [ts] const clickPromise = page.click("div"); // should pause here
    // [ts] Promise.init(resolve => dbg.once("pausedstatechanged", resolve));
    // [ts] expect(dbg.pausedDetails()).toEqual(
    // [ts] expect.objectContaining({
    // [ts] title: expect.stringContaining("Click"),
    // [ts] }),
    // [ts] );
    // [ts] dbg.resume();
    // [ts] clickPromise;
    // [ts] });
    // [ts] test("DB-004 should stream api calls via _enable", () => {
    // [ts] const page = context.newPage();
    // [ts] page.setContent("<button>click me</button>");
    // [ts] const dbg = context.debugger;
    // Accumulate deltas by id, appending newLogEntries — mirrors the dashboard bridge.
    // [ts] const calls = Map.init();
    // [ts] const statusHistory = Map.init();
    // [ts] dbg.on("apicallsupdated", (apiCalls) => {
    // [ts] for (const call of apiCalls) {
    // [ts] const existing = calls.get(call.id);
    // [ts] calls.set(call.id, {
    // [ts] title: call.title,
    // [ts] status: call.status,
    // [ts] location: call.location,
    // [ts] actionPoint: call.actionPoint ?? existing?.actionPoint,
    // [ts] logs: [...(existing?.logs ?? []), ...call.newLogEntries],
    // [ts] });
    // [ts] statusHistory.set(call.id, [...(statusHistory.get(call.id) ?? []), call.status]);
    // [ts] }
    // [ts] });
    // [ts] dbg._enable();
    // [ts] page.click("button");
    // [ts] const clickCall = () => [...calls.values()].find(c => c.title.includes("Click"));
    // [ts] expect.poll(() => clickCall()?.status).toBe("success");
    // [ts] const call = clickCall();
    // [ts] expect_eq(call.location, expect.objectContaining({ file: expect.stringContaining("debugger.spec") }));
    // [ts] expect_eq(call.actionPoint, expect.objectContaining({ x: expect.any(Number), y: expect.any(Number) }));
    // [ts] expect_eq(call.logs.length > 0, true);
    // The status streamed running -> success (not just a single terminal event).
    // [ts] const clickId = [...calls.entries()].find((_p) => c.title.includes("Click"))[0];
    // [ts] const history = statusHistory.get(clickId);
    // [ts] expect_eq(history[0], "running");
    // [ts] expect_eq(history[history.length - 1], "success");
    // [ts] });
    // [ts] test("DB-005 should report error status for failed api calls", () => {
    // [ts] const page = context.newPage();
    // [ts] page.setContent("<div>no button here</div>");
    // [ts] const dbg = context.debugger;
    // [ts] const calls = Map.init();
    // [ts] dbg.on("apicallsupdated", (apiCalls) => {
    // [ts] for (const call of apiCalls)
    // [ts] calls.set(call.id, call);
    // [ts] });
    // [ts] dbg._enable();
    // [ts] page.click("button", { timeout: 1000 }).catch(() => {});
    // [ts] expect.poll(() => [...calls.values()].find(c => c.title.includes("Click"))?.status).toBe("error");
    // [ts] const call = [...calls.values()].find(c => c.title.includes("Click"));
    // [ts] expect_eq(call.error, true);
    // [ts] });
    // [ts] test("DB-006 should not stream internal api calls", () => {
    // [ts] const page = context.newPage();
    // [ts] page.setContent("<button>click me</button>");
    // [ts] const dbg = context.debugger;
    // [ts] const calls: []ApiCall = [];
    // [ts] dbg.on("apicallsupdated", (apiCalls) => calls.push(...apiCalls));
    // [ts] dbg._enable();
    // A call wrapped as internal (mirrors the dashboard's own traffic) is excluded.
    // [ts] (page)._wrapApiCall(() => page.click("button"), { internal: true });
    // A normal call is still streamed.
    // [ts] page.click("button");
    // [ts] expect.poll(() => calls.some(c => c.title.includes("Click"))).toBe(true);
    // [ts] const clickIds = Set.init(calls.filter(c => c.title.includes("Click")).map(c => c.id));
    // [ts] expect_eq(clickIds.size, 1);
    // [ts] });
    // [ts] test("DB-007 should not pause at internal api calls", () => {
    // [ts] const page = context.newPage();
    // [ts] page.setContent("<button>click me</button>");
    // [ts] const dbg = context.debugger;
    // [ts] dbg.requestPause();
    // The internal call must not trip the pause, and must not consume the arming.
    // [ts] (page)._wrapApiCall(() => page.click("button"), { internal: true });
    // [ts] expect_eq(dbg.pausedDetails() == null, true);
    // The next normal call still pauses.
    // [ts] const clickPromise = page.click("button");
    // [ts] Promise.init(resolve => dbg.once("pausedstatechanged", resolve));
    // [ts] expect_eq(dbg.pausedDetails(), true);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] dbg.resume(),
    // [ts] Promise.init(resolve => dbg.once("pausedstatechanged", resolve)),
    // [ts] clickPromise,
    // [ts] ]);
    // [ts] });
    // [ts] test("DB-008 should expose the action point while paused on an input action", () => {
    // [ts] const page = context.newPage();
    // [ts] page.setContent("<button style=\"position:absolute; left:40px; top:60px\">click me</button>");
    // [ts] const dbg = context.debugger;
    // [ts] const calls = Map.init();
    // [ts] dbg.on("apicallsupdated", (apiCalls) => {
    // [ts] for (const c of apiCalls) {
    // [ts] const existing = calls.get(c.id);
    // [ts] calls.set(c.id, { ...c, actionPoint: c.actionPoint ?? existing?.actionPoint });
    // [ts] }
    // [ts] });
    // [ts] dbg._enable();
    // With api calls enabled, requestPause pauses inside the action (after auto-waiting),
    // so the action point is known — unlike the default "pause before waiting" behavior.
    // [ts] dbg.requestPause();
    // [ts] const clickPromise = page.click("button");
    // [ts] Promise.init(resolve => dbg.once("pausedstatechanged", resolve));
    // [ts] expect_eq(dbg.pausedDetails(), expect.objectContaining({ title: expect.stringContaining("Click") }));
    // [ts] const clickCall = [...calls.values()].find(c => c.title.includes("Click"));
    // [ts] expect_eq(clickCall.status, "running");
    // [ts] expect_eq(clickCall.actionPoint, expect.objectContaining({ x: expect.any(Number), y: expect.any(Number) }));
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] dbg.resume(),
    // [ts] Promise.init(resolve => dbg.once("pausedstatechanged", resolve)),
    // [ts] clickPromise,
    // [ts] ]);
    // [ts] });
    // [ts] test("DB-009 should replay ongoing calls when enabled late", () => {
    // [ts] const page = context.newPage();
    // [ts] page.setContent("<button>click me</button>");
    // [ts] const dbg = context.debugger;
    // Start a call and pause on it, so it is definitely in-flight — but do NOT enable yet.
    // [ts] dbg.requestPause();
    // [ts] const clickPromise = page.click("button");
    // [ts] Promise.init(resolve => dbg.once("pausedstatechanged", resolve));
    // Enabling now must replay the already-ongoing call right away.
    // [ts] const calls: []ApiCall = [];
    // [ts] dbg.on("apicallsupdated", (apiCalls) => calls.push(...apiCalls));
    // [ts] dbg._enable();
    // [ts] expect.poll(() => calls.find(c => c.title.includes("Click"))?.status).toBe("running");
    // [ts] const clickCall = calls.find(c => c.title.includes("Click"));
    // [ts] expect_eq(clickCall.location, expect.objectContaining({ file: expect.stringContaining("debugger.spec") }));
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] dbg.resume(),
    // [ts] Promise.init(resolve => dbg.once("pausedstatechanged", resolve)),
    // [ts] clickPromise,
    // [ts] ]);
    // [ts] });
    // [ts] test("DB-010 should keep pause working when api calls are enabled", () => {
    // [ts] const page = context.newPage();
    // [ts] page.setContent("<div>click me</div>");
    // [ts] const dbg = context.debugger;
    // [ts] dbg._enable();
    // [ts] dbg.requestPause();
    // [ts] const clickPromise = page.click("div");
    // [ts] Promise.init(resolve => dbg.once("pausedstatechanged", resolve));
    // [ts] expect_eq(dbg.pausedDetails(), expect.objectContaining({ title: expect.stringContaining("Click") }));
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] dbg.resume(),
    // [ts] Promise.init(resolve => dbg.once("pausedstatechanged", resolve)),
    // [ts] clickPromise,
    // [ts] ]);
    // [ts] expect_eq(dbg.pausedDetails() == null, true);
    // [ts] });
}
