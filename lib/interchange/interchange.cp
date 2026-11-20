// Source: bun/src/interchange/interchange.cp -> cp/lib/interchange/interchange.cp (Unified Interchange Facade)
// Format auto-detection, unified parse/stringify, and lossless cross-format conversion.

const std = @import("std");
import {
    InterchangeFormat,
    InterchangeError,
    InterchangeValue,
} from "./value.cp";
import { Json5Codec } from "./json5.cp";
import { TomlYamlBridge } from "./bridge.cp";

pub class Interchange {
    pub static fn detectFormat(path_or_ext: []const u8): InterchangeFormat? {
        if (path_or_ext.len == 0) return null;
        let ext = path_or_ext;
        if (dot := std.mem.lastIndexOfScalar(u8, path_or_ext, '.')) {
            ext = path_or_ext[dot + 1 ..];
        }
        if (ext.len == 0) return null;

        if (std.ascii.eqlIgnoreCase(ext, "json")) return InterchangeFormat.Json;
        if (std.ascii.eqlIgnoreCase(ext, "jsonc")) return InterchangeFormat.Jsonc;
        if (std.ascii.eqlIgnoreCase(ext, "json5")) return InterchangeFormat.Json5;
        if (std.ascii.eqlIgnoreCase(ext, "toml")) return InterchangeFormat.Toml;
        if (std.ascii.eqlIgnoreCase(ext, "yaml") || std.ascii.eqlIgnoreCase(ext, "yml")) {
            return InterchangeFormat.Yaml;
        }
        return null;
    }

    pub static fn parse(
        alloc: std.mem.Allocator,
        input: []const u8,
        format: InterchangeFormat,
    ): InterchangeValue! {
        return switch (format) {
            .Json | .Jsonc | .Json5 => Json5Codec.parse(alloc, input, format),
            .Toml => TomlYamlBridge.parseToml(alloc, input),
            .Yaml => TomlYamlBridge.parseYaml(alloc, input),
        };
    }

    pub static fn parseAuto(
        alloc: std.mem.Allocator,
        input: []const u8,
        path_or_ext: []const u8,
    ): InterchangeValue! {
        const fmt = Interchange.detectFormat(path_or_ext) orelse return InterchangeError.UnsupportedFormat;
        return Interchange.parse(alloc, input, fmt);
    }

    pub static fn stringify(
        alloc: std.mem.Allocator,
        val: InterchangeValue,
        format: InterchangeFormat,
    ): ([]u8)! {
        return Interchange.stringifyPretty(alloc, val, format, false);
    }

    pub static fn stringifyPretty(
        alloc: std.mem.Allocator,
        val: InterchangeValue,
        format: InterchangeFormat,
        pretty: bool,
    ): ([]u8)! {
        return switch (format) {
            .Json | .Jsonc | .Json5 => Json5Codec.stringify(alloc, val, format, pretty),
            .Toml => TomlYamlBridge.stringifyToml(alloc, val),
            .Yaml => TomlYamlBridge.stringifyYaml(alloc, val),
        };
    }

    pub static fn convert(
        alloc: std.mem.Allocator,
        input: []const u8,
        from_fmt: InterchangeFormat,
        to_fmt: InterchangeFormat,
    ): ([]u8)! {
        let val = Interchange.parse(alloc, input, from_fmt)?;
        defer val.deinit(alloc);
        return Interchange.stringify(alloc, val, to_fmt);
    }

    pub static fn convertPretty(
        alloc: std.mem.Allocator,
        input: []const u8,
        from_fmt: InterchangeFormat,
        to_fmt: InterchangeFormat,
        pretty: bool,
    ): ([]u8)! {
        let val = Interchange.parse(alloc, input, from_fmt)?;
        defer val.deinit(alloc);
        return Interchange.stringifyPretty(alloc, val, to_fmt, pretty);
    }
}

pub fn detectFormat(path_or_ext: []const u8): InterchangeFormat? {
    return Interchange.detectFormat(path_or_ext);
}

pub fn parse(
    alloc: std.mem.Allocator,
    input: []const u8,
    format: InterchangeFormat,
): InterchangeValue! {
    return Interchange.parse(alloc, input, format);
}

pub fn stringify(
    alloc: std.mem.Allocator,
    val: InterchangeValue,
    format: InterchangeFormat,
): ([]u8)! {
    return Interchange.stringify(alloc, val, format);
}
