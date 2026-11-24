// @cap/expat — Stream-oriented XML 1.0 SAX parser (libexpat 2.6.4) for CP
// License: MIT (Upstream: libexpat/libexpat R_2_6_4)

import std from "std";

pub @c {
    fn cp_expat_version(): [*:0]const u8;
    fn cp_expat_count_elements(
        xml: [*]const u8?!,
        len: usize,
        out_elem_count: *usize!
    ): i32;
}

pub err ExpatError {
    InvalidInput,
    ParserInitFailed,
    ParseFailed,
}

/// Returns the linked libexpat version string (e.g. "expat_2.6.4").
pub fn versionString(): string {
    unsafe {
        return std.mem.span(cp_expat_version());
    }
}

/// Parses an XML document with libexpat SAX parser and returns the total element count.
pub fn countElements(xml: []const u8): usize! {
    let count: usize = 0;
    let rc: i32 = 0;
    unsafe {
        const ptr: [*]const u8?! = if (xml.len == 0) null else xml.ptr;
        rc = cp_expat_count_elements(ptr, xml.len, &count);
    }
    if (rc == -1) return ExpatError.InvalidInput;
    if (rc == -2) return ExpatError.ParserInitFailed;
    if (rc != 0) return ExpatError.ParseFailed;
    return count;
}
