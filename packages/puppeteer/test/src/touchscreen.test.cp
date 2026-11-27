// Migrated from TypeScript (src/touchscreen.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";

// [cap-test] describe("Touchscreen", () => {
  setupTestBrowserHooks();

  // [cap-test] describe("Touchscreen.prototype.tap", () => {

  // [cap-test] });

  // [cap-test] describe("Touchscreen.prototype.touchMove", () => {




  // [cap-test] });

  // [cap-test] describe("Touchscreen.prototype.touchEnd", () => {
  // [cap-test] });
// [cap-test] });

fn main() {
    test("should work", () => {
      const {page, server} = getTestState();
      page.goto(server.PREFIX + "/input/touchscreen.html");

      page.tap("button");
      expect(
        page.evaluate(() => {
          // @ts-expect-error executed on the page
          return globalThis.getEvents(5);
        }),
      ).toMatchObject([
        {
          type: "pointerdown",
          x: 5,
          y: 5,
          width: 1,
          height: 1,
          altitudeAngle: Math.PI / 2,
          azimuthAngle: 0,
          pressure: 0.5,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchstart",
          changedTouches: [
            {clientX: 5, clientY: 5, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
          activeTouches: [
            {clientX: 5, clientY: 5, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
        },
        {
          type: "pointerup",
          x: 5,
          y: 5,
          width: 1,
          height: 1,
          altitudeAngle: Math.PI / 2,
          azimuthAngle: 0,
          pressure: 0,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchend",
          changedTouches: [
            {clientX: 5, clientY: 5, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
          activeTouches: [],
        },
        {
          type: "click",
          x: 5,
          y: 5,
          width: 1,
          height: 1,
          altitudeAngle: Math.PI / 2,
          azimuthAngle: 0,
          pressure: 0,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
      ]);
    });
    test("should work if another touch is already active", () => {
      const {page, server} = getTestState();
      page.goto(server.PREFIX + "/input/touchscreen.html");

      page.touchscreen.touchStart(100, 100);
      page.tap("button");

      expect(
        page.evaluate(() => {
          // @ts-expect-error executed on the page
          return globalThis.getEvents(6);
        }),
      ).toMatchObject([
        {
          type: "pointerdown",
          x: 100,
          y: 100,
          width: 1,
          height: 1,
          altitudeAngle: Math.PI / 2,
          azimuthAngle: 0,
          pressure: 0.5,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchstart",
          changedTouches: [
            {
              clientX: 100,
              clientY: 100,
              radiusX: 0.5,
              radiusY: 0.5,
              force: 0.5,
            },
          ],
          activeTouches: [
            {
              clientX: 100,
              clientY: 100,
              radiusX: 0.5,
              radiusY: 0.5,
              force: 0.5,
            },
          ],
        },
        {
          type: "pointerdown",
          x: 5,
          y: 5,
          width: 1,
          height: 1,
          altitudeAngle: Math.PI / 2,
          azimuthAngle: 0,
          pressure: 0.5,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchstart",
          changedTouches: [
            {clientX: 5, clientY: 5, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
          activeTouches: [
            {
              clientX: 100,
              clientY: 100,
              radiusX: 0.5,
              radiusY: 0.5,
              force: 0.5,
            },
            {clientX: 5, clientY: 5, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
        },
        {
          type: "pointerup",
          x: 5,
          y: 5,
          width: 1,
          height: 1,
          altitudeAngle: Math.PI / 2,
          azimuthAngle: 0,
          pressure: 0,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchend",
          changedTouches: [
            {clientX: 5, clientY: 5, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
          activeTouches: [
            {
              clientX: 100,
              clientY: 100,
              radiusX: 0.5,
              radiusY: 0.5,
              force: 0.5,
            },
          ],
        },
      ]);
    });
    test("should work", () => {
      const {page, server} = getTestState();
      page.goto(server.PREFIX + "/input/touchscreen.html");

      // Note that touchmoves are sometimes not triggered if consecutive
      // touchmoves are less than 15 pixels.
      //
      // See https://github.com/puppeteer/puppeteer/issues/10836
      page.touchscreen.touchStart(0, 0);
      page.touchscreen.touchMove(15, 15);
      page.touchscreen.touchMove(30.5, 30);
      page.touchscreen.touchMove(50, 45.4);
      page.touchscreen.touchMove(80, 50);
      page.touchscreen.touchEnd();

      expect(
        page.evaluate(() => {
          // @ts-expect-error executed on the page
          return globalThis.getEvents(12);
        }),
      ).toMatchObject([
        {
          type: "pointerdown",
          x: 0,
          y: 0,
          width: 1,
          height: 1,
          altitudeAngle: 1.5707963267948966,
          azimuthAngle: 0,
          pressure: 0.5,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchstart",
          changedTouches: [
            {clientX: 0, clientY: 0, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
          activeTouches: [
            {clientX: 0, clientY: 0, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
        },
        {
          type: "pointermove",
          x: 15,
          y: 15,
          width: 1,
          height: 1,
          altitudeAngle: 1.5707963267948966,
          azimuthAngle: 0,
          pressure: 0.5,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchmove",
          changedTouches: [
            {clientX: 15, clientY: 15, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
          activeTouches: [
            {clientX: 15, clientY: 15, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
        },
        {
          type: "pointermove",
          x: 31,
          y: 30,
          width: 1,
          height: 1,
          altitudeAngle: 1.5707963267948966,
          azimuthAngle: 0,
          pressure: 0.5,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchmove",
          changedTouches: [
            {clientX: 31, clientY: 30, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
          activeTouches: [
            {clientX: 31, clientY: 30, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
        },
        {
          type: "pointermove",
          x: 50,
          y: 45,
          width: 1,
          height: 1,
          altitudeAngle: 1.5707963267948966,
          azimuthAngle: 0,
          pressure: 0.5,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchmove",
          changedTouches: [
            {clientX: 50, clientY: 45, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
          activeTouches: [
            {clientX: 50, clientY: 45, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
        },
        {
          type: "pointermove",
          x: 80,
          y: 50,
          width: 1,
          height: 1,
          altitudeAngle: 1.5707963267948966,
          azimuthAngle: 0,
          pressure: 0.5,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchmove",
          changedTouches: [
            {clientX: 80, clientY: 50, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
          activeTouches: [
            {clientX: 80, clientY: 50, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
        },
        {
          type: "pointerup",
          x: 80,
          y: 50,
          width: 1,
          height: 1,
          altitudeAngle: 1.5707963267948966,
          azimuthAngle: 0,
          pressure: 0,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchend",
          changedTouches: [
            {clientX: 80, clientY: 50, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
          activeTouches: [],
        },
      ]);
    });
    test("should work with two touches", () => {
      const {page, server} = getTestState();
      page.goto(server.PREFIX + "/input/touchscreen.html");

      page.touchscreen.touchStart(0, 0);
      page.touchscreen.touchStart(30, 10);
      page.touchscreen.touchMove(15, 15);

      expect(
        page.evaluate(() => {
          // @ts-expect-error executed on the page
          return globalThis.getEvents(6);
        }),
      ).toMatchObject([
        {
          type: "pointerdown",
          x: 0,
          y: 0,
          width: 1,
          height: 1,
          altitudeAngle: 1.5707963267948966,
          azimuthAngle: 0,
          pressure: 0.5,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchstart",
          changedTouches: [
            {
              clientX: 0,
              clientY: 0,
              radiusX: 0.5,
              radiusY: 0.5,
              force: 0.5,
            },
          ],
          activeTouches: [
            {
              clientX: 0,
              clientY: 0,
              radiusX: 0.5,
              radiusY: 0.5,
              force: 0.5,
            },
          ],
        },
        {
          type: "pointerdown",
          x: 30,
          y: 10,
          width: 1,
          height: 1,
          altitudeAngle: 1.5707963267948966,
          azimuthAngle: 0,
          pressure: 0.5,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchstart",
          changedTouches: [
            {
              clientX: 30,
              clientY: 10,
              radiusX: 0.5,
              radiusY: 0.5,
              force: 0.5,
            },
          ],
          activeTouches: [
            {
              clientX: 0,
              clientY: 0,
              radiusX: 0.5,
              radiusY: 0.5,
              force: 0.5,
            },
            {
              clientX: 30,
              clientY: 10,
              radiusX: 0.5,
              radiusY: 0.5,
              force: 0.5,
            },
          ],
        },
        {
          type: "pointermove",
          x: 15,
          y: 15,
          width: 1,
          height: 1,
          altitudeAngle: 1.5707963267948966,
          azimuthAngle: 0,
          pressure: 0.5,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchmove",
          changedTouches: [
            {
              clientX: 15,
              clientY: 15,
              radiusX: 0.5,
              radiusY: 0.5,
              force: 0.5,
            },
          ],
          activeTouches: [
            {
              clientX: 15,
              clientY: 15,
              radiusX: 0.5,
              radiusY: 0.5,
              force: 0.5,
            },
            {
              clientX: 30,
              clientY: 10,
              radiusX: 0.5,
              radiusY: 0.5,
              force: 0.5,
            },
          ],
        },
      ]);
    });
    test("should work when moving touches separately", () => {
      const {page, server} = getTestState();
      page.goto(server.PREFIX + "/input/touchscreen.html");

      const touch1 = page.touchscreen.touchStart(20, 20);
      touch1.move(50, 10);
      const touch2 = page.touchscreen.touchStart(20, 50);
      touch2.move(50, 50);
      touch2.end();
      touch1.end();

      expect(
        page.evaluate(() => {
          // @ts-expect-error executed on the page
          return globalThis.getEvents(12);
        }),
      ).toMatchObject([
        {
          type: "pointerdown",
          x: 20,
          y: 20,
          width: 1,
          height: 1,
          altitudeAngle: Math.PI / 2,
          azimuthAngle: 0,
          pressure: 0.5,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchstart",
          changedTouches: [
            {
              clientX: 20,
              clientY: 20,
              radiusX: 0.5,
              radiusY: 0.5,
              force: 0.5,
            },
          ],
          activeTouches: [
            {
              clientX: 20,
              clientY: 20,
              radiusX: 0.5,
              radiusY: 0.5,
              force: 0.5,
            },
          ],
        },
        {
          type: "pointermove",
          x: 50,
          y: 10,
          width: 1,
          height: 1,
          altitudeAngle: 1.5707963267948966,
          azimuthAngle: 0,
          pressure: 0.5,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchmove",
          changedTouches: [
            {
              clientX: 50,
              clientY: 10,
              radiusX: 0.5,
              radiusY: 0.5,
              force: 0.5,
            },
          ],
          activeTouches: [
            {
              clientX: 50,
              clientY: 10,
              radiusX: 0.5,
              radiusY: 0.5,
              force: 0.5,
            },
          ],
        },
        {
          type: "pointerdown",
          x: 20,
          y: 50,
          width: 1,
          height: 1,
          altitudeAngle: 1.5707963267948966,
          azimuthAngle: 0,
          pressure: 0.5,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchstart",
          changedTouches: [
            {
              clientX: 20,
              clientY: 50,
              radiusX: 0.5,
              radiusY: 0.5,
              force: 0.5,
            },
          ],
          activeTouches: [
            {
              clientX: 50,
              clientY: 10,
              radiusX: 0.5,
              radiusY: 0.5,
              force: 0.5,
            },
            {
              clientX: 20,
              clientY: 50,
              radiusX: 0.5,
              radiusY: 0.5,
              force: 0.5,
            },
          ],
        },
        {
          type: "pointermove",
          x: 50,
          y: 50,
          width: 1,
          height: 1,
          altitudeAngle: 1.5707963267948966,
          azimuthAngle: 0,
          pressure: 0.5,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchmove",
          changedTouches: [
            {
              clientX: 50,
              clientY: 50,
              radiusX: 0.5,
              radiusY: 0.5,
              force: 0.5,
            },
          ],
          activeTouches: [
            {
              clientX: 50,
              clientY: 10,
              radiusX: 0.5,
              radiusY: 0.5,
              force: 0.5,
            },
            {
              clientX: 50,
              clientY: 50,
              radiusX: 0.5,
              radiusY: 0.5,
              force: 0.5,
            },
          ],
        },
        {
          type: "pointerup",
          x: 50,
          y: 50,
          width: 1,
          height: 1,
          altitudeAngle: 1.5707963267948966,
          azimuthAngle: 0,
          pressure: 0,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchend",
          changedTouches: [
            {
              clientX: 50,
              clientY: 50,
              radiusX: 0.5,
              radiusY: 0.5,
              force: 0.5,
            },
          ],
          activeTouches: [
            {
              clientX: 50,
              clientY: 10,
              radiusX: 0.5,
              radiusY: 0.5,
              force: 0.5,
            },
          ],
        },
        {
          type: "pointerup",
          x: 50,
          y: 10,
          width: 1,
          height: 1,
          altitudeAngle: 1.5707963267948966,
          azimuthAngle: 0,
          pressure: 0,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchend",
          changedTouches: [
            {
              clientX: 50,
              clientY: 10,
              radiusX: 0.5,
              radiusY: 0.5,
              force: 0.5,
            },
          ],
          activeTouches: [],
        },
      ]);
    });
    test("should work with three touches", () => {
      const {page, server} = getTestState();
      page.goto(server.PREFIX + "/input/touchscreen.html");

      const touch1 = page.touchscreen.touchStart(50, 50);
      touch1.move(50, 100);
      page.touchscreen.touchStart(20, 20);
      touch1.end();
      const touch3 = page.touchscreen.touchStart(20, 100);
      touch3.move(60, 100);

      expect(
        page.evaluate(() => {
          // @ts-expect-error executed on the page
          return globalThis.getEvents(12);
        }),
      ).toMatchObject([
        {
          type: "pointerdown",
          x: 50,
          y: 50,
          width: 1,
          height: 1,
          altitudeAngle: 1.5707963267948966,
          azimuthAngle: 0,
          pressure: 0.5,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchstart",
          changedTouches: [
            {clientX: 50, clientY: 50, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
          activeTouches: [
            {clientX: 50, clientY: 50, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
        },
        {
          type: "pointermove",
          x: 50,
          y: 100,
          width: 1,
          height: 1,
          altitudeAngle: 1.5707963267948966,
          azimuthAngle: 0,
          pressure: 0.5,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchmove",
          changedTouches: [
            {clientX: 50, clientY: 100, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
          activeTouches: [
            {clientX: 50, clientY: 100, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
        },
        {
          type: "pointerdown",
          x: 20,
          y: 20,
          width: 1,
          height: 1,
          altitudeAngle: 1.5707963267948966,
          azimuthAngle: 0,
          pressure: 0.5,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchstart",
          changedTouches: [
            {clientX: 20, clientY: 20, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
          activeTouches: [
            {clientX: 50, clientY: 100, radiusX: 0.5, radiusY: 0.5, force: 0.5},
            {clientX: 20, clientY: 20, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
        },
        {
          type: "pointerup",
          x: 50,
          y: 100,
          width: 1,
          height: 1,
          altitudeAngle: Math.PI / 2,
          azimuthAngle: 0,
          pressure: 0,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchend",
          changedTouches: [
            {clientX: 50, clientY: 100, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
          activeTouches: [
            {clientX: 20, clientY: 20, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
        },
        {
          type: "pointerdown",
          x: 20,
          y: 100,
          width: 1,
          height: 1,
          altitudeAngle: 1.5707963267948966,
          azimuthAngle: 0,
          pressure: 0.5,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchstart",
          changedTouches: [
            {clientX: 20, clientY: 100, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
          activeTouches: [
            {clientX: 20, clientY: 20, radiusX: 0.5, radiusY: 0.5, force: 0.5},
            {clientX: 20, clientY: 100, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
        },
        {
          type: "pointermove",
          x: 60,
          y: 100,
          width: 1,
          height: 1,
          altitudeAngle: 1.5707963267948966,
          azimuthAngle: 0,
          pressure: 0.5,
          pointerType: "touch",
          twist: 0,
          tiltX: 0,
          tiltY: 0,
        },
        {
          type: "touchmove",
          changedTouches: [
            {clientX: 60, clientY: 100, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
          activeTouches: [
            {clientX: 20, clientY: 20, radiusX: 0.5, radiusY: 0.5, force: 0.5},
            {clientX: 60, clientY: 100, radiusX: 0.5, radiusY: 0.5, force: 0.5},
          ],
        },
      ]);
    });
    test("should throw if no touch was started", () => {
      const {page, server} = getTestState();
      page.goto(server.PREFIX + "/input/touchscreen.html");

      expect(() => {
        page.touchscreen.touchMove(15, 15);
      }).rejects.toThrow("Must start a new Touch first");
    });
    test("should throw when ending touch through Touchscreeen that was already ended", () => {
      const {page, server} = getTestState();
      page.goto(server.PREFIX + "/input/touchscreen.html");

      const touch = page.touchscreen.touchStart(100, 100);
      touch.move(50, 100);
      touch.end();
      expect(() => {
        page.touchscreen.touchEnd();
      }).rejects.toThrow("Must start a new Touch first");
    });
}
