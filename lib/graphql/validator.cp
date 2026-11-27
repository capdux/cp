// Source: cp/lib/graphql/validator.cp (GraphQL Validation, Complexity, Depth & Formatter)
const std = @import("std");
import {
    GqlError,
    GqlSelection,
    GqlFragmentDef,
    GqlDocument,
} from "./types.cp";
import { GqlPrinter } from "./printer.cp";

fn findFragmentDef(doc: GqlDocument, name: []const u8): GqlFragmentDef? {
    for (frag of doc.fragments) {
        if (std.mem.eql(u8, frag.name, name)) {
            return frag;
        }
    }
    return null;
}

fn validateSelectionSet(
    doc: GqlDocument,
    sels: []const GqlSelection,
    visiting: *std.ArrayList([]const u8),
    alloc: std.mem.Allocator,
): void ! GqlError {
    for (sel of sels) {
        switch (sel) {
            .Field f => {
                validateSelectionSet(doc, f.selections, visiting, alloc)?;
            },
            .InlineFragment inf => {
                validateSelectionSet(doc, inf.selections, visiting, alloc)?;
            },
            .FragmentSpread fs => {
                const frag = findFragmentDef(doc, fs.name) orelse return GqlError.UnknownFragment;
                for (v of visiting.items) {
                    if (std.mem.eql(u8, v, fs.name)) {
                        return GqlError.CyclicFragment;
                    }
                }
                visiting.append(alloc, fs.name) catch return GqlError.OutOfMemory;
                defer * = visiting.pop();
                validateSelectionSet(doc, frag.selections, visiting, alloc)?;
            },
        }
    }
}

/// Validates a parsed `GqlDocument` for duplicate operation/fragment names,
/// unknown fragment spreads, and cyclic fragment references.
pub fn validateDocument(alloc: std.mem.Allocator, doc: GqlDocument): void ! GqlError {
    let i: usize = 0;
    while (i < doc.operations.len) : (i += 1) {
        if (na := doc.operations[i].name) {
            let j: usize = i + 1;
            while (j < doc.operations.len) : (j += 1) {
                if (nb := doc.operations[j].name) {
                    if (std.mem.eql(u8, na, nb)) {
                        return GqlError.DuplicateOperation;
                    }
                }
            }
        }
    }

    let fi: usize = 0;
    while (fi < doc.fragments.len) : (fi += 1) {
        let fj: usize = fi + 1;
        while (fj < doc.fragments.len) : (fj += 1) {
            if (std.mem.eql(u8, doc.fragments[fi].name, doc.fragments[fj].name)) {
                return GqlError.DuplicateFragment;
            }
        }
    }

    let visiting: std.ArrayList([]const u8) = .empty;
    defer visiting.deinit(alloc);

    for (op of doc.operations) {
        validateSelectionSet(doc, op.selections, &visiting, alloc)?;
    }

    for (frag of doc.fragments) {
        visiting.append(alloc, frag.name) catch return GqlError.OutOfMemory;
        defer * = visiting.pop();
        validateSelectionSet(doc, frag.selections, &visiting, alloc)?;
    }
}

