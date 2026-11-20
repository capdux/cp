// Source: cp/lib/jsonpath/query.cp (RFC 9535 JSONPath Evaluation Engine)
// Pure CP zero-C-dependency JSONPath query evaluator with Normalized Path generation,
// slice/union/descendant traversal, and RFC 9535 filter expression evaluation.

const std = @import("std");
import { JsonValue, JsonTag, JSON } from "cap:json";
import {
    JsonPathError,
    CompiledProgram,
    QueryPlan,
    Segment,
    Selector,
    FilterNode,
    FilterKind,
} from "./parser.cp";

pub struct PathMatch {
    path: string,
    value: JsonValue,
}

struct NodeItem {
    path: []const u8,
    value: JsonValue,
}

struct EvalValue {
    exists: bool = false,
    value: JsonValue = .{},
}

pub struct JsonPath {
    alloc: std.mem.Allocator;
    program: CompiledProgram;

    pub static fn compile(alloc: std.mem.Allocator, expr: []const u8): JsonPath ! JsonPathError {
        const prog = CompiledProgram.compile(alloc, expr)?;
        return JsonPath{
            alloc: alloc,
            program: prog,
        };
    }

    pub fn deinit(*_) {
        _.program.deinit();
    }

    pub static fn freeMatches(alloc: std.mem.Allocator, matches: []PathMatch) {
        for (m of matches) {
            if (m.path.len > 0) {
                alloc.free(m.path);
            }
        }
        if (matches.len > 0) {
            alloc.free(matches);
        }
    }

    pub fn query(alloc: std.mem.Allocator, root: JsonValue): ([]PathMatch)! {
        let items = _.evalQuery(alloc, 0, root, root, true)?;
        defer {
            for (it of items.items) {
                if (it.path.len > 0) alloc.free(it.path);
            }
            items.deinit(alloc);
        }

        let out = alloc.alloc(PathMatch, items.items.len)?;
        for (i of 0..items.items.len) {
            out[i] = PathMatch{
                path: items.items[i].path,
                value: items.items[i].value,
            };
            items.items[i].path = "";
        }
        return out;
    }

    pub fn queryValues(alloc: std.mem.Allocator, root: JsonValue): ([]JsonValue)! {
        let items = _.evalQuery(alloc, 0, root, root, false)?;
        defer items.deinit(alloc);

        let out = alloc.alloc(JsonValue, items.items.len)?;
        for (i of 0..items.items.len) {
            out[i] = items.items[i].value;
        }
        return out;
    }

    pub fn queryOne(root: JsonValue): JsonValue? {
        let items = _.evalQuery(_.alloc, 0, root, root, false) catch return null;
        defer items.deinit(_.alloc);
        if (items.items.len == 0) return null;
        return items.items[0].value;
    }

    pub fn exists(root: JsonValue): bool {
        let items = _.evalQuery(_.alloc, 0, root, root, false) catch return false;
        defer items.deinit(_.alloc);
        return items.items.len > 0;
    }

    pub static fn evalJson(alloc: std.mem.Allocator, json_str: []const u8, expr: []const u8): ([]PathMatch)! {
        const root = JSON.parse(alloc, json_str)?;
        let jp = JsonPath.compile(alloc, expr)?;
        defer jp.deinit();
        return jp.query(alloc, root);
    }

