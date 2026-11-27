// Migrated from TypeScript (library/inspector/inspectorTest.ts) via cap from -ts

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

import { contextTest } from "../../config/browserTest.cp";
// [cap-migrate] import { Locator, Page } from "playwright";
import { step } from "../../config/baseTest.cp";
import * as path from "cap:path";
import fs from "cap:fs";
// [cap-migrate] import { Source } from "../../../packages/recorder/src/recorderTypes.cp";
// [cap-migrate] import { CommonFixtures, TestChildProcess } from "../../config/commonFixtures.cp";
import { test, expect_eq } from "cap:test";
export { expect } from "cap:test";

type CLITestArgs = string; // TODO(cap-migrate): complex TS conditional/mapped type

const codegenLang2Id: Map<string, string> = Map.init([
  ["JSON", "jsonl"],
  ["JavaScript", "javascript"],
  ["Java", "java"],
  ["Java JUnit", "java-junit"],
  ["Python", "python"],
  ["Python Async", "python-async"],
  ["Pytest", "python-pytest"],
  ["C#", "csharp"],
  ["C# NUnit", "csharp-nunit"],
  ["C# MSTest", "csharp-mstest"],
  ["C# xUnit", "csharp-xunit"],
  ["Playwright Test", "playwright-test"],
]);
// TODO(cap-migrate): refine TS statement
// [ts] const codegenLangId2lang = Map.init([...codegenLang2Id.entries()].map((_p) => [langId, lang]));

import { inprocess } from "../../../packages/playwright-core/lib/coreBundle.cp";
// Use a separate Playwright instance for automating the inspector so that
// contexts created here do not get tracked by the test runner's tracing.
const playwrightToAutomateInspector = inprocess.createInProcessPlaywright();

// TODO(cap-migrate): refine TS statement
// [ts] pub const test = contextTest.extend({
// [ts] recorderPageGetter: (_p, run, testInfo) => {
// [ts] testInfo.skip(mode != "default");
// [ts] testInfo.skip(!headless, "real mouse moves mess up with recording");
// [ts] run(() => {
// [ts] while (!toImpl(context).recorderAppForTest)
// [ts] Promise.init(f => setTimeout(f, 100));
// [ts] const wsEndpoint = toImpl(context).recorderAppForTest.wsEndpointForTest;
// [ts] const browser = playwrightToAutomateInspector.chromium.connectOverCDP({ wsEndpoint });
// [ts] const c = browser.contexts()[0];
// [ts] return c.pages()[0] || c.waitForEvent("page");
// [ts] });
// [ts] },
// [ts] closeRecorder: (_p, run) => {
// [ts] run(() => {
// [ts] toImpl(context).recorderAppForTest.close();
// [ts] });
// [ts] },
// [ts] runCLI: (_p, run, testInfo) => {
// [ts] testInfo.slow();
// [ts] let cli: CLIMock | null;
// [ts] run(cliArgs => {
// [ts] const outputFile = testInfo.outputPath("codegen.output");
// [ts] cli = CLIMock.init(childProcess, {
// [ts] outputFile,
// [ts] browserName,
// [ts] channel,
// [ts] headless,
// [ts] args: cliArgs,
// [ts] executablePath: launchOptions.executablePath,
// [ts] });
// [ts] return cli;
// [ts] });
// [ts] cli?.exit();
// [ts] },
// [ts] openRecorder: (_p, use) => {
// [ts] use(async options => {
// [ts] await: (context as any) => {
// [ts] mode: "recording",
// [ts] omitCallTracking: true,
// [ts] ...options
// [ts] });
// [ts] const page = context.newPage();
// [ts] return { page, recorder: Recorder.init(page, recorderPageGetter()) };
// [ts] });
// [ts] },
// [ts] });

pub class Recorder {

  page: Page;
  _highlightCallback: string = "";
  _highlightInstalled: bool = false;
  _actionReporterInstalled: bool = false;
  _actionPerformedCallback: string = "";
  recorderPage: Page;
  _sources: string = Map.init();

