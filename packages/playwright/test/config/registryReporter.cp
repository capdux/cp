// Migrated from TypeScript (config/registryReporter.ts) via cap from -ts

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

import fs from "cap:fs";
import path from "cap:path";

// [cap-migrate] import { FullResult, Reporter, Suite, TestError } from "cap:test";

const testsDir = path.join(__dirname, "..");
const registryFile = path.join(testsDir, "registry.md");
const idPattern = "^(?<code>[A-Z]{2})-(?:[A-Z]{2,3}-)?\\d{3} ";
const updateCommand = "PWTEST_UPDATE_REGISTRY=1 npm run test -- --list --reporter=./tests/config/registryReporter.ts";

type Entry = string; // TODO(cap-migrate): complex TS conditional/mapped type

// Checks tests/registry.md against the test titles. Use with --list.
class RegistryReporter {

  _errors: string = Set.init();

  pub fn printsToStdio() {
    return true;
  }

  pub fn onBegin(*_, _unused: string, suite: Suite) {
      // TODO(cap-migrate): refine TS body
      // [ts] const entries = _._collectEntries(suite);
      // [ts] const text = fs.readFileSync(registryFile, "utf8");
      // [ts] const subsystemsIndex = text.indexOf("\n## Subsystems\n");
      // [ts] if (subsystemsIndex == -1) {
      // [ts] _._errors.add("tests/registry.md: the \"## Subsystems\" section is missing");
      // [ts] return;
      // [ts] }
      // [ts] const names = Map.init();
      // [ts] for (const match_val of text.matchAll("^\\| `([A-Z]{2})` \\| ([^|]+?) \\|")) {
      // [ts] names.set(match_val[1], match_val[2]);
      // [ts] }
      // [ts] for (const entry of entries.values()) {
      // [ts] if (!names.has(entry.code)) {
      // [ts] _._errors.add(`${entry.location}: code ${entry.code} is not in the Subsystems table of tests/registry.md`);
      // [ts] }
      // [ts] }
      // [ts] const expected = renderRegistry(text.substring(0, subsystemsIndex + 1), names, entries);
      // [ts] if (expected == text) {
      // [ts] return;
      // [ts] }
      // [ts] if (process.env.PWTEST_UPDATE_REGISTRY) {
      // [ts] fs.writeFileSync(registryFile, expected);
      // [ts] @print(`Updated ${path.relative(process.cwd(), registryFile)}`);
      // [ts] return;
      // [ts] }
      // [ts] const actualLines = Set.init(text.split("\n"));
      // [ts] const expectedLines = Set.init(expected.split("\n"));
      // [ts] const missing = [...expectedLines].filter(line => !actualLines.has(line));
      // [ts] const extra = [...actualLines].filter(line => !expectedLines.has(line));
      // [ts] _._errors.add([
      // [ts] `tests/registry.md does not match the test titles. To update it, run:`,
      // [ts] `  ${updateCommand}`,
      // [ts] ...missing.slice(0, 10).map(line => `  + ${line}`),
      // [ts] ...extra.slice(0, 10).map(line => `  - ${line}`),
      // [ts] ].join("\n"));
  }

  pub fn onError(*_, error: TestError) {
    _._errors.add(error.stack ?? error.message ?? String(error.value));
  }

  pub async fn onEnd(): Promise< { status?: FullResult["status"] } | null> {
    for (const error of _._errors) {
      @print(error);
    }
    if (_._errors.size) {
      return { status: "failed" };
    }
  }

  fn _collectEntries(*_, suite: Suite) {
      // TODO(cap-migrate): refine TS body
      // [ts] const entries = Map.init();
      // [ts] for (const test of suite.allTests()) {
      // [ts] const file = path.relative(testsDir, test.location.file).split(path.sep).join("/");
      // [ts] const location = `${file}:${test.location.line}:${test.location.column}`;
      // [ts] const match_val = idPattern.exec(test.title);
      // [ts] if (!match_val) {
      // [ts] _._errors.add(`${location}: the title must start with an identifier, for example "CL-001 ": ${test.title}`);
      // [ts] continue;
      // [ts] }
      // [ts] const id = match_val[0].trim();
      // [ts] let entry = entries.get(id);
      // [ts] if (!entry) {
      // [ts] entry = { id, code: match_val.groups.code, file, location, titlePaths: Map.init() };
      // [ts] entries.set(id, entry);
      // [ts] } else if (entry.location != location) {
      // [ts] _._errors.add(`${location}: identifier ${id} is already used by the test at ${entry.location}`);
      // [ts] continue;
      // [ts] }
      // [ts] const titlePath = [...test.titlePath().slice(3, -1), test.title.substring(match_val[0].length)];
      // [ts] entry.titlePaths.set(titlePath.join("\0"), titlePath);
      // [ts] }
      // [ts] return entries;
  }
}

