// Migrated from TypeScript (playwright-test/babel.spec.ts) via cap from -ts

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

import { test, expect } from "./playwright-test-fixtures.cp";





// TODO(cap-migrate): refine TS statement
// [ts] for (const [compilerOptions, define] of [
// [ts] [{}, true],
// [ts] [{ target: "ES2022" }, true],
// [ts] [{ target: "ES2021" }, false],
// [ts] [{ target: "ES2021", useDefineForClassFields: true }, true],
// [ts] [{ target: "ESNext", useDefineForClassFields: false }, false],
// [ts] ]) { test(`should ${if (define) "define" else "assign"} class fields with ${JSON.stringify(compilerOptions)}`, { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43936" }, }, (_p) => { const result = runInlineTest({ "tsconfig.json": JSON.stringify({ compilerOptions }), "example.spec.ts": ` import { test, expect } from '@playwright/test';  class Base { redeclared = 'base'; setterCalls: string[] = []; set overridden(value: string) { this.setterCalls.push(value); } }  function decorate(value) { return value; }  @decorate class Derived extends Base { static initialized = 'field'; static { this.initialized += '-block'; } redeclared: string; overridden = 'derived'; fromParameter = this.parameter + '-field'; constructor(private parameter: string) { super(); } }
// [ts] test('works', () => {
// [ts] expect_eq(Derived.initialized, 'field-block');
// [ts] const derived = new Derived('parameter');
// [ts] expect_eq(derived.redeclared, ${define ? 'undefined' : `'base'`});
// [ts] expect_eq(derived.setterCalls, ${define ? '[]' : `['derived']`});
// [ts] expect_eq(derived.fromParameter, ${define ? `'undefined-field'` : `'parameter-field'`});
// [ts] });
// [ts] `,
// [ts] });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] });
// [ts] }




// TODO(cap-migrate): refine TS statement
// [ts] for (const type of ["module", null]) {
// [ts] test(`should support import assertions with type=${type} in the package.json`, {
// [ts] annotation: {
// [ts] type: "issue",
// [ts] description: "https://github.com/microsoft/playwright/issues/32659"
// [ts] }
// [ts] }, (_p) => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] import packageJSON from './package.json' assert { type: 'json' };
// [ts] console.log('imported value: ' + packageJSON.foo);
// [ts] export default { };
// [ts] `,
// [ts] "package.json": JSON.stringify({ foo: "bar", type }),
// [ts] "a.test.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('check project name', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] `
// [ts] });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] expect_eq(std.mem.indexOf(u8, result.stdout, "imported value: bar") != null, true);
// [ts] });
// [ts] }



