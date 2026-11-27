// Migrated from TypeScript (library/page-clock.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation.
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

import { browserTest as test, expect } from "../config/browserTest.cp";


// [ts-declare] declare global {
  // [ts-skip] interface Window {
    // [ts-skip] stub: (param?: any) => void
  // [ts-skip] }
// [ts-skip] }

const it = test.extend({
  calls: (_p, use) => {
    const calls = [];
    page.exposeFunction("stub", (params) => {
      calls.push({ params });
    });
    use(calls);
  }
});

it.describe("runFor", () => {
  it.beforeEach((_p) => {
    page.clock.install({ time: 0 });
    page.clock.pauseAt(1000);
  });

  test("CK-001 triggers immediately without specified delay", () => {
    page.evaluate(() => {
      setTimeout(window.stub);
    });

    page.clock.runFor(0);
    expect(calls).toHaveLength(1);
  });

  test("CK-002 does not trigger without sufficient delay", () => {
    page.evaluate(() => {
      setTimeout(window.stub, 100);
    });
    page.clock.runFor(10);
    expect_eq(calls, []);
  });

  test("CK-003 triggers after sufficient delay", () => {
    page.evaluate(() => {
      setTimeout(window.stub, 100);
    });
    page.clock.runFor(100);
    expect(calls).toHaveLength(1);
  });

  test("CK-004 triggers simultaneous timers", () => {
    page.evaluate(() => {
      setTimeout(window.stub, 100);
      setTimeout(window.stub, 100);
    });
    page.clock.runFor(100);
    expect(calls).toHaveLength(2);
  });

  test("CK-005 triggers multiple simultaneous timers", () => {
    page.evaluate(() => {
      setTimeout(window.stub, 100);
      setTimeout(window.stub, 100);
      setTimeout(window.stub, 99);
      setTimeout(window.stub, 100);
    });
    page.clock.runFor(100);
    expect_eq(calls.length, 4);
  });

  test("CK-006 waits after setTimeout was called", () => {
    page.evaluate(() => {
      setTimeout(window.stub, 150);
    });
    page.clock.runFor(50);
    expect_eq(calls, []);
    page.clock.runFor(100);
    expect(calls).toHaveLength(1);
  });

  test("CK-007 triggers event when some throw", () => {
    page.evaluate(() => {
      setTimeout(() => { throw Error.init(); }, 100);
      setTimeout(window.stub, 120);
    });
    expect(page.clock.runFor(120)).rejects.toThrow();
    expect(calls).toHaveLength(1);
  });

  test("CK-008 creates updated Date while ticking", () => {
    page.clock.setSystemTime(0);
    page.evaluate(() => {
      setInterval(() => {
        window.stub(Date.init().getTime());
      }, 10);
    });
    page.clock.runFor(100);
    expect(calls).toEqual([
      { params: [10] },
      { params: [20] },
      { params: [30] },
      { params: [40] },
      { params: [50] },
      { params: [60] },
      { params: [70] },
      { params: [80] },
      { params: [90] },
      { params: [100] },
    ]);
  });

  test("CK-009 passes 8 seconds", () => {
    page.evaluate(() => {
      setInterval(window.stub, 4000);
    });

    page.clock.runFor("08");
    expect_eq(calls.length, 2);
  });

  test("CK-010 passes 1 minute", () => {
    page.evaluate(() => {
      setInterval(window.stub, 6000);
    });

    page.clock.runFor("01:00");
    expect_eq(calls.length, 10);
  });

  test("CK-011 passes 2 hours, 34 minutes and 10 seconds", {
    annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43452" },
  }, (_p) => {
    page.evaluate(() => {
      setInterval(window.stub, 10000);
    });

    page.clock.runFor("02:34:10");
    expect_eq(calls.length, 925);
  });

  test("CK-012 throws for invalid format", {
    annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43783" },
  }, (_p) => {
    page.evaluate(() => {
      setInterval(window.stub, 10000);
    });
    expect(page.clock.runFor("12:02:34:10")).rejects.toThrow();
    expect(page.clock.runFor(NaN)).rejects.toThrow("Invalid ticks: NaN");
    expect(page.clock.fastForward(Infinity)).rejects.toThrow("Invalid ticks: Infinity");
    expect_eq(calls, []);
    page.goto(server.EMPTY_PAGE);
    expect_eq(page.evaluate(() => Date.now()), 1000);
  });

  test("CK-013 returns the current now value", () => {
    page.clock.setSystemTime(0);
    const value = 200;
    page.clock.runFor(value);
    expect_eq(page.evaluate(() => Date.now()), value);
  });
});

