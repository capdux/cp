// Source: cp/lib/graphql/types.cp (GraphQL AST & Token Types)
const std = @import("std");

pub const GqlError = error{
    UnexpectedToken,
    UnterminatedString,
    InvalidNumber,
    InvalidCharacter,
    MaxDepthExceeded,
    MaxComplexityExceeded,
    CyclicFragment,
    UnknownFragment,
    DuplicateOperation,
    DuplicateFragment,
    OutOfMemory,
};

pub enum GqlTokenKind {
    Bang,
    Dollar,
    Amp,
    ParenL,
    ParenR,
    Spread,
    Colon,
    Equals,
    At,
    BracketL,
    BracketR,
    BraceL,
    Pipe,
    BraceR,
    Name,
    IntValue,
    FloatValue,
    StringValue,
    BlockStringValue,
    Eof,
}

pub struct GqlToken {
    kind: GqlTokenKind,
    lexeme: []const u8,
    line: usize,
    column: usize,
}

pub struct GqlObjectField {
    name: []const u8,
    value: GqlValue,
}

pub union(enum) GqlValue {
    Variable: []const u8,
    Int: i64,
    Float: f64,
    String: []const u8,
    Boolean: bool,
    Null: void,
    Enum: []const u8,
    List: []GqlValue,
    Object: []GqlObjectField,
}

pub union(enum) GqlTypeRef {
    Named: []const u8,
    List: *GqlTypeRef,
    NonNull: *GqlTypeRef,
}

pub struct GqlArgument {
    name: []const u8,
    value: GqlValue,
}

pub struct GqlDirective {
    name: []const u8,
    arguments: []GqlArgument,
}

pub struct GqlVariableDef {
    name: []const u8,
    type_ref: GqlTypeRef,
    default_value: GqlValue?,
}

pub struct GqlField {
    alias: ([]const u8)?,
    name: []const u8,
    arguments: []GqlArgument,
    directives: []GqlDirective,
    selection_set: []GqlSelection,
}

pub struct GqlFragmentSpread {
    name: []const u8,
    directives: []GqlDirective,
}

pub struct GqlInlineFragment {
    type_condition: ([]const u8)?,
    directives: []GqlDirective,
    selection_set: []GqlSelection,
}

pub union(enum) GqlSelection {
    Field: GqlField,
    FragmentSpread: GqlFragmentSpread,
    InlineFragment: GqlInlineFragment,
}

pub enum GqlOperationType {
    Query,
    Mutation,
    Subscription,
}

pub struct GqlOperation {
    op_type: GqlOperationType,
    name: ([]const u8)?,
    variable_defs: []GqlVariableDef,
    directives: []GqlDirective,
    selection_set: []GqlSelection,
}

pub struct GqlFragment {
    name: []const u8,
    type_condition: []const u8,
    directives: []GqlDirective,
    selection_set: []GqlSelection,
}

pub struct GqlDocument {
    operations: []GqlOperation,
    fragments: []GqlFragment,
    owned_strings: [][]u8,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        for (*op of _.operations) {
            freeOperation(op, alloc);
        }
        alloc.free(_.operations);

        for (*frag of _.fragments) {
            freeFragment(frag, alloc);
        }
        alloc.free(_.fragments);

        for (s of _.owned_strings) {
            alloc.free(s);
        }
        alloc.free(_.owned_strings);
    }
}

pub fn freeValue(val: *GqlValue, alloc: std.mem.Allocator) {
    switch (val.*) {
        .List => |items| {
            for (*item of items) {
                freeValue(item, alloc);
            }
            alloc.free(items);
        },
        .Object => |fields| {
            for (*f of fields) {
                freeValue(&f.value, alloc);
            }
            alloc.free(fields);
        },
        else => {},
    }
}

pub fn freeTypeRef(tref: *GqlTypeRef, alloc: std.mem.Allocator) {
    switch (tref.*) {
        .List => |inner| {
            freeTypeRef(inner, alloc);
            alloc.destroy(inner);
        },
        .NonNull => |inner| {
            freeTypeRef(inner, alloc);
            alloc.destroy(inner);
        },
        .Named => {},
    }
}

pub fn freeArguments(args: []GqlArgument, alloc: std.mem.Allocator) {
    for (*arg of args) {
        freeValue(&arg.value, alloc);
    }
    alloc.free(args);
}

pub fn freeDirectives(dirs: []GqlDirective, alloc: std.mem.Allocator) {
    for (*d of dirs) {
        freeArguments(d.arguments, alloc);
    }
    alloc.free(dirs);
}

pub fn freeSelectionSet(sels: []GqlSelection, alloc: std.mem.Allocator) {
    for (*sel of sels) {
        switch (sel.*) {
            .Field => |*f| {
                freeArguments(f.arguments, alloc);
                freeDirectives(f.directives, alloc);
                freeSelectionSet(f.selection_set, alloc);
            },
            .FragmentSpread => |*fs| {
                freeDirectives(fs.directives, alloc);
            },
            .InlineFragment => |*inf| {
                freeDirectives(inf.directives, alloc);
                freeSelectionSet(inf.selection_set, alloc);
            },
        }
    }
    alloc.free(sels);
}

pub fn freeVariableDefs(vdefs: []GqlVariableDef, alloc: std.mem.Allocator) {
    for (*vd of vdefs) {
        freeTypeRef(&vd.type_ref, alloc);
        if (*dv := vd.default_value) {
            freeValue(dv, alloc);
        }
    }
    alloc.free(vdefs);
}

pub fn freeOperation(op: *GqlOperation, alloc: std.mem.Allocator) {
    freeVariableDefs(op.variable_defs, alloc);
    freeDirectives(op.directives, alloc);
    freeSelectionSet(op.selection_set, alloc);
}

pub fn freeFragment(frag: *GqlFragment, alloc: std.mem.Allocator) {
    freeDirectives(frag.directives, alloc);
    freeSelectionSet(frag.selection_set, alloc);
}
