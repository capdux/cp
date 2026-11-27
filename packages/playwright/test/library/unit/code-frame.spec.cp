// Migrated from TypeScript (library/unit/code-frame.spec.ts) via cap from -ts

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
import { codeFrame } from "../../../packages/isomorphic/codeFrame.cp";
import { noColors } from "../../../packages/isomorphic/colors.cp";

// [cap-migrate] import { Colors } from "../../../packages/isomorphic/colors.cp";

// TODO(cap-migrate): refine TS statement
// [ts] const tagColors = Object.fromEntries(Object.keys(noColors).map(name => [name, name == "enabled" ? true : (text) => `<${name}>${text}</${name}>`]));

const source = Array.from({ length: 12 }, (_unused, i) => `line${i + 1} = foo(${i + 1});`).join("\n");












fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("CF-001 should mark the column", () => {
    // [ts] expect(codeFrame(source, { line: 5, column: 3 })).toBe([
    // [ts] "  3 | line3 = foo(3);",
    // [ts] "  4 | line4 = foo(4);",
    // [ts] "> 5 | line5 = foo(5);",
    // [ts] "    |   ^",
    // [ts] "  6 | line6 = foo(6);",
    // [ts] "  7 | line7 = foo(7);",
    // [ts] "  8 | line8 = foo(8);",
    // [ts] ].join("\n"));
    // [ts] });
    // [ts] test("CF-002 should mark the line without a column", () => {
    // [ts] expect(codeFrame(source, { line: 5 })).toBe([
    // [ts] "  3 | line3 = foo(3);",
    // [ts] "  4 | line4 = foo(4);",
    // [ts] "> 5 | line5 = foo(5);",
    // [ts] "  6 | line6 = foo(6);",
    // [ts] "  7 | line7 = foo(7);",
    // [ts] "  8 | line8 = foo(8);",
    // [ts] ].join("\n"));
    // [ts] });
    // [ts] test("CF-003 should respect linesAbove and linesBelow", () => {
    // [ts] expect(codeFrame(source, { line: 6, column: 2 }, { linesAbove: 0, linesBelow: 1 })).toBe([
    // [ts] "> 6 | line6 = foo(6);",
    // [ts] "    |  ^",
    // [ts] "  7 | line7 = foo(7);",
    // [ts] ].join("\n"));
    // [ts] });
    // [ts] test("CF-004 should clip to the source lines", () => {
    // [ts] expect(codeFrame(source, { line: 12, column: 2 })).toBe([
    // [ts] "  10 | line10 = foo(10);",
    // [ts] "  11 | line11 = foo(11);",
    // [ts] "> 12 | line12 = foo(12);",
    // [ts] "     |  ^",
    // [ts] ].join("\n"));
    // [ts] });
    // [ts] test("CF-005 should pad line numbers to the widest one", () => {
    // [ts] expect(codeFrame(source, { line: 9, column: 1 })).toBe([
    // [ts] "   7 | line7 = foo(7);",
    // [ts] "   8 | line8 = foo(8);",
    // [ts] ">  9 | line9 = foo(9);",
    // [ts] "     | ^",
    // [ts] "  10 | line10 = foo(10);",
    // [ts] "  11 | line11 = foo(11);",
    // [ts] "  12 | line12 = foo(12);",
    // [ts] ].join("\n"));
    // [ts] });
    // [ts] test("CF-006 should print the message after the marker", () => {
    // [ts] expect(codeFrame(source, { line: 5, column: 3 }, { linesAbove: 0, linesBelow: 0, message: "Boom" })).toBe([
    // [ts] "> 5 | line5 = foo(5);",
    // [ts] "    |   ^ Boom",
    // [ts] ].join("\n"));
    // [ts] });
    // [ts] test("CF-007 should print the message above the frame without a column", () => {
    // [ts] expect(codeFrame(source, { line: 5 }, { linesAbove: 0, linesBelow: 0, message: "Boom" })).toBe([
    // [ts] "  Boom",
    // [ts] "> 5 | line5 = foo(5);",
    // [ts] ].join("\n"));
    // [ts] });
    // [ts] test("CF-008 should keep tabs before the marker", () => {
    // [ts] expect(codeFrame("a\n\t\tfoo(\tbar)\nb", { line: 2, column: 6 })).toBe([
    // [ts] "  1 | a",
    // [ts] "> 2 | \t\tfoo(\tbar)",
    // [ts] "    | \t\t   ^",
    // [ts] "  3 | b",
    // [ts] ].join("\n"));
    // [ts] });
    // [ts] test("CF-009 should not add a space after the gutter for empty lines", () => {
    // [ts] expect(codeFrame("a\n\nfoo\n   \nb", { line: 3, column: 1 })).toBe([
    // [ts] "  1 | a",
    // [ts] "  2 |",
    // [ts] "> 3 | foo",
    // [ts] "    | ^",
    // [ts] "  4 |    ",
    // [ts] "  5 | b",
    // [ts] ].join("\n"));
    // [ts] });
    // [ts] test("CF-010 should split lines on all line terminators", () => {
    // [ts] expect(codeFrame("a\r\nb\rc\u2028d\ne", { line: 3, column: 1 }, { linesAbove: 5, linesBelow: 5 })).toBe([
    // [ts] "  1 | a",
    // [ts] "  2 | b",
    // [ts] "> 3 | c",
    // [ts] "    | ^",
    // [ts] "  4 | d",
    // [ts] "  5 | e",
    // [ts] ].join("\n"));
    // [ts] });
    // [ts] test("CF-011 should highlight code", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44354" } }, () => {
    // [ts] const code = `const page = new Page(/re/g, 'a', 42) + x; // comment`;
    // [ts] expect(codeFrame(code, { line: 1, column: 7 }, { colors: tagColors, message: "Boom" })).toBe([
    // [ts] "<red><bold>></bold></red><gray> 1 |</gray> " +
    // [ts] "<cyan>const</cyan> page <yellow>=</yellow> <cyan>new</cyan> <yellow>Page</yellow>(<magenta>/re/g</magenta><yellow>,</yellow> " +
    // [ts] `<green>'a'</green><yellow>,</yellow> <magenta>42</magenta>) <yellow>+</yellow> x<yellow>;</yellow> <gray>// comment</gray>`,
    // [ts] " <gray>   |</gray>       <red><bold>^</bold></red> <red><bold>Boom</bold></red>",
    // [ts] ].join("\n"));
    // [ts] let highlightedKeywords = 0;
    // [ts] const colors = {
    // [ts] ...tagColors,
    // [ts] cyan: (text) => {
    // [ts] highlightedKeywords++;
    // [ts] return tagColors.cyan(text);
    // [ts] },
    // [ts] };
    // [ts] const largeSource = "const hidden = 42;\n".repeat(300000) + code + "\nconst hidden = 42;";
    // [ts] const frame = codeFrame(largeSource, { line: 300001, column: 7 }, { colors, linesAbove: 0, linesBelow: 0, message: "Boom" });
    // [ts] expect_eq(std.mem.indexOf(u8, frame, "<gray> 300001 |</gray> <cyan>const</cyan>") != null, true);
    // [ts] expect_eq(highlightedKeywords, 2);
    // [ts] });
    // [ts] test("CF-012 should highlight multi-line tokens on each line", () => {
    // [ts] expect(codeFrame("a = `x ${`y`}\nz`; /* b\nc */", { line: 1 }, { colors: tagColors })).toBe([
    // [ts] "<red><bold>></bold></red><gray> 1 |</gray> a <yellow>=</yellow> <green>`x ${`y`}</green>",
    // [ts] " <gray> 2 |</gray> <green>z`</green><yellow>;</yellow> <gray>/* b</gray>",
    // [ts] " <gray> 3 |</gray> <gray>c */</gray>",
    // [ts] ].join("\n"));
    // [ts] });
    // [ts] test("CF-013 should highlight keywords used as property names as identifiers", () => {
    // [ts] expect(codeFrame("promise.catch(e => e).finally(done);", { line: 1 }, { colors: tagColors })).toBe(
    // [ts] "<red><bold>></bold></red><gray> 1 |</gray> promise<yellow>.</yellow>catch(e <yellow>=></yellow> e)<yellow>.</yellow>finally(done)<yellow>;</yellow>"
    // [ts] );
    // [ts] });
}
