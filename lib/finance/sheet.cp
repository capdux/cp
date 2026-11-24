// Source: cp/lib/sheet/sheet.cp (Spreadsheet Engine with DAG Topological Recalculation)
// Pure CP spreadsheet container with bidirectional dependency graph, Kahn topological sort
// cycle detection (`CycleDetected`), cascading formula recalculation, and CSV export.

const std = @import("std");
import {
    SheetError,
    CellCoord,
    CellValue,
    CellLookup,
    parseAndExtractDeps,
    evaluateFormula,
} from "./formula.cp";

enum CellKind {
    Empty,
    Number: f64,
    Text: []u8,
    Formula: FormulaData,
}

struct FormulaData {
    expr: []u8,
    deps: std.ArrayList(u64),
    cached: CellValue,
    dirty: bool,
}

/// Pure CP spreadsheet with A1..ZZ9999 cell addressing and DAG topological recalculation.
pub struct Spreadsheet {
    alloc: std.mem.Allocator,
    cells: std.AutoHashMap(u64, CellKind),
    dependents: std.AutoHashMap(u64, std.ArrayList(u64)),

    /// Initialize an empty spreadsheet bound to `alloc`.
    pub static fn init(alloc: std.mem.Allocator): Spreadsheet {
        return Spreadsheet{
            alloc: alloc,
            cells: std.AutoHashMap(u64, CellKind).init(alloc),
            dependents: std.AutoHashMap(u64, std.ArrayList(u64)).init(alloc),
        };
    }

    fn freeCellKind(*_, kind: *CellKind) {
        switch (kind.*) {
            .Empty, .Number => {},
            .Text s => {
                _.alloc.free(s);
            },
            .Formula => |*f| {
                _.alloc.free(f.expr);
                f.deps.deinit(_.alloc);
                switch (f.cached) {
                    .Text cs => _.alloc.free(cs),
                    else => {},
                }
            },
        }
    }

    /// Release all owned cell strings, formula expressions, dependency edges, and maps.
    pub fn deinit(*_) {
        let it = _.cells.iterator();
        while (entry := it.next()) {
            _.freeCellKind(entry.value_ptr);
        }
        _.cells.deinit();

        let dit = _.dependents.iterator();
        while (entry := dit.next()) {
            entry.value_ptr.deinit(_.alloc);
        }
        _.dependents.deinit();
    }

    fn removeDependentEdge(*_, upstream: u64, downstream: u64) {
        if (list := _.dependents.getPtr(upstream)) {
            let i: usize = 0;
            while (i < list.items.len) {
                if (list.items[i] == downstream) {
                    * = list.swapRemove(i);
                } else {
                    i += 1;
                }
            }
        }
    }

    fn addDependentEdge(*_, upstream: u64, downstream: u64): ! {
        const res = _.dependents.getOrPut(upstream)?;
        if (!res.found_existing) {
            res.value_ptr.* = .empty;
        }
        for (existing of res.value_ptr.items) {
            if (existing == downstream) return;
        }
        res.value_ptr.append(_.alloc, downstream)?;
    }

    /// Remove old formula dependency edges and free the existing cell payload if present.
    fn clearExistingCell(*_, key: u64) {
        if (existing := _.cells.getPtr(key)) {
            switch (existing.*) {
                .Formula => |*f| {
                    for (dep_key of f.deps.items) {
                        _.removeDependentEdge(dep_key, key);
                    }
                },
                else => {},
            }
            _.freeCellKind(existing);
            existing.* = .Empty;
        }
    }

    /// Mark all transitive downstream formula cells as `dirty = true` using BFS.
    fn markDownstreamDirty(*_, start_key: u64): ! {
        let queue: std.ArrayList(u64) = .empty;
        defer queue.deinit(_.alloc);
        let visited = std.AutoHashMap(u64, void).init(_.alloc);
        defer visited.deinit();

        queue.append(_.alloc, start_key)?;
        visited.put(start_key, {})?;

        let head: usize = 0;
        while (head < queue.items.len) : (head += 1) {
            const cur = queue.items[head];
            if (dep_list := _.dependents.getPtr(cur)) {
                for (nxt of dep_list.items) {
                    if (!visited.contains(nxt)) {
                        visited.put(nxt, {})?;
                        queue.append(_.alloc, nxt)?;
                        if (cell := _.cells.getPtr(nxt)) {
                            switch (cell.*) {
                                .Formula => |*f| {
                                    f.dirty = true;
                                },
                                else => {},
                            }
                        }
                    }
                }
            }
        }
    }

