// Source: cp/lib/graphql/validator.cp (GraphQL Validation, Complexity, Depth & Formatter)
const std = @import("std");
import {
    GqlError,
    GqlValue,
    GqlTypeRef,
    GqlArgument,
    GqlDirective,
    GqlSelection,
    GqlOperation,
    GqlFragment,
    GqlDocument,
} from "./types.cp";

fn findFragment(doc: *const GqlDocument, name: []const u8): (*const GqlFragment)? {
    for (*frag of doc.fragments) {
        if (std.mem.eql(u8, frag.name, name)) {
            return frag;
        }
    }
    return null;
}

fn validateSelectionSet(
    doc: *const GqlDocument,
    sels: []const GqlSelection,
    visiting: *std.ArrayList([]const u8),
    alloc: std.mem.Allocator,
): GqlError!void {
    for (*sel of sels) {
        switch (sel.*) {
            .Field => |*f| {
                try validateSelectionSet(doc, f.selection_set, visiting, alloc);
            },
            .InlineFragment => |*inf| {
                try validateSelectionSet(doc, inf.selection_set, visiting, alloc);
            },
            .FragmentSpread => |*fs| {
                const frag = findFragment(doc, fs.name) orelse return error.UnknownFragment;
                for (v of visiting.items) {
                    if (std.mem.eql(u8, v, fs.name)) {
                        return error.CyclicFragment;
                    }
                }
                try visiting.append(alloc, fs.name);
                defer * = visiting.pop();
                try validateSelectionSet(doc, frag.selection_set, visiting, alloc);
            },
        }
    }
}

pub fn validateDocument(alloc: std.mem.Allocator, doc: *const GqlDocument): GqlError!void {
    // Check duplicate operation names
    let i: usize = 0;
    while (i < doc.operations.len) {
        if (na := doc.operations[i].name) {
            let j: usize = i + 1;
            while (j < doc.operations.len) {
                if (nb := doc.operations[j].name) {
                    if (std.mem.eql(u8, na, nb)) {
                        return error.DuplicateOperation;
                    }
                }
                j += 1;
            }
        }
        i += 1;
    }

    // Check duplicate fragment names
    let fi: usize = 0;
    while (fi < doc.fragments.len) {
        let fj: usize = fi + 1;
        while (fj < doc.fragments.len) {
            if (std.mem.eql(u8, doc.fragments[fi].name, doc.fragments[fj].name)) {
                return error.DuplicateFragment;
            }
            fj += 1;
        }
        fi += 1;
    }

    let visiting = std.ArrayList(string).empty;
    defer visiting.deinit(alloc);

    for (*op of doc.operations) {
        try validateSelectionSet(doc, op.selection_set, &visiting, alloc);
    }

    for (*frag of doc.fragments) {
        try visiting.append(alloc, frag.name);
        defer * = visiting.pop();
        try validateSelectionSet(doc, frag.selection_set, &visiting, alloc);
    }
}

fn calcSelectionSetDepth(
    doc: *const GqlDocument,
    sels: []const GqlSelection,
    visiting: *std.ArrayList([]const u8),
    alloc: std.mem.Allocator,
    max_depth: usize,
    cur_depth: usize,
): GqlError!usize {
    if (max_depth > 0 and cur_depth > max_depth) {
        return error.MaxDepthExceeded;
    }
    let max_sub: usize = 0;
    for (*sel of sels) {
        switch (sel.*) {
            .Field => |*f| {
                let sub: usize = 0;
                if (f.selection_set.len > 0) {
                    sub = try calcSelectionSetDepth(doc, f.selection_set, visiting, alloc, max_depth, cur_depth + 1);
                }
                const field_depth = 1 + sub;
                if (max_depth > 0 and cur_depth + field_depth > max_depth) {
                    return error.MaxDepthExceeded;
                }
                if (field_depth > max_sub) max_sub = field_depth;
            },
            .InlineFragment => |*inf| {
                const sub = try calcSelectionSetDepth(doc, inf.selection_set, visiting, alloc, max_depth, cur_depth);
                if (sub > max_sub) max_sub = sub;
            },
            .FragmentSpread => |*fs| {
                const frag = findFragment(doc, fs.name) orelse return error.UnknownFragment;
                for (v of visiting.items) {
                    if (std.mem.eql(u8, v, fs.name)) {
                        return error.CyclicFragment;
                    }
                }
                try visiting.append(alloc, fs.name);
                defer * = visiting.pop();
                const sub = try calcSelectionSetDepth(doc, frag.selection_set, visiting, alloc, max_depth, cur_depth);
                if (sub > max_sub) max_sub = sub;
            },
        }
    }
    return max_sub;
}

pub fn computeQueryDepth(alloc: std.mem.Allocator, doc: *const GqlDocument, max_depth: usize): GqlError!usize {
    let visiting = std.ArrayList(string).empty;
    defer visiting.deinit(alloc);

    let best: usize = 0;
    for (*op of doc.operations) {
        const d = try calcSelectionSetDepth(doc, op.selection_set, &visiting, alloc, max_depth, 0);
        if (d > best) best = d;
    }
    return best;
}

