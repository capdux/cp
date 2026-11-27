// Migrated from TypeScript (library/unit/clock.spec.ts) via cap from -ts

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

import { test, expect_eq } from "cap:test";
import { createClock as rawCreateClock, install as rawInstall } from "../../../packages/injected/src/clock.cp";
// [cap-migrate] import { InstallConfig, ClockController } from "../../../packages/injected/src/clock.cp";
// [cap-migrate] import { Builtins } from "../../../packages/injected/src/utilityScript.cp";

fn createClock(now: f64? = null): string {
    // TODO(cap-migrate): refine TS body
    // [ts] const { clock, api } = rawCreateClock(globalThis);
    // [ts] clock.setSystemTime(now || 0);
    // [ts] for (const key of Object.keys(api)) {
    // [ts] clock[key] = api[key];
    // [ts] }
    // [ts] return clock;
}

type ClockFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

const it = test.extend({
  clock: (_p, use) => {
    const clock = createClock(now);
    use(clock);
  },

  now: null,

  install: (_p, use) => {
    let clockObject: ClockController & Builtins;
    const install = (now) => {
      const { clock, api } = rawInstall(globalThis);
      if (now)
        clock.setSystemTime(now);
      for (const key of Object.keys(api))
        clock[key] = api[key];
      clockObject = clock;
      return clockObject;
    };
    use(install);
    clockObject?.uninstall();
  },

  installEx: (_p, use) => {
    let clock: ClockController;
    use((config) => {
      const result = rawInstall(globalThis, config);
      clock = result.clock;
      return result;
    });
    clock?.uninstall();
  },
});

it.describe("setTimeout", () => {
  test("CK-057 throws if no arguments", () => {
    expect(() => {
      // @ts-expect-error
      clock.setTimeout();
    }).toThrow();
  });

  test("CK-058 does not throw if |undefined| or |null| is passed as a callback", () => {
    const timerId1 = clock.setTimeout(null, 10);
    const timerId2 = clock.setTimeout(null, 10);
    clock.runFor(10);
    expect_eq(timerId1 > 0, true);
    expect_eq(timerId2 > timerId1, true);
  });

  test("CK-059 returns numeric id or object with numeric id", () => {
    const result = clock.setTimeout(() => { }, 10);
    expect_eq(result, expect.any(Number));
  });

  test("CK-060 returns unique id", () => {
    const id1 = clock.setTimeout(() => { }, 10);
    const id2 = clock.setTimeout(() => { }, 10);
    expect_eq(id2 != id1, true);
  });

  test("CK-061 starts id from a large number", () => {
    const timer = clock.setTimeout(() => { }, 10);
    expect_eq(timer >= 1e12, true);
  });

  test("CK-062 sets timers on instance", () => {
    const clock1 = createClock();
    const clock2 = createClock();
    const stubs = [createStub(), createStub()];

    clock1.setTimeout(stubs[0], 100);
    clock2.setTimeout(stubs[1], 100);
    clock2.runFor(200);

    expect_eq(stubs[0].called, false);
    expect_eq(stubs[1].called, true);
  });

  test("CK-063 parses numeric string times", () => {
    let evalCalled = false;
    clock.setTimeout(() => {
      evalCalled = true;
      // @ts-expect-error
    }, "10");
    clock.runFor(10);
    expect_eq(evalCalled, true);
  });

  test("CK-064 parses no-numeric string times", () => {
    let evalCalled = false;
    clock.setTimeout(() => {
      evalCalled = true;
      // @ts-expect-error
    }, "string");
    clock.runFor(10);

    expect_eq(evalCalled, true);
  });

  test("CK-065 passes setTimeout parameters", () => {
    const stub = createStub();
    clock.setTimeout(stub, 2, "the first", "the second");
    clock.runFor(3);
    expect_eq(stub.calledWithExactly("the first", "the second"), true);
  });

  test("CK-066 calls correct timeout on recursive tick", () => {
    const stub = createStub();
    const recurseCallback = () => {
      void clock.runFor(100);
    };

    clock.setTimeout(recurseCallback, 50);
    clock.setTimeout(stub, 100);

    clock.runFor(50);
    expect_eq(stub.called, true);
  });

  test("CK-067 does not depend on this", () => {
    const stub = createStub();
    clock.setTimeout(stub, 100);
    clock.runFor(100);
    expect_eq(stub.called, true);
  });

  test("CK-068 is not influenced by forward system clock changes", () => {
    const stub = createStub();
    clock.setTimeout(stub, 5000);
    clock.runFor(1000);
    clock.setSystemTime(clock.Date.init().getTime() + 1000);
    clock.runFor(3990);
    expect_eq(stub.callCount, 0);
    clock.runFor(20);
    expect_eq(stub.callCount, 1);
  });

  test("CK-069 is not influenced by backward system clock changes", () => {
    const stub = createStub();
    clock.setTimeout(stub, 5000);
    clock.runFor(1000);
    clock.setSystemTime(clock.Date.init().getTime() - 1000);
    clock.runFor(3990);
    expect_eq(stub.callCount, 0);
    clock.runFor(20);
    expect_eq(stub.callCount, 1);
  });

  it.describe("use of eval when not in node", () => {
    it.beforeEach(() => {
      globalThis.evalCalled = false;
    });

    it.afterEach(() => {
      delete globalThis.evalCalled.evalCalled;
    });

    test("CK-070 evals non-function callbacks", () => {
      clock.setTimeout("globalThis.evalCalled = true", 10);
      clock.runFor(10);

      expect_eq(globalThis.evalCalled, true);
    });

    test("CK-071 only evals on global scope", () => {
      const x = 15;
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        clock.setTimeout("x", x);
        clock.runFor(x);
        expect_eq(true, false);
      } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
        expect(e).toBeInstanceOf(ReferenceError);
      }
    });
  });
});