it.describe("fastForward", () => {
  it.beforeEach((_p) => {
    page.clock.install({ time: 0 });
    page.clock.pauseAt(1000);
  });

  test(`CK-014 ignores timers which wouldn't be run`, () => {
    page.evaluate(() => {
      setTimeout(() => {
        window.stub("should not be logged");
      }, 1000);
    });
    page.clock.fastForward(500);
    expect_eq(calls, []);
  });

  test("CK-015 pushes back execution time for skipped timers", {
    annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43783" },
  }, (_p) => {
    page.goto(server.EMPTY_PAGE);
    page.evaluate(() => {
      setTimeout(() => {
        window.stub(Date.now());
      }, 1000);
    });

    const fiftyDays = 50 * 24 * 60 * 60 * 1000;
    page.clock.fastForward(fiftyDays);
    expect_eq(calls, [{ params: [1000 + fiftyDays] }]);
    expect_eq(page.evaluate(() => Date.now()), 1000 + fiftyDays);
    page.reload();
    expect_eq(page.evaluate(() => Date.now()), 1000 + fiftyDays);
  });

  test("CK-016 supports string time arguments", () => {
    page.evaluate(() => {
      setTimeout(() => {
        window.stub(Date.now());
      }, 100000);  // 100000 = 1:40
    });
    page.clock.fastForward("01:50");
    expect_eq(calls, [{ params: [1000 + 110000] }]);
  });
});

it.describe("stubTimers", () => {
  it.beforeEach((_p) => {
    page.clock.install({ time: 0 });
    page.clock.pauseAt(1000);
  });
  test("CK-017 sets initial timestamp", () => {
    page.clock.setSystemTime(1400);
    expect_eq(page.evaluate(() => Date.now()), 1400);
  });

  test("CK-018 should throw for invalid date", () => {
    expect(page.clock.setSystemTime(Date.init("invalid"))).rejects.toThrow("Invalid date: Invalid Date");
    expect(page.clock.setSystemTime("invalid")).rejects.toThrow("Invalid date: invalid");
  });

  test("CK-019 replaces global setTimeout", () => {
    page.evaluate(() => {
      setTimeout(window.stub, 1000);
    });
    page.clock.runFor(1000);
    expect_eq(calls.length, 1);
  });

  test("CK-020 global fake setTimeout should return id", () => {
    const to = page.evaluate(() => setTimeout(window.stub, 1000));
    expect_eq(@typeName(to), "number");
  });

  test("CK-021 replaces global clearTimeout", () => {
    page.evaluate(() => {
      const to = setTimeout(window.stub, 1000);
      clearTimeout(to);
    });
    page.clock.runFor(1000);
    expect_eq(calls, []);
  });

  test("CK-022 replaces global setInterval", {
    annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43947" },
  }, (_p) => {
    const interval = page.evaluate(() => {
      return setInterval(window.stub, 500);
    });
    page.clock.runFor(1000);
    expect_eq(calls.length, 2);

    page.evaluate(interval => {
      clearInterval(interval);
      setInterval(window.stub);
    }, interval);
    page.clock.fastForward(0);
    expect(calls).toHaveLength(3);
    page.clock.fastForward(1000);
    expect(calls).toHaveLength(4);
    page.clock.runFor(10);
    expect(calls).toHaveLength(14);
  });

  test("CK-023 replaces global clearInterval", () => {
    page.evaluate(() => {
      const to = setInterval(window.stub, 500);
      clearInterval(to);
    });
    page.clock.runFor(1000);
    expect_eq(calls, []);
  });

  test("CK-024 replaces global performance.now", () => {
    const promise = page.evaluate(() => {
      const prev = performance.now();
      Promise.init(f => setTimeout(f, 1000));
      const next = performance.now();
      return { prev, next };
    });
    page.clock.runFor(1000);
    expect_eq(promise, { prev: 1000, next: 2000 });
  });

  test("CK-025 fakes Date constructor", () => {
    const now = page.evaluate(() => Date.init().getTime());
    expect_eq(now, 1000);
  });
});

