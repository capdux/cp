// Source: cp/lib/lzw/lzw.cp (Pure CP Variable-Width LZW Dictionary Compression Codec)
// Supports GIF (LsbFirst) and TIFF/PDF (MsbFirst) bit orders, 2..8 min_code_size, up to 12-bit dynamic codes.

const std = @import("std");

/// Error set for LZW compression and decompression.
pub err LzwError {
    InvalidMinCodeSize,
    InvalidCode,
    MissingEoiCode,
    TruncatedInput,
    OutputTooLarge,
    OutOfMemory,
}

/// Bit packing order for variable-width LZW codes.
pub enum LzwBitOrder {
    /// Least-significant bit first (GIF style).
    LsbFirst,
    /// Most-significant bit first (TIFF / PDF style).
    MsbFirst,
}

/// Configuration for LZW compression and decompression.
pub struct LzwConfig {
    min_code_size: u8 = 8,
    bit_order: LzwBitOrder = LzwBitOrder.LsbFirst,
    max_code_size: u8 = 12,
}

const HASH_SIZE: usize = 8192;
const HASH_MASK: usize = HASH_SIZE - 1;
const MAX_DICT_ENTRIES: usize = 4096;
const EMPTY_KEY: u32 = 0xffffffff;
const ROOT_PREFIX: u16 = 0xffff;

fn validateConfig(config: LzwConfig): void ! LzwError {
    if (config.min_code_size < 2 || config.min_code_size > 8) {
        return LzwError.InvalidMinCodeSize;
    }
    if (config.max_code_size > 12 || config.max_code_size <= config.min_code_size) {
        return LzwError.InvalidMinCodeSize;
    }
}

struct LzwBitWriter {
    bit_order: LzwBitOrder,
    bit_buf: u32 = 0,
    bit_count: u6 = 0,

    fn writeCode(*_, alloc: std.mem.Allocator, out: *std.ArrayList(u8), code: u16, width: u8): void ! LzwError {
        const w: u5 = width as u5;
        if (_.bit_order == LzwBitOrder.LsbFirst) {
            _.bit_buf |= (code as u32) << _.bit_count;
            _.bit_count += w;
            while (_.bit_count >= 8) {
                const byte = (_.bit_buf & 0xff) as u8;
                out.append(alloc, byte) catch return LzwError.OutOfMemory;
                _.bit_buf >>= 8;
                _.bit_count -= 8;
            }
        } else {
            _.bit_buf = (_.bit_buf << w) | (code as u32);
            _.bit_count += w;
            while (_.bit_count >= 8) {
                const shift: u5 = (_.bit_count - 8) as u5;
                const byte = ((_.bit_buf >> shift) & 0xff) as u8;
                out.append(alloc, byte) catch return LzwError.OutOfMemory;
                _.bit_count -= 8;
                if (_.bit_count == 0) {
                    _.bit_buf = 0;
                } else {
                    const mask = ((1 as u32) << (_.bit_count as u5)) - 1;
                    _.bit_buf &= mask;
                }
            }
        }
    }

    fn flush(*_, alloc: std.mem.Allocator, out: *std.ArrayList(u8)): void ! LzwError {
        if (_.bit_count > 0) {
            if (_.bit_order == LzwBitOrder.LsbFirst) {
                const byte = (_.bit_buf & 0xff) as u8;
                out.append(alloc, byte) catch return LzwError.OutOfMemory;
            } else {
                const shift: u5 = (8 - _.bit_count) as u5;
                const byte = ((_.bit_buf << shift) & 0xff) as u8;
                out.append(alloc, byte) catch return LzwError.OutOfMemory;
            }
            _.bit_buf = 0;
            _.bit_count = 0;
        }
    }
}

struct LzwBitReader {
    data: []const u8,
    bit_order: LzwBitOrder,
    byte_pos: usize = 0,
    bit_buf: u32 = 0,
    bit_count: u6 = 0,

    fn readCode(*_, width: u8): u16 ! LzwError {
        const w: u5 = width as u5;
        while (_.bit_count < width) {
            if (_.byte_pos >= _.data.len) {
                if (_.bit_count > 0) {
                    return LzwError.TruncatedInput;
                }
                return LzwError.MissingEoiCode;
            }
            const b = _.data[_.byte_pos] as u32;
            _.byte_pos += 1;
            if (_.bit_order == LzwBitOrder.LsbFirst) {
                _.bit_buf |= b << (_.bit_count as u5);
            } else {
                _.bit_buf = (_.bit_buf << 8) | b;
            }
            _.bit_count += 8;
        }

        const mask: u32 = ((1 as u32) << w) - 1;
        if (_.bit_order == LzwBitOrder.LsbFirst) {
            const code = (_.bit_buf & mask) as u16;
            _.bit_buf >>= w;
            _.bit_count -= width;
            return code;
        } else {
            const shift: u5 = (_.bit_count - width) as u5;
            const code = ((_.bit_buf >> shift) & mask) as u16;
            _.bit_count -= width;
            if (_.bit_count == 0) {
                _.bit_buf = 0;
            } else {
                const rem_mask = ((1 as u32) << (_.bit_count as u5)) - 1;
                _.bit_buf &= rem_mask;
            }
            return code;
        }
    }
}

