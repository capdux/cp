// Type definitions for CP

import { stripIndent } from "../../strip-indent/src/index.cp";
import { indentString, indentStringWith } from "../../indent-string/src/index.cp";
export fn redentWithIndent(s: string, count: usize, indent: string, include_empty_lines: bool, out: []u8): usize;
export fn redent(s: string, count: usize, out: []u8): usize;
