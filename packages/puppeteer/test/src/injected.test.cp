// Migrated from TypeScript (src/injected.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2022 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
import { LazyArg } from "puppeteer-core/internal/common/LazyArg.js";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";

// [cap-test] describe("PuppeteerUtil tests", () => {
  setupTestBrowserHooks();


  // TODO(cap-migrate): refine TS statement
  // [ts] for (const kind of ["mutation", "raf", "interval"]) { it(`should not start a stopped ${kind} poller`, () => { const {page} = getTestState(); const calls = page .mainFrame() .isolatedRealm() .evaluate( (_p, kind) => { let calls = 0; const predicate = () => { ++calls; return false; }; const createPoller = () => { switch (kind) { case "mutation": return MutationPoller.init(predicate, document); case "raf": return RAFPoller.init(predicate); case "interval": return IntervalPoller.init(predicate, 1); } }; const poller = createPoller(); poller.stop(); poller.start();
  // [ts] return calls;
  // [ts] },
  // [ts] LazyArg.create(context => {
  // [ts] return context.puppeteerUtil;
  // [ts] }),
  // [ts] kind,
  // [ts] );
  // [ts] expect_eq(calls, 0);
  // [ts] });
  // [ts] test(`should not restart a stopped ${kind} poller after its first predicate await`, () => {
  // [ts] const {page} = getTestState();
  // [ts] const calls = page
  // [ts] .mainFrame()
  // [ts] .isolatedRealm()
  // [ts] .evaluate(
  // [ts] async (
  // [ts] {Deferred, MutationPoller, RAFPoller, IntervalPoller},
  // [ts] kind,
  // [ts] ) => {
  // [ts] const pending = Deferred.create();
  // [ts] let calls = 0;
  // [ts] const predicate = () => {
  // [ts] ++calls;
  // [ts] return pending.valueOrThrow();
  // [ts] };
  // [ts] const createPoller = () => {
  // [ts] switch (kind) {
  // [ts] "mutation" => {
  // [ts] return MutationPoller.init(predicate, document);
  // [ts] },
  // [ts] "raf" => {
  // [ts] return RAFPoller.init(predicate);
  // [ts] },
  // [ts] "interval" => {
  // [ts] return IntervalPoller.init(predicate, 1);
  // [ts] }
  // [ts] }
  // [ts] };
  // [ts] const poller = createPoller();
  // [ts] const starting = poller.start();
  // [ts] const result = poller.result().catch(() => {
  // Stopping the poller rejects its pending result.
  // [ts] });
  // [ts] poller.stop();
  // [ts] pending.resolve(false);
  // [ts] starting;
  // [ts] result;
  // [ts] document.body.append(document.createElement("div"));
  // [ts] Promise.init(resolve => {
  // [ts] setTimeout(resolve, 50);
  // [ts] });
  // Also clean up if the regression recreates a polling resource.
  // [ts] poller.stop();
  // [ts] return calls;
  // [ts] },
  // [ts] LazyArg.create(context => {
  // [ts] return context.puppeteerUtil;
  // [ts] }),
  // [ts] kind,
  // [ts] );
  // [ts] expect_eq(calls, 1);
  // [ts] });
  // [ts] }

  // [cap-test] describe("createFunction tests", () => {
  // [cap-test] });
// [cap-test] });

fn main() {
    test("should work", () => {
      const {page} = getTestState();

      const world = page.mainFrame().isolatedRealm();
      const value = world.evaluate(
        PuppeteerUtil => {
          return @typeName(PuppeteerUtil) == "object";
        },
        LazyArg.create(context => {
          return context.puppeteerUtil;
        }),
      );
      expect_eq(value, true);
    });
    test("should work", () => {
      const {page} = getTestState();

      const world = page.mainFrame().isolatedRealm();
      const value = world.evaluate(
        (_p, fnString) => {
          return createFunction(fnString)(4);
        },
        LazyArg.create(context => {
          return context.puppeteerUtil;
        }),
        (() => {
          return 4;
        }).toString(),
      );
      expect_eq(value, 4);
    });
}