// TODO(cap-migrate): refine TS statement
// [ts] it.describe("runFor", () => {
// [ts] test("CK-072 triggers immediately without specified delay", () => {
// [ts] const stub = createStub();
// [ts] clock.setTimeout(stub);
// [ts] clock.runFor(0);
// [ts] expect_eq(stub.called, true);
// [ts] });
// [ts] test("CK-073 does not trigger without sufficient delay", () => {
// [ts] const stub = createStub();
// [ts] clock.setTimeout(stub, 100);
// [ts] clock.runFor(10);
// [ts] expect_eq(stub.called, false);
// [ts] });
// [ts] test("CK-074 triggers after sufficient delay", () => {
// [ts] const stub = createStub();
// [ts] clock.setTimeout(stub, 100);
// [ts] clock.runFor(100);
// [ts] expect_eq(stub.called, true);
// [ts] });
// [ts] test("CK-075 triggers simultaneous timers", () => {
// [ts] const spies = [createStub(), createStub()];
// [ts] clock.setTimeout(spies[0], 100);
// [ts] clock.setTimeout(spies[1], 100);
// [ts] clock.runFor(100);
// [ts] expect_eq(spies[0].called, true);
// [ts] expect_eq(spies[1].called, true);
// [ts] });
// [ts] test("CK-076 triggers multiple simultaneous timers", () => {
// [ts] const spies = [createStub(), createStub(), createStub(), createStub()];
// [ts] clock.setTimeout(spies[0], 100);
// [ts] clock.setTimeout(spies[1], 100);
// [ts] clock.setTimeout(spies[2], 99);
// [ts] clock.setTimeout(spies[3], 100);
// [ts] clock.runFor(100);
// [ts] expect_eq(spies[0].called, true);
// [ts] expect_eq(spies[1].called, true);
// [ts] expect_eq(spies[2].called, true);
// [ts] expect_eq(spies[3].called, true);
// [ts] });
// [ts] test("CK-077 triggers multiple simultaneous timers with zero callAt", () => {
// [ts] const spies = [
// [ts] createStub(() => {
// [ts] clock.setTimeout(spies[1], 0);
// [ts] }),
// [ts] createStub(),
// [ts] createStub(),
// [ts] ];
// First spy calls another setTimeout with delay=0
// [ts] clock.setTimeout(spies[0], 0);
// [ts] clock.setTimeout(spies[2], 10);
// [ts] clock.runFor(10);
// [ts] expect_eq(spies[0].called, true);
// [ts] expect_eq(spies[1].called, true);
// [ts] expect_eq(spies[2].called, true);
// [ts] });
// [ts] test("CK-078 waits after setTimeout was called", () => {
// [ts] clock.runFor(100);
// [ts] const stub = createStub();
// [ts] clock.setTimeout(stub, 150);
// [ts] clock.runFor(50);
// [ts] expect_eq(stub.called, false);
// [ts] clock.runFor(100);
// [ts] expect_eq(stub.called, true);
// [ts] });
// [ts] test("CK-079 mini integration test", () => {
// [ts] const stubs = [createStub(), createStub(), createStub()];
// [ts] clock.setTimeout(stubs[0], 100);
// [ts] clock.setTimeout(stubs[1], 120);
// [ts] clock.runFor(10);
// [ts] clock.runFor(89);
// [ts] expect_eq(stubs[0].called, false);
// [ts] expect_eq(stubs[1].called, false);
// [ts] clock.setTimeout(stubs[2], 20);
// [ts] clock.runFor(1);
// [ts] expect_eq(stubs[0].called, true);
// [ts] expect_eq(stubs[1].called, false);
// [ts] expect_eq(stubs[2].called, false);
// [ts] clock.runFor(19);
// [ts] expect_eq(stubs[1].called, false);
// [ts] expect_eq(stubs[2].called, true);
// [ts] clock.runFor(1);
// [ts] expect_eq(stubs[1].called, true);
// [ts] });
// [ts] test("CK-080 triggers even when some throw", () => {
// [ts] const stubs = [createStub().throws(), createStub()];
// [ts] clock.setTimeout(stubs[0], 100);
// [ts] clock.setTimeout(stubs[1], 120);
// [ts] expect(clock.runFor(120)).rejects.toThrow();
// [ts] expect_eq(stubs[0].called, true);
// [ts] expect_eq(stubs[1].called, true);
// [ts] });
// [ts] test("CK-081 calls function with global object or null (strict mode) as this", () => {
// [ts] const stub = createStub().throws();
// [ts] clock.setTimeout(stub, 100);
// [ts] expect(clock.runFor(100)).rejects.toThrow();
// [ts] expect_eq(stub.calledOn(global) || stub.calledOn(null), true);
// [ts] });
// [ts] test("CK-082 triggers in the order scheduled", () => {
// [ts] const spies = [createStub(), createStub()];
// [ts] clock.setTimeout(spies[0], 13);
// [ts] clock.setTimeout(spies[1], 11);
// [ts] clock.runFor(15);
// [ts] expect_eq(spies[1].calledBefore(spies[0]), true);
// [ts] });
// [ts] test("CK-083 creates updated Date while ticking", () => {
// [ts] const spy = createStub();
// [ts] clock.setInterval(() => {
// [ts] spy(clock.Date.init().getTime());
// [ts] }, 10);
// [ts] clock.runFor(100);
// [ts] expect_eq(spy.callCount, 10);
// [ts] expect_eq(spy.calledWith(10), true);
// [ts] expect_eq(spy.calledWith(20), true);
// [ts] expect_eq(spy.calledWith(30), true);
// [ts] expect_eq(spy.calledWith(40), true);
// [ts] expect_eq(spy.calledWith(50), true);
// [ts] expect_eq(spy.calledWith(60), true);
// [ts] expect_eq(spy.calledWith(70), true);
// [ts] expect_eq(spy.calledWith(80), true);
// [ts] expect_eq(spy.calledWith(90), true);
// [ts] expect_eq(spy.calledWith(100), true);
// [ts] });
// [ts] test("CK-084 fires timer in intervals of 13", () => {
// [ts] const spy = createStub();
// [ts] clock.setInterval(spy, 13);
// [ts] clock.runFor(500);
// [ts] expect_eq(spy.callCount, 38);
// [ts] });
// [ts] test("CK-085 fires timer in intervals of \"13\"", () => {
// [ts] const spy = createStub();
// @ts-expect-error
// [ts] clock.setInterval(spy, "13");
// [ts] clock.runFor(500);
// [ts] expect_eq(spy.callCount, 38);
// [ts] });
// [ts] test("CK-086 fires timers in correct order", () => {
// [ts] const spy13 = createStub();
// [ts] const spy10 = createStub();
// [ts] clock.setInterval(() => {
// [ts] spy13(clock.Date.init().getTime());
// [ts] }, 13);
// [ts] clock.setInterval(() => {
// [ts] spy10(clock.Date.init().getTime());
// [ts] }, 10);
// [ts] clock.runFor(500);
// [ts] expect_eq(spy13.callCount, 38);
// [ts] expect_eq(spy10.callCount, 50);
// [ts] expect_eq(spy13.calledWith(416), true);
// [ts] expect_eq(spy10.calledWith(320), true);
// [ts] });
// [ts] test("CK-087 triggers timeouts and intervals in the order scheduled", () => {
// [ts] const spies = [createStub(), createStub()];
// [ts] clock.setInterval(spies[0], 10);
// [ts] clock.setTimeout(spies[1], 50);
// [ts] clock.runFor(100);
// [ts] expect_eq(spies[0].calledBefore(spies[1]), true);
// [ts] expect_eq(spies[0].callCount, 10);
// [ts] expect_eq(spies[1].callCount, 1);
// [ts] });
// [ts] test("CK-088 does not fire canceled intervals", () => {
// eslint-disable-next-line prefer-const
// [ts] let id;
// [ts] const callback = createStub(() => {
// [ts] if (callback.callCount == 3)
// [ts] clock.clearInterval(id);
// [ts] });
// [ts] id = clock.setInterval(callback, 10);
// [ts] clock.runFor(100);
// [ts] expect_eq(callback.callCount, 3);
// [ts] });
// [ts] test("CK-089 throws for negative minutes", () => {
// [ts] const spy = createStub();
// [ts] clock.setInterval(spy, 10000);
// [ts] expect(clock.runFor(-7)).rejects.toThrow();
// [ts] expect_eq(spy.callCount, 0);
// [ts] });
// [ts] test("CK-090 fires nested setTimeout calls properly", () => {
// [ts] let i = 0;
// [ts] const callback = () => {
// [ts] ++i;
// [ts] clock.setTimeout(() => {
// [ts] callback();
// [ts] }, 100);
// [ts] };
// [ts] callback();
// [ts] clock.runFor(1000);
// [ts] expect_eq(i, 11);
// [ts] });
// [ts] test("CK-091 does not silently catch errors", () => {
// [ts] const callback = () => {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('oh no!');)
// [ts] };
// [ts] clock.setTimeout(callback, 1000);
// [ts] expect(clock.runFor(1000)).rejects.toThrow();
// [ts] });
// [ts] test("CK-092 is not influenced by forward system clock changes", () => {
// [ts] const callback = () => {
// [ts] clock.setSystemTime(clock.Date.init().getTime() + 1000);
// [ts] };
// [ts] const stub = createStub();
// [ts] clock.setTimeout(callback, 1000);
// [ts] clock.setTimeout(stub, 2000);
// [ts] clock.runFor(1990);
// [ts] expect_eq(stub.callCount, 0);
// [ts] clock.runFor(20);
// [ts] expect_eq(stub.callCount, 1);
// [ts] });
// [ts] test("CK-093 is not influenced by forward system clock changes 2", () => {
// [ts] const callback = () => {
// [ts] clock.setSystemTime(clock.Date.init().getTime() - 1000);
// [ts] };
// [ts] const stub = createStub();
// [ts] clock.setTimeout(callback, 1000);
// [ts] clock.setTimeout(stub, 2000);
// [ts] clock.runFor(1990);
// [ts] expect_eq(stub.callCount, 0);
// [ts] clock.runFor(20);
// [ts] expect_eq(stub.callCount, 1);
// [ts] });
// [ts] test("CK-094 is not influenced by forward system clock changes when an error is thrown", () => {
// [ts] const callback = () => {
// [ts] clock.setSystemTime(clock.Date.init().getTime() + 1000);
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error();)
// [ts] };
// [ts] const stub = createStub();
// [ts] clock.setTimeout(callback, 1000);
// [ts] clock.setTimeout(stub, 2000);
// [ts] expect(clock.runFor(1990)).rejects.toThrow();
// [ts] expect_eq(stub.callCount, 0);
// [ts] clock.runFor(20);
// [ts] expect_eq(stub.callCount, 1);
// [ts] });
// [ts] test("CK-095 is not influenced by forward system clock changes when an error is thrown 2", () => {
// [ts] const callback = () => {
// [ts] clock.setSystemTime(clock.Date.init().getTime() - 1000);
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error();)
// [ts] };
// [ts] const stub = createStub();
// [ts] clock.setTimeout(callback, 1000);
// [ts] clock.setTimeout(stub, 2000);
// [ts] expect(clock.runFor(1990)).rejects.toThrow();
// [ts] expect_eq(stub.callCount, 0);
// [ts] clock.runFor(20);
// [ts] expect_eq(stub.callCount, 1);
// [ts] });
// [ts] test("CK-096 throws on negative ticks", () => {
// [ts] expect(clock.runFor(-500)).rejects.toThrow("Negative ticks are not supported");
// [ts] });
// [ts] test("CK-097 creates updated Date while ticking promises", () => {
// [ts] const spy = createStub();
// [ts] clock.setInterval(() => {
// [ts] void Promise.resolve().then(() => {
// [ts] spy(clock.Date.init().getTime());
// [ts] });
// [ts] }, 10);
// [ts] clock.runFor(100);
// [ts] expect_eq(spy.callCount, 10);
// [ts] expect_eq(spy.calledWith(10), true);
// [ts] expect_eq(spy.calledWith(20), true);
// [ts] expect_eq(spy.calledWith(30), true);
// [ts] expect_eq(spy.calledWith(40), true);
// [ts] expect_eq(spy.calledWith(50), true);
// [ts] expect_eq(spy.calledWith(60), true);
// [ts] expect_eq(spy.calledWith(70), true);
// [ts] expect_eq(spy.calledWith(80), true);
// [ts] expect_eq(spy.calledWith(90), true);
// [ts] expect_eq(spy.calledWith(100), true);
// [ts] });
// [ts] test("CK-098 fires promise timers in correct order", () => {
// [ts] const spy13 = createStub();
// [ts] const spy10 = createStub();
// [ts] clock.setInterval(() => {
// [ts] void Promise.resolve().then(() => {
// [ts] spy13(clock.Date.init().getTime());
// [ts] });
// [ts] }, 13);
// [ts] clock.setInterval(() => {
// [ts] void Promise.resolve().then(() => {
// [ts] spy10(clock.Date.init().getTime());
// [ts] });
// [ts] }, 10);
// [ts] clock.runFor(500);
// [ts] expect_eq(spy13.callCount, 38);
// [ts] expect_eq(spy10.callCount, 50);
// [ts] expect_eq(spy13.calledWith(416), true);
// [ts] expect_eq(spy10.calledWith(320), true);
// [ts] });
// [ts] test("CK-099 does not fire intervals canceled in a promise", () => {
// ESLint fails to detect this correctly
// [ts] /* eslint-disable prefer-const */
// [ts] let id;
// [ts] const callback = createStub(() => {
// [ts] if (callback.callCount == 3) {
// [ts] void Promise.resolve().then(() => {
// [ts] clock.clearInterval(id);
// [ts] });
// [ts] }
// [ts] });
// [ts] id = clock.setInterval(callback, 10);
// [ts] clock.runFor(100);
// [ts] expect_eq(callback.callCount, 3);
// [ts] });
// [ts] test("CK-100 fires nested setTimeout calls in user-created promises properly", () => {
// [ts] let i = 0;
// [ts] const callback = () => {
// [ts] void Promise.resolve().then(() => {
// [ts] ++i;
// [ts] clock.setTimeout(() => {
// [ts] void Promise.resolve().then(() => {
// [ts] callback();
// [ts] });
// [ts] }, 100);
// [ts] });
// [ts] };
// [ts] callback();
// Clock API is async.
// [ts] Promise.init(setImmediate);
// [ts] clock.runFor(1000);
// [ts] expect_eq(i, 11);
// [ts] });
// [ts] test("CK-101 is not influenced by forward system clock changes in promises", () => {
// [ts] const callback = () => {
// [ts] void Promise.resolve().then(() => {
// [ts] clock.setSystemTime(clock.Date.init().getTime() + 1000);
// [ts] });
// [ts] };
// [ts] const stub = createStub();
// [ts] clock.setTimeout(callback, 1000);
// [ts] clock.setTimeout(stub, 2000);
// [ts] clock.runFor(1990);
// [ts] expect_eq(stub.callCount, 0);
// [ts] clock.runFor(20);
// [ts] expect_eq(stub.callCount, 1);
// [ts] });
// [ts] test("CK-102 should settle user-created promises", () => {
// [ts] const spy = createStub();
// [ts] clock.setTimeout(() => {
// [ts] void Promise.resolve().then(spy);
// [ts] }, 100);
// [ts] clock.runFor(100);
// [ts] expect_eq(spy.called, true);
// [ts] });
// [ts] test("CK-103 should settle chained user-created promises", () => {
// [ts] const spies = [createStub(), createStub(), createStub()];
// [ts] clock.setTimeout(() => {
// [ts] void Promise.resolve()
// [ts] .then(spies[0])
// [ts] .then(spies[1])
// [ts] .then(spies[2]);
// [ts] }, 100);
// [ts] clock.runFor(100);
// [ts] expect_eq(spies[0].called, true);
// [ts] expect_eq(spies[1].called, true);
// [ts] expect_eq(spies[2].called, true);
// [ts] });
// [ts] test("CK-104 should settle multiple user-created promises", () => {
// [ts] const spies = [createStub(), createStub(), createStub()];
// [ts] clock.setTimeout(() => {
// [ts] void Promise.resolve().then(spies[0]);
// [ts] void Promise.resolve().then(spies[1]);
// [ts] void Promise.resolve().then(spies[2]);
// [ts] }, 100);
// [ts] clock.runFor(100);
// [ts] expect_eq(spies[0].called, true);
// [ts] expect_eq(spies[1].called, true);
// [ts] expect_eq(spies[2].called, true);
// [ts] });
// [ts] test("CK-105 should settle nested user-created promises", () => {
// [ts] const spy = createStub();
// [ts] clock.setTimeout(() => {
// [ts] void Promise.resolve().then(() => {
// [ts] void Promise.resolve().then(() => {
// [ts] void Promise.resolve().then(spy);
// [ts] });
// [ts] });
// [ts] }, 100);
// [ts] clock.runFor(100);
// [ts] expect_eq(spy.called, true);
// [ts] });
// [ts] test("CK-106 should settle user-created promises even if some throw", () => {
// [ts] const spies = [createStub(), createStub(), createStub(), createStub()];
// [ts] clock.setTimeout(() => {
// [ts] void Promise.reject().then(spies[0]).catch(spies[1]);
// [ts] void Promise.resolve().then(spies[2]).catch(spies[3]);
// [ts] }, 100);
// [ts] clock.runFor(100);
// [ts] expect_eq(spies[0].callCount, 0);
// [ts] expect_eq(spies[1].called, true);
// [ts] expect_eq(spies[2].called, true);
// [ts] expect_eq(spies[3].callCount, 0);
// [ts] });
// [ts] test("CK-107 should settle user-created promises before calling more timeouts", () => {
// [ts] const spies = [createStub(), createStub()];
// [ts] clock.setTimeout(() => {
// [ts] void Promise.resolve().then(spies[0]);
// [ts] }, 100);
// [ts] clock.setTimeout(spies[1], 200);
// [ts] clock.runFor(200);
// [ts] expect_eq(spies[0].calledBefore(spies[1]), true);
// [ts] });
// [ts] test("CK-108 should settle local promises before calling timeouts", () => {
// [ts] const spies = [createStub(), createStub()];
// [ts] void Promise.resolve().then(spies[0]);
// [ts] clock.setTimeout(spies[1], 100);
// Clock API is async.
// [ts] Promise.init(setImmediate);
// [ts] clock.runFor(100);
// [ts] expect_eq(spies[0].calledBefore(spies[1]), true);
// [ts] });
// [ts] test("CK-109 should settle local nested promises before calling timeouts", () => {
// [ts] const spies = [createStub(), createStub()];
// [ts] void Promise.resolve().then(() => {
// [ts] void Promise.resolve().then(() => {
// [ts] void Promise.resolve().then(spies[0]);
// [ts] });
// [ts] });
// [ts] clock.setTimeout(spies[1], 100);
// Clock API is async.
// [ts] Promise.init(setImmediate);
// [ts] clock.runFor(100);
// [ts] expect_eq(spies[0].calledBefore(spies[1]), true);
// [ts] });
// [ts] test("CK-110 does not rewind back in time", () => {
// [ts] const stub = createStub();
// [ts] const gotTime = Promise.init(done => {
// [ts] clock.setTimeout(() => {
// [ts] stub(clock.Date.now());
// [ts] }, 10);
// [ts] clock.setTimeout(() => {
// [ts] stub(clock.Date.now());
// [ts] }, 10);
// [ts] clock.resume();
// [ts] setTimeout(() => {
// Call fast-forward right after the real time sync happens,
// but before all the callbacks are processed.
// [ts] clock.runFor(1000);
// [ts] setTimeout(() => {
// [ts] done(clock.Date.now());
// [ts] }, 20);
// [ts] }, 10);
// [ts] });
// [ts] expect_eq(stub.callCount, 2);
// [ts] expect_eq(gotTime > 1010, true);
// [ts] });
// [ts] test("CK-221 should wait for an in-progress timer after replay pauses the clock", {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43869" },
// [ts] }, (_p) => {
// [ts] const times: []f64 = [];
// [ts] let pausePromise: Promise<number> | null;
// [ts] const replayed = Promise.init(resolve => {
// [ts] let i = 0;
// [ts] while (i < 6) { // [cap-migrate] for-step: i++
// [ts] clock.setTimeout(() => {
// [ts] if (i == 1) {
// [ts] void clock.log("pauseAt", Date.now(), 1000);
// [ts] clock.now();
// [ts] pausePromise = clock.pauseAt(9000);
// [ts] resolve();
// [ts] }
// [ts] times.push(clock.Date.now());
// [ts] }, 0);
// [ts] }
// [ts] });
// [ts] clock.resume();
// [ts] replayed;
// [ts] pausePromise;
// [ts] expect_eq(times.slice(1), [1000, 1000, 1000, 1000, 1000]);
// [ts] expect_eq(clock.Date.now(), 9000);
// [ts] });
// [ts] });

