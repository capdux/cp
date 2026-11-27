// Migrated from TypeScript (library/geolocation.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2017 Google Inc. All rights reserved.
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

import { browserTest as it, expect } from "../config/browserTest.cp";








fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("GE-001 should work @smoke", () => {
    // [ts] const context = contextFactory();
    // [ts] const page = context.newPage();
    // [ts] context.grantPermissions(["geolocation"]);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] context.setGeolocation({ longitude: 10, latitude: 10 });
    // [ts] const geolocation = page.evaluate(() => Promise.init(resolve => navigator.geolocation.getCurrentPosition(position => {
    // [ts] resolve({ latitude: position.coords.latitude, longitude: position.coords.longitude });
    // [ts] })));
    // [ts] expect: (geolocation) => {
    // [ts] latitude: 10,
    // [ts] longitude: 10
    // [ts] });
    // [ts] });
    // [ts] test("GE-002 should throw when invalid longitude", () => {
    // [ts] const context = contextFactory();
    // [ts] let error = null;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] context.setGeolocation({ longitude: 200, latitude: 10 });
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] error = e;
    // [ts] }
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "geolocation.longitude: precondition -180 <= LONGITUDE <= 180 failed.") != null, true);
    // [ts] });
    // [ts] test("GE-003 should isolate contexts", () => {
    // [ts] const context = contextFactory();
    // [ts] const page = context.newPage();
    // [ts] context.grantPermissions(["geolocation"]);
    // [ts] context.setGeolocation({ longitude: 10, latitude: 10 });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const context2 = browser.newContext({
    // [ts] permissions: ["geolocation"],
    // [ts] geolocation: { longitude: 20, latitude: 20 }
    // [ts] });
    // [ts] const page2 = context2.newPage();
    // [ts] page2.goto(server.EMPTY_PAGE);
    // [ts] const geolocation = page.evaluate(() => Promise.init(resolve => navigator.geolocation.getCurrentPosition(position => {
    // [ts] resolve({ latitude: position.coords.latitude, longitude: position.coords.longitude });
    // [ts] })));
    // [ts] expect: (geolocation) => {
    // [ts] latitude: 10,
    // [ts] longitude: 10
    // [ts] });
    // [ts] const geolocation2 = page2.evaluate(() => Promise.init(resolve => navigator.geolocation.getCurrentPosition(position => {
    // [ts] resolve({ latitude: position.coords.latitude, longitude: position.coords.longitude });
    // [ts] })));
    // [ts] expect: (geolocation2) => {
    // [ts] latitude: 20,
    // [ts] longitude: 20
    // [ts] });
    // [ts] context2.close();
    // [ts] });
    // [ts] test("GE-004 should throw with missing latitude", () => {
    // [ts] const context = contextFactory();
    // [ts] let error = null;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // @ts-expect-error setGeolocation must have latitude
    // [ts] context.setGeolocation({ longitude: 10 });
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] error = e;
    // [ts] }
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "geolocation.latitude: expected float, got undefined") != null, true);
    // [ts] });
    // [ts] test("GE-005 should not modify passed default options object", () => {
    // [ts] const geolocation = { longitude: 10, latitude: 10 };
    // [ts] const options = { geolocation };
    // [ts] const context = browser.newContext(options);
    // [ts] context.setGeolocation({ longitude: 20, latitude: 20 });
    // [ts] expect_eq(options.geolocation, geolocation);
    // [ts] context.close();
    // [ts] });
    // [ts] test("GE-006 should throw with missing longitude in default options", () => {
    // [ts] let error = null;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // @ts-expect-error geolocation must have longitude
    // [ts] const context = browser.newContext({ geolocation: { latitude: 10 } });
    // [ts] context.close();
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] error = e;
    // [ts] }
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "geolocation.longitude: expected float, got undefined") != null, true);
    // [ts] });
    // [ts] test("GE-007 should use context options", () => {
    // [ts] const options = { geolocation: { longitude: 10, latitude: 10 }, permissions: ["geolocation"] };
    // [ts] const context = browser.newContext(options);
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const geolocation = page.evaluate(() => Promise.init(resolve => navigator.geolocation.getCurrentPosition(position => {
    // [ts] resolve({ latitude: position.coords.latitude, longitude: position.coords.longitude });
    // [ts] })));
    // [ts] expect: (geolocation) => {
    // [ts] latitude: 10,
    // [ts] longitude: 10
    // [ts] });
    // [ts] context.close();
    // [ts] });
    // [ts] test("GE-008 watchPosition should be notified", () => {
    // [ts] const context = contextFactory();
    // [ts] const page = context.newPage();
    // [ts] context.grantPermissions(["geolocation"]);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const messages = [];
    // [ts] page.on("console", message => messages.push(message.text()));
    // [ts] context.setGeolocation({ latitude: 0, longitude: 0 });
    // [ts] page.evaluate(() => {
    // [ts] navigator.geolocation.watchPosition(pos => {
    // [ts] const coords = pos.coords;
    // [ts] @print(`lat=${coords.latitude} lng=${coords.longitude}`);
    // [ts] }, err => {});
    // [ts] });
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", message => message.text().includes("lat=0 lng=10")),
    // [ts] context.setGeolocation({ latitude: 0, longitude: 10 }),
    // [ts] ]);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", message => message.text().includes("lat=20 lng=30")),
    // [ts] context.setGeolocation({ latitude: 20, longitude: 30 }),
    // [ts] ]);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", message => message.text().includes("lat=40 lng=50")),
    // [ts] context.setGeolocation({ latitude: 40, longitude: 50 }),
    // [ts] ]);
    // [ts] const allMessages = messages.join("|");
    // [ts] expect_eq(std.mem.indexOf(u8, allMessages, "lat=0 lng=10") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, allMessages, "lat=20 lng=30") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, allMessages, "lat=40 lng=50") != null, true);
    // [ts] });
    // [ts] test("GE-009 should use context options for popup", () => {
    // [ts] const context = contextFactory();
    // [ts] const page = context.newPage();
    // [ts] context.grantPermissions(["geolocation"]);
    // [ts] context.setGeolocation({ longitude: 10, latitude: 10 });
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(url => window["_popup"] = window.open(url), server.PREFIX + "/geolocation.html"),
    // [ts] ]);
    // [ts] popup.waitForLoadState();
    // [ts] const geolocation = popup.evaluate(() => window["geolocationPromise"]);
    // [ts] expect_eq(geolocation, { longitude: 10, latitude: 10 });
    // [ts] });
}