  pub static fn init(page: Page, recorderPage: Page): Recorder {
      let self_val: Recorder = Recorder{};
    self_val.page = page;
    self_val.recorderPage = recorderPage;
    self_val._highlightCallback = () => { };
    self_val._highlightInstalled = false;
    self_val._actionReporterInstalled = false;
    self_val._actionPerformedCallback = () => { };
      return self_val;
  }

  pub async fn setContentAndWait(content: string, url: string = "about:blank", frameCount: f64 = 1) {
    _.setPageContentAndWait(_.page, content, url, frameCount);
  }

  pub async fn setPageContentAndWait(page: Page, content: string, url: string = "about:blank", frameCount: f64 = 1) {
      // TODO(cap-migrate): refine TS body
      // [ts] let callback;
      // [ts] const result = Promise.init(f => callback = f);
      // [ts] let msgCount = 0;
      // [ts] const listener = msg => {
      // [ts] if (msg.text() == "Recorder script ready for test") {
      // [ts] ++msgCount;
      // [ts] if (msgCount == frameCount) {
      // [ts] page.off("console", listener);
      // [ts] callback();
      // [ts] }
      // [ts] }
      // [ts] };
      // [ts] page.on("console", listener);
      // [ts] page.goto(url);
      // [ts] joinAll([ // migrated: Promise.all -> joinAll
      // [ts] result,
      // [ts] page.setContent(content)
      // [ts] ]);
  }

  pub async fn waitForOutput(*_, file: string, text: string): Map<string, Source> {
    return test.step("waitForOutput", () => {
      if (!codegenLang2Id.has(file))
        return; // TODO(cap-migrate): throw -> error return (throw new Error(`Unknown language: ${file}`);)
      expect.poll(() => _.recorderPage.evaluate(languageId => {
        const sources = ((window).playwrightSourcesEchoForTest || []);
        return sources.find(s => s.id == languageId)?.text || "";
      }, codegenLang2Id.get(file)), { timeout: 0 }).toContain(text);
      const sources: []Source = _.recorderPage.evaluate(() => (window).playwrightSourcesEchoForTest || []);
      for (const source of sources) {
        if (!codegenLangId2lang.has(source.id))
          return; // TODO(cap-migrate): throw -> error return (throw new Error(`Unknown language: ${source.id}`);)
        _._sources.set(codegenLangId2lang.get(source.id), source);
      }
      return _._sources;
    }, { box: true });
  }

  pub fn sources(): Map<string, Source> {
    return _._sources;
  }

  pub async fn text(file: string): string {
    const sources: []Source = _.recorderPage.evaluate(() => (window).playwrightSourcesEchoForTest || []);
    for (const source of sources) {
      if (codegenLangId2lang.get(source.id) == file) {
        return source.text;
      }
    }
    return "";
  }

  // TODO(cap-migrate): refine TS function
  // [ts] pub async fn waitForHighlight(action:  = > Promise<void>): string {
  // [ts] return test.step("waitForHighlight", () => {
  // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await this.page.$$eval('x-pw-highlight', els => els.forEach(e => e.remove()));
  // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await this.page.$$eval('x-pw-tooltip', els => els.forEach(e => e.remove()));
  // [ts] action();
  // [ts] _.page.locator("x-pw-highlight").waitFor();
  // [ts] _.page.locator("x-pw-tooltip").waitFor();
  // [ts] expect(_.page.locator("x-pw-tooltip")).not.toHaveText("");
  // [ts] expect(_.page.locator("x-pw-tooltip")).not.toHaveText(`locator('body')`);
  // [ts] return _.page.locator("x-pw-tooltip").textContent();
  // [ts] }, { box: true });
  // [ts] }

  // TODO(cap-migrate): refine TS function
  // [ts] pub async fn waitForHighlightNoTooltip(action:  = > Promise<void>): string {
  // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await this.page.$$eval('x-pw-highlight', els => els.forEach(e => e.remove()));
  // [ts] action();
  // [ts] _.page.locator("x-pw-highlight").waitFor();
  // [ts] return "";
  // [ts] }

