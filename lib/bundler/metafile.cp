// Source: Absorbed from /data/dev/bun/src/bundler/linker_context/MetafileBuilder.cp | Synced: 2026-11-22T04:28:26Z
// lib/bundler/metafile.cp
// esbuild/Bun-compatible Metafile graph builder, JSON serializer & colored bundle size analysis tree

const std = @import("std");
import { ImportKind } from "./bundle.cp";

pub struct MetafileImport {
    pub path: []const u8 = "",
    pub kind: ImportKind = .ImportStmt,
    pub original: []const u8 = "",
    pub external: bool = false,
    pub with_type: []const u8 = "",
}

pub struct MetafileInput {
    pub path: []const u8 = "",
    pub bytes: usize = 0,
    pub format: []const u8 = "esm",
    pub imports: std.ArrayList(MetafileImport) = .empty,
}

pub struct MetafileOutputInput {
    pub path: []const u8 = "",
    pub bytes_in_output: usize = 0,
}

type MetaStr = []const u8;

pub struct MetafileOutput {
    pub path: []const u8 = "",
    pub bytes: usize = 0,
    pub entry_point: []const u8 = "",
    pub css_bundle: []const u8 = "",
    pub inputs: std.ArrayList(MetafileOutputInput) = .empty,
    pub imports: std.ArrayList(MetafileImport) = .empty,
    pub exports: std.ArrayList(MetaStr) = .empty,
}

pub err MetafileError {
    OutOfMemory,
    InvalidIndex,
}

pub struct Metafile {
    inputs: std.ArrayList(MetafileInput) = .empty,
    outputs: std.ArrayList(MetafileOutput) = .empty,
    allocator: std.mem.Allocator = std.heap.page_allocator,

    pub static fn init(allocator: std.mem.Allocator): Metafile {
        return Metafile{
            inputs: .empty,
            outputs: .empty,
            allocator: allocator,
        };
    }

    pub fn deinit(*_) {
        for (*inp of _.inputs.items) {
            inp.imports.deinit(_.allocator);
        }
        _.inputs.deinit(_.allocator);
        for (*outp of _.outputs.items) {
            outp.inputs.deinit(_.allocator);
            outp.imports.deinit(_.allocator);
            outp.exports.deinit(_.allocator);
        }
        _.outputs.deinit(_.allocator);
    }

    pub fn inputCount(): usize {
        return _.inputs.items.len;
    }

    pub fn outputCount(): usize {
        return _.outputs.items.len;
    }

    pub fn findInput(path: []const u8): usize? {
        let i: usize = 0;
        while (i < _.inputs.items.len) : (i += 1) {
            if (std.mem.eql(u8, _.inputs.items[i].path, path)) {
                return i;
            }
        }
        return null;
    }

    pub fn findOutput(path: []const u8): usize? {
        let i: usize = 0;
        while (i < _.outputs.items.len) : (i += 1) {
            if (std.mem.eql(u8, _.outputs.items[i].path, path)) {
                return i;
            }
        }
        return null;
    }

    pub fn addInput(*_, path: []const u8, bytes: usize, format: []const u8 = "esm"): usize! {
        if (existing := _.findInput(path)) {
            _.inputs.items[existing].bytes = bytes;
            if (format.len > 0) {
                _.inputs.items[existing].format = format;
            }
            return existing;
        }
        const idx = _.inputs.items.len;
        _.inputs.append(_.allocator, MetafileInput{
            path: path,
            bytes: bytes,
            format: format,
            imports: .empty,
        }) catch return MetafileError.OutOfMemory;
        return idx;
    }

    pub fn addInputImport(*_, input_idx: usize, imp: MetafileImport): ! {
        if (input_idx >= _.inputs.items.len) {
            return MetafileError.InvalidIndex;
        }
        _.inputs.items[input_idx].imports.append(_.allocator, imp) catch return MetafileError.OutOfMemory;
    }

    pub fn addOutput(*_, path: []const u8, bytes: usize, entry_point: []const u8 = ""): usize! {
        if (existing := _.findOutput(path)) {
            _.outputs.items[existing].bytes = bytes;
            if (entry_point.len > 0) {
                _.outputs.items[existing].entry_point = entry_point;
            }
            return existing;
        }
        const idx = _.outputs.items.len;
        _.outputs.append(_.allocator, MetafileOutput{
            path: path,
            bytes: bytes,
            entry_point: entry_point,
            css_bundle: "",
            inputs: .empty,
            imports: .empty,
            exports: .empty,
        }) catch return MetafileError.OutOfMemory;
        return idx;
    }

