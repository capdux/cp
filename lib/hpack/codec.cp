// Source: cp/lib/hpack/codec.cp (RFC 7541 HPACK Varint, String, Encoder & Decoder)
// Pure CP zero-C-dependency HPACK encoder and decoder supporting all four
// header field representations, dynamic table size updates, and Huffman coding.

const std = @import("std");
import {
    HpackError,
    huffmanEncodedLength,
    huffmanEncode,
    huffmanDecode,
} from "./huffman.cp";
import {
    HeaderField,
    DynamicTable,
    TableMatch,
    lookupEntry,
    findBestMatch,
} from "./table.cp";

/// Indexing behavior for encoding a literal header field.
pub enum IndexingMode {
    Incremental,
    WithoutIndexing,
    NeverIndexed,
}

/// Result of decoding an HPACK variable-length prefix integer.
pub struct IntDecodeResult {
    value: u64 = 0,
    consumed: usize = 0,
}

/// Result of decoding an HPACK string literal.
pub struct StringDecodeResult {
    data: []u8 = [],
    consumed: usize = 0,
}

/// Owned list of decoded HTTP/2 header fields.
pub struct DecodedHeaders {
    alloc: std.mem.Allocator,
    items: []HeaderField,

    pub fn deinit(*_) {
        let i: usize = 0;
        while (i < _.items.len) : (i += 1) {
            _.alloc.free(_.items[i].name);
            _.alloc.free(_.items[i].value);
        }
        if (_.items.len > 0) {
            _.alloc.free(_.items);
            _.items = [];
        }
    }

    pub fn len(): usize {
        return _.items.len;
    }

    pub fn get(index: usize): HeaderField? {
        if (index >= _.items.len) {
            return null;
        }
        return _.items[index];
    }
}

/// Encode an unsigned integer with an N-bit prefix (1 <= prefix_bits <= 8) per RFC 7541 Section 5.1.
pub fn encodeInt(
    out: *std.ArrayList(u8),
    alloc: std.mem.Allocator,
    value: u64,
    prefix_bits: u8,
    prefix_mask: u8,
): ! HpackError {
    if (prefix_bits < 1 || prefix_bits > 8) {
        return HpackError.InvalidVarint;
    }
    const max_prefix: u64 = if (prefix_bits == 8) 255 else (((1 as u64) << (prefix_bits as u6)) - 1);
    if (value < max_prefix) {
        const b = prefix_mask | (value as u8);
        out.append(alloc, b) catch return HpackError.OutOfMemory;
        return;
    }

    out.append(alloc, prefix_mask | (max_prefix as u8)) catch return HpackError.OutOfMemory;
    let rem = value - max_prefix;
    while (rem >= 128) {
        const b = ((rem & 0x7F) as u8) | 0x80;
        out.append(alloc, b) catch return HpackError.OutOfMemory;
        rem >>= 7;
    }
    out.append(alloc, rem as u8) catch return HpackError.OutOfMemory;
}

/// Decode an unsigned integer with an N-bit prefix (1 <= prefix_bits <= 8) per RFC 7541 Section 5.1.
pub fn decodeInt(input: []const u8, prefix_bits: u8): IntDecodeResult ! HpackError {
    if (prefix_bits < 1 || prefix_bits > 8) {
        return HpackError.InvalidVarint;
    }
    if (input.len == 0) {
        return HpackError.TruncatedInput;
    }

    const max_prefix: u64 = if (prefix_bits == 8) 255 else (((1 as u64) << (prefix_bits as u6)) - 1);
    let value: u64 = (input[0] as u64) & max_prefix;
    if (value < max_prefix) {
        return IntDecodeResult{ value: value, consumed: 1 };
    }

    let m: u6 = 0;
    let i: usize = 1;
    while (i < input.len) : (i += 1) {
        const b = input[i];
        if (m >= 63) {
            return HpackError.VarintOverflow;
        }
        const chunk = (b & 0x7F) as u64;
        if (chunk > ((std.math.maxInt(u64) - value) >> m)) {
            return HpackError.VarintOverflow;
        }
        value += chunk << m;
        if ((b & 0x80) == 0) {
            return IntDecodeResult{ value: value, consumed: i + 1 };
        }
        m += 7;
    }

    return HpackError.TruncatedInput;
}