fn hashKey(prefix: u16, byte: u8): usize {
    const k = ((prefix as u32) << 8) | (byte as u32);
    const h = k ^ (k >> 7) ^ (k >> 13);
    return (h as usize) & HASH_MASK;
}

/// Compresses `input` using variable-width LZW dictionary compression.
/// Caller owns the returned slice and must free it with `allocator.free(...)`.
pub fn compress(allocator: std.mem.Allocator, input: []const u8, config: LzwConfig): []u8 ! LzwError {
    validateConfig(config)?;

    const clear_code: u16 = (1 as u16) << (config.min_code_size as u4);
    const eoi_code: u16 = clear_code + 1;
    const first_free: u16 = clear_code + 2;
    const max_table_size: u16 = (1 as u16) << (config.max_code_size as u4);
    const init_code_size: u8 = config.min_code_size + 1;

    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(allocator);

    let writer = LzwBitWriter{ bit_order: config.bit_order };
    let code_size: u8 = init_code_size;

    // Always emit initial ClearCode.
    writer.writeCode(allocator, &out, clear_code, code_size)?;

    if (input.len == 0) {
        writer.writeCode(allocator, &out, eoi_code, code_size)?;
        writer.flush(allocator, &out)?;
        return out.toOwnedSlice(allocator) catch return LzwError.OutOfMemory;
    }

    let hash_keys = allocator.alloc(u32, HASH_SIZE) catch return LzwError.OutOfMemory;
    defer allocator.free(hash_keys);
    let hash_vals = allocator.alloc(u16, HASH_SIZE) catch return LzwError.OutOfMemory;
    defer allocator.free(hash_vals);

    @memset(hash_keys, EMPTY_KEY);

    const first_b = input[0];
    if ((first_b as u16) >= clear_code) {
        return LzwError.InvalidCode;
    }

    let prefix: u16 = first_b as u16;
    let next_code: u16 = first_free;

    let i: usize = 1;
    while (i < input.len) : (i += 1) {
        const b = input[i];
        if ((b as u16) >= clear_code) {
            return LzwError.InvalidCode;
        }

        const key: u32 = ((prefix as u32) << 8) | (b as u32);
        let idx = hashKey(prefix, b);
        let found = false;
        let found_code: u16 = 0;

        while (true) {
            const slot = hash_keys[idx];
            if (slot == EMPTY_KEY) {
                break;
            }
            if (slot == key) {
                found = true;
                found_code = hash_vals[idx];
                break;
            }
            idx = (idx + 1) & HASH_MASK;
        }

        if (found) {
            prefix = found_code;
        } else {
            writer.writeCode(allocator, &out, prefix, code_size)?;

            // Insert (prefix, b) -> next_code
            hash_keys[idx] = key;
            hash_vals[idx] = next_code;
            next_code += 1;

            if (next_code == max_table_size) {
                // Table full: emit ClearCode at current code_size and reset dictionary.
                writer.writeCode(allocator, &out, clear_code, code_size)?;
                @memset(hash_keys, EMPTY_KEY);
                next_code = first_free;
                code_size = init_code_size;
            } else {
                const limit: u16 = (1 as u16) << (code_size as u4);
                if (next_code == limit && code_size < config.max_code_size) {
                    code_size += 1;
                }
            }

            prefix = b as u16;
        }
    }

    writer.writeCode(allocator, &out, prefix, code_size)?;
    writer.writeCode(allocator, &out, eoi_code, code_size)?;
    writer.flush(allocator, &out)?;

    return out.toOwnedSlice(allocator) catch return LzwError.OutOfMemory;
}

/// Decompresses LZW-compressed bytes using `config`.
/// Caller owns the returned slice and must free it with `allocator.free(...)`.
pub fn decompress(allocator: std.mem.Allocator, compressed: []const u8, config: LzwConfig): []u8 ! LzwError {
    return decompressWithMaxSize(allocator, compressed, config, std.math.maxInt(usize));
}

