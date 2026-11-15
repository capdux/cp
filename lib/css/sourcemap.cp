import css from "./css_parser.zig";
import css_values from "./values/values.zig";
pub const Error = css.Error;

pub struct SourceMap {
    project_root: []const u8,
    inner: SourceMapInner,
}

pub struct SourceMapInner {
    sources: ArrayList(bun.string),
    sources_content: ArrayList(bun.string),
    names: ArrayList(bun.string),
    mapping_lines: ArrayList(MappingLine),
}

pub const MappingLine = struct { mappings: ArrayList(LineMapping), last_column: u32, is_sorted: bool };

pub const LineMapping = struct { generated_column: u32, original: OriginalLocation? };

pub struct OriginalLocation {
    original_line: u32,
    original_column: u32,
    source: u32,
    name: u32?,
}

import bun from "bun";
const ArrayList = std.ArrayListUnmanaged;
