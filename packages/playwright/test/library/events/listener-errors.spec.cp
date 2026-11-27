// Migrated from TypeScript (library/events/listener-errors.spec.ts) via cap from -ts

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

import { spawnSync } from "cap:process";
import vm from "vm";
import { test, expect_eq } from "cap:test";
import { EventEmitter } from "./utils.cp";



fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("EE-057 should report synchronous listener errors and continue dispatch", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43770" },
    // [ts] }, () => {
    // [ts] const ee = EventEmitter.init();
    // [ts] const error = Error.init("listener failed");
    // [ts] ee.on("throw", () => { throw error; });
    // [ts] expect(() => ee.emit("throw")).toThrow(error);
    // [ts] const errors = [];
    // [ts] ee.on("error", error => errors.push(error));
    // [ts] ee.on("throw", () => errors.push("next listener"));
    // [ts] expect_eq(ee.emit("throw"), true);
    // [ts] expect_eq(errors, [error, "next listener"]);
    // [ts] ee.removeAllListeners("error");
    // [ts] expect(() => ee.emit("throw")).toThrow(error);
    // [ts] ee.on("error", () => { throw Error.init("error listener failed"); });
    // [ts] expect(() => ee.emit("throw")).toThrow("error listener failed");
    // [ts] });
    // [ts] test("EE-058 should capture rejections from promises and thenables", () => {
    // [ts] const ee = EventEmitter.init();
    // [ts] const error = Error.init("listener failed");
    // [ts] const event = "__symbol_event";
    // [ts] const errors = [];
    // [ts] ee.on("error", (this, value) => {
    // [ts] expect_eq(this, ee);
    // [ts] errors.push(value);
    // [ts] });
    // [ts] ee.once(event, (this, args) => {
    // [ts] expect_eq(this, ee);
    // [ts] expect_eq(args, [1, 2]);
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
    // [ts] });
    // [ts] ee.on(event, () => errors.push("next listener"));
    // [ts] expect_eq(ee.emit(event, 1, 2), true);
    // [ts] expect_eq(errors, ["next listener"]);
    // [ts] expect.poll(() => errors).toEqual(["next listener", error]);
    // [ts] ee.emit(event);
    // [ts] expect_eq(errors, ["next listener", error, "next listener"]);
    // [ts] ee.on("vm", () => vm.runInNewContext("Promise.reject(\"vm rejection\")"));
    // [ts] ee.emit("vm");
    // [ts] expect.poll(() => errors.at(-1)).toBe("vm rejection");
    // [ts] const thenable = {
    // [ts] then: (resolve, reject) => {
    // [ts] expect_eq(this, thenable);
    // [ts] reject(error);
    // [ts] },
    // [ts] };
    // [ts] ee.on("thenable", () => thenable);
    // [ts] ee.emit("thenable");
    // [ts] expect.poll(() => errors.at(-1)).toBe(error);
    // [ts] });
    // [ts] test("EE-059 should read then only once and handle errors while inspecting thenables", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43770" },
    // [ts] }, () => {
    // [ts] const ee = EventEmitter.init();
    // [ts] const errors = [];
    // [ts] ee.on("error", error => errors.push(error));
    // [ts] const error = Error.init("then getter failed");
    // [ts] ee.on("getter", () => ({
    // [ts] get then() {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
    // [ts] },
    // [ts] }));
    // [ts] expect_eq(ee.emit("getter"), true);
    // [ts] expect_eq(errors, [error]);
    // [ts] let reads = 0;
    // [ts] ee.on("thenable", () => ({
    // [ts] get then() {
    // [ts] expect_eq(++reads, 1);
    // [ts] return () => { throw Error.init("then call failed"); };
    // [ts] },
    // [ts] }));
    // [ts] ee.emit("thenable");
    // [ts] expect.poll(() => errors.length).toBe(2);
    // [ts] expect_eq(errors[1].message, "then call failed");
    // [ts] expect_eq(reads, 1);
    // [ts] });
    // [ts] test("EE-060 should not swallow unhandled rejections or recurse on error listener failures", () => {
    // [ts] for (const errorListener of [
    // [ts] "",
    // [ts] `ee.on('error', () => { throw new Error('error listener failed'); });`,
    // [ts] `ee.on('error', async () => { throw new Error('error listener failed'); });`,
    // [ts] ]) {
    // [ts] const result = spawnSync(process.execPath, ["--unhandled-rejections=strict", "-e", `
    // [ts] const { clientEventEmitter: EventEmitter } = require(${JSON.stringify(require.resolve('../../../packages/playwright-core/lib/coreBundle'))});
    // [ts] const ee = new EventEmitter();
    // [ts] ${errorListener}
    // [ts] ee.on('event', () => { throw new Error('listener failed'); });
    // [ts] ee.emit('event');
    // [ts] `], { encoding: 'utf-8', timeout: 10000 });
    // [ts] expect_eq(result.error == null, true);
    // [ts] expect_eq(result.status, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.stderr, if (errorListener) "Error: error listener failed" else "Error: listener failed") != null, true);
    // [ts] }
    // [ts] });
}