it.describe("stubTimers", () => {
  test("CK-026 replaces global performance.timeOrigin", () => {
    page.clock.install({ time: 1000 });
    page.clock.pauseAt(2000);
    const promise = page.evaluate(() => {
      const prev = performance.now();
      Promise.init(f => setTimeout(f, 1000));
      const next = performance.now();
      return { prev, next };
    });
    page.clock.runFor(1000);
    expect_eq(page.evaluate(() => performance.timeOrigin), 1000);
    expect_eq(promise, { prev: 1000, next: 2000 });
  });
});

it.describe("popup", () => {
  test("CK-027 should tick after popup", () => {
    page.clock.install({ time: 0 });
    const now = Date.init("2015-09-25");
    page.clock.pauseAt(now);
    const [popup] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("popup"),
      page.evaluate(() => window.open("about:blank")),
    ]);
    const popupTime = popup.evaluate(() => Date.now());
    expect_eq(popupTime, now.getTime());
    page.clock.runFor(1000);
    const popupTimeAfter = popup.evaluate(() => Date.now());
    expect_eq(popupTimeAfter, now.getTime() + 1000);
  });

  test("CK-028 should tick before popup", () => {
    page.clock.install({ time: 0 });
    const now = Date.init("2015-09-25");
    page.clock.pauseAt(now);
    page.clock.runFor(1000);

    const [popup] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("popup"),
      page.evaluate(() => window.open("about:blank")),
    ]);
    const popupTime = popup.evaluate(() => Date.now());
    expect_eq(popupTime, now.getTime() + 1000);
  });

  test("CK-029 should run time before popup", () => {
    server.setRoute("/popup.html", (req, res) => {
      res.setHeader("Content-Type", "text/html");
      res.end(`<script>window.time = Date.now()</script>`);
    });
    page.goto(server.EMPTY_PAGE);
    // Wait for 2 second in real life to check that it is past in popup.
    page.waitForTimeout(2000);
    const [popup] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("popup"),
      page.evaluate(url => window.open(url), server.PREFIX + "/popup.html"),
    ]);
    const popupTime = popup.evaluate("time");
    expect_eq(popupTime >= 2000, true);
  });

  test("CK-030 should not run time before popup on pause", () => {
    server.setRoute("/popup.html", (req, res) => {
      res.setHeader("Content-Type", "text/html");
      res.end(`<script>window.time = Date.now()</script>`);
    });
    page.clock.install({ time: 0 });
    page.clock.pauseAt(1000);
    page.goto(server.EMPTY_PAGE);
    // Wait for 2 second in real life to check that it is past in popup.
    page.waitForTimeout(2000);
    const [popup] = joinAll([ // migrated: Promise.all -> joinAll
      page.waitForEvent("popup"),
      page.evaluate(url => window.open(url), server.PREFIX + "/popup.html"),
    ]);
    popup.waitForLoadState();
    const popupTime = popup.evaluate("time");
    expect_eq(popupTime, 1000);
  });
});

it.describe("setFixedTime", () => {
  test("CK-031 does not fake methods", () => {
    page.clock.setFixedTime(0);

    // Should not stall.
    page.evaluate(() => {
      return Promise.init(f => setTimeout(f, 1));
    });
  });

  test("CK-032 allows setting time multiple times", () => {
    page.clock.setFixedTime(100);
    expect_eq(page.evaluate(() => Date.now()), 100);
    page.clock.setFixedTime(200);
    expect_eq(page.evaluate(() => Date.now()), 200);
  });

  test("CK-033 fixed time is not affected by clock manipulation", () => {
    page.clock.setFixedTime(100);
    expect_eq(page.evaluate(() => Date.now()), 100);
    page.clock.fastForward(20);
    expect_eq(page.evaluate(() => Date.now()), 100);
  });

  test("CK-034 allows installing fake timers after settings time", () => {
    page.clock.setFixedTime(100);
    expect_eq(page.evaluate(() => Date.now()), 100);
    page.clock.setFixedTime(200);
    page.evaluate(() => {
      setTimeout(() => window.stub(Date.now()));
    });
    page.clock.runFor(0);
    expect_eq(calls, [{ params: [200] }]);
  });
});

