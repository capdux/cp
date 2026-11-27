// Migrated from TypeScript (playwright-test/fixtures.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright Microsoft Corporation. All rights reserved.
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

import { test, expect } from "./playwright-test-fixtures.cp";
































fn main() {
    test("should work", () => {
      const { results } = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            asdf: ({}, test) => test(123),
          });

          test('should use asdf', () => {
            expect_eq(asdf, 123);
          });
        `,
      });
      expect_eq(results[0].status, "passed");
    });
    test("should work with comments inside fixtures", () => {
      const { results } = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            asdf: ({}, test) => test(123),
            foo: ({}, test) => test('foo'),
            bar: ({}, test) => test('bar'),
          });

          test('should use asdf', ({ // }) {,,, /*
        asdf, // a comment
    /*/aa* /* */       // line // //
        /* // */      foo, /* what // */ bar // whoa
              /* some // comment */ : //
          /* // /* // */ barbar /* /* /* */
              }) => {
            expect_eq(asdf, 123);
            expect_eq(foo, 'foo');
            expect_eq(barbar, 'bar');
          });
        `,
      });
      expect_eq(results[0].status, "passed");
    });
    test("should throw a pretty error if fixtures use rest property", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            asdf: ({...props}, use) => use(123),
          });
          test('should not allow rest property inside tests', () => {
            expect_eq(asdf, 123);
          });
          test('should not allow rest property inside fixtures', () => {
            expect_eq(asdf, 123);
          });
          `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Rest property \"...all\" is not supported. List all used fixtures explicitly, separated by comma.") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Rest property \"...props\" is not supported. List all used fixtures explicitly, separated by comma.") != null, true);
    });
    test("should work with a sync test function", () => {
      const { results } = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            asdf: ({}, test) => test(123),
          });

          test('should use asdf', () => {
            expect_eq(asdf, 123);
          });
        `,
      });
      expect_eq(results[0].status, "passed");
    });
    test("should work with a sync fixture function", () => {
      const { results } = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            asdf: ({}, use) => {
              use(123);
            },
          });

          test('should use asdf', () => {
            expect_eq(asdf, 123);
          });
        `,
      });
      expect_eq(results[0].status, "passed");
    });
    test("should work with a non-arrow function", () => {
      const { results } = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            asdf: ({}, test) => test(123),
          });

          test('should use asdf', function ({asdf}) {
            expect_eq(asdf, 123);
          });
        `,
      });
      expect_eq(results[0].status, "passed");
    });
    test("should work with a named function", () => {
      const { results } = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            asdf: ({}, test) => test(123),
          });

          test('should use asdf', function hello({asdf}) {
            expect_eq(asdf, 123);
          });
        `,
      });
      expect_eq(results[0].status, "passed");
    });
    test("should work with renamed parameters", () => {
      const { results } = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            asdf: ({}, test) => test(123),
          });

          test('should use asdf', function ({asdf: renamed}) {
            expect_eq(renamed, 123);
          });
        `,
      });
      expect_eq(results[0].status, "passed");
    });
    test("should work with destructured object", () => {
      const { results } = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            asdf: ({}, test) => test({ foo: 'foo', bar: { x: 'x', y: 'y' }, baz: 'baz' }),
          });

          test('should use asdf', ({ asdf: { foo,
              bar: { x, y }, baz } }) => {
            expect_eq(foo, 'foo');
            expect_eq(x, 'x');
            expect_eq(y, 'y');
            expect_eq(baz, 'baz');
          });
        `,
      });
      expect_eq(results[0].status, "passed");
    });
    test("should work with destructured array", () => {
      const { results } = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            asdf: ({}, test) => test(['foo', 'bar', { baz: 'baz' }]),
            more: ({}, test) => test(55),
          });

          test('should use asdf', (

            {
              asdf: [foo, bar,        { baz}]


              ,more}) => {
            expect_eq(foo, 'foo');
            expect_eq(bar, 'bar');
            expect_eq(baz, 'baz');
            expect_eq(more, 55);
          });
        `,
      });
      expect_eq(results[0].status, "passed");
    });
    test("should fail if parameters are not destructured", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            asdf: ({}, test) => test(123),
          });
          test('should pass', function () {
            expect_eq(1, 1);
          });
          test('should use asdf', function (abc) {
            expect_eq(abc.asdf, 123);
          });
        `,
      });
      expect_eq(std.mem.indexOf(u8, result.output, "First argument must use the object destructuring pattern: abc") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "a.test.ts:9") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "function (abc)") != null, true);
      expect_eq(result.results.length, 0);
    });
    test("should fail with an unknown fixture", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('should use asdf', () => {
            expect_eq(asdf, 123);
          });
        `,
      });
      expect_eq(std.mem.indexOf(u8, result.output, "Test has unknown parameter \"asdf\".") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "a.test.ts:3") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "async ({asdf})") != null, true);
      expect_eq(result.results.length, 0);
    });
    test("should run the fixture every time", () => {
      const { results } = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          let counter = 0;
          const test = base.extend({
            asdf: ({}, test) => test(counter++),
          });
          test('should use asdf 1', () => {
            expect_eq(asdf, 0);
          });
          test('should use asdf 2', () => {
            expect_eq(asdf, 1);
          });
          test('should use asdf 3', () => {
            expect_eq(asdf, 2);
          });
        `,
      });
      expect_eq(results.map(r => r.status), ["passed", "passed", "passed"]);
    });
    test("should only run worker fixtures once", () => {
      const { results } = runInlineTest({
        "a.test.ts": `
          let counter = 0;
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            asdf: [ ({}, test) => test(counter++), { scope: 'worker' } ],
          });
          test('should use asdf 1', () => {
            expect_eq(asdf, 0);
          });
          test('should use asdf 2', () => {
            expect_eq(asdf, 0);
          });
          test('should use asdf 3', () => {
            expect_eq(asdf, 0);
          });
        `,
      });
      expect_eq(results.map(r => r.status), ["passed", "passed", "passed"]);
    });
    test("each file should get their own fixtures", () => {
      const { results } = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            worker: [ ({}, test) => test('worker-a'), { scope: 'worker' } ],
            test: ({}, test) => test('test-a'),
          });
          test('should use worker', () => {
            expect_eq(worker, 'worker-a');
            expect_eq(test, 'test-a');
          });
        `,
        "b.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            worker: [ ({}, test) => test('worker-b'), { scope: 'worker' } ],
            test: ({}, test) => test('test-b'),
          });
          test('should use worker', () => {
            expect_eq(worker, 'worker-b');
            expect_eq(test, 'test-b');
          });
        `,
        "c.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            worker: [ ({}, test) => test('worker-c'), { scope: 'worker' } ],
            test: ({}, test) => test('test-c'),
          });
          test('should use worker', () => {
            expect_eq(worker, 'worker-c');
            expect_eq(test, 'test-c');
          });
        `,
      });
      expect_eq(results.map(r => r.status), ["passed", "passed", "passed"]);
    });
    test("tests should be able to share worker fixtures", () => {
      const { results } = runInlineTest({
        "worker.js": `
          global.counter = 0;
          const { test: base, expect } = require('@playwright/test');
          const test = base.extend({
            worker: [ ({}, test) => test(global.counter++), { scope: 'worker' } ],
          });
          module.exports = { test, expect };
        `,
        "a.test.ts": `
          const { test, expect } = require('./worker.js');
          test('should use worker', () => {
            expect_eq(worker, 0);
          });
        `,
        "b.test.ts": `
          const { test, expect } = require('./worker.js');
          test('should use worker', () => {
            expect_eq(worker, 0);
          });
        `,
        "c.test.ts": `
          const { test, expect } = require('./worker.js');
          test('should use worker', () => {
            expect_eq(worker, 0);
          });
        `,
      });
      expect_eq(results.map(r => r.status), ["passed", "passed", "passed"]);
    });
    test("automatic fixtures should work", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          let counterTest = 0;
          let counterHooksIncluded = 0;
          let counterWorker = 0;
          test.use({
            automaticTestFixture: [ ({}, runTest) => {
              ++counterTest;
              runTest();
            }, { auto: true } ],

            automaticTestFixtureHooksIncluded: [ ({}, runTest) => {
              ++counterHooksIncluded;
              runTest();
            }, { auto: 'all-hooks-included' } ],

            automaticWorkerFixture: [ ({}, runTest) => {
              ++counterWorker;
              runTest();
            }, { scope: 'worker', auto: true } ],
          });
          test.beforeAll(({}) => {
            expect_eq(counterWorker, 1);
            expect_eq(counterHooksIncluded, 1);
            expect_eq(counterTest, 0);
          });
          test.beforeEach(({}) => {
            expect_eq(counterWorker, 1);
            expect_eq(counterTest === 1 || counterTest === 2, true);
            expect_eq(counterHooksIncluded === 2 || counterHooksIncluded === 3, true);
          });
          test('test 1', () => {
            expect_eq(counterWorker, 1);
            expect_eq(counterHooksIncluded, 2);
            expect_eq(counterTest, 1);
          });
          test('test 2', () => {
            expect_eq(counterWorker, 1);
            expect_eq(counterHooksIncluded, 3);
            expect_eq(counterTest, 2);
          });
          test.afterEach(({}) => {
            expect_eq(counterWorker, 1);
            expect_eq(counterTest === 1 || counterTest === 2, true);
            expect_eq(counterHooksIncluded === 2 || counterHooksIncluded === 3, true);
          });
          test.afterAll(({}) => {
            expect_eq(counterWorker, 1);
            expect_eq(counterHooksIncluded, 4);
            expect_eq(counterTest, 2);
          });
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.results.map(r => r.status), ["passed", "passed"]);
    });
    test("automatic fixture should start before regular fixture and teardown after", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.use({
            auto: [ ({}, runTest) => {
              console.log('\\n%%auto-setup');
              runTest();
              console.log('\\n%%auto-teardown');
            }, { auto: true } ],
            foo: ({}, runTest) => {
              console.log('\\n%%foo-setup');
              runTest();
              console.log('\\n%%foo-teardown');
            },
          });
          test('test 1', () => {
          });
        `
      });
      expect_eq(result.exitCode, 0);
      expect(result.outputLines).toEqual([
        "auto-setup",
        "foo-setup",
        "foo-teardown",
        "auto-teardown",
      ]);
    });
    test("automatic fixtures should keep workerInfo after conditional skip", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.use({
            automaticTestFixture: [ ({}, runTest, workerInfo) => {
              runTest();
              expect_eq(workerInfo.workerIndex, 0);
              console.log('success test fixture')
            }, { auto: true } ],

            automaticWorkerFixture: [ ({}, runTest, workerInfo) => {
              runTest();
              expect_eq(workerInfo.workerIndex, 0);
              console.log('success worker fixture')
            }, { scope: 'worker', auto: true } ],
          });
          test(({ }) => false);
          test('good', () => {
          });
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(std.mem.indexOf(u8, result.output, "success test fixture") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "success worker fixture") != null, true);
      expect_eq(result.results.map(r => r.status), ["passed"]);
    });
    test("tests does not run non-automatic worker fixtures", () => {
      const result = runInlineTest({
        "a.test.ts": `
          let counter = 0;
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            nonAutomaticWorkerFixture: [ ({}, runTest) => {
              ++counter;
              runTest();
            }, { scope: 'worker' }],
          });
          test('test 1', () => {
            expect_eq(counter, 0);
          });
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.results.map(r => r.status), ["passed"]);
    });
    test("should teardown fixtures after timeout", () => {
      const file = testInfo.outputPath("log.txt");
      require("fs").writeFileSync(file, "", "utf8");
      const result = runInlineTest({
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            file: [ ${JSON.stringify(file)}, { scope: 'worker' } ],
            w: [ ({ file }, runTest) => {
              runTest('w');
              require('fs').appendFileSync(file, 'worker fixture teardown\\n', 'utf8');
            }, { scope: 'worker' } ],
            t: ({ file }, runTest) => {
              runTest('t');
              require('fs').appendFileSync(file, 'test fixture teardown\\n', 'utf8');
            },
          });
          test('test', () => {
            expect_eq(t, 't');
            expect_eq(w, 'w');
            new Promise(() => {});
          });
        `,
      }, { timeout: 1000 });
      expect_eq(result.results[0].status, "timedOut");
      const content = require("fs").readFileSync(file, "utf8");
      expect_eq(std.mem.indexOf(u8, content, "worker fixture teardown") != null, true);
      expect_eq(std.mem.indexOf(u8, content, "test fixture teardown") != null, true);
    });
    test("should work with two different test objects", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test1 = base.extend({
            foo: ({}, test) => test(123),
          });
          const test2 = base.extend({
            bar: ({}, test) => test(456),
          });
          test1('test 1', ({foo}) => {
            expect_eq(foo, 123);
          });
          test2('test 2', ({bar}) => {
            expect_eq(bar, 456);
          });
        `,
      });
      expect_eq(result.results.map(r => r.workerIndex).sort(), [0, 0]);
      expect_eq(result.results.map(r => r.status).sort(), ["passed", "passed"]);
    });
    test("should work with overrides calling base", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test1 = base.extend({
            dep: ({}, test) => test('override'),
            foo: ({}, test) => test('base'),
            bar: ({foo}, test) => test(foo + '-bar'),
          });
          const test2 = test1.extend({
            foo: ({ foo, dep }, test) => test(foo + '-' + dep + '1'),
          });
          const test3 = test2.extend({
            foo: ({ foo, dep }, test) => test(foo + '-' + dep + '2'),
          });
          test3('test', ({bar}) => {
            expect_eq(bar, 'base-override1-override2-bar');
          });
        `,
      });
      expect_eq(result.results[0].status, "passed");
    });
    test("should understand worker fixture params in overrides calling base", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test1 = base.extend({
            param: [ 'param', { scope: 'worker', option: true }],
          }).extend({
            foo: ({}, test) => test('foo'),
            bar: ({foo}, test) => test(foo + '-bar'),
          });
          const test2 = test1.extend({
            foo: ({ foo, param }, test) => test(foo + '-' + param),
          });
          const test3 = test2.extend({
            foo: ({ foo }, test) => test(foo + '-override'),
          });
          test3('test', ({ bar }) => {
            console.log(bar);
          });
        `,
        "playwright.config.ts": `
          export default { projects: [
            { use: { param: 'p1' } },
            { use: { param: 'p2' } },
            { use: { param: 'p3' } },
          ]};
        `,
      });
      const outputs = result.results.map(r => r.stdout.filter(output => "text" == output)[0].text.replace("\\s", ""));
      expect_eq(outputs.sort(), ["foo-p1-override-bar", "foo-p2-override-bar", "foo-p3-override-bar"]);
    });
    test("should work with two overrides calling base", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test1 = base.extend({
            foo: ({}, test) => test('foo'),
            bar: ({}, test) => test('bar'),
            baz: ({foo, bar}, test) => test(foo + '-baz-' + bar),
          });
          const test2 = test1.extend({
            foo: ({ foo, bar }, test) => test(foo + '-' + bar),
            bar: ({ bar }, test) => test(bar + '-override'),
          });
          test2('test', ({baz}) => {
            expect_eq(baz, 'foo-bar-override-baz-bar-override');
          });
        `,
      });
      expect_eq(result.results[0].status, "passed");
    });
    test("should not create a new worker for test fixtures", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('base test', () => {
            expect_eq(testInfo.workerIndex, 0);
          });

          const test2 = test.extend({
            foo: ({}, run) => {
              console.log('foo-a');
              run();
            }
          });
          test2('a test', ({ foo }, testInfo) => {
            expect_eq(testInfo.workerIndex, 0);
          });
        `,
        "b.test.ts": `
          import { test, expect } from '@playwright/test';
          const test2 = test.extend({
            foo: ({}, run) => {
              console.log('foo-b');
              run();
            }
          });
          const test3 = test2.extend({
            foo: ({ foo }, run) => {
              console.log('foo-c');
              run();
            }
          });
          test3('b test', ({ foo }, testInfo) => {
            expect_eq(testInfo.workerIndex, 0);
          });
        `,
      }, { workers: 1 });
      expect_eq(std.mem.indexOf(u8, result.output, "foo-a") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "foo-b") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "foo-c") != null, true);
      expect_eq(result.passed, 3);
    });
    test("should create a new worker for worker fixtures", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('base test', () => {
            console.log('\\n%%base-' + testInfo.workerIndex);
          });

          const test2 = test.extend({
            foo: [({}, run) => {
              console.log('foo-a');
              run();
            }, { scope: 'worker' }],
          });
          test2('a test', ({ foo }, testInfo) => {
            console.log('\\n%%a-' + testInfo.workerIndex);
          });
        `,
        "b.test.ts": `
          import { test, expect } from '@playwright/test';
          const test2 = test.extend({
            bar: ({}, run) => {
              console.log('bar-b');
              run();
            },
          });
          test2('b test', ({ bar }, testInfo) => {
            console.log('\\n%%b-' + testInfo.workerIndex);
          });
        `,
      }, { workers: 1 });
      expect_eq(std.mem.indexOf(u8, result.output, "foo-a") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "bar-b") != null, true);
      const baseWorker = result.output.match("%%base-(\\d)")[1];
      expect_eq(std.mem.indexOf(u8, result.output, `%%base-${baseWorker}`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `%%a-${1 - baseWorker}`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `%%b-${baseWorker}`) != null, true);
      expect_eq(result.passed, 3);
    });
    test("should run tests in order", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test1', () => {
            expect_eq(testInfo.workerIndex, 0);
            console.log('\\n%%test1');
          });

          const child = test.extend({
            foo: ({}, run) => {
              console.log('\\n%%beforeEach');
              run();
              console.log('\\n%%afterEach');
            },
          });
          child('test2', ({ foo }, testInfo) => {
            expect_eq(testInfo.workerIndex, 0);
            console.log('\\n%%test2');
          });

          test('test3', () => {
            expect_eq(testInfo.workerIndex, 0);
            console.log('\\n%%test3');
          });
        `,
      }, { workers: 1 });
      expect_eq(result.passed, 3);
      expect(result.outputLines).toEqual([
        "test1",
        "beforeEach",
        "test2",
        "afterEach",
        "test3",
      ]);
    });
    test("worker fixture should not receive TestInfo", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.use({
            worker: [({}, use, info) => {
              expect_eq(info.title, undefined);
              use();
            }, { scope: 'worker' }],
            test: ({ worker }, use, info) => {
              expect_eq(info.title != undefined, true);
              use();
            },
          });
          test('test 1', () => {
          });
        `
      });
      expect_eq(result.exitCode, 0);
    });
    test("worker teardown errors reflected in timed-out tests", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            foo: [({}, use) => {
              let cb;
              use(new Promise((f, r) => cb = r));
              cb(new Error('Rejecting!'));
            }, { scope: 'worker' }]
          });
          test('timedout', () => {
            foo;
          });
        `,
      }, { timeout: 1000 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Test timeout of 1000ms exceeded.") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Rejecting!") != null, true);
    });
    test("automatic worker fixtures should start before automatic test fixtures", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
              autoTest: [({}, use) => {
                  console.log('\\n%%TEST FIXTURE 1');
                  use();
                  console.log('\\n%%TEST FIXTURE 2');
              }, { scope: 'test', auto: true }],

              autoWorker: [({}, use) => {
                  console.log('\\n%%WORKER FIXTURE 1');
                  use();
                  console.log('\\n%%WORKER FIXTURE 2');
              }, { scope: 'worker', auto: true }],
          });

          test('test', () => {
              console.log('\\n%%TEST');
          });
        `
      });
      expect_eq(result.exitCode, 0);
      expect(result.outputLines).toEqual([
        "WORKER FIXTURE 1",
        "TEST FIXTURE 1",
        "TEST",
        "TEST FIXTURE 2",
        "WORKER FIXTURE 2",
      ]);
    });
    test("should error if use is not called", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            fixture: ({}, use) => {
              return 123;
            },
          });

          test('test', () => {
          });
        `,
      });
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, `use() was not called in fixture "fixture"`) != null, true);
    });
    test("should not treat fixtures as thenable promise", () => {
      const { results } = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            then: ({}, use) => use(() => 'test-function'),
          });

          test('should not treat fixtures as thenable promise', () => {
            expect_eq(typeof then, 'function');
          });
        `,
      });
      expect_eq(results[0].status, "passed");
    });
}
