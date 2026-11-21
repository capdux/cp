// Source: Absorbed from /data/dev/bun/src/router/Router.cp | Synced: 2026-11-20T05:04:52Z
// lib/router/pattern.cp
// Reverse Route Path Formatter & Parameter Placeholder Extractor
// Absorbed from Bun src/router/

const std = @import("std");
import { ParamMap } from "./param_map.cp";

pub struct RouteFormatter {
    pub static fn appendSlice(out: []u8, pos: usize, s: []const u8): usize! {
        if (pos + s.len > out.len) {
            return error.BufferTooSmall;
        }
        let i: usize = 0;
        while (i < s.len) {
            out[pos + i] = s[i];
            i += 1;
        }
        return pos + s.len;
    }

    pub static fn lookupParam(params: *const ParamMap, name: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < params.len) {
            if (std.mem.eql(u8, params.entries[i].name, name)) {
                return params.entries[i].value;
            }
            i += 1;
        }
        return null;
    }

    /// Extracts all `:param` placeholder names from `pattern` into `out_names` and returns the count.
    pub static fn extractParamNames(pattern: []const u8, out_names: [][]const u8): usize {
        let count: usize = 0;
        let iter = std.mem.splitScalar(u8, pattern, '/');
        while (seg := iter.next()) {
            if (seg.len > 1 && seg[0] == ':') {
                if (count >= out_names.len) {
                    break;
                }
                out_names[count] = seg[1..];
                count += 1;
            }
        }
        return count;
    }

    /// Substitutes `:param` and `*` segments in `pattern` using `params` and writes the resolved path into `out`.
    pub static fn formatPath(pattern: []const u8, params: *const ParamMap, out: []u8): usize! {
        if (pattern.len == 0) {
            return 0 as usize;
        }
        let pos: usize = 0;
        let iter = std.mem.splitScalar(u8, pattern, '/');
        let first: bool = true;

        while (seg := iter.next()) {
            if (first && seg.len == 0 && pattern[0] == '/') {
                first = false;
                continue;
            }
            first = false;

            if (pos + 1 > out.len) {
                return error.BufferTooSmall;
            }
            out[pos] = '/';
            pos += 1;

            if (seg.len > 1 && seg[0] == ':') {
                const param_name = seg[1..];
                const val_opt = RouteFormatter.lookupParam(params, param_name);
                if (val_opt == null) {
                    return error.MissingRouteParam;
                }
                pos = RouteFormatter.appendSlice(out, pos, val_opt.?)?;
            } else if (std.mem.eql(u8, seg, "*")) {
                const wild_opt = RouteFormatter.lookupParam(params, "*");
                if (wild_opt == null) {
                    return error.MissingRouteParam;
                }
                pos = RouteFormatter.appendSlice(out, pos, wild_opt.?)?;
            } else {
                pos = RouteFormatter.appendSlice(out, pos, seg)?;
            }
        }

        if (pos == 0 && pattern[0] == '/') {
            if (out.len < 1) {
                return error.BufferTooSmall;
            }
            out[0] = '/';
            return 1 as usize;
        }
        return pos;
    }
}