    /// Set cell `ref_str` to a numeric value and mark downstream formulas dirty.
    pub fn setNumber(*_, ref_str: string, val: f64): ! {
        const coord = CellCoord.parseRef(ref_str)?;
        const key = coord.key();
        _.clearExistingCell(key);
        _.cells.put(key, CellKind.Number(val))?;
        _.markDownstreamDirty(key)?;
    }

    /// Set cell `ref_str` to a text string (copied) and mark downstream formulas dirty.
    pub fn setText(*_, ref_str: string, text: string): ! {
        const coord = CellCoord.parseRef(ref_str)?;
        const key = coord.key();
        const owned = _.alloc.dupe(u8, text)?;
        errdefer _.alloc.free(owned);

        _.clearExistingCell(key);
        _.cells.put(key, CellKind.Text(owned))?;
        _.markDownstreamDirty(key)?;
    }

    /// Set cell `ref_str` to a formula expression (`=` prefix optional) and update DAG edges.
    pub fn setFormula(*_, ref_str: string, raw_expr: string): ! {
        const coord = CellCoord.parseRef(ref_str)?;
        const key = coord.key();

        let new_deps: std.ArrayList(u64) = .empty;
        errdefer new_deps.deinit(_.alloc);

        const norm_expr = parseAndExtractDeps(_.alloc, raw_expr, *new_deps)?;
        const owned_expr = _.alloc.dupe(u8, norm_expr)?;
        errdefer _.alloc.free(owned_expr);

        _.clearExistingCell(key);

        for (dep_key of new_deps.items) {
            _.addDependentEdge(dep_key, key)?;
        }

        _.cells.put(key, CellKind.Formula(FormulaData{
            expr: owned_expr,
            deps: new_deps,
            cached: .Empty,
            dirty: true,
        }))?;
        _.markDownstreamDirty(key)?;
    }

    /// Clear cell `ref_str` back to `Empty` and mark downstream formulas dirty.
    pub fn clear(*_, ref_str: string): ! {
        const coord = CellCoord.parseRef(ref_str)?;
        const key = coord.key();
        _.clearExistingCell(key);
        * = _.cells.remove(key);
        _.markDownstreamDirty(key)?;
    }

    fn lookupCellByKey(ctx: *const anyopaque, key: u64): CellValue {
        const sheet = unsafe { ctx as *const Spreadsheet! };
        if (cell := sheet.cells.get(key)) {
            return switch (cell) {
                .Empty => .Empty,
                .Number v => CellValue.Number(v),
                .Text s => CellValue.Text(s),
                .Formula f => f.cached,
            };
        }
        return .Empty;
    }

    /// Topologically recalculate all dirty formula cells via Kahn's algorithm.
    /// Returns the number of formula cells recalculated, or `CycleDetected` on cycles.
    pub fn recalculate(*_): usize ! SheetError {
        let dirty_set = std.AutoHashMap(u64, void).init(_.alloc);
        defer dirty_set.deinit();

        let it = _.cells.iterator();
        while (entry := it.next()) {
            switch (entry.value_ptr.*) {
                .Formula f => {
                    if (f.dirty) {
                        dirty_set.put(entry.key_ptr.*, {})?;
                    }
                },
                else => {},
            }
        }

        const total_dirty = dirty_set.count();
        if (total_dirty == 0) {
            return 0;
        }

        let in_degree = std.AutoHashMap(u64, usize).init(_.alloc);
        defer in_degree.deinit();

        let queue: std.ArrayList(u64) = .empty;
        defer queue.deinit(_.alloc);

        let dit = dirty_set.iterator();
        while (d_entry := dit.next()) {
            const key = d_entry.key_ptr.*;
            let deg: usize = 0;
            if (cell := _.cells.getPtr(key)) {
                switch (cell.*) {
                    .Formula f => {
                        for (dep_key of f.deps.items) {
                            if (dirty_set.contains(dep_key)) {
                                deg += 1;
                            }
                        }
                    },
                    else => {},
                }
            }
            in_degree.put(key, deg)?;
            if (deg == 0) {
                queue.append(_.alloc, key)?;
            }
        }

        const lookup = CellLookup{
            ctx: unsafe { _ as *const anyopaque! },
            get_fn: Spreadsheet.lookupCellByKey,
        };

        let processed: usize = 0;
        let head: usize = 0;
        while (head < queue.items.len) : (head += 1) {
            const cur = queue.items[head];
            processed += 1;

            if (cell := _.cells.getPtr(cur)) {
                switch (cell.*) {
                    .Formula => |*f| {
                        let tmp_arena = std.heap.ArenaAllocator.init(_.alloc);
                        defer tmp_arena.deinit();
                        const eval_val = evaluateFormula(tmp_arena.allocator(), f.expr, lookup)?;
                        const owned_val: CellValue = switch (eval_val) {
                            .Text s => CellValue.Text(_.alloc.dupe(u8, s)?),
                            else => eval_val,
                        };
                        switch (f.cached) {
                            .Text old_s => _.alloc.free(old_s),
                            else => {},
                        }
                        f.cached = owned_val;
                        f.dirty = false;
                    },
                    else => {},
                }
            }

            if (dep_list := _.dependents.getPtr(cur)) {
                for (nxt of dep_list.items) {
                    if (deg_ptr := in_degree.getPtr(nxt)) {
                        if (deg_ptr.* > 0) {
                            deg_ptr.* -= 1;
                            if (deg_ptr.* == 0) {
                                queue.append(_.alloc, nxt)?;
                            }
                        }
                    }
                }
            }
        }

        if (processed < total_dirty) {
            return SheetError.CycleDetected;
        }

        return processed;
    }

