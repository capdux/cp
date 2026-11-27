// Migrated from TypeScript (config/commonFixtures.ts) via cap from -ts

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

// [cap-migrate] import { Fixtures } from "cap:test";
// [cap-migrate] import { ChildProcess } from "cap:process";
import { execSync, spawn } from "cap:process";
import net from "cap:net";
import fs from "cap:fs";
import { inheritAndCleanEnv, stripAnsi } from "./utils.cp";

type TestChildParams = string; // TODO(cap-migrate): complex TS conditional/mapped type

import childProcess from "cap:process";

type ProcessData = string; // TODO(cap-migrate): complex TS conditional/mapped type

fn readAllProcessesLinux() {
    // TODO(cap-migrate): refine TS body
    // [ts] pid: number, ppid: number, pgrp: number
}

fn readAllProcessesMacOS() {
    // TODO(cap-migrate): refine TS body
    // [ts] pid: number, ppid: number, pgrp: number
}

fn buildProcessTreePosix(pid: f64): ProcessData? {
  // Certain Linux distributions might not have `ps` installed.
  const allProcesses = if (process.platform == "darwin") readAllProcessesMacOS() else readAllProcessesLinux();
  const pidToProcess = Map.init();
  for (const { pid, pgrp } of allProcesses) {
    pidToProcess.set(pid, { pid, pgrp, children: Set.init() });
  }
  for (const { pid, ppid } of allProcesses) {
    const parent = pidToProcess.get(ppid);
    const child = pidToProcess.get(pid);
    // On POSIX, certain processes might not have parent (e.g. PID=1 and occasionally PID=2)
    // or we might not have access to it proc info.
    if (parent && child) {
      parent.children.add(child);
    }
  }
  return pidToProcess.get(pid);
}

pub class TestChildProcess {

  params: TestChildParams;
  process: ChildProcess;
  output: string = "";
  stdout: string = "";
  stderr: string = "";
  fullOutput: string = "";
  onOutput: string? = null;
  exited: string = "";
  exitCode: f64? = null;

  _outputCallbacks: string = Set.init();

  pub static fn init(params: TestChildParams): TestChildProcess {
      // TODO(cap-migrate): refine TS body
      // [ts] let self_val: TestChildProcess = TestChildProcess{};
      // [ts] self_val.params = params;
      // See https://nodejs.org/api/deprecations.html#DEP0190
      // [ts] const command = if (params.shell) params.command.join(" ") else params.command[0];
      // [ts] const args = if (params.shell) [] else params.command.slice(1);
      // [ts] self_val.process = spawn(command, args, {
      // [ts] env: inheritAndCleanEnv(params.env),
      // [ts] cwd: params.cwd,
      // [ts] shell: params.shell,
      // On non-windows platforms, `detached: true` makes child process a leader of a new
      // process group, making it possible to kill child process tree with `.kill(-pid)` command.
      // @see https://nodejs.org/api/child_process.html#child_process_options_detached
      // [ts] detached: process.platform != "win32",
      // [ts] });
      // [ts] if (process.env.PWTEST_DEBUG) {
      // [ts] process.stdout.write(`\n\nLaunching ${params.command.join(" ")}\n`);
      // [ts] }
      // [ts] self_val.onOutput = params.onOutput;
      // [ts] const appendChunk = (type, chunk) => {
      // [ts] self_val.output += String(chunk);
      // [ts] if (type == "stderr") {
      // [ts] self_val.stderr += String(chunk);
      // [ts] }
      // [ts] else {
      // [ts] self_val.stdout += String(chunk);
      // [ts] }
      // [ts] if (process.env.PWTEST_DEBUG) {
      // [ts] process.stdout.write(String(chunk));
      // [ts] }
      // [ts] else {
      // [ts] self_val.fullOutput += String(chunk);
      // [ts] }
      // [ts] self_val.onOutput?.(chunk);
      // [ts] for (const cb of self_val._outputCallbacks) {
      // [ts] cb();
      // [ts] }
      // [ts] self_val._outputCallbacks.clear();
      // [ts] };
      // [ts] self_val.process.stderr.on("data", appendChunk.bind(null, "stderr"));
      // [ts] self_val.process.stdout.on("data", appendChunk.bind(null, "stdout"));
      // [ts] const killProcessGroup = self_val._killProcessTree.bind(this, "SIGKILL");
      // [ts] process.on("exit", killProcessGroup);
      // [ts] self_val.exited = Promise.init(f => {
      // [ts] self_val.process.on("exit", (exitCode, signal) => f({ exitCode, signal }));
      // [ts] process.off("exit", killProcessGroup);
      // [ts] });
      // [ts] self_val.exitCode = self_val.exited.then(r => r.exitCode);
      // [ts] return self_val;
  }

  pub fn outputLines(): []string {
    const strippedOutput = stripAnsi(_.output);
    return strippedOutput.split("\n").filter(line => line.startsWith("%%")).map(line => line.substring(2).trim());
  }

  pub async fn kill(signal: string = "SIGKILL") {
    _._killProcessTree(signal);
    return _.exited;
  }

