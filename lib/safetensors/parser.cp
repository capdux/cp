// Source: cp/lib/safetensors/parser.cp (Safetensors binary parser and zero-copy tensor views)
const std = @import("std");
import { Dtype, dtypeSize, parseDtype, computeShapeElements } from "./dtype.cp";
import { bf16ToF32, f16ToF32 } from "./float16.cp";

pub const MAX_HEADER_SIZE: u64 = 100 * 1024 * 1024;

pub struct TensorInfo {
    name: []const u8,
    dtype: Dtype,
    shape: []usize,
    data_begin: usize,
    data_end: usize,
}

pub struct TensorView {
    name: []const u8,
    dtype: Dtype,
    shape: []const usize,
    data: []const u8,
}

pub struct MetadataEntry {
    key: []const u8,
    value: []const u8,
}

pub struct SafetensorsFile {
    alloc: std.mem.Allocator,
    raw_bytes: []const u8,
    header_len: usize,
    data_offset: usize,
    tensors: []TensorInfo,
    metadata: []MetadataEntry,

    pub fn deinit(*_) {
        for (t of _.tensors) {
            _.alloc.free(t.name);
            _.alloc.free(t.shape);
        }
        _.alloc.free(_.tensors);
        for (m of _.metadata) {
            _.alloc.free(m.key);
            _.alloc.free(m.value);
        }
        _.alloc.free(_.metadata);
    }

    pub fn tensorCount(): usize {
        return _.tensors.len;
    }

    pub fn getMetadata(key: []const u8): ([]const u8)? {
        for (m of _.metadata) {
            if (std.mem.eql(u8, m.key, key)) {
                return m.value;
            }
        }
        return null;
    }

    pub fn getTensor(name: []const u8): TensorView? {
        for (t of _.tensors) {
            if (std.mem.eql(u8, t.name, name)) {
                const abs_begin = _.data_offset + t.data_begin;
                const abs_end = _.data_offset + t.data_end;
                return TensorView{
                    .name = t.name,
                    .dtype = t.dtype,
                    .shape = t.shape,
                    .data = _.raw_bytes[abs_begin..abs_end],
                };
            }
        }
        return null;
    }

    pub fn tensorAsF32(alloc: std.mem.Allocator, name: []const u8): ![]f32 {
        const view = _.getTensor(name) orelse return error.TensorNotFound;
        if (view.dtype == .F32) {
            const count = view.data.len / 4;
            let out = try alloc.alloc(f32, count);
            let i: usize = 0;
            while (i < count) {
                const off = i * 4;
                const bits = std.mem.readInt(u32, view.data[off..][0..4], .little);
                out[i] = @bitCast(bits);
                i += 1;
            }
            return out;
        }
        if (view.dtype == .BF16) {
            const count = view.data.len / 2;
            let out = try alloc.alloc(f32, count);
            let i: usize = 0;
            while (i < count) {
                const off = i * 2;
                const bits = std.mem.readInt(u16, view.data[off..][0..2], .little);
                out[i] = bf16ToF32(bits);
                i += 1;
            }
            return out;
        }
        if (view.dtype == .F16) {
            const count = view.data.len / 2;
            let out = try alloc.alloc(f32, count);
            let i: usize = 0;
            while (i < count) {
                const off = i * 2;
                const bits = std.mem.readInt(u16, view.data[off..][0..2], .little);
                out[i] = f16ToF32(bits);
                i += 1;
            }
            return out;
        }
        return error.UnsupportedConversion;
    }
}

const Interval = struct {
    begin: usize,
    end: usize,
};

fn intervalLessThan(_: void, a: Interval, b: Interval): bool {
    if (a.begin != b.begin) {
        return a.begin < b.begin;
    }
    return a.end < b.end;
}

fn jsonIntToUsize(val: std.json.Value): usize? {
    return switch (val) {
        .integer => |n| blk: {
            if (n < 0) { break :blk null; }
            break :blk std.math.cast(usize, n);
        },
        else => null,
    };
}

