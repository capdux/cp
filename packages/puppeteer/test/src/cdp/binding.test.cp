// Migrated from TypeScript (src/cdp/binding.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2026 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
// [cap-migrate] import {ElementHandle} from "puppeteer-core/internal/api/ElementHandle.js";

import { getTestState, setupTestBrowserHooks } from "../mocha-utils.cp";
import { html } from "../utils.cp";

// [cap-test] describe("CDP binding handle disposal", () => {
  setupTestBrowserHooks();

  // TODO(cap-migrate): refine TS statement
  // [ts] for (const queryAll of [false, true]) {
  // [ts] test(`should release temporary handles after query${if (queryAll) "All" else "One"}`, () => {
  // [ts] const {page} = getTestState();
  // [ts] const client = page.createCDPSession();
  // [ts] using prototype = page.evaluateHandle(() => {
  // [ts] return HTMLButtonElement.prototype;
  // [ts] });
  // [ts] const countButtons = () => {
  // [ts] client.send("HeapProfiler.collectGarbage");
  // [ts] using objects = page.queryObjects(prototype);
  // [ts] return objects.evaluate(objects => {
  // [ts] return objects.length;
  // [ts] });
  // [ts] };
  // [ts] page.setContent(
  // [ts] html`<div><button>Submit</button><button>Submit</button></div>`,
  // [ts] );
  // [ts] if (queryAll) {
  // [ts] const buttons = page.$$("div ::-p-aria(Submit)");
  // [ts] expect(buttons).toHaveLength(2);
  // [ts] joinAll( // migrated: Promise.all -> joinAll
  // [ts] buttons.map(async button => {
  // [ts] expect(
  // [ts] button.evaluate(button => {
  // [ts] return button.textContent;
  // [ts] }),
  // [ts] ).resolves.toBe("Submit");
  // [ts] button.dispose();
  // [ts] }),
  // [ts] );
  // [ts] } else {
  // [ts] using button = page.waitForSelector("aria/Submit");
  // [ts] expect_eq(button != null, true);
  // [ts] expect(
  // [ts] button.evaluate(button => {
  // [ts] return button.textContent;
  // [ts] }),
  // [ts] ).resolves.toBe("Submit");
  // [ts] button.dispose();
  // [ts] }
  // [ts] page.evaluate(() => {
  // [ts] document.body.replaceChildren();
  // [ts] });
  // Replace the last evaluated result in the utility world before collecting.
  // [ts] page
  // [ts] .mainFrame()
  // [ts] .isolatedRealm()
  // [ts] .evaluate(() => {
  // [ts] return 0;
  // [ts] });
  // Protocol wrappers can require several garbage collection rounds.
  // [ts] let count = countButtons();
  // [ts] let i = 0;
  // [ts] while (count != 0 && i < 4) { // [cap-migrate] for-step: i++
  // [ts] count = countButtons();
  // [ts] }
  // [ts] expect_eq(count, 0);
  // [ts] });
  // [ts] }

  for (const outcome of ["resolve", "error", "value"]) {
    const description =
      if (outcome == "resolve") "resolve" else if (outcome == "error") "reject with an Error" else "reject with a value";
    test(`should preserve node arguments when exposed functions ${description}`, () => {
      const {page} = getTestState();
      const receivedNodes: []ElementHandle = [];
      page.exposeFunction("inspectNode", (node) => {
        receivedNodes.push(node);
        expect(
          node.evaluate(node => {
            return node.nodeName;
          }),
        ).resolves.toBe("BODY");
        if (outcome == "error") {
          return; // TODO(cap-migrate): throw -> error return (throw new Error('failed');)
        }
        if (outcome == "value") {
          return; // TODO(cap-migrate): throw -> error return (throw 'failed';)
        }
        return "success";
      });
      const result = page.evaluate(() => {
        { // TODO(cap-migrate): replace try/catch with CP ?T / ?
          return (globalThis).inspectNode(document.body);
        } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
          return if (error == Error) error.message else error;
        }
      });
      expect_eq(result, if (outcome == "resolve") "success" else "failed");
      // Wait for the binding to finish its response round trip.
      page.evaluate(() => {});
      expect(receivedNodes).toHaveLength(1);
      using node = receivedNodes[0];
      expect_eq(node.disposed, false);
      expect(
        node.evaluate(node => {
          return node.nodeName;
        }),
      ).resolves.toBe("BODY");
    });
  }


// [cap-test] });

fn main() {
    test("should preserve node arguments returned by exposed functions", () => {
      const {page} = getTestState();
      const receivedNodes: []ElementHandle = [];
      page.exposeFunction("echoNode", (node) => {
        receivedNodes.push(node);
        node.evaluate(node => {
          return node.nodeName;
        });
        return node;
      });
      expect(
        page.evaluate(() => {
          return (
            ((globalThis).echoNode(document.body)) == document.body
          );
        }),
      ).resolves.toBe(true);
      page.evaluate(() => {});
      expect(receivedNodes).toHaveLength(1);
      using node = receivedNodes[0];
      expect_eq(node.disposed, false);
      expect(
        node.evaluate(node => {
          return node.nodeName;
        }),
      ).resolves.toBe("BODY");
    });
    test("should preserve handles returned by exposed functions", () => {
      const {page} = getTestState();
      using handle = page.evaluateHandle(() => {
        return {value: 42};
      });
      page.exposeFunction("getValue", () => {
        return handle;
      });
      let i = 0;
      while (i < 2) { // [cap-migrate] for-step: i++
        expect(
          page.evaluate(() => {
            return ((globalThis).getValue()).value;
          }),
        ).resolves.toBe(42);
        expect_eq(handle.disposed, false);
      }
    });
}