    /// Get the current value of cell `ref_str` (for formulas, returns the cached value).
    pub fn getValue(ref_str: string): CellValue ! SheetError {
        const coord = CellCoord.parseRef(ref_str)?;
        const key = coord.key();
        if (cell := _.cells.get(key)) {
            return switch (cell) {
                .Empty => .Empty,
                .Number v => CellValue.Number(v),
                .Text s => CellValue.Text(s),
                .Formula f => f.cached,
            };
        }
        return .Empty;
    }

    /// Get the numeric value of cell `ref_str`, returning `null` if Empty or `TypeMismatch` if Text.
    pub fn getNumber(ref_str: string): f64? ! SheetError {
        const val = _.getValue(ref_str)?;
        return switch (val) {
            .Empty => null,
            .Number n => n,
            .Text => SheetError.TypeMismatch,
        };
    }

    static fn appendCsvText(alloc: std.mem.Allocator, out: *std.ArrayList(u8), s: string): ! {
        let needs_quote = false;
        for (ch of s) {
            if (ch == ',' || ch == '"' || ch == '\n' || ch == '\r') {
                needs_quote = true;
                break;
            }
        }
        if (!needs_quote) {
            out.appendSlice(alloc, s)?;
            return;
        }
        out.append(alloc, '"')?;
        for (ch of s) {
            if (ch == '"') {
                out.appendSlice(alloc, "\"\"")?;
            } else {
                out.append(alloc, ch)?;
            }
        }
        out.append(alloc, '"')?;
    }

    /// Export evaluated cells in `[0..max_row, 0..max_col)` as a CSV byte slice owned by `alloc`.
    pub fn toCsv(alloc: std.mem.Allocator, max_row: usize, max_col: usize): ([]u8) ! SheetError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        if (max_row == 0 || max_col == 0) {
            return out.toOwnedSlice(alloc);
        }

        let r: usize = 0;
        while (r < max_row) : (r += 1) {
            if (r > 0) {
                out.append(alloc, '\n')?;
            }
            let c: usize = 0;
            while (c < max_col) : (c += 1) {
                if (c > 0) {
                    out.append(alloc, ',')?;
                }
                const coord = CellCoord{ row: r, col: c };
                const key = coord.key();
                if (cell := _.cells.get(key)) {
                    const val: CellValue = switch (cell) {
                        .Empty => .Empty,
                        .Number v => CellValue.Number(v),
                        .Text s => CellValue.Text(s),
                        .Formula f => f.cached,
                    };
                    switch (val) {
                        .Empty => {},
                        .Number v => {
                            const num_s = std.fmt.allocPrint(alloc, "{d}", .{v})?;
                            defer alloc.free(num_s);
                            out.appendSlice(alloc, num_s)?;
                        },
                        .Text s => {
                            Spreadsheet.appendCsvText(alloc, *out, s)?;
                        },
                    }
                }
            }
        }

        return out.toOwnedSlice(alloc);
    }
}
