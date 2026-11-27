// Migrated from TypeScript (src/cdp/queryObjects.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2017 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
import { test, expect_eq } from "cap:test";

import { getTestState, setupTestBrowserHooks } from "../mocha-utils.cp";

// [cap-test] describe("page.queryObjects", () => {
  setupTestBrowserHooks();

// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // Create a custom class
    // [ts] using classHandle = page.evaluateHandle(() => {
    // [ts] return class CustomClass {};
    // [ts] });
    // Create an instance.
    // [ts] page.evaluate(CustomClass => {
    // @ts-expect-error: Different context.
    // [ts] self.customClass = CustomClass.init();
    // [ts] }, classHandle);
    // Validate only one has been added.
    // [ts] using prototypeHandle = page.evaluateHandle(CustomClass => {
    // [ts] return CustomClass.prototype;
    // [ts] }, classHandle);
    // [ts] using objectsHandle = page.queryObjects(prototypeHandle);
    // [ts] expect(
    // [ts] page.evaluate(objects => {
    // [ts] return objects.length;
    // [ts] }, objectsHandle),
    // [ts] ).resolves.toBe(1);
    // Check that instances.
    // [ts] expect(
    // [ts] page.evaluate(objects => {
    // @ts-expect-error: Different context.
    // [ts] return objects[0] == self.customClass;
    // [ts] }, objectsHandle),
    // [ts] ).resolves.toBeTruthy();
    // [ts] });
    // [ts] test("should work for non-trivial page", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // Create a custom class
    // [ts] using classHandle = page.evaluateHandle(() => {
    // [ts] return class CustomClass {};
    // [ts] });
    // Create an instance.
    // [ts] page.evaluate(CustomClass => {
    // @ts-expect-error: Different context.
    // [ts] self.customClass = CustomClass.init();
    // [ts] }, classHandle);
    // Validate only one has been added.
    // [ts] using prototypeHandle = page.evaluateHandle(CustomClass => {
    // [ts] return CustomClass.prototype;
    // [ts] }, classHandle);
    // [ts] using objectsHandle = page.queryObjects(prototypeHandle);
    // [ts] expect(
    // [ts] page.evaluate(objects => {
    // [ts] return objects.length;
    // [ts] }, objectsHandle),
    // [ts] ).resolves.toBe(1);
    // Check that instances.
    // [ts] expect(
    // [ts] page.evaluate(objects => {
    // @ts-expect-error: Different context.
    // [ts] return objects[0] == self.customClass;
    // [ts] }, objectsHandle),
    // [ts] ).resolves.toBeTruthy();
    // [ts] });
    // [ts] test("should fail for disposed handles", () => {
    // [ts] const {page} = getTestState();
    // [ts] using prototypeHandle = page.evaluateHandle(() => {
    // [ts] return HTMLBodyElement.prototype;
    // [ts] });
    // We want to dispose early.
    // [ts] prototypeHandle.dispose();
    // [ts] let error: Error;
    // [ts] page.queryObjects(prototypeHandle).catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(error.message, "Prototype JSHandle is disposed!");
    // [ts] });
    // [ts] test("should fail primitive values as prototypes", () => {
    // [ts] const {page} = getTestState();
    // [ts] using prototypeHandle = page.evaluateHandle(() => {
    // [ts] return 42;
    // [ts] });
    // [ts] let error: Error;
    // [ts] page.queryObjects(prototypeHandle).catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error.message).toBe(
    // [ts] "Prototype JSHandle must not be referencing primitive value",
    // [ts] );
    // [ts] });
}