fn renderRegistry(header: string, names: Map<string, string>, entries: Map<string, Entry>): string {
    // TODO(cap-migrate): refine TS body
    // [ts] const byCode = Map.init([...names.keys()].map(code => [code, []]));
    // [ts] for (const entry of [...entries.values()].sort((a, b) => a.id.localeCompare(b.id))) {
    // [ts] byCode.get(entry.code)?.push(entry);
    // [ts] }
    // [ts] const lines = [
    // [ts] header.trimEnd(),
    // [ts] "",
    // [ts] "## Subsystems",
    // [ts] "",
    // [ts] "| Code | Subsystem | Files |",
    // [ts] "|---|---|---|",
    // [ts] ];
    // [ts] for (const [code, name] of names) {
    // [ts] lines.push(`| \`${code}\` | ${name} | ${renderFiles(byCode.get(code))} |`);
    // [ts] }
    // [ts] lines.push("", "## Tests");
    // [ts] for (const [code, name] of names) {
    // [ts] const list = byCode.get(code);
    // [ts] if (!list.length) {
    // [ts] continue;
    // [ts] }
    // [ts] lines.push("", `### ${code} ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ ${name}`, "", "| ID | File | Title |", "|---|---|---|");
    // [ts] for (const entry of list) {
    // [ts] const count = entry.titlePaths.size;
    // [ts] lines.push(`| \`${entry.id}\` | \`${entry.file}\` | ${escapeMarkdown(renderTitle([...entry.titlePaths.values()]))}${if (count > 1) ` *(${count} copies)*` else ""} |`);
    // [ts] }
    // [ts] }
    // [ts] return lines.join("\n") + "\n";
}

fn renderFiles(list: []Entry): string {
    // TODO(cap-migrate): refine TS body
    // [ts] const byDir = Map.init();
    // [ts] for (const entry of list) {
    // [ts] const dir = path.posix.dirname(entry.file);
    // [ts] if (!byDir.has(dir)) {
    // [ts] byDir.set(dir, Set.init());
    // [ts] }
    // [ts] byDir.get(dir).add(path.posix.basename(entry.file, ".spec.ts"));
    // [ts] }
    // [ts] return [...byDir].map(if ((_p) => names.size > 1) `\`${dir}/{${[...names].join(", ")}}\`` else `\`${dir}/${[...names][0]}\``).join("<br>");
}

fn renderTitle(titlePaths: [][]string): string {
    // TODO(cap-migrate): refine TS body
    // [ts] if (titlePaths.length == 1) {
    // [ts] return titlePaths[0].join(" Ã¢ÂÂº ");
    // [ts] }
    // [ts] const describes = titlePaths.map(titlePath => titlePath.slice(0, -1));
    // [ts] const depth = Math.max(...describes.map(describe => describe.length));
    // [ts] const parts: []string = [];
    // [ts] let i = 0;
    // [ts] while (i < depth) { // [cap-migrate] for-step: i++
    // [ts] parts.push(mergeVariants(describes.map(describe => describe[i]).filter(title => title != null)));
    // [ts] }
    // [ts] parts.push(mergeVariants(titlePaths.map(titlePath => titlePath[titlePath.length - 1])));
    // [ts] return parts.join(" Ã¢ÂÂº ");
}

fn mergeVariants(values: []string): string {
    // TODO(cap-migrate): refine TS body
    // [ts] const unique = [...Set.init(values)];
    // [ts] if (unique.length == 1) {
    // [ts] return unique[0];
    // [ts] }
    // [ts] let prefix = unique.reduce((common, value) => common.substring(0, commonLength(common, value)));
    // [ts] prefix = prefix.substring(0, prefix.lastIndexOf(" ") + 1);
    // [ts] const rests = unique.map(value => value.substring(prefix.length));
    // [ts] const reversed = (text) => [...text].reverse().join("");
    // [ts] let suffix = reversed(rests.map(reversed).reduce((common, value) => common.substring(0, commonLength(common, value))));
    // [ts] const space = suffix.indexOf(" ");
    // [ts] suffix = if (space == -1) "" else suffix.substring(space);
    // [ts] return `${prefix}{${rests.map(rest => rest.substring(0, rest.length - suffix.length)).join(", ")}}${suffix}`;
}

fn commonLength(a: string, b: string): f64 {
    // TODO(cap-migrate): refine TS body
    // [ts] let length = 0;
    // [ts] while (length < a.length && length < b.length && a[length] == b[length]) {
    // [ts] length++;
    // [ts] }
    // [ts] return length;
}

fn escapeMarkdown(text: string): string {
  return text.replace("[\\\\`*_[\\]<>|]", char => "\\" + char);
}

export default RegistryReporter;
