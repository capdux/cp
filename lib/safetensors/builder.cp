// Source: cp/lib/safetensors/builder.cp (Safetensors binary serializer and builder)
const std = @import("std");
import { Dtype, dtypeSize, dtypeName, computeShapeElements } from "./dtype.cp";
import { f32ToBf16, f32ToF16 } from "./float16.cp";
import { MetadataEntry } from "./parser.cp";

struct BuilderTensor {
    name: []const u8,
    dtype: Dtype,
    shape: []usize,
    data_begin: usize,
    data_end: usize,
}

fn appendJsonString(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, s: []const u8): !void {
    try buf.append(alloc, '"');
    for (ch of s) {
        if (ch == '"') {
            try buf.appendSlice(alloc, "\\\"");
        } else if (ch == '\\') {
            try buf.appendSlice(alloc, "\\\\");
        } else if (ch == '\n') {
            try buf.appendSlice(alloc, "\\n");
        } else if (ch == '\r') {
            try buf.appendSlice(alloc, "\\r");
        } else if (ch == '\t') {
            try buf.appendSlice(alloc, "\\t");
        } else {
            try buf.append(alloc, ch);
        }
    }
    try buf.append(alloc, '"');
}

fn appendUsize(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, val: usize): !void {
    let tmp: [32]u8 = undefined;
    const str = try std.fmt.bufPrint(&tmp, "{d}", .{val});
    try buf.appendSlice(alloc, str);
}

pub struct SafetensorsBuilder {
    alloc: std.mem.Allocator,
    tensors: std.ArrayList(BuilderTensor),
    metadata: std.ArrayList(MetadataEntry),
    data_buf: std.ArrayList(u8),

    pub static fn init(alloc: std.mem.Allocator): SafetensorsBuilder {
        return SafetensorsBuilder{
            .alloc = alloc,
            .tensors = .empty,
            .metadata = .empty,
            .data_buf = .empty,
        };
    }

    pub fn deinit(*_) {
        for (t of _.tensors.items) {
            _.alloc.free(t.name);
            _.alloc.free(t.shape);
        }
        _.tensors.deinit(_.alloc);
        for (m of _.metadata.items) {
            _.alloc.free(m.key);
            _.alloc.free(m.value);
        }
        _.metadata.deinit(_.alloc);
        _.data_buf.deinit(_.alloc);
    }

    pub fn addMetadata(*_, key: []const u8, value: []const u8): !void {
        const k_copy = try _.alloc.dupe(u8, key);
        errdefer _.alloc.free(k_copy);
        const v_copy = try _.alloc.dupe(u8, value);
        try _.metadata.append(_.alloc, MetadataEntry{
            .key = k_copy,
            .value = v_copy,
        });
    }

    pub fn addTensorRaw(*_, name: []const u8, dtype: Dtype, shape: []const usize, raw_data: []const u8): !void {
        for (existing of _.tensors.items) {
            if (std.mem.eql(u8, existing.name, name)) {
                return error.DuplicateTensorName;
            }
        }
        const elem_count = try computeShapeElements(shape);
        const expected_mul = @mulWithOverflow(elem_count, dtypeSize(dtype));
        if (expected_mul[1] != 0 or expected_mul[0] != raw_data.len) {
            return error.ShapeSizeMismatch;
        }

        const name_copy = try _.alloc.dupe(u8, name);
        errdefer _.alloc.free(name_copy);
        const shape_copy = try _.alloc.dupe(usize, shape);
        errdefer _.alloc.free(shape_copy);

        const begin = _.data_buf.items.len;
        try _.data_buf.appendSlice(_.alloc, raw_data);
        const end = _.data_buf.items.len;

        try _.tensors.append(_.alloc, BuilderTensor{
            .name = name_copy,
            .dtype = dtype,
            .shape = shape_copy,
            .data_begin = begin,
            .data_end = end,
        });
    }

    pub fn addTensorF32(*_, name: []const u8, shape: []const usize, data: []const f32): !void {
        const elem_count = try computeShapeElements(shape);
        if (elem_count != data.len) {
            return error.ShapeSizeMismatch;
        }
        const byte_len = data.len * 4;
        let tmp = try _.alloc.alloc(u8, byte_len);
        defer _.alloc.free(tmp);
        let i: usize = 0;
        for (v of data) {
            const bits: u32 = @bitCast(v);
            std.mem.writeInt(u32, tmp[i * 4 ..][0..4], bits, .little);
            i += 1;
        }
        try _.addTensorRaw(name, .F32, shape, tmp);
    }

