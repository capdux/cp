// Source: /data/dev/bun/src/interchange/interchange.cp
/**
 * CP Standard JSON & Serde (cap:json / cap:serde)
 * 工业级 SIMD 零拷贝流式与编译期强类型序列化与反序列化体系
 */

export { JsonTag, JsonField, JsonValue } from "./value";
export { JsonParser, JsonParseOptions, JsonError, FastJsonScanner } from "./parser";
export { JsonBufferPool, JsonBufferNode } from "./pool";
export { JsonSerializer } from "./serializer";

const std = @import("std");
import { JsonParser, JsonParseOptions } from "./parser";
import { JsonValue } from "./value";
import { JsonSerializer } from "./serializer";

pub class JSON {
    /// High-performance zero-copy dynamic DOM JSON parser (strict RFC 8259)
    pub static fn parse(allocator: std.mem.Allocator, src: []const u8): JsonValue! {
        return JsonParser.parse(allocator, src);
    }

    /// Dynamic DOM JSON parser with configurable options (e.g. max_depth)
    pub static fn parseWithOptions(allocator: std.mem.Allocator, src: []const u8, opts: JsonParseOptions): JsonValue! {
        return JsonParser.parseWithOptions(allocator, src, opts);
    }

    /// High-performance SIMD compiled-in typed JSON parser (replaces std.json.parseFromSliceLeaky)
    pub static fn parseTyped(comptime T: type, allocator: std.mem.Allocator, src: []const u8): T! {
        return JsonParser.parseTyped(T, allocator, src);
    }

    /// Serializes a `JsonValue` AST into compact JSON text
    pub static fn stringifyValue(allocator: std.mem.Allocator, val: JsonValue): ([]u8)! {
        return JsonSerializer.stringify(allocator, val);
    }

    /// Serializes a `JsonValue` AST into pretty-printed JSON text with `indent` spaces per level
    pub static fn stringifyPretty(allocator: std.mem.Allocator, val: JsonValue, indent: usize): ([]u8)! {
        return JsonSerializer.stringifyPretty(allocator, val, indent);
    }

    /// Evaluates an RFC 6901 JSON Pointer query against a `JsonValue`
    pub static fn pointer(val: JsonValue, ptr: []const u8): JsonValue? {
        return val.pointer(ptr);
    }

    /// Deep structural equality comparison of two `JsonValue` trees
    pub static fn equals(a: JsonValue, b: JsonValue): bool {
        return a.equals(b);
    }

    pub fn stringify(obj: auto): string {
        return JSON.stringify(obj);
    }
}

pub const json = new JSON();