/// Encode an HPACK string literal (raw or Huffman-compressed) per RFC 7541 Section 5.2.
pub fn encodeString(
    out: *std.ArrayList(u8),
    alloc: std.mem.Allocator,
    s: []const u8,
    use_huffman: bool,
): ! HpackError {
    if (use_huffman) {
        const encoded = huffmanEncode(alloc, s)?;
        defer alloc.free(encoded);
        encodeInt(out, alloc, encoded.len as u64, 7, 0x80)?;
        out.appendSlice(alloc, encoded) catch return HpackError.OutOfMemory;
        return;
    }
    encodeInt(out, alloc, s.len as u64, 7, 0x00)?;
    out.appendSlice(alloc, s) catch return HpackError.OutOfMemory;
}

/// Decode an HPACK string literal (raw or Huffman-compressed) per RFC 7541 Section 5.2.
pub fn decodeString(alloc: std.mem.Allocator, input: []const u8): StringDecodeResult ! HpackError {
    if (input.len == 0) {
        return HpackError.TruncatedInput;
    }
    const is_huffman = (input[0] & 0x80) != 0;
    const len_res = decodeInt(input, 7)?;
    if (len_res.value > (std.math.maxInt(usize) as u64)) {
        return HpackError.VarintOverflow;
    }
    const str_len = len_res.value as usize;
    if (str_len > input.len - len_res.consumed) {
        return HpackError.TruncatedInput;
    }
    const raw = input[len_res.consumed .. len_res.consumed + str_len];
    if (is_huffman) {
        const decoded = huffmanDecode(alloc, raw)?;
        return StringDecodeResult{
            data: decoded,
            consumed: len_res.consumed + str_len,
        };
    }
    const dup = alloc.dupe(u8, raw) catch return HpackError.OutOfMemory;
    return StringDecodeResult{
        data: dup,
        consumed: len_res.consumed + str_len,
    };
}

/// Stateful RFC 7541 HPACK Header Encoder.
pub struct HpackEncoder {
    alloc: std.mem.Allocator,
    table: DynamicTable,
    use_huffman: bool = true,
    pending_size_update: bool = false,
    min_pending_size: usize = 0,

    pub static fn init(alloc: std.mem.Allocator, max_table_size: usize): HpackEncoder {
        return HpackEncoder{
            alloc: alloc,
            table: DynamicTable.init(alloc, max_table_size),
            use_huffman: true,
            pending_size_update: false,
            min_pending_size: max_table_size,
        };
    }

    pub fn deinit(*_) {
        _.table.deinit();
    }

    pub fn setHuffman(*_, enabled: bool) {
        _.use_huffman = enabled;
    }

    pub fn setMaxTableSize(*_, new_size: usize) {
        _.table.setMaxSize(new_size);
        if (!_.pending_size_update) {
            _.pending_size_update = true;
            _.min_pending_size = new_size;
        } else if (new_size < _.min_pending_size) {
            _.min_pending_size = new_size;
        }
    }

    pub fn dynamicTableLen(): usize {
        return _.table.len();
    }

    pub fn dynamicTableSize(): usize {
        return _.table.currentSize();
    }

    fn emitPendingSizeUpdates(*_, out: *std.ArrayList(u8)): ! HpackError {
        if (!_.pending_size_update) {
            return;
        }
        const cur_max = _.table.maxSize();
        if (_.min_pending_size < cur_max) {
            encodeInt(out, _.alloc, _.min_pending_size as u64, 5, 0x20)?;
        }
        encodeInt(out, _.alloc, cur_max as u64, 5, 0x20)?;
        _.pending_size_update = false;
        _.min_pending_size = cur_max;
    }