fn calcSelectionSetDepth(
    doc: GqlDocument,
    sels: []const GqlSelection,
    visiting: *std.ArrayList([]const u8),
    alloc: std.mem.Allocator,
    max_depth: usize,
    cur_depth: usize,
): usize ! GqlError {
    if (max_depth > 0 && cur_depth > max_depth) {
        return GqlError.MaxDepthExceeded;
    }
    let max_sub: usize = 0;
    for (sel of sels) {
        switch (sel) {
            .Field f => {
                let sub: usize = 0;
                if (f.selections.len > 0) {
                    sub = calcSelectionSetDepth(doc, f.selections, visiting, alloc, max_depth, cur_depth + 1)?;
                }
                const field_depth = (1 as usize) + sub;
                if (max_depth > 0 && cur_depth + field_depth > max_depth) {
                    return GqlError.MaxDepthExceeded;
                }
                if (field_depth > max_sub) max_sub = field_depth;
            },
            .InlineFragment inf => {
                const sub = calcSelectionSetDepth(doc, inf.selections, visiting, alloc, max_depth, cur_depth)?;
                if (sub > max_sub) max_sub = sub;
            },
            .FragmentSpread fs => {
                const frag = findFragmentDef(doc, fs.name) orelse return GqlError.UnknownFragment;
                for (v of visiting.items) {
                    if (std.mem.eql(u8, v, fs.name)) {
                        return GqlError.CyclicFragment;
                    }
                }
                visiting.append(alloc, fs.name) catch return GqlError.OutOfMemory;
                defer * = visiting.pop();
                const sub = calcSelectionSetDepth(doc, frag.selections, visiting, alloc, max_depth, cur_depth)?;
                if (sub > max_sub) max_sub = sub;
            },
        }
    }
    return max_sub;
}

/// Computes the maximum selection depth across all operations in `doc`,
/// enforcing `max_depth` when non-zero.
pub fn computeQueryDepth(alloc: std.mem.Allocator, doc: GqlDocument, max_depth: usize): usize ! GqlError {
    let visiting: std.ArrayList([]const u8) = .empty;
    defer visiting.deinit(alloc);

    let best: usize = 0;
    for (op of doc.operations) {
        const d = calcSelectionSetDepth(doc, op.selections, &visiting, alloc, max_depth, 0)?;
        if (d > best) best = d;
    }
    return best;
}

fn calcSelectionSetComplexity(
    doc: GqlDocument,
    sels: []const GqlSelection,
    visiting: *std.ArrayList([]const u8),
    alloc: std.mem.Allocator,
    max_complexity: usize,
    acc: *usize,
): usize ! GqlError {
    let cost: usize = 0;
    for (sel of sels) {
        switch (sel) {
            .Field f => {
                let child_cost: usize = 0;
                if (f.selections.len > 0) {
                    child_cost = calcSelectionSetComplexity(doc, f.selections, visiting, alloc, max_complexity, acc)?;
                }
                cost += (1 as usize) + child_cost;
                acc.* += 1;
                if (max_complexity > 0 && acc.* > max_complexity) {
                    return GqlError.MaxComplexityExceeded;
                }
            },
            .InlineFragment inf => {
                cost += calcSelectionSetComplexity(doc, inf.selections, visiting, alloc, max_complexity, acc)?;
            },
            .FragmentSpread fs => {
                const frag = findFragmentDef(doc, fs.name) orelse return GqlError.UnknownFragment;
                for (v of visiting.items) {
                    if (std.mem.eql(u8, v, fs.name)) {
                        return GqlError.CyclicFragment;
                    }
                }
                visiting.append(alloc, fs.name) catch return GqlError.OutOfMemory;
                defer * = visiting.pop();
                cost += calcSelectionSetComplexity(doc, frag.selections, visiting, alloc, max_complexity, acc)?;
            },
        }
    }
    return cost;
}

/// Computes total field complexity across all operations in `doc`,
/// enforcing `max_complexity` when non-zero.
pub fn computeQueryComplexity(alloc: std.mem.Allocator, doc: GqlDocument, max_complexity: usize): usize ! GqlError {
    let visiting: std.ArrayList([]const u8) = .empty;
    defer visiting.deinit(alloc);

    let total: usize = 0;
    let acc: usize = 0;
    for (op of doc.operations) {
        total += calcSelectionSetComplexity(doc, op.selections, &visiting, alloc, max_complexity, &acc)?;
    }
    if (max_complexity > 0 && total > max_complexity) {
        return GqlError.MaxComplexityExceeded;
    }
    return total;
}

/// Formats a `GqlDocument` into canonical pretty-printed GraphQL source text.
pub fn formatDocument(alloc: std.mem.Allocator, doc: GqlDocument): ([]u8) ! GqlError {
    return GqlPrinter.printPretty(alloc, doc);
}