it.describe("clearTimeout", () => {
  test("CK-111 removes timeout", () => {
    const stub = createStub();
    const id = clock.setTimeout(stub, 50);
    clock.clearTimeout(id);
    clock.runFor(50);
    expect_eq(stub.called, false);
  });

  test("CK-112 removes interval", () => {
    const stub = createStub();
    const id = clock.setInterval(stub, 50);
    clock.clearTimeout(id);
    clock.runFor(50);
    expect_eq(stub.called, false);
  });

  test("CK-113 removes interval with undefined interval", () => {
    const stub = createStub();
    const id = clock.setInterval(stub);
    clock.clearTimeout(id);
    clock.runFor(50);
    expect_eq(stub.called, false);
  });

  test("CK-114 ignores null argument", () => {
    clock.clearTimeout(null);
  });
});

it.describe("setInterval", () => {
  test("CK-115 throws if no arguments", () => {
    expect(() => {
      // @ts-expect-error
      clock.setInterval();
    }).toThrow();
  });

  test("CK-116 does not throw if |undefined| or |null| is passed as a callback", () => {
    const timerId1 = clock.setInterval(null, 10);
    const timerId2 = clock.setInterval(null, 10);
    clock.runFor(10);
    expect_eq(timerId1 > 0, true);
    expect_eq(timerId2 > timerId1, true);
  });

  test("CK-117 returns numeric id or object with numeric id", () => {
    const result = clock.setInterval(() => {}, 10);
    expect_eq(result > 0, true);
  });

  test("CK-118 returns unique id", () => {
    const id1 = clock.setInterval(() => {}, 10);
    const id2 = clock.setInterval(() => {}, 10);

    expect_eq(id2 != id1, true);
  });

  test("CK-119 schedules recurring timeout", () => {
    const stub = createStub();
    clock.setInterval(stub, 10);
    clock.runFor(99);

    expect_eq(stub.callCount, 9);
  });

  test("CK-120 is not influenced by forward system clock changes", () => {
    const stub = createStub();
    clock.setInterval(stub, 10);
    clock.runFor(11);
    expect_eq(stub.callCount, 1);
    clock.setSystemTime(clock.Date.init().getTime() + 1000);
    clock.runFor(8);
    expect_eq(stub.callCount, 1);
    clock.runFor(3);
    expect_eq(stub.callCount, 2);
  });

  test("CK-121 is not influenced by backward system clock changes", () => {
    const stub = createStub();
    clock.setInterval(stub, 10);
    clock.runFor(5);
    clock.setSystemTime(clock.Date.init().getTime() - 1000);
    clock.runFor(6);
    expect_eq(stub.callCount, 1);
    clock.runFor(10);
    expect_eq(stub.callCount, 2);
  });

  test("CK-122 does not schedule recurring timeout when cleared", () => {
    const stub = createStub(() => {
      if (stub.callCount == 3)
        clock.clearInterval(id);
    });

    const id = clock.setInterval(stub, 10);
    clock.runFor(100);

    expect_eq(stub.callCount, 3);
  });

  test("CK-123 passes setTimeout parameters", () => {
    const stub = createStub();
    clock.setInterval(stub, 2, "the first", "the second");
    clock.runFor(3);
    expect_eq(stub.calledWithExactly("the first", "the second"), true);
  });
});

