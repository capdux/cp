// Source: cp/lib/tlv/decoder.cp (Strict ASN.1 DER Recursive TLV Decoder)
// Pure CP zero-C-dependency strict DER decoder with max_depth guard and zero-copy value slices.

const std = @import("std");
import {
    TLV_MAX_DEPTH,
    TlvError,
    TlvTag,
    TlvNode,
    decodeDerLength,
} from "./types.cp";

struct NodeDecodeResult {
    node: TlvNode = TlvNode{},
    bytes_read: usize = 0,
}

fn freeNodeList(alloc: std.mem.Allocator, list: []TlvNode, count: usize) {
    let i: usize = 0;
    while (i < count) : (i += 1) {
        list[i].deinit(alloc);
    }
    if (list.len > 0) {
        alloc.free(list);
    }
}

fn decodeNodeAt(alloc: std.mem.Allocator, bytes: []const u8, depth: usize, max_depth: usize): NodeDecodeResult ! TlvError {
    if (depth > max_depth) {
        return TlvError.MaxDepthExceeded;
    }

    const tag_res = TlvTag.decode(bytes)?;
    const len_res = decodeDerLength(bytes[tag_res.bytes_read ..])?;
    const header_len = tag_res.bytes_read + len_res.bytes_read;

    if (bytes.len - header_len < len_res.length) {
        return TlvError.UnexpectedEof;
    }

    const raw_value = bytes[header_len .. header_len + len_res.length];
    const total_len = header_len + len_res.length;

    if (!tag_res.tag.constructed) {
        return NodeDecodeResult{
            node: TlvNode{
                tag: tag_res.tag,
                raw_value: raw_value,
                children: [],
            },
            bytes_read: total_len,
        };
    }

    if (raw_value.len == 0) {
        return NodeDecodeResult{
            node: TlvNode{
                tag: tag_res.tag,
                raw_value: raw_value,
                children: [],
            },
            bytes_read: total_len,
        };
    }

    let cap: usize = 4;
    let list = alloc.alloc(TlvNode, cap) catch return TlvError.OutOfMemory;
    let count: usize = 0;
    errdefer freeNodeList(alloc, list, count);

    let offset: usize = 0;
    while (offset < raw_value.len) {
        const child_res = decodeNodeAt(alloc, raw_value[offset ..], depth + 1, max_depth)?;
        if (count == cap) {
            const new_cap = cap * 2;
            const next_list = alloc.realloc(list, new_cap) catch return TlvError.OutOfMemory;
            list = next_list;
            cap = new_cap;
        }
        list[count] = child_res.node;
        count += 1;
        offset += child_res.bytes_read;
    }

    let final_children = alloc.alloc(TlvNode, count) catch return TlvError.OutOfMemory;
    let i: usize = 0;
    while (i < count) : (i += 1) {
        final_children[i] = list[i];
    }
    alloc.free(list);

    return NodeDecodeResult{
        node: TlvNode{
            tag: tag_res.tag,
            raw_value: raw_value,
            children: final_children,
        },
        bytes_read: total_len,
    };
}

/// Decodes a single top-level DER TLV element using the default `TLV_MAX_DEPTH` (32).
/// Rejects trailing bytes after the root element with `TlvError.TrailingBytes`.
pub fn decodeDer(alloc: std.mem.Allocator, bytes: []const u8): TlvNode ! TlvError {
    return decodeDerWithMaxDepth(alloc, bytes, TLV_MAX_DEPTH);
}

/// Decodes a single top-level DER TLV element with a custom `max_depth` recursion guard.
/// Rejects trailing bytes after the root element with `TlvError.TrailingBytes`.
pub fn decodeDerWithMaxDepth(alloc: std.mem.Allocator, bytes: []const u8, max_depth: usize): TlvNode ! TlvError {
    let res = decodeNodeAt(alloc, bytes, 0, max_depth)?;
    if (res.bytes_read != bytes.len) {
        res.node.deinit(alloc);
        return TlvError.TrailingBytes;
    }
    return res.node;
}