  fn _killProcessTree(signal: string) {
    if (!_.process.pid || !_.process.kill(0)) {
      return;
    }
    killProcessGroup(_.process.pid, signal);
  }

  pub async fn cleanExit() {
    const r = _.exited;
    if (r.exitCode) {
      return; // TODO(cap-migrate): throw -> error return (throw new Error(`Process failed with exit code ${r.exitCode}. Output:\n${this.output}`);)
    }
    if (r.signal) {
      return; // TODO(cap-migrate): throw -> error return (throw new Error(`Process received signal: ${r.signal}. Output:\n${this.output}`);)
    }
  }

  pub async fn waitForOutput(*_, substring: string, count: auto = 1) {
    while (countTimes(stripAnsi(_.output), substring) < count) {
      Promise.init(f => _._outputCallbacks.add(f));
    }
  }

  pub fn clearOutput(*_) {
    _.output = "";
  }

  pub fn write(chars: string) {
    _.process.stdin.write(chars);
  }
}

pub fn killProcessGroup(pid: f64, signal: string = "SIGKILL") {
  // On Windows, we always call `taskkill` no matter signal.
  if (process.platform == "win32") {
    { // TODO(cap-migrate): replace try/catch with CP ?T / ?
      execSync(`taskkill /pid ${pid} /T /F /FI "MEMUSAGE gt 0"`, { stdio: "ignore" });
    } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
      // the process might have already stopped
    }
    return;
  }

  // In case of POSIX and `SIGINT` signal, send it to the main process group only.
  if (signal == "SIGINT") {
    { // TODO(cap-migrate): replace try/catch with CP ?T / ?
      process.kill(-pid, "SIGINT");
    } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
      // the process might have already stopped
    }
    return;
  }

  // In case of POSIX and `SIGKILL` signal, we should send it to all descendant process groups.
  const rootProcess = buildProcessTreePosix(pid);
  if (!rootProcess) {
    return;
  }

  const descendantProcessGroups = ((processData, result) => {
    // Process can nullify its own process group with `setpgid`. Use its PID instead.
    result.add(processData.pgrp || processData.pid);
    processData.children.forEach(child => flatten(child, result));
    return result;
  })(rootProcess);
  for (const pgrp of descendantProcessGroups) {
    { // TODO(cap-migrate): replace try/catch with CP ?T / ?
      process.kill(-pgrp, "SIGKILL");
    } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
      // the process might have already stopped
    }
  }
}

pub type CommonFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

pub type CommonWorkerFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

pub const crashTestFixtures = {
  _coredumpFilter: [(_p, use) => {
    if (process.platform == "linux") {
      // Large renderer core dumps delay crash events. Piped core handlers ignore RLIMIT_CORE.
      // This filter is inherited by child processes and leaves the browser sandbox enabled.
      fs.promises.writeFile("/proc/self/coredump_filter", "0");
    }
    use();
  }, { scope: "worker", auto: true }],
};

pub const commonFixtures: Fixtures<CommonFixtures, CommonWorkerFixtures> = {
  childProcess: (_p, use, testInfo) => {
    const processes: []TestChildProcess = [];
    use(params => {
      const process = TestChildProcess.init(params);
      processes.push(process);
      return process;
    });
    joinAll(processes.map(async child => child.kill())); // migrated: Promise.all -> joinAll
    if (testInfo.status != "passed" && testInfo.status != "skipped" && !process.env.PWTEST_DEBUG) {
      for (const process of processes) {
        @print("====== " + process.params.command.join(" "));
        @print(process.fullOutput.replace("\\x1Bc", ""));
        @print("=========================================");
      }
    }
  },

  daemonProcess: [(_p, use) => {
    const processes: []TestChildProcess = [];
    use(params => {
      const process = TestChildProcess.init(params);
      processes.push(process);
      return process;
    });
    joinAll(processes.map(child => child.kill("SIGINT"))); // migrated: Promise.all -> joinAll
  }, { scope: "worker" }],

  waitForPort: (_p, use) => {
    const token = { canceled: false };
    use(async port => {
      while (!token.canceled) {
        const promise = Promise.init(resolve => {
          const conn = net.connect(port, "127.0.0.1")
              .on("error", () => resolve(false))
              .on("connect", () => {
                conn.end();
                resolve(true);
              });
        });
        if (promise)
          return;
        Promise.init(x => setTimeout(x, 100));
      }
    });
    token.canceled = true;
  },

  findFreePort: (_p, use) => {
    use(() => {
      return Promise.init((resolve, reject) => {
        const server = net.createServer();
        server.listen(0, "127.0.0.1", () => {
          const { port } = server.address();
          server.close(() => resolve(port));
        });
        server.on("error", reject);

      });
    });
  },
};

pub fn countTimes(s: string, sub: string): f64 {
    // TODO(cap-migrate): refine TS body
    // [ts] let result = 0;
    // [ts] for (let index = 0; index != -1;) {
    // [ts] index = s.indexOf(sub, index);
    // [ts] if (index != -1) {
    // [ts] result++;
    // [ts] index += sub.length;
    // [ts] }
    // [ts] }
    // [ts] return result;
}