    fn evalQuery(
        alloc: std.mem.Allocator,
        query_id: usize,
        root: JsonValue,
        current: JsonValue,
        track_paths: bool,
    ): (std.ArrayList(NodeItem))! {
        const plan = _.program.queries.items[query_id];
        const start_val = if (plan.is_root) root else current;

        let current_list: std.ArrayList(NodeItem) = .empty;
        errdefer {
            if (track_paths) {
                for (it of current_list.items) {
                    if (it.path.len > 0) alloc.free(it.path);
                }
            }
            current_list.deinit(alloc);
        }

        let init_path: []u8 = "";
        if (track_paths) {
            init_path = alloc.dupe(u8, "$")?;
        }
        current_list.append(alloc, NodeItem{ path: init_path, value: start_val }) catch |e| {
            if (init_path.len > 0) alloc.free(init_path);
            return e;
        };

        for (seg of plan.segments) {
            let next_list: std.ArrayList(NodeItem) = .empty;
            errdefer {
                if (track_paths) {
                    for (it of next_list.items) {
                        if (it.path.len > 0) alloc.free(it.path);
                    }
                }
                next_list.deinit(alloc);
            }

            for (node of current_list.items) {
                if (seg.is_descendant) {
                    _.applyDescendant(alloc, root, node.path, node.value, seg.selectors, *next_list, track_paths)?;
                } else {
                    _.applySelectors(alloc, root, node.path, node.value, seg.selectors, *next_list, track_paths)?;
                }
            }

            if (track_paths) {
                for (it of current_list.items) {
                    if (it.path.len > 0) alloc.free(it.path);
                }
            }
            current_list.deinit(alloc);
            current_list = next_list;
        }

        return current_list;
    }

    fn applyDescendant(
        alloc: std.mem.Allocator,
        root: JsonValue,
        base_path: []const u8,
        val: JsonValue,
        selectors: []Selector,
        out: *std.ArrayList(NodeItem),
        track_paths: bool,
    ): ! {
        // Apply selectors to the current node itself first (RFC 9535 pre-order)
        _.applySelectors(alloc, root, base_path, val, selectors, out, track_paths)?;

        if (val.tag == .Array) {
            for (i of 0..val.arr_val.len) {
                const idx = i as usize;
                const child_path = if (track_paths) JsonPath.formatIndexPath(alloc, base_path, idx)? else "";
                errdefer if (child_path.len > 0) alloc.free(child_path);
                _.applyDescendant(alloc, root, child_path, val.arr_val[idx], selectors, out, track_paths)?;
                if (child_path.len > 0) alloc.free(child_path);
            }
        } else if (val.tag == .Object) {
            for (field of val.obj_val) {
                const child_path = if (track_paths) JsonPath.formatKeyPath(alloc, base_path, field.key)? else "";
                errdefer if (child_path.len > 0) alloc.free(child_path);
                _.applyDescendant(alloc, root, child_path, field.value, selectors, out, track_paths)?;
                if (child_path.len > 0) alloc.free(child_path);
            }
        }
    }