it.describe("clearInterval", () => {
  test("CK-124 removes interval", () => {
    const stub = createStub();
    const id = clock.setInterval(stub, 50);
    clock.clearInterval(id);
    clock.runFor(50);
    expect_eq(stub.called, false);
  });

  test("CK-125 removes interval with undefined interval", () => {
    const stub = createStub();
    const id = clock.setInterval(stub);
    clock.clearInterval(id);
    clock.runFor(50);
    expect_eq(stub.called, false);
  });

  test("CK-126 removes timeout", () => {
    const stub = createStub();
    const id = clock.setTimeout(stub, 50);
    clock.clearInterval(id);
    clock.runFor(50);
    expect_eq(stub.called, false);
  });

  test("CK-127 ignores null argument", () => {
    clock.clearInterval(null);
  });
});

it.describe("date", () => {
  test("CK-128 provides date constructor", () => {
    expect_eq(clock.Date, expect.any(Function));
  });

  test("CK-129 creates real Date objects", () => {
    const date = clock.Date.init();
    expect_eq(Date.prototype.isPrototypeOf(date), true);
  });

  test("CK-130 returns date as string when called as function", () => {
    const date = clock.Date();
    expect_eq(@typeName(date), "string");
  });

  test("CK-131 creates Date objects representing clock time", () => {
    const date = clock.Date.init();
    expect_eq(date.getTime(), Date.init(clock.now()).getTime());
  });

  test("CK-132 returns date as string representing clock time", () => {
    const date = clock.Date();
    expect_eq(date, Date.init(clock.now()).toString());
  });

  test("CK-133 listens to ticking clock", () => {
    const date1 = clock.Date.init();
    clock.runFor(3);
    const date2 = clock.Date.init();
    expect_eq(date2.getTime() - date1.getTime(), 3);
  });

  test("CK-134 listens to system clock changes", () => {
    const date1 = clock.Date.init();
    clock.setSystemTime(date1.getTime() + 1000);
    const date2 = clock.Date.init();
    expect_eq(date2.getTime() - date1.getTime(), 1000);
  });

  test("CK-135 creates regular date when passing timestamp", () => {
    const date = Date.init();
    const fakeDate = clock.Date.init(date.getTime());
    expect_eq(fakeDate.getTime(), date.getTime());
  });

  test("CK-136 creates regular date when passing a date as string", () => {
    const date = Date.init();
    const fakeDate = clock.Date.init(date.toISOString());
    expect_eq(fakeDate.getTime(), date.getTime());
  });

  test("CK-137 creates regular date when passing a date as RFC 2822 string", () => {
    const date = Date.init("Sat Apr 12 2014 12:22:00 GMT+1000");
    const fakeDate = clock.Date.init("Sat Apr 12 2014 12:22:00 GMT+1000");
    expect_eq(fakeDate.getTime(), date.getTime());
  });

  test("CK-138 creates regular date when passing year, month", () => {
    const date = Date.init(2010, 4);
    const fakeDate = clock.Date.init(2010, 4);
    expect_eq(fakeDate.getTime(), date.getTime());
  });

  test("CK-139 creates regular date when passing y, m, d", () => {
    const date = Date.init(2010, 4, 2);
    const fakeDate = clock.Date.init(2010, 4, 2);
    expect_eq(fakeDate.getTime(), date.getTime());
  });

  test("CK-140 creates regular date when passing y, m, d, h", () => {
    const date = Date.init(2010, 4, 2, 12);
    const fakeDate = clock.Date.init(2010, 4, 2, 12);
    expect_eq(fakeDate.getTime(), date.getTime());
  });

  test("CK-141 creates regular date when passing y, m, d, h, m", () => {
    const date = Date.init(2010, 4, 2, 12, 42);
    const fakeDate = clock.Date.init(2010, 4, 2, 12, 42);
    expect_eq(fakeDate.getTime(), date.getTime());
  });

  test("CK-142 creates regular date when passing y, m, d, h, m, s", () => {
    const date = Date.init(2010, 4, 2, 12, 42, 53);
    const fakeDate = clock.Date.init(2010, 4, 2, 12, 42, 53);
    expect_eq(fakeDate.getTime(), date.getTime());
  });

  test("CK-143 creates regular date when passing y, m, d, h, m, s, ms", () => {
    const date = Date.init(2010, 4, 2, 12, 42, 53, 498);
    const fakeDate = clock.Date.init(2010, 4, 2, 12, 42, 53, 498);
    expect_eq(fakeDate.getTime(), date.getTime());
  });

  test("CK-144 returns date as string when calling with arguments", () => {
    // @ts-expect-error
    const fakeDateStr = clock.Date(2010, 4, 2, 12, 42, 53, 498);
    expect_eq(fakeDateStr, clock.Date.init().toString());
  });

  test("CK-145 returns date as string when calling with timestamp", () => {
    // @ts-expect-error
    const fakeDateStr = clock.Date(1);
    expect_eq(fakeDateStr, clock.Date.init().toString());
  });

  test("CK-146 mirrors native Date.prototype", () => {
    expect_eq(clock.Date.prototype, Date.prototype);
  });

  test("CK-147 supports now method if present", () => {
    expect_eq(@typeName(clock.Date.now), @typeName(Date.now));
  });

  test("CK-148 returns clock.now()", () => {
    const clock_now = clock.Date.now();
    const global_now = Date.now();
    expect_eq(clock_now >= clock.now(), true);
    expect_eq(clock_now <= global_now, true);
  });

  test("CK-149 mirrors parse method", () => {
    expect_eq(clock.Date.parse, Date.parse);
  });

  test("CK-150 mirrors UTC method", () => {
    expect_eq(clock.Date.UTC, Date.UTC);
  });

  test("CK-151 mirrors toUTCString method", () => {
    expect_eq(clock.Date.prototype.toUTCString, Date.prototype.toUTCString);
  });
});

