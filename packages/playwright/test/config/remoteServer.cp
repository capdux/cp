// Migrated from TypeScript (config/remoteServer.ts) via cap from -ts

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

import path from "cap:path";
// [cap-migrate] import { BrowserType, Browser } from "playwright";
// [cap-migrate] import { CommonFixtures, TestChildProcess } from "./commonFixtures.cp";

pub struct PlaywrightServer {
  // TODO(cap-migrate): interface method `wsEndpoint(): string;` -> move to trait/impl
  // TODO(cap-migrate): interface method `close(): Promise<void>;` -> move to trait/impl
}

pub class RunServer {

  _process: TestChildProcess;
  _wsEndpoint: string = "";

  pub async fn start(*_, childProcess: string, options: string? = null) {
      // TODO(cap-migrate): refine TS body
      // [ts] const command = ["node", path.join(__dirname, "..", "..", "packages", "playwright-core", "cli.js"), "run-server"];
      // [ts] if (options?.mode == "extension") {
      // [ts] command.push("--mode=extension");
      // [ts] }
      // [ts] if (options?.artifactsDir) {
      // [ts] command.push(`--artifacts-dir=${options.artifactsDir}`);
      // [ts] }
      // [ts] if (options?.unsafe) {
      // [ts] command.push("--unsafe");
      // [ts] }
      // [ts] _._process = childProcess({
      // [ts] command,
      // [ts] env: { NODE_OPTIONS: process.env.NODE_OPTIONS, ...options?.env },
      // [ts] });
      // [ts] let wsEndpointCallback: (value) => void;
      // [ts] const wsEndpointPromise = Promise.init(f => wsEndpointCallback = f);
      // [ts] _._process.onOutput = data => {
      // [ts] const prefix = "Listening on ";
      // [ts] const line = data.toString();
      // [ts] if (line.startsWith(prefix)) {
      // [ts] wsEndpointCallback(line.substr(prefix.length));
      // [ts] }
      // [ts] };
      // [ts] _._wsEndpoint = await wsEndpointPromise;
  }

  pub fn wsEndpoint() {
    return _._wsEndpoint;
  }

  pub async fn close() {
    await _._process.kill("SIGINT");
  }
}

pub type RemoteServerOptions = string; // TODO(cap-migrate): complex TS conditional/mapped type

pub class RemoteServer {
  _process: TestChildProcess;
  _output: string = Map.init();
  _outputCallback: string = Map.init();
  _browserType: BrowserType;
  _exitAndDisconnectPromise: string = "";
  _browser: Browser? = null;
  _wsEndpoint: string = "";

  pub async fn _start(*_, childProcess: string, browserType: BrowserType, channel: string, remoteServerOptions: RemoteServerOptions = {}) {
    _._browserType = browserType;
    const browserOptions = (browserType)._playwright._defaultLaunchOptions;
    // Copy options to prevent a large JSON string when launching subprocess.
    // Otherwise, we get `Error: spawn ENAMETOOLONG` on Windows.
    const launchOptions: string = {
      args: browserOptions.args,
      headless: browserOptions.headless,
      channel: browserOptions.channel,
      executablePath: browserOptions.executablePath,
      handleSIGINT: true,
      handleSIGTERM: true,
      handleSIGHUP: true,
      logger: null,
    };
    if (remoteServerOptions.sharedBrowser) {
      (launchOptions)._sharedBrowser = true;
    }
    if (remoteServerOptions.artifactsDir) {
      launchOptions.artifactsDir = remoteServerOptions.artifactsDir;
    }
    const options = {
      browserTypeName: browserType.name(),
      channel,
      launchOptions,
      ...remoteServerOptions,
    };
    _._process = childProcess({
      command: ["node", path.join(__dirname, "remote-server-impl.js"), JSON.stringify(options)],
      env: { NODE_OPTIONS: process.env.NODE_OPTIONS },
    });

    let index = 0;
    _._process.onOutput = () => {
      let match_val;
      while ((match_val = _._process.output.substring(index).match("\\(([^()]+)=>([^()]+)\\)"))) {
        const key = match_val[1];
        const value = match_val[2];
        _._addOutput(key, value);
        index += match_val.index + match_val[0].length;
      }
    };

    _._wsEndpoint = await _.out("wsEndpoint");

    if (remoteServerOptions.url) {
      _._browser = await _._browserType.connect(_._wsEndpoint);
      const page = await _._browser.newPage();
      await page.goto(remoteServerOptions.url);
    }
  }

  pub fn _addOutput(*_, key: string, value: string) {
    _._output.set(key, value);
    const cb = _._outputCallback.get(key);
    _._outputCallback.delete(key);
    if (cb) {
      cb();
    }
  }

  pub async fn out(*_, key: string): string {
    if (!_._output.has(key)) {
      await Promise.init(f => _._outputCallback.set(key, f));
    }
    return _._output.get(key);
  }

  pub fn wsEndpoint() {
    return _._wsEndpoint;
  }

  pub fn child() {
    return _._process.process;
  }

  pub async fn childExitCode() {
    return await _._process.exitCode;
  }

  pub async fn childSignal() {
    return (await _._process.exited).signal;
  }

  pub async fn close(*_) {
    if (_._browser) {
      await _._browser.close();
      _._browser = null;
    }
    void _._process.kill("SIGINT");
    const killTimer = setTimeout(() => void _._process.kill("SIGINT"), 30000);
    { // TODO(cap-migrate): replace try/catch with CP ?T / ?
      await _.childExitCode();
    } { // TODO(cap-migrate): replace finally with CP defer
      clearTimeout(killTimer);
    }
  }
}