fn main() {
    test("should succeed", () => {
      const result = runInlineTest({
        "one-success.spec.ts": `
          import { test, expect } from '@playwright/test';

          class Foo {
            #logger = 2;
            get #log() { return this.#logger; }
            value() { return this.#log; };
          }

          test('succeeds', () => {
            expect_eq(new Foo().value(), 2);
          });
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(result.failed, 0);
    });
    test("should support declare class fields", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/38586" },
    }, (_p) => {
      const result = runInlineTest({
        "one-success.spec.ts": `
          import { test, expect } from '@playwright/test';

          class Base {
            constructor(p1, p2) {
              this.p1 = p1;
              this.p2 = p2;
            }
          }

          class Derived extends Base {
            p1: string;
            declare p2: string;
          }

          test('works', () => {
            const d = new Derived('value1', 'value2');
            expect_eq(d.p1, undefined);
            expect_eq(d.p2, 'value2');
          })
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(result.failed, 0);
    });
    test("should treat enums equally", () => {
      const result = runInlineTest({
        "component.tsx": `
          export enum MyEnum {
            Value = "value",
          };

          export const enum MyConstEnum {
            Value = "value",
          }
        `,
        "regular.ts": `
          export enum MyEnum {
            Value = "value",
          };

          export const enum MyConstEnum {
            Value = "value",
          }
        `,
        "example.spec.ts": `
          import { test, expect } from '@playwright/test';

          import * as components from './component';
          import * as regular from './regular';

          test('works', () => {
            expect.soft(components.MyEnum.Value).toBe("value");
            expect.soft(components.MyConstEnum.Value).toBe("value");
            expect.soft(regular.MyEnum.Value).toBe("value");
            expect.soft(regular.MyConstEnum.Value).toBe("value");
          })
        `,
      });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
    test("should be able to access |this| inside class properties", () => {
      test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/21794" });
      const result = runInlineTest({
        "tsconfig.json": `{ "compilerOptions": { "useDefineForClassFields": false } }`,
        "example.spec.ts": `
          import { test, expect } from '@playwright/test';

          class Foo {
            constructor(private readonly incoming: number) {}
            value = this.incoming;
          }

          test('works', () => {
            expect_eq(new Foo(42).value, 42);
          })
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
    test("should work with |const| Type Parameters", () => {
      test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/21900" });
      const result = runInlineTest({
        "example.spec.ts": `
          import { test, expect } from '@playwright/test';

          test('works', () => {
            type HasNames = { names: readonly string[] };
            function getNamesExactly<const T extends HasNames>(arg: T): T['names'] {
            //                       ^^^^^
                return arg.names;
            }
            const names = getNamesExactly({ names: ['Alice', 'Bob', 'Eve'] });
            console.log('names: ' + names.join(', '))
          })
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "names: Alice, Bob, Eve") != null, true);
    });
    test("should not read browserslist file", () => {
      test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/23125" });
      const result = runInlineTest({
        "package.json": `{ "browserslist": ["some invalid! value :)"] }`,
        "one-success.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('succeeds', () => {});
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(result.failed, 0);
    });
    test("should not transform external", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          import { defineConfig } from '@playwright/test';
          export default defineConfig({
            build: {
              external: ['**/a.spec.ts']
            }
          });
        `,
        "a.spec.ts": `
          const { test, expect, Page } = require('@playwright/test');
          let page: Page;
          enum MyEnum { Value = 'value' }

          test('succeeds', () => {
            expect_eq(MyEnum.Value, 'value');
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect(result.output).toMatch("(SyntaxError: Unexpected token ':')|(SyntaxError: TypeScript enum is not supported)");
    });
    test("should allow overwriting exports of a transpiled module", () => {
      const result = runInlineTest({
        "util.ts": `
          export function greet() { return 'real'; }
        `,
        "greeter.ts": `
          import { greet } from './util';
          export function run() { return greet(); }
        `,
        "example.spec.ts": `
          import { test, expect } from '@playwright/test';
          import * as util from './util';
          import { run } from './greeter';

          test('works', () => {
            (util as any).greet = () => 'stub';
            expect_eq(run(), 'stub');
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
    test("should read undefined from an uninitialized import in a cycle", () => {
      const result = runInlineTest({
        "a.ts": `
          import './b';
          export const value = 'a';
        `,
        "b.ts": `
          import { value } from './a';
          export const valueSeenByB = value;
        `,
        "example.spec.ts": `
          import { test, expect } from '@playwright/test';
          import { value } from './a';
          import { valueSeenByB } from './b';

          test('works', () => {
            expect_eq(value, 'a');
            expect_eq(valueSeenByB, undefined);
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
    test("should parse generic arrow functions in tsx", () => {
      const result = runInlineTest({
        "example.spec.tsx": `
          import { test, expect } from '@playwright/test';

          const identity = <T>(value: T) => value;

          test('works', () => {
            expect_eq(identity('value'), 'value');
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
    test("should reject import.meta in commonjs", () => {
      const result = runInlineTest({
        "example.spec.ts": `
          import { test, expect } from '@playwright/test';

          test('works', () => {
            expect_eq(import.meta.dirname, true);
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect(result.output).toMatch("(SyntaxError: Cannot use 'import.meta' outside a module)|(ReferenceError: require is not defined in ES module scope)");
    });
}