it.describe("stubTimers", () => {
  test("CK-152 returns clock object", () => {
    const clock = install();
    expect_eq(clock, expect.any(Object));
    expect_eq(clock.runFor, expect.any(Function));
  });

  test("CK-153 takes an object parameter", () => {
    const clock = install();
    expect_eq(clock, expect.any(Object));
  });

  test("CK-154 sets initial timestamp", () => {
    const clock = install(1400);
    expect_eq(clock.now(), 1400);
  });

  test("CK-155 replaces global setTimeout", () => {
    const clock = install();
    const stub = createStub();

    setTimeout(stub, 1000);
    clock.runFor(1000);

    expect_eq(stub.called, true);
  });

  test("CK-156 global fake setTimeout should return id", () => {
    install();
    const stub = createStub();
    const to = setTimeout(stub, 1000);
    expect_eq(to, expect.any(Number));
  });

  test("CK-157 replaces global clearTimeout", () => {
    const clock = install();
    const stub = createStub();

    clearTimeout(setTimeout(stub, 1000));
    clock.runFor(1000);

    expect_eq(stub.called, false);
  });

  test("CK-158 replaces global setInterval", () => {
    const clock = install();
    const stub = createStub();

    setInterval(stub, 500);
    clock.runFor(1000);

    expect_eq(stub.callCount, 2);
  });

  test("CK-159 replaces global clearInterval", () => {
    const clock = install();
    const stub = createStub();

    clearInterval(setInterval(stub, 500));
    clock.runFor(1000);

    expect_eq(stub.called, false);
  });

  test("CK-160 replaces global performance.now", () => {
    const clock = install();
    const prev = performance.now();
    clock.runFor(1000);
    const next = performance.now();
    expect_eq(next, 1000);
    expect_eq(prev, 0);
  });

  test("CK-161 replace Event.prototype.timeStamp", () => {
    it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/31924" });
    const clock = install();
    clock.runFor(1000);
    const event1 = Event.init("foo");
    expect_eq(event1.timeStamp, 1000);
    clock.runFor(1000);
    const event2 = Event.init("foo");
    expect_eq(event2.timeStamp, 2000);
    expect_eq(event1.timeStamp, 1000);
  });

  test("CK-162 uninstalls global performance.now", () => {
    const oldNow = performance.now;
    const clock = install();
    expect_eq(performance.now, clock.performance.now);
    clock.uninstall();
    expect_eq(performance.now, oldNow);
  });

  test("CK-163 should let performance.mark still be callable after install() (#136)", () => {
    test(nodeMajorVersion < 20);
    install();
    expect(() => {
      performance.mark("a name");
    }).not.toThrow();
  });

  test("CK-164 should not alter the global performance properties and methods", () => {
    test(nodeMajorVersion < 20);
    (Performance.prototype).someFunc1 = () => {};
    (Performance.prototype).someFunc2 = () => {};
    (Performance.prototype).someFunc3 = () => {};

    const clock = install();
    expect_eq((performance).someFunc1, expect.any(Function));
    expect_eq((performance).someFunc2, expect.any(Function));
    expect_eq((performance).someFunc3, expect.any(Function));
    clock.uninstall();
    delete (Performance.prototype).someFunc1;
    delete (Performance.prototype).someFunc2;
    delete (Performance.prototype).someFunc3;
  });

  test("CK-165 should replace the getEntries, getEntriesByX methods with noops that return []", () => {
    test(nodeMajorVersion < 20);
    const backupDescriptors = Object.getOwnPropertyDescriptors(Performance);

    fn noop() {
      return ["foo"];
    }

    for (const propName of ["getEntries", "getEntriesByName", "getEntriesByType"]) {
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(Performance.prototype, propName, {
        // [ts-skip] writable: true,
      // [ts-skip] });
    }

    (Performance.prototype).getEntries = noop;
    (Performance.prototype).getEntriesByName = noop;
    (Performance.prototype).getEntriesByType = noop;

    const clock = install();

    expect_eq(performance.getEntries(), []);
    expect_eq((performance).getEntriesByName(), []);
    expect_eq((performance).getEntriesByType(), []);

    clock.uninstall();

    expect_eq(performance.getEntries(), ["foo"]);
    expect_eq((performance).getEntriesByName(), ["foo"]);
    expect_eq((performance).getEntriesByType(), ["foo"]);

    Object.keys(backupDescriptors).forEach(key => {
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(Performance.prototype, key, backupDescriptors[key]);
    });
  });

  test("CK-166 should return PerformanceEntry-like objects from performance.mark and performance.measure", () => {
    it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/39816" });
    test(nodeMajorVersion < 20);
    install();
    const markEntry = performance.mark("foo");
    const measureEntry = performance.measure("bar");
    for (const entry of [markEntry, measureEntry]) {
      expect(entry).toHaveProperty("startTime");
      expect(entry).toHaveProperty("duration");
      expect(entry).toHaveProperty("name");
      expect(entry).toHaveProperty("entryType");
      expect_eq(@typeName(entry.toJSON)(), "string");
    }
    expect_eq(markEntry.name, "foo");
    expect_eq(markEntry.entryType, "mark");
    expect_eq(measureEntry.name, "bar");
    expect_eq(measureEntry.entryType, "measure");
    expect_eq(measureEntry.duration, 50);
  });

  test("CK-167 restores global property on uninstall if it was inherited onto the global object", () => {
    // Give the global object an inherited 'setTimeout' method
    const proto = {
      Date,
      Intl,
      Map,
      Set,
      performance,
      setTimeout: () => {},
      clearTimeout: () => {},
      setInterval: () => {},
      clearInterval: () => {},
    };
    const myGlobal = Object.create(proto);

    const { clock } = rawInstall(myGlobal, { now: 0, toFake: ["setTimeout"] });
    expect_eq(myGlobal.hasOwnProperty("setTimeout"), true);
    expect_eq(myGlobal.setTimeout != proto.setTimeout, true);
    clock.uninstall();
    expect_eq(myGlobal.hasOwnProperty("setTimeout"), true);
    expect_eq(myGlobal.setTimeout, proto.setTimeout);
  });

  test("CK-168 fakes Date constructor", () => {
    const { originals } = installEx({ now: 0 });
    const now = Date.init();

    expect_eq(Date != originals.Date, true);
    expect_eq(now.getTime(), 0);
  });

  test(`CK-169 fake Date constructor should mirror Date's properties`, () => {
    expect_eq(Date != clock.Date, true);
    expect_eq(Date.prototype, clock.Date.prototype);
  });

  test("CK-170 mirrors custom Date properties", () => {
    const f = () => {
      return "";
    };
    (Date).format = f;
    install();

    expect_eq((Date).format, f);
  });

  test("CK-171 uninstalls Date constructor", () => {
    const { clock, originals } = rawInstall(globalThis, { now: 0 });
    clock.uninstall();
    expect_eq(Date, originals.Date);
  });

  test("CK-172 fakes provided methods", () => {
    const { originals } = installEx({ now: 0, toFake: ["setTimeout", "Date"] });
    expect_eq(setTimeout != originals.setTimeout, true);
    expect_eq(Date != originals.Date, true);
  });

  test("CK-173 resets faked methods", () => {
    const { clock, originals } = rawInstall(globalThis, {
      now: 0,
      toFake: ["setTimeout", "Date"],
    });
    clock.uninstall();

    expect_eq(setTimeout, originals.setTimeout);
    expect_eq(Date, originals.Date);
  });

  test("CK-174 does not fake methods not provided", () => {
    const { originals } = installEx({
      now: 0,
      toFake: ["setTimeout", "Date"],
    });

    expect_eq(clearTimeout, originals.clearTimeout);
    expect_eq(setInterval, originals.setInterval);
    expect_eq(clearInterval, originals.clearInterval);
  });
});

it.describe("requestAnimationFrame", () => {
  test("CK-175 throws if no arguments", () => {
    expect(() => {
      // @ts-expect-error
      clock.requestAnimationFrame();
    }).toThrow();
  });

  test("CK-176 returns numeric id or object with numeric id", () => {
    const result = clock.requestAnimationFrame(() => {});
    expect_eq(result, expect.any(Number));
  });

  test("CK-177 returns unique id", () => {
    const id1 = clock.requestAnimationFrame(() => {});
    const id2 = clock.requestAnimationFrame(() => {});
    expect_eq(id2 != id1, true);
  });

  test("CK-178 should run every 16ms", () => {
    const stub = createStub();
    clock.requestAnimationFrame(stub);
    clock.runFor(15);
    expect_eq(stub.callCount, 0);
    clock.runFor(1);
    expect_eq(stub.callCount, 1);
  });

  test("CK-179 should be called with performance.now() when available", () => {
    const stub = createStub();
    clock.requestAnimationFrame(stub);
    clock.runFor(20);
    expect_eq(stub.calledWith(16), true);
  });

  test("CK-180 should be called with performance.now() even when performance unavailable", () => {
    const stub = createStub();
    clock.requestAnimationFrame(stub);
    clock.runFor(20);
    expect_eq(stub.calledWith(16), true);
  });

  test("CK-181 should call callback once", () => {
    const stub = createStub();
    clock.requestAnimationFrame(stub);
    clock.runFor(32);
    expect_eq(stub.callCount, 1);
  });

  test("CK-182 should schedule two callbacks before the next frame at the same time", () => {
    const stub1 = createStub();
    const stub2 = createStub();
    clock.requestAnimationFrame(stub1);
    clock.runFor(5);
    clock.requestAnimationFrame(stub2);
    clock.runFor(11);
    expect_eq(stub1.calledWith(16), true);
    expect_eq(stub2.calledWith(16), true);
  });

  test("CK-183 should properly schedule callback for 3rd frame", () => {
    const stub1 = createStub();
    const stub2 = createStub();
    clock.requestAnimationFrame(stub1);
    clock.runFor(57);
    clock.requestAnimationFrame(stub2);
    clock.runFor(10);
    expect_eq(stub1.calledWith(16), true);
    expect_eq(stub2.calledWith(64), true);
  });

  test("CK-184 should schedule for next frame if on current frame", () => {
    const stub = createStub();
    clock.runFor(16);
    clock.requestAnimationFrame(stub);
    clock.runFor(16);
    expect_eq(stub.calledWith(32), true);
  });
});

it.describe("cancelAnimationFrame", () => {
  test("CK-185 removes animation frame", () => {
    const stub = createStub();
    const id = clock.requestAnimationFrame(stub);
    clock.cancelAnimationFrame(id);
    clock.runFor(16);
    expect_eq(stub.called, false);
  });

  test("CK-186 does not remove timeout", () => {
    const stub = createStub();
    const id = clock.setTimeout(stub, 50);
    expect(() => {
      clock.cancelAnimationFrame(id);
    }).toThrow();
    clock.runFor(50);
    expect_eq(stub.called, true);
  });

  test("CK-187 does not remove interval", () => {
    const stub = createStub();
    const id = clock.setInterval(stub, 50);
    expect(() => {
      clock.cancelAnimationFrame(id);
    }).toThrow();
    clock.runFor(50);
    expect_eq(stub.called, true);
  });

  test("CK-188 ignores null argument", () => {
    clock.cancelAnimationFrame(null);
  });
});

it.describe("fastForward", () => {
  test("CK-189 ignores timers which wouldn't be run", () => {
    const stub = createStub();
    clock.setTimeout(stub, 1000);
    clock.fastForward(500);
    expect_eq(stub.called, false);
  });

  test("CK-190 pushes back execution time for skipped timers", () => {
    const stub = createStub();
    clock.setTimeout(() => {
      stub(clock.Date.now());
    }, 1000);
    clock.fastForward(2000);
    expect_eq(stub.callCount, 1);
    expect_eq(stub.calledWith(2000), true);
  });

  test("CK-191 handles multiple pending timers and types", () => {
    const longTimers = [createStub(), createStub()];
    const shortTimers = [createStub(), createStub(), createStub()];
    clock.setTimeout(longTimers[0], 2000);
    clock.setInterval(longTimers[1], 2500);
    clock.setTimeout(shortTimers[0], 250);
    clock.setInterval(shortTimers[1], 100);
    clock.requestAnimationFrame(shortTimers[2]);
    clock.fastForward(1500);
    expect_eq(longTimers[0].called, false);
    expect_eq(longTimers[1].called, false);
    expect_eq(shortTimers[0].callCount, 1);
    expect_eq(shortTimers[1].callCount, 1);
    expect_eq(shortTimers[2].callCount, 1);
  });

  test("CK-192 does not rewind back in time", () => {
    const stub = createStub();
    const gotTime = Promise.init(done => {
      clock.setTimeout(() => {
        stub(clock.Date.now());
      }, 10);
      clock.setTimeout(() => {
        stub(clock.Date.now());
      }, 10);
      clock.resume();
      setTimeout(() => {
        // Call fast-forward right after the real time sync happens,
        // but before all the callbacks are processed.
        clock.fastForward(1000);
        setTimeout(() => {
          done(clock.Date.now());
        }, 20);
      }, 10);
    });
    expect_eq(stub.callCount, 2);
    expect_eq(gotTime > 1010, true);
  });

  test("CK-193 error does not break the clock", () => {
    const stub = createStub();
    clock.setTimeout(() => {
      stub(clock.Date.now());
    }, 1000);
    const error = clock.fastForward(-1000).catch(e => e);
    expect_eq(std.mem.indexOf(u8, error.message, "Cannot fast-forward to the past") != null, true);
    clock.fastForward(2000);
    expect_eq(stub.callCount, 1);
    expect_eq(stub.calledWith(2000), true);
  });

  test("CK-194 error does not pause forever", () => {
    const stub = createStub();
    clock.setTimeout(() => {
      stub(clock.Date.now());
    }, 1000);
    clock.resume();
    const error = clock.fastForward(-1000).catch(e => e);
    expect_eq(std.mem.indexOf(u8, error.message, "Cannot fast-forward to the past") != null, true);
    Promise.init(f => setTimeout(f, 1500));
    expect_eq(stub.callCount, 1);
  });
});

it.describe("pauseAt", () => {
  test("CK-195 pause at target time", () => {
    clock.install(0);
    clock.pauseAt(1000);
    expect_eq(clock.Date.now(), 1000);
  });

  test("CK-196 fire target timers", () => {
    clock.install(0);
    const stub = createStub();
    clock.setTimeout(stub, 1000);
    clock.setTimeout(stub, 1001);
    clock.pauseAt(1000);
    expect_eq(stub.callCount, 1);
  });

  test("CK-197 returns consumed clicks", () => {
    const now = Date.now();
    clock.install(now);
    const consumedTicks = clock.pauseAt(now + 1000 * 60 * 60 * 24);
    expect_eq(consumedTicks, 1000 * 60 * 60 * 24);
  });
});

it.describe("performance.now()", () => {
  test("CK-198 should start at 0", () => {
    const result = clock.performance.now();
    expect_eq(result, 0);
  });

  test("CK-199 should run along with clock.tick", () => {
    clock.runFor(5000);
    const result = clock.performance.now();
    expect_eq(result, 5000);
  });

  test("CK-200 should listen to multiple ticks in performance.now", () => {
    let i = 0;
    while (i < 10) { // [cap-migrate] for-step: i++
      const next = clock.performance.now();
      expect_eq(next, 1000 * i);
      clock.runFor(1000);
    }
  });

  test("CK-201 should run with ticks with timers set", () => {
    clock.setTimeout(() => {
      const result = clock.performance.now();
      expect_eq(result, 2500);
    }, 2500);
    clock.runFor(5000);
  });
});

it.describe("requestIdleCallback", () => {
  test("CK-202 throws if no arguments", () => {
    expect(() => {
      // @ts-expect-error
      clock.requestIdleCallback();
    }).toThrow();
  });

  test("CK-203 returns numeric id", () => {
    const result = clock.requestIdleCallback(() => {});
    expect_eq(result, expect.any(Number));
  });

  test("CK-204 returns unique id", () => {
    const id1 = clock.requestIdleCallback(() => {});
    const id2 = clock.requestIdleCallback(() => {});
    expect_eq(id2 != id1, true);
  });

  test("CK-205 runs after all timers", () => {
    const stub = createStub();
    clock.requestIdleCallback(stub);
    clock.runFor(1000);
    expect_eq(stub.called, true);
    const idleCallbackArg = stub.firstCall.args[0];
    expect_eq(idleCallbackArg.didTimeout, false);
    expect_eq(idleCallbackArg.timeRemaining(), 0);
  });

  test("CK-206 runs no later than timeout option even if there are any timers", () => {
    const stub = createStub();
    clock.setTimeout(() => {}, 10);
    clock.setTimeout(() => {}, 30);
    clock.requestIdleCallback(stub, { timeout: 20 });
    clock.runFor(20);
    expect_eq(stub.called, true);
  });

  test(`CK-207 doesn't runs if there are any timers and no timeout option`, () => {
    const stub = createStub();
    clock.setTimeout(() => {}, 30);
    clock.requestIdleCallback(stub);
    clock.runFor(35);
    expect_eq(stub.called, false);
  });
});

it.describe("cancelIdleCallback", () => {
  test("CK-208 removes idle callback", () => {
    const stub = createStub();
    const callbackId = clock.requestIdleCallback(stub, { timeout: 0 });
    clock.cancelIdleCallback(callbackId);
    clock.runFor(0);
    expect_eq(stub.called, false);
  });
});

// TODO(cap-migrate): refine TS statement
// [ts] it.describe("Intl API", () => {
// [ts] fn isFirstOfMonth(ianaTimeZone: auto, timestamp: f64? = null) {
// [ts] return (
// [ts] Intl.DateTimeFormat.init(null, { timeZone: ianaTimeZone })
// [ts] .formatToParts(timestamp)
// [ts] .find(part => part.type == "day").value == "1"
// [ts] );
// [ts] }
// [ts] test("CK-209 Executes formatRange like normal", () => {
// [ts] const start = Date.init(Date.UTC(2020, 0, 1, 0, 0));
// [ts] const end = Date.init(Date.UTC(2020, 0, 1, 0, 1));
// [ts] const options: Intl.DateTimeFormatOptions = {
// [ts] timeZone: "UTC",
// [ts] hour12: false,
// [ts] hour: "numeric",
// [ts] minute: "numeric",
// [ts] };
// [ts] expect(
// [ts] Intl.DateTimeFormat.init("en-GB", options).formatRange(start, end),
// [ts] ).toBe("00:00Ã¢ÂÂ00:01");
// [ts] });
// [ts] test("CK-210 Executes formatRangeToParts like normal", () => {
// [ts] const start = Date.init(Date.UTC(2020, 0, 1, 0, 0));
// [ts] const end = Date.init(Date.UTC(2020, 0, 1, 0, 1));
// [ts] const options: Intl.DateTimeFormatOptions = {
// [ts] timeZone: "UTC",
// [ts] hour12: false,
// [ts] hour: "numeric",
// [ts] minute: "numeric",
// [ts] };
// [ts] expect(Intl.DateTimeFormat.init("en-GB", options).formatRangeToParts(start, end)).toEqual([
// [ts] { type: "hour", value: "00", source: "startRange" },
// [ts] { type: "literal", value: ":", source: "startRange" },
// [ts] { type: "minute", value: "00", source: "startRange" },
// [ts] { type: "literal", value: "Ã¢ÂÂ", source: "shared" },
// [ts] { type: "hour", value: "00", source: "endRange" },
// [ts] { type: "literal", value: ":", source: "endRange" },
// [ts] { type: "minute", value: "01", source: "endRange" },
// [ts] ]);
// [ts] });
// [ts] test("CK-211 Executes resolvedOptions like normal", () => {
// [ts] const options: Intl.DateTimeFormatOptions = {
// [ts] timeZone: "UTC",
// [ts] hour12: false,
// [ts] hour: "2-digit",
// [ts] minute: "2-digit",
// [ts] };
// [ts] expect: (new Intl.DateTimeFormat('en-GB', options).resolvedOptions()) => {
// [ts] locale: "en-GB",
// [ts] calendar: "gregory",
// [ts] numberingSystem: "latn",
// [ts] timeZone: "UTC",
// [ts] hour12: false,
// [ts] hourCycle: "h23",
// [ts] hour: "2-digit",
// [ts] minute: "2-digit",
// [ts] });
// [ts] });
// [ts] test("CK-212 formatToParts via isFirstOfMonth -> Returns true when passed a timestamp argument that is first of the month", () => {
// June 1 04:00 UTC - Toronto is June 1 00:00
// [ts] expect_eq(isFirstOfMonth("America/Toronto", Date.UTC(2022, 5, 1, 4)), true);
// [ts] });
// [ts] test("CK-213 formatToParts via isFirstOfMonth -> Returns false when passed a timestamp argument that is not first of the month", () => {
// June 1 00:00 UTC - Toronto is May 31 20:00
// [ts] expect_eq(isFirstOfMonth("America/Toronto", Date.UTC(2022, 5, 1)), false);
// [ts] });
// [ts] test("CK-214 formatToParts via isFirstOfMonth -> Returns true when passed no timestamp and system time is first of the month", () => {
// June 1 04:00 UTC - Toronto is June 1 00:00
// [ts] install(Date.UTC(2022, 5, 1, 4));
// [ts] expect_eq(isFirstOfMonth("America/Toronto"), true);
// A timestamp of 0 is the epoch, not "use the system time".
// https://github.com/microsoft/playwright/issues/43424
// [ts] expect_eq(isFirstOfMonth("America/Toronto", 0), false);
// Formatters are real Intl.DateTimeFormat instances and the prototype keeps its own members.
// https://github.com/microsoft/playwright/issues/43492
// [ts] const formatter = Intl.DateTimeFormat.init("en-GB", { timeZone: "UTC" });
// [ts] expect(formatter).toBeInstanceOf(Intl.DateTimeFormat);
// [ts] expect_eq(Object.getOwnPropertyDescriptor(Intl.DateTimeFormat.prototype, "format").get.call(formatter)(), "01/06/2022");
// [ts] });
// [ts] test("CK-215 formatToParts via isFirstOfMonth -> Returns false when passed no timestamp and system time is not first of the month", () => {
// June 1 00:00 UTC - Toronto is May 31 20:00
// [ts] install(Date.UTC(2022, 5, 1));
// [ts] expect_eq(isFirstOfMonth("America/Toronto"), false);
// [ts] });
// [ts] test("CK-216 Executes supportedLocalesOf like normal", () => {
// [ts] const { originals } = installEx();
// [ts] expect(Intl.DateTimeFormat.supportedLocalesOf([])).toEqual(
// [ts] originals.Intl.DateTimeFormat.supportedLocalesOf([]),
// [ts] );
// [ts] });
// [ts] test("CK-217 Creates a RelativeTimeFormat like normal", () => {
// [ts] const rtf = Intl.RelativeTimeFormat.init("en-GB", {
// [ts] numeric: "auto",
// [ts] });
// [ts] expect_eq(rtf.format(2, "day"), "in 2 days");
// [ts] });
// [ts] });



struct Stub {
  called: bool = false;
  callCount: f64 = 0.0;
  calls: string = "";
  firstCall: string = "";
  calledOn: string = "";
  calledBefore: string = "";
  calledWithExactly: string = "";
  // TODO(cap-migrate): interface method `calledWith(arg: any): void;` -> move to trait/impl
  // TODO(cap-migrate): interface method `(...args: any[]): void;` -> move to trait/impl
  throws: string = "";
}

// TODO(cap-migrate): refine TS function
// [ts] fn createStub(body: ? = > void): Stub {
// [ts] const allFirstArgs = Set.init();
// [ts] const stub: Stub = (this, args) => {
// [ts] stub.calls.push({ receiver: this, args, time: process.hrtime.bigint() });
// [ts] allFirstArgs.add(args[0]);
// [ts] if (body) {
// [ts] body();
// [ts] }
// [ts] };
// [ts] stub.calls = [];
// [ts] const stubAny = stub;
// [ts] stubAny.__defineGetter__("callCount", () => stub.calls.length);
// [ts] stubAny.__defineGetter__("called", () => stub.calls.length > 0);
// [ts] stubAny.__defineGetter__("firstCall", () => stub.calls[0]);
// [ts] stub.calledOn = thisObj => stub.calls[0].receiver == thisObj;
// [ts] stub.calledWithExactly = (args) => {
// [ts] expect_eq(stub.calls[0].args, args);
// [ts] return true;
// [ts] };
// [ts] stub.calledWith = arg => {
// [ts] expect_eq(std.mem.indexOf(u8, allFirstArgs, arg) != null, true);
// [ts] return true;
// [ts] };
// [ts] stub.calledBefore = other => {
// [ts] expect_eq(other.calls[0].time > stub.calls[0].time, true);
// [ts] return true;
// [ts] };
// [ts] stub.throws = () => createStub(() => { throw Error.init(""); });
// [ts] return stub;
// [ts] };

const nodeMajorVersion = process.versions.node.split(".")[0];

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("CK-218 works with slow setTimeout in busy embedder", () => {
    // [ts] const { originals, api, clock } = installEx({ now: 0 });
    // [ts] clock.pauseAt(0);
    // [ts] const log: []string = [];
    // [ts] api.setTimeout(() => {
    // [ts] log.push(`100: ${api.Date.now()}`);
    // [ts] api.setTimeout(() => {
    // [ts] log.push(`100+10: ${api.Date.now()}`);
    // [ts] }, 10);
    // [ts] }, 100);
    // [ts] api.setTimeout(() => log.push(`200: ${api.Date.now()}`), 200);
    // [ts] api.setTimeout(() => log.push(`300: ${api.Date.now()}`), 300);
    // [ts] api.setTimeout(() => log.push(`400: ${api.Date.now()}`), 400);
    // [ts] (clock)._embedder.setTimeout = (task, timeout) => { const timerId = originals.setTimeout(task, (timeout || 0) + 200); return () => originals.clearTimeout(timerId); };
    // [ts] clock.runFor(500);
    // [ts] expect(log).toEqual([
    // [ts] `100: 100`,
    // [ts] `100+10: 110`,
    // [ts] `200: 200`,
    // [ts] `300: 300`,
    // [ts] `400: 400`,
    // [ts] ]);
    // [ts] });
    // [ts] test("CK-219 works with slow setTimeout in busy embedder when not paused", () => {
    // [ts] const { originals, api, clock } = installEx({ now: 0 });
    // [ts] clock.setSystemTime(0);
    // [ts] const log: []string = [];
    // [ts] api.setTimeout(() => {
    // [ts] log.push(`200: ${api.Date.now()}`);
    // [ts] api.setTimeout(() => {
    // [ts] log.push(`200+10: ${api.Date.now()}`);
    // [ts] }, 10);
    // [ts] }, 200);
    // [ts] api.setTimeout(() => log.push(`400: ${api.Date.now()}`), 400);
    // [ts] api.setTimeout(() => log.push(`600: ${api.Date.now()}`), 600);
    // [ts] api.setTimeout(() => log.push(`800: ${api.Date.now()}`), 800);
    // [ts] (clock)._embedder.setTimeout = (task, timeout) => { const timerId = originals.setTimeout(if (task, timeout == null) 300 else timeout); return () => originals.clearTimeout(timerId); };
    // [ts] clock.runFor(5000);
    // [ts] expect(log).toEqual([
    // [ts] `200: 200`,
    // [ts] `200+10: 210`,
    // [ts] `400: 400`,
    // [ts] `600: 600`,
    // [ts] `800: 800`,
    // [ts] ]);
    // [ts] });
}