    /// Encode a single header field into `out` using the specified `IndexingMode`.
    pub fn encodeHeader(
        *_,
        out: *std.ArrayList(u8),
        name: []const u8,
        value: []const u8,
        mode: IndexingMode,
    ): ! HpackError {
        _.emitPendingSizeUpdates(out)?;

        const m: TableMatch = findBestMatch(&_.table, name, value);
        if (m.index > 0 && m.exact && mode == IndexingMode.Incremental) {
            encodeInt(out, _.alloc, m.index as u64, 7, 0x80)?;
            return;
        }

        let prefix_bits: u8 = 4;
        let prefix_mask: u8 = 0x00;
        switch (mode) {
            .Incremental => {
                prefix_bits = 6;
                prefix_mask = 0x40;
            },
            .WithoutIndexing => {
                prefix_bits = 4;
                prefix_mask = 0x00;
            },
            .NeverIndexed => {
                prefix_bits = 4;
                prefix_mask = 0x10;
            },
        }

        if (m.index > 0) {
            encodeInt(out, _.alloc, m.index as u64, prefix_bits, prefix_mask)?;
        } else {
            encodeInt(out, _.alloc, 0, prefix_bits, prefix_mask)?;
            encodeString(out, _.alloc, name, _.use_huffman)?;
        }
        encodeString(out, _.alloc, value, _.use_huffman)?;

        if (mode == IndexingMode.Incremental) {
            _.table.insert(name, value)?;
        }
    }

    /// Encode a slice of `HeaderField`s with incremental indexing into an owned byte slice.
    pub fn encode(*_, headers: []const HeaderField): ([]u8) ! HpackError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(_.alloc);

        _.emitPendingSizeUpdates(&out)?;
        let i: usize = 0;
        while (i < headers.len) : (i += 1) {
            _.encodeHeader(&out, headers[i].name, headers[i].value, IndexingMode.Incremental)?;
        }
        return out.toOwnedSlice(_.alloc) catch return HpackError.OutOfMemory;
    }

    /// Encode a slice of `HeaderField`s with per-header `IndexingMode`s into an owned byte slice.
    pub fn encodeWithModes(
        *_,
        headers: []const HeaderField,
        modes: []const IndexingMode,
    ): ([]u8) ! HpackError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(_.alloc);

        _.emitPendingSizeUpdates(&out)?;
        let i: usize = 0;
        while (i < headers.len) : (i += 1) {
            const mode = if (i < modes.len) modes[i] else IndexingMode.Incremental;
            _.encodeHeader(&out, headers[i].name, headers[i].value, mode)?;
        }
        return out.toOwnedSlice(_.alloc) catch return HpackError.OutOfMemory;
    }
}

/// Stateful RFC 7541 HPACK Header Decoder with dynamic table size limit protection.
pub struct HpackDecoder {
    alloc: std.mem.Allocator,
    table: DynamicTable,
    max_allowed_table_size: usize,

    pub static fn init(alloc: std.mem.Allocator, max_allowed_table_size: usize): HpackDecoder {
        return HpackDecoder{
            alloc: alloc,
            table: DynamicTable.init(alloc, max_allowed_table_size),
            max_allowed_table_size: max_allowed_table_size,
        };
    }

    pub fn deinit(*_) {
        _.table.deinit();
    }

    pub fn setMaxAllowedTableSize(*_, limit: usize) {
        _.max_allowed_table_size = limit;
        if (_.table.maxSize() > limit) {
            _.table.setMaxSize(limit);
        }
    }

    pub fn dynamicTableLen(): usize {
        return _.table.len();
    }

    pub fn dynamicTableSize(): usize {
        return _.table.currentSize();
    }

