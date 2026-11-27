// Type definitions for CP

import { startsWith } from "cap:ascii";
import { writeStr } from "cap:fmt";
export fn needsHtmlEscape(input: string): bool;
export fn escapedHtmlLen(input: string): usize;
export fn escapeHtml(input: string, out: []u8): usize;
export fn unescapeHtml(input: string, out: []u8): usize;