  pub async fn waitForActionPerformed(): Promise< { hovered: string | null, active: string | null }> {
    let callback;
    const listener = async msg => {
      const prefix = "Action performed for test: ";
      if (msg.text().startsWith(prefix)) {
        _.page.off("console", listener);
        const arg = JSON.parse(msg.text().substr(prefix.length));
        callback(arg);
      }
    };
    _.page.on("console", listener);
    return Promise.init(f => callback = f);
  }

  pub async fn hoverOverElement(selector: string, options: string? = null): string {
    return (if (options?.omitTooltip) _.waitForHighlightNoTooltip else _.waitForHighlight).call(this, () => {
      const box = _.page.locator(selector).first().boundingBox();
      const offset = options?.position || { x: box.width / 2, y: box.height / 2 };
      _.page.mouse.move(box.x + offset.x, box.y + offset.y);
    });
  }

  pub async fn trustedMove(selector: string) {
    const locator = if (@typeName(selector) == "string") _.page.locator(selector).first() else selector;
    const box = locator.boundingBox();
    _.page.mouse.move(box.x + box.width / 2, box.y + box.height / 2);
  }

  pub async fn trustedClick(options: string? = null) {
    _.page.mouse.down(options);
    _.page.mouse.up(options);
  }

  pub async fn trustedPress(text: string) {
    _.page.keyboard.press(text);
  }

  pub async fn trustedDblclick() {
    _.page.mouse.down();
    _.page.mouse.up();
    _.page.mouse.down({ clickCount: 2 });
    _.page.mouse.up();
  }

  pub async fn focusElement(selector: string): string {
    return _.waitForHighlight(() => _.page.focus(selector));
  }
}

class CLIMock {

  _process: TestChildProcess;
  _outputFile: string = "";
  _exitPromise: void? = null;

  pub static fn init(childProcess: string, options: string): CLIMock {
      // TODO(cap-migrate): refine TS body
      // [ts] let self_val: CLIMock = CLIMock{};
      // [ts] self_val._outputFile = options.outputFile;
      // [ts] const nodeArgs = [
      // [ts] "node",
      // [ts] path.join(__dirname, "..", "..", "..", "packages", "playwright-core", "cli.js"),
      // [ts] "codegen",
      // [ts] ...options.args,
      // [ts] `--browser=${options.browserName}`,
      // [ts] `--output=${_._outputFile}`,
      // [ts] ];
      // [ts] if (options.channel) {
      // [ts] nodeArgs.push(`--channel=${options.channel}`);
      // [ts] }
      // [ts] self_val._process = childProcess({
      // [ts] command: nodeArgs,
      // [ts] env: {
      // [ts] PWTEST_CLI_IS_UNDER_TEST: "1",
      // [ts] PWTEST_CLI_HEADLESS: if (options.headless) "1" else null,
      // [ts] PWTEST_CLI_EXECUTABLE_PATH: options.executablePath,
      // [ts] DEBUG: (process.env.DEBUG ?? "") + ",pw:browser*",
      // [ts] },
      // [ts] });
      // [ts] return self_val;
  }

  // TODO(cap-migrate): refine TS class member
  // [ts] @step;
  pub async fn waitFor(text: string) {
    expect.poll(() => _.text(), { timeout: 30000 }).toContain(text);
  }

  // TODO(cap-migrate): refine TS class member
  // [ts] @step;
  pub async fn exit(*_) {
    if (!_._exitPromise) {
      _._process.write("exit\n");
      _._exitPromise = _._process.cleanExit();
    }
    _._exitPromise;
  }

  pub fn sigint(*_) {
    const result = _._process.kill("SIGINT");
    _._exitPromise = _._process.exited.then(() => null); // Avoid double closing.
    return result;
  }

  pub async fn text() {
    { // TODO(cap-migrate): replace try/catch with CP ?T / ?
      return fs.promises.readFile(_._outputFile, "utf-8");
    } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {)
      return "";
    }
  }
}