    fn applySelectors(
        alloc: std.mem.Allocator,
        root: JsonValue,
        base_path: []const u8,
        val: JsonValue,
        selectors: []Selector,
        out: *std.ArrayList(NodeItem),
        track_paths: bool,
    ): ! {
        for (sel of selectors) {
            switch (sel.kind) {
                .Name => {
                    if (val.tag == .Object) {
                        for (field of val.obj_val) {
                            if (std.mem.eql(u8, field.key, sel.name)) {
                                const p = if (track_paths) JsonPath.formatKeyPath(alloc, base_path, field.key)? else "";
                                out.append(alloc, NodeItem{ path: p, value: field.value }) catch |e| {
                                    if (p.len > 0) alloc.free(p);
                                    return e;
                                };
                            }
                        }
                    }
                },
                .Wildcard => {
                    if (val.tag == .Object) {
                        for (field of val.obj_val) {
                            const p = if (track_paths) JsonPath.formatKeyPath(alloc, base_path, field.key)? else "";
                            out.append(alloc, NodeItem{ path: p, value: field.value }) catch |e| {
                                if (p.len > 0) alloc.free(p);
                                return e;
                            };
                        }
                    } else if (val.tag == .Array) {
                        for (i of 0..val.arr_val.len) {
                            const idx = i as usize;
                            const p = if (track_paths) JsonPath.formatIndexPath(alloc, base_path, idx)? else "";
                            out.append(alloc, NodeItem{ path: p, value: val.arr_val[idx] }) catch |e| {
                                if (p.len > 0) alloc.free(p);
                                return e;
                            };
                        }
                    }
                },
                .Index => {
                    if (val.tag == .Array) {
                        const len = val.arr_val.len as i64;
                        const norm = if (sel.index >= 0) sel.index else len + sel.index;
                        if (norm >= 0 && norm < len) {
                            const idx = norm as usize;
                            const p = if (track_paths) JsonPath.formatIndexPath(alloc, base_path, idx)? else "";
                            out.append(alloc, NodeItem{ path: p, value: val.arr_val[idx] }) catch |e| {
                                if (p.len > 0) alloc.free(p);
                                return e;
                            };
                        }
                    }
                },
                .Slice => {
                    if (val.tag == .Array && val.arr_val.len > 0) {
                        const len = val.arr_val.len as i64;
                        const step: i64 = sel.slice_step ?? 1;
                        if (step == 0) continue;

                        const def_start: i64 = if (step >= 0) 0 else len - 1;
                        const def_end: i64 = if (step >= 0) len else -len - 1;

                        const raw_start = sel.slice_start ?? def_start;
                        const raw_end = sel.slice_end ?? def_end;

                        const n_start = if (raw_start >= 0) raw_start else len + raw_start;
                        const n_end = if (raw_end >= 0) raw_end else len + raw_end;

                        if (step > 0) {
                            let lower: i64 = if (n_start < 0) 0 else n_start;
                            if (lower > len) lower = len;
                            let upper: i64 = if (n_end < 0) 0 else n_end;
                            if (upper > len) upper = len;

                            let i = lower;
                            while (i < upper) : (i += step) {
                                const idx = i as usize;
                                const p = if (track_paths) JsonPath.formatIndexPath(alloc, base_path, idx)? else "";
                                out.append(alloc, NodeItem{ path: p, value: val.arr_val[idx] }) catch |e| {
                                    if (p.len > 0) alloc.free(p);
                                    return e;
                                };
                            }
                        } else {
                            let upper: i64 = if (n_start < -1) -1 else n_start;
                            if (upper > len - 1) upper = len - 1;
                            let lower: i64 = if (n_end < -1) -1 else n_end;
                            if (lower > len - 1) lower = len - 1;

                            let i = upper;
                            while (i > lower) : (i += step) {
                                const idx = i as usize;
                                const p = if (track_paths) JsonPath.formatIndexPath(alloc, base_path, idx)? else "";
                                out.append(alloc, NodeItem{ path: p, value: val.arr_val[idx] }) catch |e| {
                                    if (p.len > 0) alloc.free(p);
                                    return e;
                                };
                            }
                        }
                    }
                },
                .Filter => {
                    if (val.tag == .Array) {
                        for (i of 0..val.arr_val.len) {
                            const idx = i as usize;
                            const elem = val.arr_val[idx];
                            if (_.evalFilter(alloc, sel.filter_root, root, elem)?) {
                                const p = if (track_paths) JsonPath.formatIndexPath(alloc, base_path, idx)? else "";
                                out.append(alloc, NodeItem{ path: p, value: elem }) catch |e| {
                                    if (p.len > 0) alloc.free(p);
                                    return e;
                                };
                            }
                        }
                    } else if (val.tag == .Object) {
                        for (field of val.obj_val) {
                            if (_.evalFilter(alloc, sel.filter_root, root, field.value)?) {
                                const p = if (track_paths) JsonPath.formatKeyPath(alloc, base_path, field.key)? else "";
                                out.append(alloc, NodeItem{ path: p, value: field.value }) catch |e| {
                                    if (p.len > 0) alloc.free(p);
                                    return e;
                                };
                            }
                        }
                    }
                },
            }
        }
    }