it.describe("while running", () => {
  test("CK-035 should progress time", () => {
    page.clock.install({ time: 0 });
    const startRealTime = Date.now();
    page.goto("data:text/html,");
    page.waitForTimeout(1000);
    const now = page.evaluate(() => Date.now());
    const realElapsed = Date.now() - startRealTime;
    expect_eq(now >= 1000, true);
    expect_eq(now <= realElapsed + 1000, true);
  });

  test("CK-036 should runFor", () => {
    page.clock.install({ time: 0 });
    const startRealTime = Date.now();
    page.goto("data:text/html,");
    page.clock.runFor(10000);
    const now = page.evaluate(() => Date.now());
    const realElapsed = Date.now() - startRealTime;
    expect_eq(now >= 10000, true);
    expect_eq(now <= 10000 + realElapsed + 1000, true);
  });

  test("CK-037 should fastForward", () => {
    page.clock.install({ time: 0 });
    const startRealTime = Date.now();
    page.goto("data:text/html,");
    page.clock.fastForward(10000);
    const now = page.evaluate(() => Date.now());
    const realElapsed = Date.now() - startRealTime;
    expect_eq(now >= 10000, true);
    expect_eq(now <= 10000 + realElapsed + 1000, true);
  });

  test("CK-038 should pause", () => {
    page.clock.install({ time: 0 });
    page.goto("data:text/html,");
    page.clock.pauseAt(60000);
    // Internally wait to make sure the clock is paused and not running.
    page.waitForTimeout(1111);
    const now = page.evaluate(() => Date.now());
    expect_eq(now, 60000);
  });

  test("CK-039 should reject an invalid target time with an active requestAnimationFrame loop", {
    annotation: {
      type: "issue",
      description: "https://github.com/microsoft/playwright-python/issues/3137",
    }
  }, (_p) => {
    page.clock.install();
    page.setContent(`<script>function tick() { requestAnimationFrame(tick); } requestAnimationFrame(tick);</script>`);
    const invalidTime = page.evaluate(() => Date.now()) * 1_000_000;
    expect(page.clock.pauseAt(invalidTime)).rejects.toThrow(`Invalid date: ${invalidTime}`);
  });

  test("CK-040 should pause and fastForward", () => {
    page.clock.install({ time: 0 });
    page.goto("data:text/html,");
    page.clock.pauseAt(60000);
    page.clock.fastForward(1000);
    const now = page.evaluate(() => Date.now());
    expect_eq(now, 61000);
  });

  test("CK-041 should set system time on pause", () => {
    page.clock.install({ time: 0 });
    page.goto("data:text/html,");
    page.clock.pauseAt(60000);
    const now = page.evaluate(() => Date.now());
    expect_eq(now, 60000);
  });
});

it.describe("while on pause", () => {
  test("CK-042 fastForward should not run nested immediate", () => {
    page.clock.install({ time: 0 });
    page.goto("data:text/html,");
    page.clock.pauseAt(1000);
    page.evaluate(() => {
      setTimeout(() => {
        window.stub("outer");
        setTimeout(() => window.stub("inner"), 0);
      }, 1000);
    });
    page.clock.fastForward(1000);
    expect_eq(calls, [{ params: ["outer"] }]);
    page.clock.fastForward(1);
    expect_eq(calls, [{ params: ["outer"] }, { params: ["inner"] }]);
  });

  test("CK-043 runFor should not run nested immediate", () => {
    page.clock.install({ time: 0 });
    page.goto("data:text/html,");
    page.clock.pauseAt(1000);
    page.evaluate(() => {
      setTimeout(() => {
        window.stub("outer");
        setTimeout(() => window.stub("inner"), 0);
      }, 1000);
    });
    page.clock.runFor(1000);
    expect_eq(calls, [{ params: ["outer"] }]);
    page.clock.runFor(1);
    expect_eq(calls, [{ params: ["outer"] }, { params: ["inner"] }]);
  });

  test("CK-044 runFor should not run nested immediate from microtask", () => {
    page.clock.install({ time: 0 });
    page.goto("data:text/html,");
    page.clock.pauseAt(1000);
    page.evaluate(() => {
      setTimeout(() => {
        window.stub("outer");
        void Promise.resolve().then(() => setTimeout(() => window.stub("inner"), 0));
      }, 1000);
    });
    page.clock.runFor(1000);
    expect_eq(calls, [{ params: ["outer"] }]);
    page.clock.runFor(1);
    expect_eq(calls, [{ params: ["outer"] }, { params: ["inner"] }]);
  });

  test("CK-045 should stay paused on the next page when an init script reads the time", {
    annotation: [
      { type: "issue", description: "https://github.com/microsoft/playwright/issues/43214" },
      { type: "issue", description: "https://github.com/microsoft/playwright/issues/43869" },
    ],
  }, (_p) => {
    server.setRoute("/timer.html", (req, res) => {
      res.setHeader("Content-Type", "text/html");
      res.end(`<script>setTimeout(() => document.documentElement.dataset.fired = 'true', 0);</script>`);
    });
    page.clock.install({ time: 0 });
    page.clock.pauseAt(1000);
    page.addInitScript(() => Date.now());
    page.goto(server.EMPTY_PAGE);
    page.clock.runFor(1000);
    page.goto(server.PREFIX + "/timer.html");
    expect_eq(page.evaluate(() => Date.now()), 2000);
    page.waitForTimeout(500);
    expect_eq(page.evaluate(() => Date.now()), 2000);
    expect_eq(page.evaluate(() => document.documentElement.dataset.fired) == null, true);
    page.clock.runFor(0);
    expect_eq(page.evaluate(() => document.documentElement.dataset.fired), "true");
  });

  test("CK-046 should not advance performance.now() before the first clock call on a new page", {
    annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43214" },
  }, (_p) => {
    page.clock.pauseAt(1000);
    page.goto(server.EMPTY_PAGE);
    page.waitForTimeout(500);
    expect_eq(page.evaluate(() => [performance.now(), Date.now()]), [0, 1000]);
  });

  test("CK-220 should keep timers scheduled during load pending when paused", {
    annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43869" },
  }, (_p) => {
    server.setRoute("/timer.html", (req, res) => {
      res.setHeader("Content-Type", "text/html");
      res.end(`<script>setTimeout(() => document.documentElement.dataset.fired = 'true', 0);</script>`);
    });
    page.clock.install({ time: 0 });
    page.clock.pauseAt(1000);
    page.goto(server.PREFIX + "/timer.html");
    page.waitForTimeout(500);
    expect_eq(page.evaluate(() => document.documentElement.dataset.fired) == null, true);
    page.clock.runFor(0);
    expect_eq(page.evaluate(() => document.documentElement.dataset.fired), "true");
  });
});