fn calcSelectionSetComplexity(
    doc: *const GqlDocument,
    sels: []const GqlSelection,
    visiting: *std.ArrayList([]const u8),
    alloc: std.mem.Allocator,
    max_complexity: usize,
    acc: *usize,
): GqlError!usize {
    let cost: usize = 0;
    for (*sel of sels) {
        switch (sel.*) {
            .Field => |*f| {
                let child_cost: usize = 0;
                if (f.selection_set.len > 0) {
                    child_cost = try calcSelectionSetComplexity(doc, f.selection_set, visiting, alloc, max_complexity, acc);
                }
                const node_cost = 1 + child_cost;
                cost += node_cost;
                acc.* += 1;
                if (max_complexity > 0 and acc.* > max_complexity) {
                    return error.MaxComplexityExceeded;
                }
            },
            .InlineFragment => |*inf| {
                cost += try calcSelectionSetComplexity(doc, inf.selection_set, visiting, alloc, max_complexity, acc);
            },
            .FragmentSpread => |*fs| {
                const frag = findFragment(doc, fs.name) orelse return error.UnknownFragment;
                for (v of visiting.items) {
                    if (std.mem.eql(u8, v, fs.name)) {
                        return error.CyclicFragment;
                    }
                }
                try visiting.append(alloc, fs.name);
                defer * = visiting.pop();
                cost += try calcSelectionSetComplexity(doc, frag.selection_set, visiting, alloc, max_complexity, acc);
            },
        }
    }
    return cost;
}

pub fn computeQueryComplexity(alloc: std.mem.Allocator, doc: *const GqlDocument, max_complexity: usize): GqlError!usize {
    let visiting = std.ArrayList(string).empty;
    defer visiting.deinit(alloc);

    let total: usize = 0;
    let acc: usize = 0;
    for (*op of doc.operations) {
        total += try calcSelectionSetComplexity(doc, op.selection_set, &visiting, alloc, max_complexity, &acc);
    }
    if (max_complexity > 0 and total > max_complexity) {
        return error.MaxComplexityExceeded;
    }
    return total;
}

fn writeIndent(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, indent: usize): GqlError!void {
    let i: usize = 0;
    while (i < indent * 2) {
        try buf.append(alloc, ' ');
        i += 1;
    }
}

fn writeEscapedString(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, s: []const u8): GqlError!void {
    try buf.append(alloc, '"');
    for (c of s) {
        switch (c) {
            '"' => try buf.appendSlice(alloc, "\\\""),
            '\\' => try buf.appendSlice(alloc, "\\\\"),
            '\n' => try buf.appendSlice(alloc, "\\n"),
            '\r' => try buf.appendSlice(alloc, "\\r"),
            '\t' => try buf.appendSlice(alloc, "\\t"),
            else => try buf.append(alloc, c),
        }
    }
    try buf.append(alloc, '"');
}

fn formatValue(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, val: GqlValue): GqlError!void {
    switch (val) {
        .Variable => |v| {
            try buf.append(alloc, '$');
            try buf.appendSlice(alloc, v);
        },
        .Int => |n| {
            let tmp: [32]u8 = undefined;
            const s = std.fmt.bufPrint(&tmp, "{d}", .{n}) catch return error.OutOfMemory;
            try buf.appendSlice(alloc, s);
        },
        .Float => |f| {
            let tmp: [64]u8 = undefined;
            const s = std.fmt.bufPrint(&tmp, "{d}", .{f}) catch return error.OutOfMemory;
            try buf.appendSlice(alloc, s);
        },
        .String => |s| try writeEscapedString(buf, alloc, s),
        .Boolean => |b| try buf.appendSlice(alloc, if (b) "true" else "false"),
        .Null => try buf.appendSlice(alloc, "null"),
        .Enum => |e| try buf.appendSlice(alloc, e),
        .List => |items| {
            try buf.append(alloc, '[');
            let i: usize = 0;
            while (i < items.len) {
                if (i > 0) try buf.appendSlice(alloc, ", ");
                try formatValue(buf, alloc, items[i]);
                i += 1;
            }
            try buf.append(alloc, ']');
        },
        .Object => |fields| {
            try buf.append(alloc, '{');
            let i: usize = 0;
            while (i < fields.len) {
                if (i > 0) try buf.appendSlice(alloc, ", ");
                try buf.appendSlice(alloc, fields[i].name);
                try buf.appendSlice(alloc, ": ");
                try formatValue(buf, alloc, fields[i].value);
                i += 1;
            }
            try buf.append(alloc, '}');
        },
    }
}

