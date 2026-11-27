// Type definitions for CP

import {  } from "std";
export fn cmark_markdown_to_html(text: [*]&u8!, len: usize, options: i32): [*:0]u8!?;
export fn cmark_version(): i32;
export fn cmark_version_string(): [*:0]&u8!?;
export fn free(ptr: *auto!?);
export error CmarkError { RenderFailed }

export fn versionString(): string;
export fn versionNumber(): i32;
export class HtmlString {
    raw: [*:0]u8!?,
    len: usize,
    fn slice(): string;
    fn __cp_deinit(_: &mut HtmlString);
}

export fn markdownToHtml(md: string, opts: i32): !HtmlString;
