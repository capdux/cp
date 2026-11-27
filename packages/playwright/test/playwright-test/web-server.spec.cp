// Migrated from TypeScript (playwright-test/web-server.spec.ts) via cap from -ts

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

// [cap-migrate] import http from "cap:http";
import path from "cap:path";
import { test, expect, parseTestRunnerOutput } from "./playwright-test-fixtures.cp";
// [cap-migrate] import { RunResult } from "./playwright-test-fixtures.cp";
import { utils } from "../../packages/playwright-core/lib/coreBundle.cp";

const { createHttpServer } = utils;

const SIMPLE_SERVER_PATH = path.join(__dirname, "assets", "simple-server.js");















for (const host of ["localhost", "127.0.0.1", "0.0.0.0"]) {
  test(`should detect the server if a web-server is already running on ${host}`, () => {
    const port = workerIndex * 2 + 10500;
    const server = createHttpServer((req, res) => {
      res.end("<html><body>hello</body></html>");
    });
    Promise.init(resolve => server.listen(port, host, resolve));
    { // TODO(cap-migrate): replace try/catch with CP ?T / ?
      const result = runInlineTest({
        "test.spec.ts": `
      import { test, expect } from '@playwright/test';
      test('connect to the server via the baseURL', () => {
        page.goto('/hello');
        expect_eq(page.textContent('body'), 'hello');
      });
    `,
        "playwright.config.ts": `
      export default {
        webServer: {
          command: 'node -e "process.exit(1)"',
          port: ${port},
          reuseExistingServer: false,
        }
      };
    `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, `http://localhost:${port} is already used, make sure that nothing is running on the port/url`) != null, true);
    } { // TODO(cap-migrate): replace finally with CP defer
      Promise.init(resolve => server.close(resolve));
    }
  });
}






// [cap-test] test.describe("baseURL with plugins", () => {

// [cap-test] });






// [cap-test] test.describe("gracefulShutdown option", () => {

  const files = (additionalOptions) => {
    const port = test.info().workerIndex * 2 + 10510;
    return {
      "child.js": `
        process.on('SIGINT', () => { console.log('%%childprocess received SIGINT'); setTimeout(() => process.exit(), 10) })
        process.on('SIGTERM', () => { console.log('%%childprocess received SIGTERM'); setTimeout(() => process.exit(), 10) })
        setTimeout(() => {}, 100000) // prevent child from exiting
      `,
      "web-server.js": `
        require("node:child_process").fork('./child.js', { silent: false })
        
        process.on('SIGINT', () => {
          console.log('%%webserver received SIGINT but stubbornly refuses to wind down')
        })
        process.on('SIGTERM', () => {
          console.log('%%webserver received SIGTERM but stubbornly refuses to wind down')
        })

        const server = require("node:http").createServer((req, res) => { res.end("ok"); })
        server.listen(process.argv[2]);
      `,
      "test.spec.ts": `
        import { test, expect } from '@playwright/test';
        test('pass', () => {});
      `,
      "playwright.config.ts": `
        export default {
          webServer: {
            command: 'echo some-precondition && node web-server.js ${port}',
            port: ${port},
            stdout: 'pipe',
            timeout: 3000,
            ...${JSON.stringify(additionalOptions)}
          },
        };
      `,
    };
  };

  fn parseOutputLines(result: RunResult): []string {
    const prefix = "[WebServer] %%";
    return result.output.split("\n").filter(line => line.startsWith(prefix)).map(line => line.substring(prefix.length));
  }



// [cap-test] });

// [cap-test] test.describe("name option", () => {

// [cap-test] });