/// Decompresses LZW-compressed bytes with a strict output size limit `max_bytes`.
/// Returns `LzwError.OutputTooLarge` if decoded output exceeds `max_bytes`.
pub fn decompressWithMaxSize(
    allocator: std.mem.Allocator,
    compressed: []const u8,
    config: LzwConfig,
    max_bytes: usize,
): []u8 ! LzwError {
    validateConfig(config)?;

    const clear_code: u16 = (1 as u16) << (config.min_code_size as u4);
    const eoi_code: u16 = clear_code + 1;
    const first_free: u16 = clear_code + 2;
    const max_table_size: u16 = (1 as u16) << (config.max_code_size as u4);
    const init_code_size: u8 = config.min_code_size + 1;

    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(allocator);

    let dict_prefix = allocator.alloc(u16, MAX_DICT_ENTRIES) catch return LzwError.OutOfMemory;
    defer allocator.free(dict_prefix);
    let dict_char = allocator.alloc(u8, MAX_DICT_ENTRIES) catch return LzwError.OutOfMemory;
    defer allocator.free(dict_char);
    let dict_first = allocator.alloc(u8, MAX_DICT_ENTRIES) catch return LzwError.OutOfMemory;
    defer allocator.free(dict_first);
    let dict_len = allocator.alloc(u16, MAX_DICT_ENTRIES) catch return LzwError.OutOfMemory;
    defer allocator.free(dict_len);
    let stack = allocator.alloc(u8, MAX_DICT_ENTRIES) catch return LzwError.OutOfMemory;
    defer allocator.free(stack);

    let init_i: u16 = 0;
    while (init_i < clear_code) : (init_i += 1) {
        dict_prefix[init_i] = ROOT_PREFIX;
        dict_char[init_i] = init_i as u8;
        dict_first[init_i] = init_i as u8;
        dict_len[init_i] = 1;
    }

    let reader = LzwBitReader{
        data: compressed,
        bit_order: config.bit_order,
    };

    let code_size: u8 = init_code_size;
    let next_code: u16 = first_free;
    let has_prev = false;
    let prev_code: u16 = 0;

    while (true) {
        const code = reader.readCode(code_size)?;

        if (code == eoi_code) {
            break;
        }

        if (code == clear_code) {
            code_size = init_code_size;
            next_code = first_free;
            has_prev = false;
            continue;
        }

        if (!has_prev) {
            if (code >= clear_code) {
                return LzwError.InvalidCode;
            }
            if (out.items.len + 1 > max_bytes) {
                return LzwError.OutputTooLarge;
            }
            out.append(allocator, code as u8) catch return LzwError.OutOfMemory;
            prev_code = code;
            has_prev = true;
            continue;
        }

        if (code > next_code) {
            return LzwError.InvalidCode;
        }

        let first_char: u8 = 0;
        let entry_len: usize = 0;
        let is_kwkwk = false;

        if (code == next_code) {
            // Classic KwKwK boundary case: code == next_code
            is_kwkwk = true;
            first_char = dict_first[prev_code];
            entry_len = (dict_len[prev_code] as usize) + 1;
        } else {
            first_char = dict_first[code];
            entry_len = dict_len[code] as usize;
        }

        if (out.items.len + entry_len > max_bytes) {
            return LzwError.OutputTooLarge;
        }

        // Expand decoded sequence in reverse onto `stack`, then emit forward.
        let sp: usize = 0;
        let cur: u16 = if (is_kwkwk) { prev_code } else { code };
        while (cur != ROOT_PREFIX) {
            if (sp >= MAX_DICT_ENTRIES) {
                return LzwError.InvalidCode;
            }
            stack[sp] = dict_char[cur];
            sp += 1;
            cur = dict_prefix[cur];
        }

        while (sp > 0) {
            sp -= 1;
            out.append(allocator, stack[sp]) catch return LzwError.OutOfMemory;
        }
        if (is_kwkwk) {
            out.append(allocator, first_char) catch return LzwError.OutOfMemory;
        }

        if (next_code < max_table_size) {
            dict_prefix[next_code] = prev_code;
            dict_char[next_code] = first_char;
            dict_first[next_code] = dict_first[prev_code];
            dict_len[next_code] = dict_len[prev_code] + 1;
            next_code += 1;

            const limit: u16 = (1 as u16) << (code_size as u4);
            if (next_code == limit && code_size < config.max_code_size) {
                code_size += 1;
            }
        }

        prev_code = code;
    }

    return out.toOwnedSlice(allocator) catch return LzwError.OutOfMemory;
}
