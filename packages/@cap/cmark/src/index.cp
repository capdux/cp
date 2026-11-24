// @cap/cmark — CommonMark 0.31.1 official C reference Markdown renderer for CP
// License: BSD-2-Clause (Upstream: commonmark/cmark 0.31.1)

import std from "std";

pub @c {
    fn cmark_markdown_to_html(text: [*]const u8!, len: usize, options: i32): [*:0]u8?;
    fn cmark_version(): i32;
    fn cmark_version_string(): [*:0]const u8?;
    fn free(ptr: *!?);
}

pub const OPT_DEFAULT: i32 = 0;
pub const OPT_SOURCEPOS: i32 = 2;
pub const OPT_HARDBREAKS: i32 = 4;
pub const OPT_NOBREAKS: i32 = 16;
pub const OPT_VALIDATE_UTF8: i32 = 512;
pub const OPT_SMART: i32 = 1024;
pub const OPT_UNSAFE: i32 = 131072;

pub err CmarkError {
    RenderFailed,
}

/// Returns the linked libcmark version string (e.g. "0.31.1").
pub fn versionString(): string {
    unsafe {
        if (ver := cmark_version_string()) {
            return std.mem.span(ver);
        }
    }
    return "0.31.1";
}

/// Returns the numeric libcmark version ((major << 16) | (minor << 8) | patch).
pub fn versionNumber(): i32 {
    unsafe { return cmark_version(); }
}

/// Owned RAII rendered HTML string allocated by libcmark (freed automatically via libc free).
pub struct HtmlString {
    raw: [*:0]u8? = null,
    len: usize = 0,

    pub fn slice(): string {
        if (p := _.raw) {
            return p[0.._.len];
        }
        return "";
    }

    ~init() {
        if (p := _.raw) {
            _.raw = null;
            _.len = 0;
            unsafe { free(@ptrCast(p)); }
        }
    }
}

/// Converts CommonMark Markdown text to an owned HTML string.
pub fn markdownToHtml(md: string, opts: i32): HtmlString! {
    let res: [*:0]u8? = null;
    unsafe {
        res = cmark_markdown_to_html(md.ptr, md.len, opts);
    }
    if (p := res) {
        const sp = std.mem.span(p);
        return HtmlString{ .raw = p, .len = sp.len };
    }
    return CmarkError.RenderFailed;
}
