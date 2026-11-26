// Type definitions for CP

import { StringWidthOptions, stringWidth, stringWidthWithOptions } from "../../string-width/src/index.cp";
export fn wlTrimCr(line: string): string;
export fn wlLineWidthWithTab(line: string, tab_width: usize): usize;
export fn countLines(s: string): usize;
export fn widestLine(s: string): usize;
export fn widestLineWithOptions(s: string, opts: StringWidthOptions): usize;
export fn lineWidths(s: string, out: []usize): usize;
export fn lineWidthsWithOptions(s: string, opts: StringWidthOptions, out: []usize): usize;
export fn widestLineWithTab(s: string, tab_width: usize): usize;