    pub fn addOutputInput(*_, output_idx: usize, input_path: []const u8, bytes_in_output: usize): ! {
        if (output_idx >= _.outputs.items.len) {
            return MetafileError.InvalidIndex;
        }
        _.outputs.items[output_idx].inputs.append(_.allocator, MetafileOutputInput{
            path: input_path,
            bytes_in_output: bytes_in_output,
        }) catch return MetafileError.OutOfMemory;
    }

    pub fn addOutputImport(*_, output_idx: usize, imp: MetafileImport): ! {
        if (output_idx >= _.outputs.items.len) {
            return MetafileError.InvalidIndex;
        }
        _.outputs.items[output_idx].imports.append(_.allocator, imp) catch return MetafileError.OutOfMemory;
    }

    pub fn addOutputExport(*_, output_idx: usize, export_name: []const u8): ! {
        if (output_idx >= _.outputs.items.len) {
            return MetafileError.InvalidIndex;
        }
        _.outputs.items[output_idx].exports.append(_.allocator, export_name) catch return MetafileError.OutOfMemory;
    }

    pub fn setOutputCssBundle(*_, output_idx: usize, css_path: []const u8): ! {
        if (output_idx >= _.outputs.items.len) {
            return MetafileError.InvalidIndex;
        }
        _.outputs.items[output_idx].css_bundle = css_path;
    }

    pub fn totalInputBytes(): usize {
        let total: usize = 0;
        for (inp of _.inputs.items) {
            total += inp.bytes;
        }
        return total;
    }

    pub fn totalOutputBytes(): usize {
        let total: usize = 0;
        for (outp of _.outputs.items) {
            total += outp.bytes;
        }
        return total;
    }

    static fn appendJsonStr(out: *std.ArrayList(u8), alloc: std.mem.Allocator, s: []const u8): ! {
        out.append(alloc, '"') catch return MetafileError.OutOfMemory;
        for (ch of s) {
            switch (ch) {
                '"' => out.appendSlice(alloc, "\\\"") catch return MetafileError.OutOfMemory,
                '\\' => out.appendSlice(alloc, "\\\\") catch return MetafileError.OutOfMemory,
                '\n' => out.appendSlice(alloc, "\\n") catch return MetafileError.OutOfMemory,
                '\r' => out.appendSlice(alloc, "\\r") catch return MetafileError.OutOfMemory,
                '\t' => out.appendSlice(alloc, "\\t") catch return MetafileError.OutOfMemory,
                else => {
                    if (ch < 0x20) {
                        let esc: [6]u8 = undefined;
                        const txt = std.fmt.bufPrint(&esc, "\\u{x:0>4}", .{ch}) catch "";
                        out.appendSlice(alloc, txt) catch return MetafileError.OutOfMemory;
                    } else {
                        out.append(alloc, ch) catch return MetafileError.OutOfMemory;
                    }
                },
            }
        }
        out.append(alloc, '"') catch return MetafileError.OutOfMemory;
    }

    static fn appendFmt(out: *std.ArrayList(u8), alloc: std.mem.Allocator, txt: []const u8): ! {
        out.appendSlice(alloc, txt) catch return MetafileError.OutOfMemory;
    }

    /// Serializes the Metafile into esbuild/Bun-compatible JSON `{ "inputs": ..., "outputs": ... }`.
    pub fn toJsonAlloc(alloc: std.mem.Allocator): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        let num_buf: [64]u8 = undefined;

        Metafile.appendFmt(*out, alloc, "{\n  \"inputs\": {")?;
        let i: usize = 0;
        while (i < _.inputs.items.len) : (i += 1) {
            const inp = _.inputs.items[i];
            if (i > 0) Metafile.appendFmt(*out, alloc, ",")?;
            Metafile.appendFmt(*out, alloc, "\n    ")?;
            Metafile.appendJsonStr(*out, alloc, inp.path)?;
            Metafile.appendFmt(*out, alloc, ": {\n      \"bytes\": ")?;
            const btxt = std.fmt.bufPrint(&num_buf, "{d}", .{inp.bytes}) catch "0";
            Metafile.appendFmt(*out, alloc, btxt)?;
            Metafile.appendFmt(*out, alloc, ",\n      \"imports\": [")?;

            let j: usize = 0;
            while (j < inp.imports.items.len) : (j += 1) {
                const imp = inp.imports.items[j];
                if (j > 0) Metafile.appendFmt(*out, alloc, ",")?;
                Metafile.appendFmt(*out, alloc, "\n        {\n          \"path\": ")?;
                Metafile.appendJsonStr(*out, alloc, imp.path)?;
                Metafile.appendFmt(*out, alloc, ",\n          \"kind\": ")?;
                Metafile.appendJsonStr(*out, alloc, imp.kind.label())?;
                if (imp.original.len > 0 && !std.mem.eql(u8, imp.original, imp.path)) {
                    Metafile.appendFmt(*out, alloc, ",\n          \"original\": ")?;
                    Metafile.appendJsonStr(*out, alloc, imp.original)?;
                }
                if (imp.external) {
                    Metafile.appendFmt(*out, alloc, ",\n          \"external\": true")?;
                }
                if (imp.with_type.len > 0) {
                    Metafile.appendFmt(*out, alloc, ",\n          \"with\": { \"type\": ")?;
                    Metafile.appendJsonStr(*out, alloc, imp.with_type)?;
                    Metafile.appendFmt(*out, alloc, " }")?;
                }
                Metafile.appendFmt(*out, alloc, "\n        }")?;
            }
            if (inp.imports.items.len > 0) Metafile.appendFmt(*out, alloc, "\n      ")?;
            Metafile.appendFmt(*out, alloc, "]")?;

            if (inp.format.len > 0) {
                Metafile.appendFmt(*out, alloc, ",\n      \"format\": ")?;
                Metafile.appendJsonStr(*out, alloc, inp.format)?;
            }
            Metafile.appendFmt(*out, alloc, "\n    }")?;
        }

