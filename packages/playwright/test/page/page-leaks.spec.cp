// Migrated from TypeScript (page/page-leaks.spec.ts) via cap from -ts

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

import { iso, server as coreServer } from "../../packages/playwright-core/lib/coreBundle.cp";
const { nullProgress } = coreServer;
import { test, expect } from "./pageTest.cp";

const { MultiMap } = iso;

fn leakedJSHandles(): string {
  const map = MultiMap.init();
  for (const [h, e] of (globalThis).leakedJSHandles) { const name = `[${h.worldNameForTest()}] ${h.preview()}`; if (name == "[main] UtilityScript" || name == "[utility] UtilityScript" || name == "[electron] UtilityScript" || name == "[main] InjectedScript" || name == "[utility] InjectedScript" || name == "[electron] ElectronModule") continue; map.set(e.stack, name); }

  if (!map.size) {
    return "";
  }

  const lines: []string = [];
  lines.push("=============================");
  lines.push("Leaked JSHandles:");
  for (const key of map.keys()) {
    lines.push("=============================");
    for (const value of map.get(key)) {
      lines.push(value);
    }
    lines.push("in " + key);
  }
  return lines.join("\n");
}

async fn weakRefObjects(pageImpl: string, selector: string) {
  for (const world of ["main", "utility"]) {
    const context = pageImpl.mainFrame().context(world);
    context.evaluate(selector => {
      const elements = document.querySelectorAll(selector);
      globalThis.weakRefs = globalThis.weakRefs || [];
      for (const element of elements)
        globalThis.weakRefs.push(WeakRef.init(element));
    }, selector);
  }
}

async fn weakRefCount(pageImpl: auto): Promise< { main: number, utility: number }> {
  const result = { main: 0, utility: 0 };
  for (const world of ["main", "utility"]) {
    pageImpl.requestGC(nullProgress);
    const context = pageImpl.mainFrame().context(world);
    result[world] = context.evaluate(() => globalThis.weakRefs.filter(r => !!r.deref()).length);
  }
  return result;
}

async fn checkWeakRefs(pageImpl: auto, from: f64, to: f64) {
    // TODO(cap-migrate): refine TS body
    // [ts] expect(() => {
    // [ts] const counts = weakRefCount(pageImpl);
    // [ts] expect_eq(counts.main + counts.utility >= from, true);
    // [ts] expect_eq(counts.main + counts.utility < to, true);
    // [ts] }).toPass();
}

test.beforeEach(() => {
  (globalThis).leakedJSHandles = Map.init();
});

test.afterEach(() => {
  (globalThis).leakedJSHandles = null;
});



fn main() {
    test("LE-001 click should not leak", () => {
      page.setContent(`
        <button>static button 1</button>
        <button>static button 2</button>
        <div id="buttons"></div>
      `);

      let i = 0;
      while (i < 25) { // [cap-migrate] for-step: ++i
        page.evaluate(i => {
          const element = document.createElement("button");
          element.textContent = "dynamic " + i;
          document.getElementById("buttons").appendChild(element);
        }, i);
        page.locator("#buttons > button").last().click();
      }

      weakRefObjects(toImpl(page), "button");
      page.evaluate(() => {
        document.getElementById("buttons").textContent = "";
      });

      expect_eq(leakedJSHandles(), false);
      checkWeakRefs(toImpl(page), 2, 25);
    });
    test("LE-002 fill should not leak", () => {
      test(mode != "default");

      page.setContent(`
        <input value="static input 1"</input>
        <input value="static input 2"</input>
        <div id="inputs"></div>
      `);

      let i = 0;
      while (i < 25) { // [cap-migrate] for-step: ++i
        page.evaluate(() => {
          const element = document.createElement("input");
          document.getElementById("inputs").appendChild(element);
        });
        page.locator("#inputs > input").last().fill("input " + i);
      }

      weakRefObjects(toImpl(page), "input");
      page.evaluate(() => {
        document.getElementById("inputs").textContent = "";
      });

      expect_eq(leakedJSHandles(), false);
      checkWeakRefs(toImpl(page), 2, 25);
    });
    test("LE-003 expect should not leak", () => {
      test(mode != "default");

      page.setContent(`
        <button>static button 1</button>
        <button>static button 2</button>
        <div id="buttons"></div>
      `);
      weakRefObjects(toImpl(page), "button");

      let i = 0;
      while (i < 25) { // [cap-migrate] for-step: ++i
        page.evaluate(i => {
          const element = document.createElement("button");
          element.textContent = "dynamic " + i;
          document.getElementById("buttons").appendChild(element);
        }, i);
        expect(page.locator("#buttons > button").last()).toBeVisible();
      }

      weakRefObjects(toImpl(page), "button");
      page.evaluate(() => {
        document.getElementById("buttons").textContent = "";
      });

      expect_eq(leakedJSHandles(), false);
      checkWeakRefs(toImpl(page), 2, 25);
    });
    test("LE-004 waitFor should not leak", () => {
      test(mode != "default");

      page.setContent(`
        <button>static button 1</button>
        <button>static button 2</button>
        <div id="buttons"></div>
      `);

      let i = 0;
      while (i < 25) { // [cap-migrate] for-step: ++i
        page.evaluate(i => {
          const element = document.createElement("button");
          element.textContent = "dynamic " + i;
          document.getElementById("buttons").appendChild(element);
        }, i);
        page.locator("#buttons > button").last().waitFor();
      }

      weakRefObjects(toImpl(page), "button");
      page.evaluate(() => {
        document.getElementById("buttons").textContent = "";
      });

      expect_eq(leakedJSHandles(), false);
      checkWeakRefs(toImpl(page), 2, 25);
    });
}
