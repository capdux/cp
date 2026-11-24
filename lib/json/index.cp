// Source: Absorbed from /data/dev/bun/src/interchange/interchange.cp | Synced: 2026-11-22T11:07:59Z
/**
 * CP Standard JSON & Serde (cap:json / cap:serde)
 * 工业级 SIMD 零拷贝流式与编译期强类型序列化与反序列化体系
 */

export { JsonTag, JsonField, JsonValue } from "./value";
export {
    JsonParser,
    JsonParseOptions,
    JsonError,
    FastJsonScanner,
    JSON_DEFAULT_MAX_DEPTH,
    validateJsonStrict,
    validateJsonWithMaxDepth,
    validateJsonStrictOrErr,
} from "./parser";
export { JsonBufferPool, JsonBufferNode } from "./pool";
export { JsonSerializer, JsonSerializeOptions } from "./serializer";
export {
    JsonPointer,
    JsonPatchOpKind,
    JsonPatchOp,
    JsonPatch,
    JsonPatchError,
    JsonMergePatch,
} from "./pointer";
export { NdjsonReader, NdjsonWriter, NdjsonError } from "./ndjson";

export {
    simdIndexOfJsonEscape,
    needsJsonEscape,
    escapeJsonContent,
    quoteJsonString,
    unescapeJsonContent,
} from "./escape.cp";

export {
    minifyJson,
    minifyJsonc,
    minifyJsoncWithOptions,
} from "./minify.cp";

export {
    isValidPointer,
    parseArrayIndex,
    unescapeToken,
    escapeToken,
    PointerIter,
    appendToken,
    parentPointer,
} from "./ptr.cp";

export {
    JSON_LINE_DEFAULT_COMPACTION_THRESHOLD,
    JsonLineBufferError,
    JsonLineNext,
    JsonLineBuffer,
    stripUtf8Bom,
} from "./lineBuffer.cp";

export {
    Json5TokenKind,
    Json5Token,
    Json5Lexer,
    toStandardJson,
} from "./json5.cp";

const std = @import("std");
import { JsonParser, JsonParseOptions, validateJsonStrict, validateJsonWithMaxDepth } from "./parser";
import { JsonValue } from "./value";
import { JsonSerializer, JsonSerializeOptions } from "./serializer";
import { JsonPatch, JsonPatchOp, JsonMergePatch } from "./pointer";
import { NdjsonReader, NdjsonWriter } from "./ndjson";

pub class JSON {
    /// Zero-heap strict RFC 8259 validator (default max depth 128, matching `serde-rs/json`)
    pub static fn validate(src: []const u8): bool {
        return validateJsonStrict(src);
    }

    /// Zero-heap strict RFC 8259 validator with explicit maximum recursion depth
    pub static fn validateWithMaxDepth(src: []const u8, max_depth: usize): bool {
        return validateJsonWithMaxDepth(src, max_depth);
    }

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

    /// Serializes a `JsonValue` AST with configurable `JsonSerializeOptions`
    pub static fn stringifyWithOptions(
        allocator: std.mem.Allocator,
        val: JsonValue,
        opts: JsonSerializeOptions,
    ): ([]u8)! {
        return JsonSerializer.stringifyWithOptions(allocator, val, opts);
    }

    /// Serializes a `JsonValue` AST into deterministic RFC 8785 Canonical JSON
    pub static fn stringifyCanonical(allocator: std.mem.Allocator, val: JsonValue): ([]u8)! {
        return JsonSerializer.stringifyCanonical(allocator, val);
    }

    /// Atomically applies an RFC 6902 JSON Patch operation list to a `JsonValue`
    pub static fn patch(
        allocator: std.mem.Allocator,
        doc: JsonValue,
        ops: []const JsonPatchOp,
    ): JsonValue! {
        return JsonPatch.apply(allocator, doc, ops);
    }

    /// Applies an RFC 7396 JSON Merge Patch to `target`
    pub static fn mergePatch(
        allocator: std.mem.Allocator,
        target: JsonValue,
        patch_val: JsonValue,
    ): JsonValue! {
        return JsonMergePatch.apply(allocator, target, patch_val);
    }

    /// Parses an NDJSON / JSON Lines payload into a slice of `JsonValue` records
    pub static fn parseNdjson(allocator: std.mem.Allocator, src: []const u8): ([]JsonValue)! {
        return NdjsonReader.parseAll(allocator, src);
    }

    /// Serializes a slice of `JsonValue` records into compact NDJSON (`\n`-delimited)
    pub static fn stringifyNdjson(allocator: std.mem.Allocator, values: []const JsonValue): ([]u8)! {
        return NdjsonWriter.stringifyAll(allocator, values);
    }

    /// Evaluates an RFC 6901 JSON Pointer query against a `JsonValue`
    pub static fn pointer(val: JsonValue, ptr: []const u8): JsonValue? {
        return val.pointer(ptr);
    }

    /// Deep structural equality comparison of two `JsonValue` trees
    pub static fn equals(a: JsonValue, b: JsonValue): bool {
        return a.equals(b);
    }

    pub static fn stringify(obj: auto): string {
        return JSON.stringify(obj);
    }
}

pub const json = new JSON();

// === W29-Sync R7: Consolidated RFC 6902 JSON Patch & RFC 7396 Merge Patch ===
import json_patch_mod from "./jsonPatch.cp";
pub const Rfc6902PatchOp = json_patch_mod.JsonPatchOp;
pub const Rfc6902PatchError = json_patch_mod.JsonPatchError;

export {
    PatchOpKind,
    freeValue,
    cloneValue,
    valuesEqual,
    resolvePointer,
    applyPatch,
    mergePatch,
    stringifyValue,
    freeOps,
    applyPatchJson,
    diff,
} from "./jsonPatch.cp";