        Metafile.appendFmt(*out, alloc, "\n  },\n  \"outputs\": {")?;
        let o_i: usize = 0;
        while (o_i < _.outputs.items.len) : (o_i += 1) {
            const outp = _.outputs.items[o_i];
            if (o_i > 0) Metafile.appendFmt(*out, alloc, ",")?;
            Metafile.appendFmt(*out, alloc, "\n    ")?;
            Metafile.appendJsonStr(*out, alloc, outp.path)?;
            Metafile.appendFmt(*out, alloc, ": {\n      \"bytes\": ")?;
            const obtxt = std.fmt.bufPrint(&num_buf, "{d}", .{outp.bytes}) catch "0";
            Metafile.appendFmt(*out, alloc, obtxt)?;

            Metafile.appendFmt(*out, alloc, ",\n      \"inputs\": {")?;
            let ii: usize = 0;
            while (ii < outp.inputs.items.len) : (ii += 1) {
                const oi = outp.inputs.items[ii];
                if (ii > 0) Metafile.appendFmt(*out, alloc, ",")?;
                Metafile.appendFmt(*out, alloc, "\n        ")?;
                Metafile.appendJsonStr(*out, alloc, oi.path)?;
                Metafile.appendFmt(*out, alloc, ": {\n          \"bytesInOutput\": ")?;
                const oib = std.fmt.bufPrint(&num_buf, "{d}", .{oi.bytes_in_output}) catch "0";
                Metafile.appendFmt(*out, alloc, oib)?;
                Metafile.appendFmt(*out, alloc, "\n        }")?;
            }
            if (outp.inputs.items.len > 0) Metafile.appendFmt(*out, alloc, "\n      ")?;
            Metafile.appendFmt(*out, alloc, "}")?;

            Metafile.appendFmt(*out, alloc, ",\n      \"imports\": [")?;
            let ci: usize = 0;
            while (ci < outp.imports.items.len) : (ci += 1) {
                const cimp = outp.imports.items[ci];
                if (ci > 0) Metafile.appendFmt(*out, alloc, ",")?;
                Metafile.appendFmt(*out, alloc, "\n        {\n          \"path\": ")?;
                Metafile.appendJsonStr(*out, alloc, cimp.path)?;
                Metafile.appendFmt(*out, alloc, ",\n          \"kind\": ")?;
                Metafile.appendJsonStr(*out, alloc, cimp.kind.label())?;
                Metafile.appendFmt(*out, alloc, "\n        }")?;
            }
            if (outp.imports.items.len > 0) Metafile.appendFmt(*out, alloc, "\n      ")?;
            Metafile.appendFmt(*out, alloc, "]")?;

            Metafile.appendFmt(*out, alloc, ",\n      \"exports\": [")?;
            let ei: usize = 0;
            while (ei < outp.exports.items.len) : (ei += 1) {
                if (ei > 0) Metafile.appendFmt(*out, alloc, ", ")?;
                Metafile.appendJsonStr(*out, alloc, outp.exports.items[ei])?;
            }
            Metafile.appendFmt(*out, alloc, "]")?;

            if (outp.entry_point.len > 0) {
                Metafile.appendFmt(*out, alloc, ",\n      \"entryPoint\": ")?;
                Metafile.appendJsonStr(*out, alloc, outp.entry_point)?;
            }
            if (outp.css_bundle.len > 0) {
                Metafile.appendFmt(*out, alloc, ",\n      \"cssBundle\": ")?;
                Metafile.appendJsonStr(*out, alloc, outp.css_bundle)?;
            }
            Metafile.appendFmt(*out, alloc, "\n    }")?;
        }
        Metafile.appendFmt(*out, alloc, "\n  }\n}\n")?;
        return out.toOwnedSlice(alloc) catch return MetafileError.OutOfMemory;
    }

    static fn formatSize(bytes: usize, buf: []u8): []const u8 {
        if (bytes >= 1024 * 1024) {
            const whole = bytes / (1024 * 1024);
            const tenths = ((bytes % (1024 * 1024)) * 10) / (1024 * 1024);
            return std.fmt.bufPrint(buf, "{d}.{d} MB", .{ whole, tenths }) catch "";
        }
        if (bytes >= 1024) {
            const whole = bytes / 1024;
            const tenths = ((bytes % 1024) * 10) / 1024;
            return std.fmt.bufPrint(buf, "{d}.{d} KB", .{ whole, tenths }) catch "";
        }
        return std.fmt.bufPrint(buf, "{d} B", .{bytes}) catch "";
    }

    /// Generates an esbuild/Bun-style colored size-breakdown tree report for all output chunks & inputs.
    pub fn formatAnalysisTable(alloc: std.mem.Allocator, use_color: bool = true): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        let sbuf: [32]u8 = undefined;
        let line_buf: [256]u8 = undefined;

        const c_bold = if (use_color) "\x1b[1m" else "";
        const c_cyan = if (use_color) "\x1b[36m" else "";
        const c_green = if (use_color) "\x1b[32m" else "";
        const c_yellow = if (use_color) "\x1b[33m" else "";
        const c_dim = if (use_color) "\x1b[2m" else "";
        const c_reset = if (use_color) "\x1b[0m" else "";

        const total_out = _.totalOutputBytes();
        const total_str = Metafile.formatSize(total_out, &sbuf);
        const hdr = std.fmt.bufPrint(
            &line_buf,
            "{s}Bundle Analysis{s} ({d} outputs, {d} inputs, total {s}{s}{s})\n",
            .{ c_bold, c_reset, _.outputs.items.len, _.inputs.items.len, c_green, total_str, c_reset },
        ) catch "";
        Metafile.appendFmt(*out, alloc, hdr)?;

        let o_i: usize = 0;
        while (o_i < _.outputs.items.len) : (o_i += 1) {
            const outp = _.outputs.items[o_i];
            const osz = Metafile.formatSize(outp.bytes, &sbuf);
            const o_pct: usize = if (total_out > 0) ((outp.bytes * 1000) / total_out) else 0;
            const o_line = std.fmt.bufPrint(
                &line_buf,
                "\n  {s}{s}{s}  {s}{s}{s} ({d}.{d}%)",
                .{ c_cyan, outp.path, c_reset, c_bold, osz, c_reset, o_pct / 10, o_pct % 10 },
            ) catch "";
            Metafile.appendFmt(*out, alloc, o_line)?;
            if (outp.entry_point.len > 0) {
                const ep_line = std.fmt.bufPrint(&line_buf, " {s}[entry: {s}]{s}", .{ c_dim, outp.entry_point, c_reset }) catch "";
                Metafile.appendFmt(*out, alloc, ep_line)?;
            }
            Metafile.appendFmt(*out, alloc, "\n")?;

            // Sort a temporary copy of output inputs by bytes_in_output descending
            if (outp.inputs.items.len > 0) {
                let sorted = alloc.dupe(MetafileOutputInput, outp.inputs.items) catch return MetafileError.OutOfMemory;
                defer alloc.free(sorted);

                let a: usize = 1;
                while (a < sorted.len) : (a += 1) {
                    const key = sorted[a];
                    let b: usize = a;
                    while (b > 0 && key.bytes_in_output > sorted[b - 1].bytes_in_output) {
                        sorted[b] = sorted[b - 1];
                        b -= 1;
                    }
                    sorted[b] = key;
                }

                let k: usize = 0;
                while (k < sorted.len) : (k += 1) {
                    const item = sorted[k];
                    const branch = if (k + 1 == sorted.len) "└─" else "├─";
                    const isz = Metafile.formatSize(item.bytes_in_output, &sbuf);
                    const pct_permille: usize = if (outp.bytes > 0) ((item.bytes_in_output * 1000) / outp.bytes) else 0;
                    const row = std.fmt.bufPrint(
                        &line_buf,
                        "   {s}{s}{s} {s}  {s}{s}{s} ({d}.{d}%)\n",
                        .{ c_dim, branch, c_reset, item.path, c_yellow, isz, c_reset, pct_permille / 10, pct_permille % 10 },
                    ) catch "";
                    Metafile.appendFmt(*out, alloc, row)?;
                }
            }
        }
        return out.toOwnedSlice(alloc) catch return MetafileError.OutOfMemory;
    }
}