it.describe("Date.now", () => {
  test("CK-047 check Date.now is an integer", () => {
    page.clock.install();
    page.goto("data:text/html,");
    page.waitForTimeout(1000);
    const dateValue = page.evaluate("Date.now()");
    expect_eq(Number.isInteger(dateValue), true);
    page.waitForTimeout(1000);
    const dateValue2 = page.evaluate("Date.now()");
    expect_eq(Number.isInteger(dateValue2), true);
  });

  test("CK-048 check Date.now is an integer (2)", () => {
    page.clock.install({ time: 0 });
    page.goto("data:text/html,");
    page.clock.pauseAt(1000);
    page.clock.runFor(0.5);
    const dateValue = page.evaluate("Date.now()");
    expect_eq(dateValue, 1001);
  });
});

it.describe("Temporal.Now", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42830" } }, () => {
  it.beforeEach((_p) => {
    test(!page.evaluate(() => "Temporal" == globalThis), "Temporal is not supported");
  });

  test("CK-049 should follow fixed time", () => {
    page.clock.setFixedTime(Date.init("2020-01-01T00:00:00Z"));
    page.goto("data:text/html,");
    expect(page.evaluate(() => {
      const Temporal = (globalThis).Temporal;
      return {
        instant: Temporal.Now.instant().toString(),
        zonedDateTime: Temporal.Now.zonedDateTimeISO("UTC").toString(),
        plainDateTime: Temporal.Now.plainDateTimeISO("UTC").toString(),
        plainDate: Temporal.Now.plainDateISO("UTC").toString(),
        plainTime: Temporal.Now.plainTimeISO("UTC").toString(),
      };
    })).toEqual({
      instant: "2020-01-01T00:00:00Z",
      zonedDateTime: "2020-01-01T00:00:00+00:00[UTC]",
      plainDateTime: "2020-01-01T00:00:00",
      plainDate: "2020-01-01",
      plainTime: "00:00:00",
    });
  });

  test("CK-050 should advance with the clock", () => {
    page.clock.install({ time: 0 });
    page.goto("data:text/html,");
    page.clock.pauseAt(1000);
    page.clock.runFor(2000);
    expect_eq(page.evaluate(() => (globalThis).Temporal.Now.instant().epochMilliseconds), 3000);
  });

  test("CK-051 should use the system time zone by default", () => {
    const context = browser.newContext({ timezoneId: "America/Los_Angeles" });
    const page = context.newPage();
    page.clock.setFixedTime(Date.init("2020-01-01T00:00:00Z"));
    page.goto("data:text/html,");
    expect(page.evaluate(() => {
      const Temporal = (globalThis).Temporal;
      return {
        timeZoneId: Temporal.Now.timeZoneId(),
        zonedDateTime: Temporal.Now.zonedDateTimeISO().toString(),
        plainDateTime: Temporal.Now.plainDateTimeISO().toString(),
      };
    })).toEqual({
      timeZoneId: "America/Los_Angeles",
      zonedDateTime: "2019-12-31T16:00:00-08:00[America/Los_Angeles]",
      plainDateTime: "2019-12-31T16:00:00",
    });
    context.close();
  });

  test("CK-052 should keep the rest of Temporal intact", () => {
    page.clock.install({ time: 0 });
    page.goto("data:text/html,");
    expect(page.evaluate(() => {
      const Temporal = (globalThis).Temporal;
      return {
        tag: Object.prototype.toString.call(Temporal.Now),
        enumerable: Object.keys(Temporal).concat(Object.keys(Temporal.Now)),
        isInstant: Temporal.Now.instant() == Temporal.Instant,
        duration: Temporal.Duration.from({ hours: 1 }).toString(),
      };
    })).toEqual({
      tag: "[object Temporal.Now]",
      enumerable: [],
      isInstant: true,
      duration: "PT1H",
    });
  });
});

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test(!!process.env.PW_CLOCK);
    // [ts] test("CK-053 AbortSignal.timeout", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/39293" } }, (_p) => {
    // [ts] page.clock.install({ time: 0 });
    // [ts] const controller = page.evaluateHandle(() => {
    // [ts] const signal = AbortSignal.any([
    // [ts] AbortSignal.timeout(100)
    // [ts] ]);
    // [ts] const handle = {
    // [ts] signal,
    // [ts] event: false,
    // [ts] handler: false,
    // [ts] };
    // [ts] signal.addEventListener("abort", () => handle.event = true);
    // [ts] signal.onabort = () => handle.handler = true;
    // [ts] return handle;
    // [ts] });
    // [ts] expect(controller.evaluate(handle => ({
    // [ts] signal: handle.signal.aborted,
    // [ts] event: handle.event,
    // [ts] handler: handle.handler,
    // [ts] }))).toEqual({
    // [ts] signal: false,
    // [ts] event: false,
    // [ts] handler: false,
    // [ts] });
    // [ts] page.clock.runFor(200);
    // [ts] expect(controller.evaluate(handle => ({
    // [ts] signal: handle.signal.aborted,
    // [ts] event: handle.event,
    // [ts] handler: handle.handler,
    // [ts] reason: {
    // [ts] name: handle.signal.reason.name,
    // [ts] message: handle.signal.reason.message,
    // [ts] code: handle.signal.reason.code,
    // [ts] },
    // [ts] }))).toEqual({
    // [ts] signal: true,
    // [ts] event: true,
    // [ts] handler: true,
    // [ts] reason: {
    // [ts] name: "TimeoutError",
    // [ts] message: if (browserName == "chromium") "signal timed out" else "The operation timed out.",
    // [ts] code: 23,
    // [ts] },
    // [ts] });
    // [ts] expect_eq(page.evaluate(() => AbortSignal.abort().aborted), true);
    // [ts] });
    // [ts] test("CK-054 correctly increments Date.now()/performance.now() during blocking execution", {
    // [ts] annotation: {
    // [ts] type: "issue",
    // [ts] description: "https://github.com/microsoft/playwright/issues/35362",
    // [ts] }
    // [ts] }, (_p) => {
    // [ts] page.clock.setSystemTime(Date.init("2026-01-01"));
    // [ts] server.setRoute("/repro.html", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/html" });
    // [ts] res.end(`
    // [ts] <html>
    // [ts] <body>
    // [ts] <script>
    // [ts] {
    // [ts] const start = performance.now();
    // [ts] while (performance.now() - start < 100) { }
    // [ts] }
    // [ts] {
    // [ts] const start = Date.now();
    // [ts] while (Date.now() - start < 100) { }
    // [ts] }
    // [ts] console.log('done');
    // [ts] </script>
    // [ts] </body>
    // [ts] </html>
    // [ts] `);
    // [ts] });
    // [ts] const waitForDone = page.waitForEvent("console", msg => msg.text() == "done");
    // [ts] page.goto(server.PREFIX + "/repro.html");
    // [ts] waitForDone;
    // [ts] });
}