fn formatTypeRef(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, tref: GqlTypeRef): GqlError!void {
    switch (tref) {
        .Named => |n| try buf.appendSlice(alloc, n),
        .List => |inner| {
            try buf.append(alloc, '[');
            try formatTypeRef(buf, alloc, inner.*);
            try buf.append(alloc, ']');
        },
        .NonNull => |inner| {
            try formatTypeRef(buf, alloc, inner.*);
            try buf.append(alloc, '!');
        },
    }
}

fn formatArguments(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, args: []const GqlArgument): GqlError!void {
    if (args.len == 0) return;
    try buf.append(alloc, '(');
    let i: usize = 0;
    while (i < args.len) {
        if (i > 0) try buf.appendSlice(alloc, ", ");
        try buf.appendSlice(alloc, args[i].name);
        try buf.appendSlice(alloc, ": ");
        try formatValue(buf, alloc, args[i].value);
        i += 1;
    }
    try buf.append(alloc, ')');
}

fn formatDirectives(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, dirs: []const GqlDirective): GqlError!void {
    for (d of dirs) {
        try buf.appendSlice(alloc, " @");
        try buf.appendSlice(alloc, d.name);
        try formatArguments(buf, alloc, d.arguments);
    }
}

fn formatSelectionSet(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, sels: []const GqlSelection, indent: usize): GqlError!void {
    try buf.appendSlice(alloc, "{\n");
    for (sel of sels) {
        try writeIndent(buf, alloc, indent + 1);
        switch (sel) {
            .Field => |f| {
                if (a := f.alias) {
                    try buf.appendSlice(alloc, a);
                    try buf.appendSlice(alloc, ": ");
                }
                try buf.appendSlice(alloc, f.name);
                try formatArguments(buf, alloc, f.arguments);
                try formatDirectives(buf, alloc, f.directives);
                if (f.selection_set.len > 0) {
                    try buf.append(alloc, ' ');
                    try formatSelectionSet(buf, alloc, f.selection_set, indent + 1);
                } else {
                    try buf.append(alloc, '\n');
                }
            },
            .FragmentSpread => |fs| {
                try buf.appendSlice(alloc, "...");
                try buf.appendSlice(alloc, fs.name);
                try formatDirectives(buf, alloc, fs.directives);
                try buf.append(alloc, '\n');
            },
            .InlineFragment => |inf| {
                try buf.appendSlice(alloc, "...");
                if (tc := inf.type_condition) {
                    try buf.appendSlice(alloc, " on ");
                    try buf.appendSlice(alloc, tc);
                }
                try formatDirectives(buf, alloc, inf.directives);
                try buf.append(alloc, ' ');
                try formatSelectionSet(buf, alloc, inf.selection_set, indent + 1);
            },
        }
    }
    try writeIndent(buf, alloc, indent);
    try buf.appendSlice(alloc, "}\n");
}

pub fn formatDocument(alloc: std.mem.Allocator, doc: *const GqlDocument): GqlError![]u8 {
    let buf = std.ArrayList(u8).empty;
    errdefer buf.deinit(alloc);

    let first = true;
    for (op of doc.operations) {
        if (!first) try buf.append(alloc, '\n');
        first = false;

        const is_shorthand = (op.op_type == .Query and op.name == null and op.variable_defs.len == 0 and op.directives.len == 0);
        if (!is_shorthand) {
            switch (op.op_type) {
                .Query => try buf.appendSlice(alloc, "query"),
                .Mutation => try buf.appendSlice(alloc, "mutation"),
                .Subscription => try buf.appendSlice(alloc, "subscription"),
            }
            if (n := op.name) {
                try buf.append(alloc, ' ');
                try buf.appendSlice(alloc, n);
            }
            if (op.variable_defs.len > 0) {
                try buf.append(alloc, '(');
                let vi: usize = 0;
                while (vi < op.variable_defs.len) {
                    if (vi > 0) try buf.appendSlice(alloc, ", ");
                    const vd = op.variable_defs[vi];
                    try buf.append(alloc, '$');
                    try buf.appendSlice(alloc, vd.name);
                    try buf.appendSlice(alloc, ": ");
                    try formatTypeRef(&buf, alloc, vd.type_ref);
                    if (dv := vd.default_value) {
                        try buf.appendSlice(alloc, " = ");
                        try formatValue(&buf, alloc, dv);
                    }
                    vi += 1;
                }
                try buf.append(alloc, ')');
            }
            try formatDirectives(&buf, alloc, op.directives);
            try buf.append(alloc, ' ');
        }
        try formatSelectionSet(&buf, alloc, op.selection_set, 0);
    }

    for (frag of doc.fragments) {
        if (!first) try buf.append(alloc, '\n');
        first = false;

        try buf.appendSlice(alloc, "fragment ");
        try buf.appendSlice(alloc, frag.name);
        try buf.appendSlice(alloc, " on ");
        try buf.appendSlice(alloc, frag.type_condition);
        try formatDirectives(&buf, alloc, frag.directives);
        try buf.append(alloc, ' ');
        try formatSelectionSet(&buf, alloc, frag.selection_set, 0);
    }

    return buf.toOwnedSlice(alloc);
}