    pub fn addTensorBF16FromF32(*_, name: []const u8, shape: []const usize, data: []const f32): !void {
        const elem_count = try computeShapeElements(shape);
        if (elem_count != data.len) {
            return error.ShapeSizeMismatch;
        }
        const byte_len = data.len * 2;
        let tmp = try _.alloc.alloc(u8, byte_len);
        defer _.alloc.free(tmp);
        let i: usize = 0;
        for (v of data) {
            const bits = f32ToBf16(v);
            std.mem.writeInt(u16, tmp[i * 2 ..][0..2], bits, .little);
            i += 1;
        }
        try _.addTensorRaw(name, .BF16, shape, tmp);
    }

    pub fn addTensorF16FromF32(*_, name: []const u8, shape: []const usize, data: []const f32): !void {
        const elem_count = try computeShapeElements(shape);
        if (elem_count != data.len) {
            return error.ShapeSizeMismatch;
        }
        const byte_len = data.len * 2;
        let tmp = try _.alloc.alloc(u8, byte_len);
        defer _.alloc.free(tmp);
        let i: usize = 0;
        for (v of data) {
            const bits = f32ToF16(v);
            std.mem.writeInt(u16, tmp[i * 2 ..][0..2], bits, .little);
            i += 1;
        }
        try _.addTensorRaw(name, .F16, shape, tmp);
    }

    pub fn addTensorI32(*_, name: []const u8, shape: []const usize, data: []const i32): !void {
        const elem_count = try computeShapeElements(shape);
        if (elem_count != data.len) {
            return error.ShapeSizeMismatch;
        }
        const byte_len = data.len * 4;
        let tmp = try _.alloc.alloc(u8, byte_len);
        defer _.alloc.free(tmp);
        let i: usize = 0;
        for (v of data) {
            std.mem.writeInt(i32, tmp[i * 4 ..][0..4], v, .little);
            i += 1;
        }
        try _.addTensorRaw(name, .I32, shape, tmp);
    }

    pub fn build(alloc: std.mem.Allocator): ![]u8 {
        let header: std.ArrayList(u8) = .empty;
        defer header.deinit(alloc);

        try header.append(alloc, '{');
        let need_comma = false;

        if (_.metadata.items.len > 0) {
            try header.appendSlice(alloc, "\"__metadata__\":{");
            let idx: usize = 0;
            for (m of _.metadata.items) {
                if (idx > 0) {
                    try header.append(alloc, ',');
                }
                try appendJsonString(&header, alloc, m.key);
                try header.append(alloc, ':');
                try appendJsonString(&header, alloc, m.value);
                idx += 1;
            }
            try header.append(alloc, '}');
            need_comma = true;
        }

        for (t of _.tensors.items) {
            if (need_comma) {
                try header.append(alloc, ',');
            }
            need_comma = true;

            try appendJsonString(&header, alloc, t.name);
            try header.appendSlice(alloc, ":{\"dtype\":\"");
            try header.appendSlice(alloc, dtypeName(t.dtype));
            try header.appendSlice(alloc, "\",\"shape\":[");
            let idx: usize = 0;
            for (dim of t.shape) {
                if (idx > 0) {
                    try header.append(alloc, ',');
                }
                try appendUsize(&header, alloc, dim);
                idx += 1;
            }
            try header.appendSlice(alloc, "],\"data_offsets\":[");
            try appendUsize(&header, alloc, t.data_begin);
            try header.append(alloc, ',');
            try appendUsize(&header, alloc, t.data_end);
            try header.appendSlice(alloc, "]}");
        }
        try header.append(alloc, '}');

        // Pad header to 8-byte alignment with ASCII spaces
        const unpadded_len = header.items.len;
        const pad_len = (8 - (unpadded_len % 8)) % 8;
        let p: usize = 0;
        while (p < pad_len) {
            try header.append(alloc, ' ');
            p += 1;
        }

        const header_len = header.items.len;
        const total_len = 8 + header_len + _.data_buf.items.len;
        let out = try alloc.alloc(u8, total_len);
        std.mem.writeInt(u64, out[0..8], @as(u64, header_len), .little);
        @memcpy(out[8 .. 8 + header_len], header.items);
        @memcpy(out[8 + header_len .. total_len], _.data_buf.items);
        return out;
    }
}