pub fn parseSafetensors(alloc: std.mem.Allocator, bytes: []const u8): !SafetensorsFile {
    if (bytes.len < 8) {
        return error.InvalidHeaderLength;
    }
    const header_len_u64 = std.mem.readInt(u64, bytes[0..8], .little);
    if (header_len_u64 > MAX_HEADER_SIZE) {
        return error.HeaderTooLarge;
    }
    if (header_len_u64 == 0) {
        return error.InvalidJson;
    }
    const header_len = std.math.cast(usize, header_len_u64) orelse return error.HeaderTooLarge;
    if (bytes.len - 8 < header_len) {
        return error.OutOfBounds;
    }

    const header_bytes = bytes[8 .. 8 + header_len];
    const data_offset = 8 + header_len;
    const payload_len = bytes.len - data_offset;

    let parsed = std.json.parseFromSlice(std.json.Value, alloc, header_bytes, .{}) catch {
        return error.InvalidJson;
    };
    defer parsed.deinit();

    const root_obj = switch (parsed.value) {
        .object => |o| o,
        else => return error.InvalidJson,
    };

    let tensor_list: std.ArrayList(TensorInfo) = .empty;
    errdefer {
        for (t of tensor_list.items) {
            alloc.free(t.name);
            alloc.free(t.shape);
        }
        tensor_list.deinit(alloc);
    }

    let meta_list: std.ArrayList(MetadataEntry) = .empty;
    errdefer {
        for (m of meta_list.items) {
            alloc.free(m.key);
            alloc.free(m.value);
        }
        meta_list.deinit(alloc);
    }

    let intervals: std.ArrayList(Interval) = .empty;
    defer intervals.deinit(alloc);

    const obj_keys = root_obj.keys();
    const obj_vals = root_obj.values();
    let k_idx: usize = 0;
    while (k_idx < obj_keys.len) {
        const key = obj_keys[k_idx];
        const val = obj_vals[k_idx];
        k_idx += 1;

        if (std.mem.eql(u8, key, "__metadata__")) {
            const meta_obj = switch (val) {
                .object => |o| o,
                else => return error.InvalidJson,
            };
            const m_keys = meta_obj.keys();
            const m_vals = meta_obj.values();
            let m_idx: usize = 0;
            while (m_idx < m_keys.len) {
                const mkey = m_keys[m_idx];
                const mval = m_vals[m_idx];
                m_idx += 1;
                const mstr = switch (mval) {
                    .string => |s| s,
                    else => return error.InvalidJson,
                };
                const k_copy = try alloc.dupe(u8, mkey);
                errdefer alloc.free(k_copy);
                const v_copy = try alloc.dupe(u8, mstr);
                try meta_list.append(alloc, MetadataEntry{
                    .key = k_copy,
                    .value = v_copy,
                });
            }
            continue;
        }

        const t_obj = switch (val) {
            .object => |o| o,
            else => return error.InvalidJson,
        };

        for (existing of tensor_list.items) {
            if (std.mem.eql(u8, existing.name, key)) {
                return error.DuplicateTensorName;
            }
        }

        const dtype_val = t_obj.get("dtype") orelse return error.InvalidDtype;
        const dtype_str = switch (dtype_val) {
            .string => |s| s,
            else => return error.InvalidDtype,
        };
        const dt = parseDtype(dtype_str) orelse return error.InvalidDtype;

        const shape_val = t_obj.get("shape") orelse return error.InvalidShape;
        const shape_arr = switch (shape_val) {
            .array => |a| a,
            else => return error.InvalidShape,
        };
        let shape_slice = try alloc.alloc(usize, shape_arr.items.len);
        errdefer alloc.free(shape_slice);
        let idx: usize = 0;
        for (dim_val of shape_arr.items) {
            shape_slice[idx] = jsonIntToUsize(dim_val) orelse return error.InvalidShape;
            idx += 1;
        }

        const offsets_val = t_obj.get("data_offsets") orelse return error.InvalidOffsets;
        const offsets_arr = switch (offsets_val) {
            .array => |a| a,
            else => return error.InvalidOffsets,
        };
        if (offsets_arr.items.len != 2) {
            return error.InvalidOffsets;
        }
        const begin = jsonIntToUsize(offsets_arr.items[0]) orelse return error.InvalidOffsets;
        const end = jsonIntToUsize(offsets_arr.items[1]) orelse return error.InvalidOffsets;

        if (begin > end) {
            return error.InvalidOffsets;
        }
        if (end > payload_len) {
            return error.OutOfBounds;
        }

        const elem_count = try computeShapeElements(shape_slice);
        const expected_mul = @mulWithOverflow(elem_count, dtypeSize(dt));
        if (expected_mul[1] != 0) {
            return error.ShapeSizeMismatch;
        }
        if ((end - begin) != expected_mul[0]) {
            return error.ShapeSizeMismatch;
        }

        if (end > begin) {
            try intervals.append(alloc, Interval{ .begin = begin, .end = end });
        }

        const name_copy = try alloc.dupe(u8, key);
        errdefer alloc.free(name_copy);

        try tensor_list.append(alloc, TensorInfo{
            .name = name_copy,
            .dtype = dt,
            .shape = shape_slice,
            .data_begin = begin,
            .data_end = end,
        });
    }

    // Overlap check: sort non-empty intervals and verify prev.end <= curr.begin
    if (intervals.items.len > 1) {
        std.mem.sort(Interval, intervals.items, {}, intervalLessThan);
        let i: usize = 1;
        while (i < intervals.items.len) {
            if (intervals.items[i - 1].end > intervals.items[i].begin) {
                return error.OverlappingTensors;
            }
            i += 1;
        }
    }

    const tensors_owned = try tensor_list.toOwnedSlice(alloc);
    const meta_owned = try meta_list.toOwnedSlice(alloc);

    return SafetensorsFile{
        .alloc = alloc,
        .raw_bytes = bytes,
        .header_len = header_len,
        .data_offset = data_offset,
        .tensors = tensors_owned,
        .metadata = meta_owned,
    };
}