    fn evalFilter(
        alloc: std.mem.Allocator,
        node_idx: usize,
        root: JsonValue,
        current: JsonValue,
    ): bool! {
        const node = _.program.filter_nodes.items[node_idx];
        return switch (node.kind) {
            .Or => {
                if (_.evalFilter(alloc, node.left, root, current)?) return true;
                return _.evalFilter(alloc, node.right, root, current);
            },
            .And => {
                const lhs_ok = _.evalFilter(alloc, node.left, root, current)?;
                if (!lhs_ok) return false;
                return _.evalFilter(alloc, node.right, root, current);
            },
            .Not => {
                const v = _.evalFilter(alloc, node.left, root, current)?;
                return !v;
            },
            .TestQuery => {
                let items = _.evalQuery(alloc, node.query_id, root, current, false)?;
                defer items.deinit(alloc);
                return items.items.len > 0;
            },
            .CmpEq, .CmpNe, .CmpLt, .CmpLe, .CmpGt, .CmpGe => {
                const lhs = _.evalComparable(alloc, node.left, root, current)?;
                const rhs = _.evalComparable(alloc, node.right, root, current)?;
                return switch (node.kind) {
                    .CmpEq => JsonPath.compareEq(lhs, rhs),
                    .CmpNe => !JsonPath.compareEq(lhs, rhs),
                    .CmpLt => JsonPath.compareLt(lhs, rhs),
                    .CmpLe => JsonPath.compareEq(lhs, rhs) || JsonPath.compareLt(lhs, rhs),
                    .CmpGt => JsonPath.compareLt(rhs, lhs),
                    .CmpGe => JsonPath.compareEq(lhs, rhs) || JsonPath.compareLt(rhs, lhs),
                    else => false,
                };
            },
            else => false,
        };
    }

    fn evalComparable(
        alloc: std.mem.Allocator,
        node_idx: usize,
        root: JsonValue,
        current: JsonValue,
    ): EvalValue! {
        const node = _.program.filter_nodes.items[node_idx];
        return switch (node.kind) {
            .LitNull => EvalValue{ exists: true, value: JsonValue{ tag: .Null } },
            .LitBool => EvalValue{ exists: true, value: JsonValue{ tag: .Bool, bool_val: node.bool_val } },
            .LitNumber => EvalValue{ exists: true, value: JsonValue{ tag: .Number, num_val: node.num_val } },
            .LitString => EvalValue{ exists: true, value: JsonValue{ tag: .String, str_val: node.str_val } },
            .QueryVal => {
                let items = _.evalQuery(alloc, node.query_id, root, current, false)?;
                defer items.deinit(alloc);
                if (items.items.len == 1) {
                    return EvalValue{ exists: true, value: items.items[0].value };
                }
                return EvalValue{ exists: false };
            },
            else => EvalValue{ exists: false },
        };
    }

    static fn compareEq(a: EvalValue, b: EvalValue): bool {
        if (!a.exists && !b.exists) return true;
        if (!a.exists || !b.exists) return false;
        return a.value.equals(b.value);
    }

    static fn compareLt(a: EvalValue, b: EvalValue): bool {
        if (!a.exists || !b.exists) return false;
        if (a.value.tag == .Number && b.value.tag == .Number) {
            return a.value.num_val < b.value.num_val;
        }
        if (a.value.tag == .String && b.value.tag == .String) {
            return std.mem.order(u8, a.value.str_val, b.value.str_val) == .lt;
        }
        return false;
    }

    static fn formatKeyPath(alloc: std.mem.Allocator, base: []const u8, key: []const u8): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        out.appendSlice(alloc, base)?;
        out.appendSlice(alloc, "['")?;
        for (c of key) {
            switch (c) {
                '\'' => out.appendSlice(alloc, "\\'")?,
                '\\' => out.appendSlice(alloc, "\\\\")?,
                0x08 => out.appendSlice(alloc, "\\b")?,
                0x0C => out.appendSlice(alloc, "\\f")?,
                '\n' => out.appendSlice(alloc, "\\n")?,
                '\r' => out.appendSlice(alloc, "\\r")?,
                '\t' => out.appendSlice(alloc, "\\t")?,
                else => out.append(alloc, c)?,
            }
        }
        out.appendSlice(alloc, "']")?;
        return out.toOwnedSlice(alloc);
    }

    static fn formatIndexPath(alloc: std.mem.Allocator, base: []const u8, idx: usize): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        out.appendSlice(alloc, base)?;
        out.append(alloc, '[')?;
        if (idx == 0) {
            out.append(alloc, '0')?;
        } else {
            let digits: [32]u8 = undefined;
            let n = idx;
            let count: usize = 0;
            while (n > 0) {
                digits[count] = ('0' as u8) + ((n % 10) as u8);
                n /= 10;
                count += 1;
            }
            while (count > 0) {
                count -= 1;
                out.append(alloc, digits[count])?;
            }
        }
        out.append(alloc, ']')?;
        return out.toOwnedSlice(alloc);
    }
}