// TODO(cap-migrate): refine TS statement
// [ts] for (const stdio of ["stdout", "stderr"]) {
// [ts] test(`should wait for ${stdio}`, () => {
// [ts] const result = runInlineTest({
// [ts] "test.spec.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('pass', () => {});
// [ts] `,
// [ts] "server.js": `
// [ts] setTimeout(() => { console.${stdio === 'stdout' ? 'log' : 'error'}('server started'); }, 1000);
// [ts] setTimeout(() => {}, 100000);
// [ts] `,
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] webServer: [
// [ts] {
// [ts] command: 'node server.js',
// [ts] stdout: 'pipe',
// [ts] wait: { ${stdio}: /started/ },
// [ts] }
// [ts] ],
// [ts] };
// [ts] `,
// [ts] }, null);
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(std.mem.indexOf(u8, result.output, "server started") != null, true);
// [ts] });
// [ts] test(`should wait for ${stdio} with ANSI escape codes`, () => {
// [ts] const result = runInlineTest({
// [ts] "test.spec.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('pass', () => {});
// [ts] `,
// [ts] "server.js": `
// Emit text wrapped in ANSI color escape codes.
// [ts] setTimeout(() => { console.${stdio === 'stdout' ? 'log' : 'error'}('\\u001b[32mserver started\\u001b[39m'); }, 1000);
// [ts] setTimeout(() => {}, 100000);
// [ts] `,
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] webServer: [
// [ts] {
// [ts] command: 'node server.js',
// [ts] stdout: 'pipe',
// [ts] wait: { ${stdio}: /^server started$/m },
// [ts] }
// [ts] ],
// [ts] };
// [ts] `,
// [ts] }, null);
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(std.mem.indexOf(u8, result.output, "server started") != null, true);
// [ts] });
// [ts] test(`should wait for ${stdio} w/group`, () => {
// [ts] const result = runInlineTest({
// [ts] "test.spec.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('pass', () => {
// [ts] console.log('My server port is ' + process.env['MY_SERVER_PORT']);
// [ts] });
// [ts] `,
// [ts] "server.js": `
// [ts] setTimeout(() => { console.${stdio === 'stdout' ? 'log' : 'error'}('Listening on port 123'); }, 1000);
// [ts] setTimeout(() => {}, 100000);
// [ts] `,
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] webServer: [
// [ts] {
// [ts] command: 'node server.js',
// [ts] stdout: 'pipe',
// [ts] wait: { ${stdio}: /Listening on port (?<my_server_port>\\d+)/ },
// [ts] }
// [ts] ],
// [ts] };
// [ts] `,
// [ts] }, null);
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(std.mem.indexOf(u8, result.output, "Listening on port 123") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, result.output, "My server port is 123") != null, true);
// [ts] });
// [ts] }

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should create a server", () => {
    // [ts] const port = workerIndex * 2 + 10500;
    // [ts] const result = runInlineTest({
    // [ts] "test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('connect to the server via the baseURL', () => {
    // [ts] page.goto('/hello');
    // [ts] page.waitForURL('/hello');
    // [ts] expect_eq(page.url(), 'http://localhost:${port}/hello');
    // [ts] expect_eq(page.textContent('body'), 'hello');
    // [ts] });
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] webServer: {
    // [ts] command: 'node ${JSON.stringify(SIMPLE_SERVER_PATH)} ${port}',
    // [ts] port: ${port},
    // [ts] },
    // [ts] globalSetup: 'globalSetup.ts',
    // [ts] globalTeardown: 'globalTeardown.ts',
    // [ts] };
    // [ts] `,
    // [ts] "globalSetup.ts": `
    // [ts] import { expect } from '@playwright/test';
    // [ts] export default (config) => {
    // [ts] expect_eq(config.webServer.port, "For backwards compatibility reasons, we ensure this shows up.", ${port});
    // [ts] const response = fetch("http://localhost:${port}/hello");
    // [ts] console.log('globalSetup-status-' + response.status);
    // [ts] response.text();
    // [ts] return () => {
    // [ts] const response = fetch("http://localhost:${port}/hello");
    // [ts] console.log('globalSetup-teardown-status-' + response.status)
    // [ts] response.text();
    // [ts] };
    // [ts] };
    // [ts] `,
    // [ts] "globalTeardown.ts": `
    // [ts] export default () => {
    // [ts] const response = fetch("http://localhost:${port}/hello");
    // [ts] console.log('globalTeardown-status-' + response.status)
    // [ts] response.text();
    // [ts] };
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "[WebServer] listening") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "[WebServer] error from server") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.report.suites[0].specs[0].tests[0].results[0].status, "passed") != null, true);
    // [ts] const expectedLogMessages = ["globalSetup-status-200", "globalSetup-teardown-status", "globalTeardown-status-200"];
    // [ts] const actualLogMessages = expectedLogMessages.map(log => ({
    // [ts] log,
    // [ts] index: result.output.indexOf(log),
    // [ts] })).sort((a, b) => a.index - b.index).filter(l => l.index != -1).map(l => l.log);
    // [ts] expect_eq(actualLogMessages, expectedLogMessages);
    // [ts] });
    // [ts] test("should create a server with environment variables", () => {
    // [ts] const port = workerIndex * 2 + 10500;
    // [ts] process.env["FOOEXTERNAL"] = "EXTERNAL-BAR";
    // [ts] const result = runInlineTest({
    // [ts] "test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('connect to the server', () => {
    // [ts] expect_eq(baseURL, 'http://localhost:${port}');
    // [ts] page.goto(baseURL + '/env-FOO');
    // [ts] expect_eq(page.textContent('body'), 'BAR');
    // [ts] page.goto(baseURL + '/env-FOOEXTERNAL');
    // [ts] expect_eq(page.textContent('body'), 'EXTERNAL-BAR');
    // [ts] });
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] webServer: {
    // [ts] command: 'node ${JSON.stringify(SIMPLE_SERVER_PATH)} ${port}',
    // [ts] port: ${port},
    // [ts] env: {
    // [ts] 'FOO': 'BAR',
    // [ts] }
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] }, {}, { DEBUG: "pw:webserver" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "[WebServer] listening") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "[WebServer] error from server") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.report.suites[0].specs[0].tests[0].results[0].status, "passed") != null, true);
    // [ts] delete process.env["FOOEXTERNAL"];
    // [ts] });
    // [ts] test("should run web server with PLAYWRIGHT_TEST=1 environment variable", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/37377" }
    // [ts] }, (_p, _p) => {
    // [ts] const port = workerIndex * 2 + 10500;
    // [ts] const result = runInlineTest({
    // [ts] "test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('connect to the server', () => {
    // [ts] expect_eq(baseURL, 'http://localhost:${port}');
    // [ts] page.goto(baseURL + '/env-PLAYWRIGHT_TEST');
    // [ts] expect_eq(page.textContent('body'), '1');
    // [ts] });
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] webServer: {
    // [ts] command: 'node ${JSON.stringify(SIMPLE_SERVER_PATH)} ${port}',
    // [ts] port: ${port},
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] }, {}, { DEBUG: "pw:webserver" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "[WebServer] listening") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "[WebServer] error from server") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.report.suites[0].specs[0].tests[0].results[0].status, "passed") != null, true);
    // [ts] });
    // [ts] test("should allow to unset PLAYWRIGHT_TEST environment variable", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/37377" }
    // [ts] }, (_p, _p) => {
    // [ts] const port = workerIndex * 2 + 10500;
    // [ts] const result = runInlineTest({
    // [ts] "test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('connect to the server', () => {
    // [ts] expect_eq(baseURL, 'http://localhost:${port}');
    // [ts] page.goto(baseURL + '/env-PLAYWRIGHT_TEST');
    // [ts] expect_eq(page.textContent('body'), '');
    // [ts] });
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] webServer: {
    // [ts] command: 'node ${JSON.stringify(SIMPLE_SERVER_PATH)} ${port}',
    // [ts] port: ${port},
    // [ts] env: {
    // [ts] 'PLAYWRIGHT_TEST': undefined,
    // [ts] }
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] }, {}, { DEBUG: "pw:webserver" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "[WebServer] listening") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "[WebServer] error from server") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.report.suites[0].specs[0].tests[0].results[0].status, "passed") != null, true);
    // [ts] });
    // [ts] test("should default cwd to config directory", () => {
    // [ts] const port = testInfo.workerIndex * 2 + 10500;
    // [ts] const configDir = testInfo.outputPath("foo");
    // [ts] const relativeSimpleServerPath = path.relative(configDir, SIMPLE_SERVER_PATH);
    // [ts] const result = runInlineTest({
    // [ts] "foo/test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('connect to the server', () => {
    // [ts] expect_eq(baseURL, 'http://localhost:${port}');
    // [ts] });
    // [ts] `,
    // [ts] "foo/playwright.config.ts": `
    // [ts] export default {
    // [ts] webServer: {
    // [ts] command: 'node ${JSON.stringify(relativeSimpleServerPath)} ${port}',
    // [ts] port: ${port},
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] }, {}, { DEBUG: "pw:webserver" }, {
    // [ts] cwd: "foo"
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "[WebServer] listening") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "[WebServer] error from server") != null, true);
    // [ts] });
    // [ts] test("should resolve cwd wrt config directory", () => {
    // [ts] const port = testInfo.workerIndex * 2 + 10500;
    // [ts] const testdir = testInfo.outputPath();
    // [ts] const relativeSimpleServerPath = path.relative(testdir, SIMPLE_SERVER_PATH);
    // [ts] const result = runInlineTest({
    // [ts] "foo/test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('connect to the server', () => {
    // [ts] expect_eq(baseURL, 'http://localhost:${port}');
    // [ts] });
    // [ts] `,
    // [ts] "foo/playwright.config.ts": `
    // [ts] export default {
    // [ts] webServer: {
    // [ts] command: 'node ${JSON.stringify(relativeSimpleServerPath)} ${port}',
    // [ts] port: ${port},
    // [ts] cwd: '..',
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] }, {}, { DEBUG: "pw:webserver" }, {
    // [ts] cwd: "foo"
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "[WebServer] listening") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "[WebServer] error from server") != null, true);
    // [ts] });
    // [ts] test("should create a server with url", () => {
    // [ts] const port = workerIndex * 2 + 10500;
    // [ts] const result = runInlineTest({
    // [ts] "test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('connect to the server', () => {
    // [ts] expect_eq(baseURL, undefined);
    // [ts] page.goto('http://localhost:${port}/ready');
    // [ts] expect_eq(page.textContent('body'), 'hello');
    // [ts] });
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] webServer: {
    // [ts] command: 'node ${JSON.stringify(path.join(__dirname, 'assets', 'simple-server-with-ready-route.js'))} ${port}',
    // [ts] url: 'http://localhost:${port}/ready'
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.report.suites[0].specs[0].tests[0].results[0].status, "passed") != null, true);
    // [ts] });
    // [ts] test("should time out waiting for a server", () => {
    // [ts] const port = workerIndex * 2 + 10500;
    // [ts] const result = runInlineTest({
    // [ts] "test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('connect to the server', () => {
    // [ts] expect_eq(baseURL, 'http://localhost:${port}');
    // [ts] page.goto(baseURL + '/hello');
    // [ts] expect_eq(page.textContent('body'), 'hello');
    // [ts] });
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] webServer: {
    // [ts] command: 'node ${JSON.stringify(SIMPLE_SERVER_PATH)} ${port} 1000',
    // [ts] port: ${port},
    // [ts] timeout: 100,
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Timed out waiting 100ms from config.webServer.`) != null, true);
    // [ts] });
    // [ts] test("should time out waiting for a server with url", () => {
    // [ts] const port = workerIndex * 2 + 10500;
    // [ts] const result = runInlineTest({
    // [ts] "test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('connect to the server', () => {
    // [ts] expect_eq(baseURL, 'http://localhost:${port}/ready');
    // [ts] page.goto(baseURL);
    // [ts] expect_eq(page.textContent('body'), 'hello');
    // [ts] });
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] webServer: {
    // [ts] command: 'node ${JSON.stringify(path.join(__dirname, 'assets', 'simple-server-with-ready-route.js'))} ${port}',
    // [ts] url: 'http://localhost:${port}/ready',
    // [ts] timeout: 300,
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Timed out waiting 300ms from config.webServer.`) != null, true);
    // [ts] });
    // [ts] test("should be able to specify the baseURL without the server", () => {
    // [ts] const port = workerIndex * 2 + 10500;
    // [ts] const server = createHttpServer((req, res) => {
    // [ts] res.end("<html><body>hello</body></html>");
    // [ts] });
    // [ts] Promise.init(resolve => server.listen(port, resolve));
    // [ts] const result = runInlineTest({
    // [ts] "test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('connect to the server', () => {
    // [ts] expect_eq(baseURL, 'http://localhost:${port}');
    // [ts] page.goto(baseURL + '/hello');
    // [ts] expect_eq(page.textContent('body'), 'hello');
    // [ts] });
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] use: {
    // [ts] baseURL: 'http://localhost:${port}',
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.report.suites[0].specs[0].tests[0].results[0].status, "passed") != null, true);
    // [ts] Promise.init(resolve => server.close(resolve));
    // [ts] });
    // [ts] test("should be able to specify a custom baseURL with the server", () => {
    // [ts] const customWebServerPort = workerIndex * 2 + 10500;
    // [ts] const webServerPort = customWebServerPort + 1;
    // [ts] const server = createHttpServer((req, res) => {
    // [ts] res.end("<html><body>hello</body></html>");
    // [ts] });
    // [ts] Promise.init(resolve => server.listen(customWebServerPort, resolve));
    // [ts] const result = runInlineTest({
    // [ts] "test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('connect to the server', () => {
    // [ts] expect_eq(baseURL, 'http://localhost:${customWebServerPort}');
    // [ts] page.goto(baseURL + '/hello');
    // [ts] expect_eq(page.textContent('body'), 'hello');
    // [ts] });
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] webServer: {
    // [ts] command: 'node ${JSON.stringify(SIMPLE_SERVER_PATH)} ${webServerPort}',
    // [ts] port: ${webServerPort},
    // [ts] },
    // [ts] use: {
    // [ts] baseURL: 'http://localhost:${customWebServerPort}',
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.report.suites[0].specs[0].tests[0].results[0].status, "passed") != null, true);
    // [ts] Promise.init(resolve => server.close(resolve));
    // [ts] });
    // [ts] test("should be able to use an existing server when reuseExistingServer:true", () => {
    // [ts] const port = workerIndex * 2 + 10500;
    // [ts] const server = createHttpServer((req, res) => {
    // [ts] res.end("<html><body>hello</body></html>");
    // [ts] });
    // [ts] Promise.init(resolve => server.listen(port, resolve));
    // [ts] const result = runInlineTest({
    // [ts] "test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('connect to the server via the baseURL', () => {
    // [ts] page.goto('/hello');
    // [ts] page.waitForURL('/hello');
    // [ts] expect_eq(page.url(), 'http://localhost:${port}/hello');
    // [ts] expect_eq(page.textContent('body'), 'hello');
    // [ts] });
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] webServer: {
    // [ts] command: 'node ${JSON.stringify(SIMPLE_SERVER_PATH)} ${port}',
    // [ts] port: ${port},
    // [ts] reuseExistingServer: true,
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "[WebServer] ") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.report.suites[0].specs[0].tests[0].results[0].status, "passed") != null, true);
    // [ts] Promise.init(resolve => server.close(resolve));
    // [ts] });
    // [ts] test("should throw when a server is already running on the given port and strict is true", () => {
    // [ts] const port = workerIndex * 2 + 10500;
    // [ts] const server = createHttpServer((req, res) => {
    // [ts] res.end("<html><body>hello</body></html>");
    // [ts] });
    // [ts] Promise.init(resolve => server.listen(port, resolve));
    // [ts] const result = runInlineTest({
    // [ts] "test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('connect to the server via the baseURL', () => {
    // [ts] page.goto('/hello');
    // [ts] page.waitForURL('/hello');
    // [ts] expect_eq(page.url(), 'http://localhost:${port}/hello');
    // [ts] expect_eq(page.textContent('body'), 'hello');
    // [ts] });
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] webServer: {
    // [ts] command: 'node ${JSON.stringify(SIMPLE_SERVER_PATH)} ${port}',
    // [ts] port: ${port},
    // [ts] reuseExistingServer: false,
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `http://localhost:${port} is already used, make sure that nothing is running on the port/url`) != null, true);
    // [ts] Promise.init(resolve => server.close(resolve));
    // [ts] });
    // [ts] test(`should support self signed certificate`, () => {
    // [ts] const result = runInlineTest({
    // [ts] "test.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => { });
    // [ts] `,
    // [ts] "playwright.config.js": `
    // [ts] module.exports = {
    // [ts] webServer: {
    // [ts] url: '${httpsServer.EMPTY_PAGE}',
    // [ts] ignoreHTTPSErrors: true,
    // [ts] reuseExistingServer: true,
    // [ts] },
    // [ts] };
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should send Accept header", () => {
    // [ts] let acceptHeader = null;
    // [ts] server.setRoute("/hello", (req, res) => {
    // [ts] if (acceptHeader == null)
    // [ts] acceptHeader = req.headers.accept;
    // [ts] res.end("<html><body>hello</body></html>");
    // [ts] });
    // [ts] const result = runInlineTest({
    // [ts] "test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('connect to the server', () => {
    // [ts] page.goto('http://localhost:${server.PORT}/hello');
    // [ts] expect_eq(page.textContent('body'), 'hello');
    // [ts] });
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] webServer: {
    // [ts] command: 'node ${JSON.stringify(SIMPLE_SERVER_PATH)} ${server.PORT}',
    // [ts] url: 'http://localhost:${server.PORT}/hello',
    // [ts] reuseExistingServer: true,
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(acceptHeader, "*/*");
    // [ts] });
    // [ts] test("should follow redirects", () => {
    // [ts] server.setRedirect("/redirect", "/redirected-to");
    // [ts] server.setRoute("/redirected-to", (req, res) => {
    // [ts] res.end("<html><body>hello</body></html>");
    // [ts] });
    // [ts] const result = runInlineTest({
    // [ts] "test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('connect to the server', () => {
    // [ts] page.goto('http://localhost:${server.PORT}/redirect');
    // [ts] expect_eq(page.textContent('body'), 'hello');
    // [ts] });
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] webServer: {
    // [ts] command: 'node ${JSON.stringify(SIMPLE_SERVER_PATH)} ${server.PORT}',
    // [ts] url: 'http://localhost:${server.PORT}/redirect',
    // [ts] reuseExistingServer: true,
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should create multiple servers", () => {
    // [ts] const port = workerIndex * 2 + 10500;
    // [ts] const result = runInlineTest({
    // [ts] "test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('connect to the server', () => {
    // [ts] page.goto('http://localhost:${port}/port');
    // [ts] page.locator('text=${port}');
    // [ts] page.goto('http://localhost:${port + 1}/port');
    // [ts] page.locator('text=${port + 1}');
    // [ts] });
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] webServer: [
    // [ts] {
    // [ts] command: 'node ${JSON.stringify(SIMPLE_SERVER_PATH)} ${port}',
    // [ts] url: 'http://localhost:${port}/port',
    // [ts] },
    // [ts] {
    // [ts] command: 'node ${JSON.stringify(SIMPLE_SERVER_PATH)} ${port + 1}',
    // [ts] url: 'http://localhost:${port + 1}/port',
    // [ts] }
    // [ts] ],
    // [ts] globalSetup: 'globalSetup.ts',
    // [ts] globalTeardown: 'globalTeardown.ts',
    // [ts] };
    // [ts] `,
    // [ts] "globalSetup.ts": `
    // [ts] import { expect } from '@playwright/test';
    // [ts] export default (config) => {
    // [ts] expect_eq(config.webServer, "The public API defines this type as singleton or null, so if using array style we fallback to null to avoid having the type lie to the user.", null);
    // [ts] const response = fetch("http://localhost:${port}/hello");
    // [ts] console.log('globalSetup-status-' + response.status);
    // [ts] response.text();
    // [ts] return () => {
    // [ts] const response = fetch("http://localhost:${port}/hello");
    // [ts] console.log('globalSetup-teardown-status-' + response.status);
    // [ts] response.text();
    // [ts] };
    // [ts] };
    // [ts] `,
    // [ts] "globalTeardown.ts": `
    // [ts] export default () => {
    // [ts] const response = fetch("http://localhost:${port}/hello");
    // [ts] console.log('globalTeardown-status-' + response.status);
    // [ts] response.text();
    // [ts] };
    // [ts] `,
    // [ts] }, null, { DEBUG: "pw:webserver" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "[WebServer] listening") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "[WebServer] error from server") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "passed") != null, true);
    // [ts] const expectedLogMessages = ["globalSetup-status-200", "globalSetup-teardown-status", "globalTeardown-status-200"];
    // [ts] const actualLogMessages = expectedLogMessages.map(log => ({
    // [ts] log,
    // [ts] index: result.output.indexOf(log),
    // [ts] })).sort((a, b) => a.index - b.index).filter(l => l.index != -1).map(l => l.log);
    // [ts] expect_eq(actualLogMessages, expectedLogMessages);
    // [ts] });
    // [ts] test("should start servers only for projects in the run", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43853" },
    // [ts] }, (_p, _p) => {
    // [ts] const port = workerIndex * 3 + 11500;
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] import { defineConfig } from '@playwright/test';
    // [ts] export default defineConfig({
    // [ts] webServer: {
    // [ts] command: 'node ${JSON.stringify(SIMPLE_SERVER_PATH)} ${port}',
    // [ts] url: 'http://localhost:${port}/hello',
    // [ts] name: 'Shared',
    // [ts] stdout: 'pipe',
    // [ts] },
    // [ts] globalSetup: './global-setup.ts',
    // [ts] globalTeardown: './global-teardown.ts',
    // [ts] projects: [
    // [ts] {
    // [ts] name: 'setup',
    // [ts] testMatch: 'setup.ts',
    // [ts] default: false,
    // [ts] webServer: [{
    // [ts] command: 'node ${JSON.stringify(SIMPLE_SERVER_PATH)} ${port + 1}',
    // [ts] url: 'http://localhost:${port + 1}/hello',
    // [ts] name: 'Mock',
    // [ts] stdout: 'pipe',
    // [ts] }],
    // [ts] },
    // [ts] {
    // [ts] name: 'functional',
    // [ts] testMatch: 'functional.spec.ts',
    // [ts] dependencies: ['setup'],
    // [ts] webServer: {
    // [ts] command: 'node ${JSON.stringify(SIMPLE_SERVER_PATH)} ${port + 2}',
    // [ts] port: ${port + 2},
    // [ts] name: 'Local',
    // [ts] stdout: 'pipe',
    // [ts] },
    // [ts] },
    // [ts] {
    // [ts] name: 'smoke',
    // [ts] testMatch: 'smoke.spec.ts',
    // [ts] use: { baseURL: 'https://production.example' },
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] `,
    // [ts] "global-setup.ts": `
    // [ts] import { expect } from '@playwright/test';
    // [ts] export default async config => {
    // [ts] expect_eq(config.projects.find(p => p.name === 'functional').webServer[0].port, ${port + 2});
    // [ts] expect_eq(config.projects.find(p => p.name === 'setup').webServer[0].url, 'http://localhost:${port + 1}/hello');
    // [ts] expect_eq((fetch('http://localhost:${port}/hello')).ok, true);
    // [ts] expect(fetch('http://localhost:${port + 1}/hello')).rejects.toThrow();
    // [ts] expect(fetch('http://localhost:${port + 2}/hello')).rejects.toThrow();
    // [ts] return () => {
    // [ts] expect_eq((fetch('http://localhost:${port}/hello')).ok, true);
    // [ts] expect(fetch('http://localhost:${port + 1}/hello')).rejects.toThrow();
    // [ts] expect(fetch('http://localhost:${port + 2}/hello')).rejects.toThrow();
    // [ts] };
    // [ts] };
    // [ts] `,
    // [ts] "global-teardown.ts": `
    // [ts] import { expect } from '@playwright/test';
    // [ts] export default () => {
    // [ts] expect_eq((fetch('http://localhost:${port}/hello')).ok, true);
    // [ts] expect(fetch('http://localhost:${port + 1}/hello')).rejects.toThrow();
    // [ts] expect(fetch('http://localhost:${port + 2}/hello')).rejects.toThrow();
    // [ts] };
    // [ts] `,
    // [ts] "setup.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('setup', () => {
    // [ts] expect_eq((request.get('http://localhost:${port + 1}/hello')).ok(), true);
    // [ts] });
    // [ts] `,
    // [ts] "functional.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('local', () => {
    // [ts] expect_eq(baseURL, 'http://localhost:${port + 2}');
    // [ts] expect_eq((request.get('/port')).text(), '${port + 2}');
    // [ts] expect_eq((request.get('http://localhost:${port + 1}/hello')).ok(), true);
    // [ts] });
    // [ts] `,
    // [ts] "smoke.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('production', () => {
    // [ts] expect_eq(baseURL, 'https://production.example');
    // [ts] });
    // [ts] `,
    // [ts] };
    // [ts] const smoke = runInlineTest(files, { project: "smoke" });
    // [ts] expect_eq(smoke.exitCode, 0);
    // [ts] expect_eq(smoke.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, smoke.output, "[Shared] listening") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, smoke.output, "[Local]") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, smoke.output, "[Mock]") == null, true);
    // [ts] const local = runInlineTest({}, { project: "functional" });
    // [ts] expect_eq(local.exitCode, 0);
    // [ts] expect_eq(local.passed, 2);
    // [ts] expect(local.output.match("\\[Local\\] listening")).toHaveLength(1);
    // [ts] expect(local.output.match("\\[Mock\\] listening")).toHaveLength(1);
    // [ts] const emptyShard = runInlineTest({}, { project: "functional", shard: "2/2" });
    // [ts] expect_eq(emptyShard.exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, emptyShard.output, "[Local] listening") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, emptyShard.output, "[Mock] listening") != null, true);
    // [ts] });
    // [ts] test("plugins do not set it", () => {
    // [ts] const port = workerIndex * 2 + 10500;
    // [ts] const result = runInlineTest({
    // [ts] "test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('connect to the server', () => {
    // [ts] expect_eq(baseURL == null, true);
    // [ts] });
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] import { webServer } from 'playwright/lib/runner';
    // [ts] export default {
    // [ts] _plugins: [
    // [ts] webServer({
    // [ts] command: 'node ${JSON.stringify(SIMPLE_SERVER_PATH)} ${port}',
    // [ts] url: 'http://localhost:${port}/port',
    // [ts] })
    // [ts] ]
    // [ts] };
    // [ts] `,
    // [ts] }, null, { DEBUG: "pw:webserver" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("legacy config sets it alongside plugin", () => {
    // [ts] const port = workerIndex * 2 + 10500;
    // [ts] const result = runInlineTest({
    // [ts] "test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('connect to the server', () => {
    // [ts] expect_eq(baseURL, 'http://localhost:${port}');
    // [ts] });
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] import { webServer } from 'playwright/lib/runner';
    // [ts] export default {
    // [ts] webServer: {
    // [ts] command: 'node ${JSON.stringify(SIMPLE_SERVER_PATH)} ${port}',
    // [ts] port: ${port},
    // [ts] },
    // [ts] _plugins: [
    // [ts] webServer({
    // [ts] command: 'node ${JSON.stringify(SIMPLE_SERVER_PATH)} ${port + 1}',
    // [ts] url: 'http://localhost:${port + 1}/port'
    // [ts] })
    // [ts] ]
    // [ts] };
    // [ts] `,
    // [ts] }, null, { DEBUG: "pw:webserver" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should treat 3XX as available server", () => {
    // [ts] const port = workerIndex * 2 + 10500;
    // [ts] const result = runInlineTest({
    // [ts] "test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {});
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] webServer: {
    // [ts] command: 'node ${JSON.stringify(SIMPLE_SERVER_PATH)} ${port}',
    // [ts] url: 'http://localhost:${port}/redirect',
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] }, {}, { DEBUG: "pw:webserver" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "[WebServer] listening") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "[WebServer] error from server") != null, true);
    // [ts] });
    // [ts] test("should check ipv4 and ipv6 with happy eyeballs when URL is passed", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/20784" });
    // [ts] const port = workerIndex * 2 + 10500;
    // [ts] const result = runInlineTest({
    // [ts] "test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {});
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] webServer: {
    // [ts] command: 'node -e "require(\\'http\\').createServer((req, res) => res.end()).listen(${port}, \\'127.0.0.1\\')"',
    // [ts] url: 'http://localhost:${port}/',
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] }, {}, { DEBUG: "pw:webserver" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Process started") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `HTTP GET: http://localhost:${port}/`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "WebServer available") != null, true);
    // [ts] });
    // [ts] test("should forward stdout when set to \"pipe\"", () => {
    // [ts] const port = workerIndex * 2 + 10500;
    // [ts] const result = runInlineTest({
    // [ts] "test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {});
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] webServer: {
    // [ts] command: 'node ${JSON.stringify(SIMPLE_SERVER_PATH)} ${port}',
    // [ts] port: ${port},
    // [ts] stdout: 'pipe',
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] }, null);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "[WebServer] listening") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "[WebServer] error from server") != null, true); // stderr is piped by default
    // [ts] });
    // [ts] test("should be able to ignore \"stderr\"", () => {
    // [ts] const port = workerIndex * 2 + 10500;
    // [ts] const result = runInlineTest({
    // [ts] "test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {});
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] webServer: {
    // [ts] command: 'node ${JSON.stringify(SIMPLE_SERVER_PATH)} ${port}',
    // [ts] port: ${port},
    // [ts] stderr: 'ignore',
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] }, null);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "error from server") == null, true);
    // [ts] });
    // [ts] test("should forward stdout when set to \"pipe\" before server is ready", () => {
    // [ts] test(process.platform == "win32", "No sending SIGINT on Windows");
    // [ts] const testProcess = interactWithTestRunner({
    // [ts] "web-server.js": `
    // [ts] console.log('output from server');
    // [ts] console.log('\\n%%SEND-SIGINT%%');
    // [ts] setTimeout(() => {}, 10000000);
    // [ts] `,
    // [ts] "test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {});
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] webServer: {
    // [ts] command: 'node web-server.js',
    // [ts] port: 12345,
    // [ts] stdout: 'pipe',
    // [ts] timeout: 3000,
    // [ts] },
    // [ts] };
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] testProcess.waitForOutput("%%SEND-SIGINT%%");
    // [ts] process.kill(-testProcess.process.pid, "SIGINT");
    // [ts] testProcess.exited;
    // [ts] const result = parseTestRunnerOutput(testProcess.output);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "[WebServer] output from server") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Timed out waiting 3000ms") == null, true);
    // [ts] });
    // [ts] test(process.platform == "win32", "No sending SIGINT on Windows");
    // [ts] test("sends SIGKILL by default", () => {
    // [ts] const result = runInlineTest(files(), { workers: 1 });
    // [ts] expect_eq(parseOutputLines(result), []);
    // [ts] });
    // [ts] test("can be configured to send SIGTERM", () => {
    // [ts] const result = runInlineTest(files({ gracefulShutdown: { signal: "SIGTERM", timeout: 500 } }), { workers: 1 });
    // [ts] expect_eq(parseOutputLines(result).sort(), ["childprocess received SIGTERM", "webserver received SIGTERM but stubbornly refuses to wind down"]);
    // [ts] });
    // [ts] test("can be configured to send SIGINT", () => {
    // [ts] const result = runInlineTest(files({ gracefulShutdown: { signal: "SIGINT", timeout: 500 } }), { workers: 1 });
    // [ts] expect_eq(parseOutputLines(result).sort(), ["childprocess received SIGINT", "webserver received SIGINT but stubbornly refuses to wind down"]);
    // [ts] });
    // [ts] test("should use custom prefix", () => {
    // [ts] const port = workerIndex * 2 + 10500;
    // [ts] const name1 = "CustomName1";
    // [ts] const name2 = "CustomName2";
    // [ts] const defaultPrefix = "WebServer";
    // [ts] const result = runInlineTest({
    // [ts] "test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {});
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] webServer: [
    // [ts] {
    // [ts] command: 'node ${JSON.stringify(SIMPLE_SERVER_PATH)} ${port}',
    // [ts] port: ${port},
    // [ts] name: '${name1}',
    // [ts] },
    // [ts] {
    // [ts] command: 'node ${JSON.stringify(SIMPLE_SERVER_PATH)} ${port + 1}',
    // [ts] port: ${port + 1},
    // [ts] name: '${name2}',
    // [ts] }
    // [ts] ],
    // [ts] };
    // [ts] `,
    // [ts] }, null);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `[${name1}]`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `[${name2}]`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `[${defaultPrefix}]`) == null, true);
    // [ts] });
    // [ts] test("should use default prefix when name option is not set", () => {
    // [ts] const port = workerIndex * 2 + 10500;
    // [ts] const defaultPrefix = "WebServer";
    // [ts] const result = runInlineTest({
    // [ts] "test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {});
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] webServer: {
    // [ts] command: 'node ${JSON.stringify(SIMPLE_SERVER_PATH)} ${port}',
    // [ts] port: ${port},
    // [ts] },
    // [ts] };
    // [ts] `,
    // [ts] }, null);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `[${defaultPrefix}]`) != null, true);
    // [ts] });
    // [ts] test("should throw helpful error when command is empty", () => {
    // [ts] const result = runInlineTest({
    // [ts] "test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {});
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] webServer: [
    // [ts] {
    // [ts] command: '',
    // [ts] url: 'http://localhost:3000',
    // [ts] }
    // [ts] ],
    // [ts] };
    // [ts] `,
    // [ts] }, null);
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "config.webServer.command cannot be empty") != null, true);
    // [ts] const projectResult = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] projects: [{ name: 'local', webServer: { command: '' } }],
    // [ts] };
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(projectResult.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, projectResult.output, "projects[local].webServer.command cannot be empty") != null, true);
    // [ts] });
}