    /// Decode an HPACK header block into an owned `DecodedHeaders` container.
    /// Caller must call `result.deinit()` to free all decoded header strings.
    pub fn decode(*_, block: []const u8): DecodedHeaders ! HpackError {
        let list: std.ArrayList(HeaderField) = .empty;
        errdefer {
            let k: usize = 0;
            while (k < list.items.len) : (k += 1) {
                _.alloc.free(list.items[k].name);
                _.alloc.free(list.items[k].value);
            }
            list.deinit(_.alloc);
        }

        let pos: usize = 0;
        let seen_header: bool = false;

        while (pos < block.len) {
            const first = block[pos];

            if ((first & 0x80) != 0) {
                // 6.1: Indexed Header Field Representation (1xxxxxxx)
                seen_header = true;
                const idx_res = decodeInt(block[pos..], 7)?;
                pos += idx_res.consumed;
                if (idx_res.value == 0 || idx_res.value > (std.math.maxInt(usize) as u64)) {
                    return HpackError.IndexOutOfBounds;
                }
                const entry = lookupEntry(&_.table, idx_res.value as usize) orelse {
                    return HpackError.IndexOutOfBounds;
                };
                const owned_name = _.alloc.dupe(u8, entry.name) catch return HpackError.OutOfMemory;
                const owned_value = _.alloc.dupe(u8, entry.value) catch {
                    _.alloc.free(owned_name);
                    return HpackError.OutOfMemory;
                };
                list.append(_.alloc, HeaderField{ name: owned_name, value: owned_value }) catch {
                    _.alloc.free(owned_name);
                    _.alloc.free(owned_value);
                    return HpackError.OutOfMemory;
                };
            } else if ((first & 0x40) != 0) {
                // 6.2.1: Literal Header Field with Incremental Indexing (01xxxxxx)
                seen_header = true;
                const idx_res = decodeInt(block[pos..], 6)?;
                pos += idx_res.consumed;

                let owned_name: []u8 = [];
                if (idx_res.value > 0) {
                    if (idx_res.value > (std.math.maxInt(usize) as u64)) {
                        return HpackError.IndexOutOfBounds;
                    }
                    const entry = lookupEntry(&_.table, idx_res.value as usize) orelse {
                        return HpackError.IndexOutOfBounds;
                    };
                    owned_name = _.alloc.dupe(u8, entry.name) catch return HpackError.OutOfMemory;
                } else {
                    const name_res = decodeString(_.alloc, block[pos..])?;
                    pos += name_res.consumed;
                    owned_name = name_res.data;
                }

                const val_res = decodeString(_.alloc, block[pos..]) catch |e| {
                    _.alloc.free(owned_name);
                    return e;
                };
                pos += val_res.consumed;

                _.table.insert(owned_name, val_res.data) catch |e| {
                    _.alloc.free(owned_name);
                    _.alloc.free(val_res.data);
                    return e;
                };

                list.append(_.alloc, HeaderField{ name: owned_name, value: val_res.data }) catch {
                    _.alloc.free(owned_name);
                    _.alloc.free(val_res.data);
                    return HpackError.OutOfMemory;
                };
            } else if ((first & 0x20) != 0) {
                // 6.3: Dynamic Table Size Update (001xxxxx)
                if (seen_header) {
                    return HpackError.TableSizeExceeded;
                }
                const sz_res = decodeInt(block[pos..], 5)?;
                pos += sz_res.consumed;
                if (sz_res.value > (_.max_allowed_table_size as u64)) {
                    return HpackError.TableSizeExceeded;
                }
                _.table.setMaxSize(sz_res.value as usize);
            } else {
                // 6.2.2: Without Indexing (0000xxxx) or 6.2.3: Never Indexed (0001xxxx)
                seen_header = true;
                const idx_res = decodeInt(block[pos..], 4)?;
                pos += idx_res.consumed;

                let owned_name: []u8 = [];
                if (idx_res.value > 0) {
                    if (idx_res.value > (std.math.maxInt(usize) as u64)) {
                        return HpackError.IndexOutOfBounds;
                    }
                    const entry = lookupEntry(&_.table, idx_res.value as usize) orelse {
                        return HpackError.IndexOutOfBounds;
                    };
                    owned_name = _.alloc.dupe(u8, entry.name) catch return HpackError.OutOfMemory;
                } else {
                    const name_res = decodeString(_.alloc, block[pos..])?;
                    pos += name_res.consumed;
                    owned_name = name_res.data;
                }

                const val_res = decodeString(_.alloc, block[pos..]) catch |e| {
                    _.alloc.free(owned_name);
                    return e;
                };
                pos += val_res.consumed;

                list.append(_.alloc, HeaderField{ name: owned_name, value: val_res.data }) catch {
                    _.alloc.free(owned_name);
                    _.alloc.free(val_res.data);
                    return HpackError.OutOfMemory;
                };
            }
        }

        const items = list.toOwnedSlice(_.alloc) catch return HpackError.OutOfMemory;
        return DecodedHeaders{
            alloc: _.alloc,
            items: items,
        };
    }
}
