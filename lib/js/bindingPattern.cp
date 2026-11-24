// Source: Absorbed from /data/dev/bun/src/js_parser/ast/Binding.cp | Synced: 2026-11-23T20:35:04Z
// ============================================================================
// lib/js/bindingPattern.cp — Zero-Alloc ECMAScript Destructuring Binding AST,
// Static Semantics (`BoundNames`), Duplicate Guard & Assignment-Target Lowering
// ============================================================================
// Absorbed from Bun `js_parser/ast/Binding.cp`. Backed entirely by caller-owned
// external slices (`[]JsBindingNode`, `[]JsArrayBindingItem`, `[]JsObjectBindingProp`)
// with zero heap allocation and no hardcoded small node limits.

pub const JS_BINDING_NONE: u32 = 0xFFFFFFFF;
pub const JS_BINDING_MAX_DEPTH: u32 = 64;

pub const JS_BINDING_PROP_NONE: u8 = 0x00;
pub const JS_BINDING_PROP_IS_SPREAD: u8 = 0x01;
pub const JS_BINDING_PROP_IS_COMPUTED: u8 = 0x02;
pub const JS_BINDING_PROP_IS_SHORTHAND: u8 = 0x04;

pub err JsBindingError {
    ArenaFull,
    InvalidNodeId,
    EmptyIdentifier,
    InvalidRestPosition,
    RestWithDefault,
    InvalidRestTarget,
    NestingTooDeep,
    OutputTooSmall,
    BufferTooSmall,
}

/// Discriminant matching Bun `ast.Binding.Tag`.
pub enum JsBindingTag: u8 {
    b_identifier = 0,
    b_array = 1,
    b_object = 2,
    b_missing = 3,

    fn asStr(): []const u8 {
        if (_ == JsBindingTag.b_identifier) return "b_identifier";
        if (_ == JsBindingTag.b_array) return "b_array";
        if (_ == JsBindingTag.b_object) return "b_object";
        return "b_missing";
    }
}

/// Element inside an array destructuring pattern (`[a, , b = 1, ...rest]`).
pub struct JsArrayBindingItem {
    binding: u32 = JS_BINDING_NONE,
    default_expr: []const u8 = "",
    is_rest: bool = false,

    static fn simple(binding_id: u32): JsArrayBindingItem {
        return JsArrayBindingItem{
            binding: binding_id,
            default_expr: "",
            is_rest: false,
        };
    }

    static fn withDefault(binding_id: u32, default_expr: []const u8): JsArrayBindingItem {
        return JsArrayBindingItem{
            binding: binding_id,
            default_expr: default_expr,
            is_rest: false,
        };
    }

    static fn rest(binding_id: u32): JsArrayBindingItem {
        return JsArrayBindingItem{
            binding: binding_id,
            default_expr: "",
            is_rest: true,
        };
    }
}

/// Property inside an object destructuring pattern (`{ a, b: c = 1, [k]: v, ...rest }`).
pub struct JsObjectBindingProp {
    key: []const u8 = "",
    value: u32 = JS_BINDING_NONE,
    default_expr: []const u8 = "",
    flags: u8 = JS_BINDING_PROP_NONE,

    static fn shorthand(key: []const u8, value_id: u32, default_expr: []const u8): JsObjectBindingProp {
        return JsObjectBindingProp{
            key: key,
            value: value_id,
            default_expr: default_expr,
            flags: JS_BINDING_PROP_IS_SHORTHAND,
        };
    }

    static fn keyed(key: []const u8, value_id: u32, default_expr: []const u8): JsObjectBindingProp {
        return JsObjectBindingProp{
            key: key,
            value: value_id,
            default_expr: default_expr,
            flags: JS_BINDING_PROP_NONE,
        };
    }

    static fn computed(key_expr: []const u8, value_id: u32, default_expr: []const u8): JsObjectBindingProp {
        return JsObjectBindingProp{
            key: key_expr,
            value: value_id,
            default_expr: default_expr,
            flags: JS_BINDING_PROP_IS_COMPUTED,
        };
    }

    static fn spread(value_id: u32): JsObjectBindingProp {
        return JsObjectBindingProp{
            key: "",
            value: value_id,
            default_expr: "",
            flags: JS_BINDING_PROP_IS_SPREAD,
        };
    }

    fn isSpread(): bool {
        return (_.flags & JS_BINDING_PROP_IS_SPREAD) != 0;
    }

    fn isComputed(): bool {
        return (_.flags & JS_BINDING_PROP_IS_COMPUTED) != 0;
    }

    fn isShorthand(): bool {
        return (_.flags & JS_BINDING_PROP_IS_SHORTHAND) != 0;
    }
}

/// Flattened AST node inside `JsBindingArena`.
pub struct JsBindingNode {
    tag: JsBindingTag = JsBindingTag.b_missing,
    loc_offset: u32 = 0,
    ident_name: []const u8 = "",
    first_child: u32 = 0,
    child_count: u32 = 0,
    has_spread: bool = false,

    static fn empty(): JsBindingNode {
        return JsBindingNode{};
    }
}

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn appendByte(out: []u8, pos: usize, ch: u8): JsBindingError!usize {
    if (pos >= out.len) return JsBindingError.BufferTooSmall;
    out[pos] = ch;
    return pos + 1;
}

fn appendSlice(out: []u8, pos: usize, src: []const u8): JsBindingError!usize {
    if (src.len > out.len - pos) return JsBindingError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

/// External-slice-backed arena for building, validating, querying, and lowering
/// ECMAScript destructuring binding patterns with zero heap allocation.
pub struct JsBindingArena {
    nodes: []JsBindingNode,
    array_items: []JsArrayBindingItem,
    object_props: []JsObjectBindingProp,
    node_len: usize = 0,
    array_item_len: usize = 0,
    object_prop_len: usize = 0,

    static fn init(
        nodes: []JsBindingNode,
        array_items: []JsArrayBindingItem,
        object_props: []JsObjectBindingProp,
    ): JsBindingArena {
        return JsBindingArena{
            nodes: nodes,
            array_items: array_items,
            object_props: object_props,
            node_len: 0,
            array_item_len: 0,
            object_prop_len: 0,
        };
    }

    fn nodeCount(): usize {
        return _.node_len;
    }

    fn getNode(id: u32): JsBindingError!JsBindingNode {
        const idx: usize = @as(usize, id);
        if (idx >= _.node_len) return JsBindingError.InvalidNodeId;
        return _.nodes[idx];
    }

    /// Adds a hole (`b_missing`) for elided array pattern slots like `[a, , b]`.
    fn addMissing(*_, loc_offset: u32): JsBindingError!u32 {
        if (_.node_len >= _.nodes.len) return JsBindingError.ArenaFull;
        const id: u32 = @as(u32, @intCast(_.node_len));
        _.nodes[_.node_len] = JsBindingNode{
            tag: JsBindingTag.b_missing,
            loc_offset: loc_offset,
        };
        _.node_len += 1;
        return id;
    }

    /// Adds an identifier binding (`b_identifier`).
    fn addIdentifier(*_, name: []const u8, loc_offset: u32): JsBindingError!u32 {
        if (name.len == 0) return JsBindingError.EmptyIdentifier;
        if (_.node_len >= _.nodes.len) return JsBindingError.ArenaFull;
        const id: u32 = @as(u32, @intCast(_.node_len));
        _.nodes[_.node_len] = JsBindingNode{
            tag: JsBindingTag.b_identifier,
            loc_offset: loc_offset,
            ident_name: name,
        };
        _.node_len += 1;
        return id;
    }

    /// Adds an array destructuring pattern (`b_array`), enforcing ECMAScript rest-element rules.
    fn addArray(*_, items: []const JsArrayBindingItem, loc_offset: u32): JsBindingError!u32 {
        if (_.node_len >= _.nodes.len) return JsBindingError.ArenaFull;
        if (items.len > _.array_items.len - _.array_item_len) return JsBindingError.ArenaFull;

        let has_spread = false;
        let i: usize = 0;
        while (i < items.len) {
            const it = items[i];
            const child = _.getNode(it.binding)?;
            if (it.is_rest) {
                if (i + 1 != items.len) return JsBindingError.InvalidRestPosition;
                if (it.default_expr.len > 0) return JsBindingError.RestWithDefault;
                if (child.tag == JsBindingTag.b_missing) return JsBindingError.InvalidRestTarget;
                has_spread = true;
            }
            i += 1;
        }

        const first_child: u32 = @as(u32, @intCast(_.array_item_len));
        i = 0;
        while (i < items.len) {
            _.array_items[_.array_item_len + i] = items[i];
            i += 1;
        }
        _.array_item_len += items.len;

        const id: u32 = @as(u32, @intCast(_.node_len));
        _.nodes[_.node_len] = JsBindingNode{
            tag: JsBindingTag.b_array,
            loc_offset: loc_offset,
            first_child: first_child,
            child_count: @as(u32, @intCast(items.len)),
            has_spread: has_spread,
        };
        _.node_len += 1;
        return id;
    }

    /// Adds an object destructuring pattern (`b_object`), enforcing ECMAScript rest-property rules.
    fn addObject(*_, props: []const JsObjectBindingProp, loc_offset: u32): JsBindingError!u32 {
        if (_.node_len >= _.nodes.len) return JsBindingError.ArenaFull;
        if (props.len > _.object_props.len - _.object_prop_len) return JsBindingError.ArenaFull;

        let has_spread = false;
        let i: usize = 0;
        while (i < props.len) {
            const pr = props[i];
            const child = _.getNode(pr.value)?;
            if (pr.isSpread()) {
                if (i + 1 != props.len) return JsBindingError.InvalidRestPosition;
                if (pr.default_expr.len > 0) return JsBindingError.RestWithDefault;
                // ECMAScript: Object rest property (`...rest`) must target an identifier
                if (child.tag != JsBindingTag.b_identifier) return JsBindingError.InvalidRestTarget;
                has_spread = true;
            } else {
                if (pr.key.len == 0) return JsBindingError.EmptyIdentifier;
                if (child.tag == JsBindingTag.b_missing) return JsBindingError.InvalidNodeId;
            }
            i += 1;
        }

        const first_child: u32 = @as(u32, @intCast(_.object_prop_len));
        i = 0;
        while (i < props.len) {
            _.object_props[_.object_prop_len + i] = props[i];
            i += 1;
        }
        _.object_prop_len += props.len;

        const id: u32 = @as(u32, @intCast(_.node_len));
        _.nodes[_.node_len] = JsBindingNode{
            tag: JsBindingTag.b_object,
            loc_offset: loc_offset,
            first_child: first_child,
            child_count: @as(u32, @intCast(props.len)),
            has_spread: has_spread,
        };
        _.node_len += 1;
        return id;
    }

    fn collectNamesRec(id: u32, out_names: [][]const u8, count_in: usize, depth: u32): JsBindingError!usize {
        if (depth > JS_BINDING_MAX_DEPTH) return JsBindingError.NestingTooDeep;
        const node = _.getNode(id)?;
        let count = count_in;

        if (node.tag == JsBindingTag.b_missing) {
            return count;
        }
        if (node.tag == JsBindingTag.b_identifier) {
            if (count >= out_names.len) return JsBindingError.OutputTooSmall;
            out_names[count] = node.ident_name;
            return count + 1;
        }
        if (node.tag == JsBindingTag.b_array) {
            const start: usize = @as(usize, node.first_child);
            const n: usize = @as(usize, node.child_count);
            let i: usize = 0;
            while (i < n) {
                count = _.collectNamesRec(_.array_items[start + i].binding, out_names, count, depth + 1)?;
                i += 1;
            }
            return count;
        }
        // b_object
        const start: usize = @as(usize, node.first_child);
        const n: usize = @as(usize, node.child_count);
        let i: usize = 0;
        while (i < n) {
            count = _.collectNamesRec(_.object_props[start + i].value, out_names, count, depth + 1)?;
            i += 1;
        }
        return count;
    }

    /// Collects all ECMAScript `BoundNames` in left-to-right DFS order into `out_names`.
    fn collectBoundNames(root_id: u32, out_names: [][]const u8): JsBindingError!usize {
        return _.collectNamesRec(root_id, out_names, 0, 0);
    }

    /// Detects duplicate bound identifier names (e.g., `const [a, { a }] = x`) using caller scratch slice.
    fn hasDuplicateBoundNames(root_id: u32, scratch_names: [][]const u8): JsBindingError!bool {
        const n = _.collectBoundNames(root_id, scratch_names)?;
        let i: usize = 0;
        while (i < n) {
            let j: usize = i + 1;
            while (j < n) {
                if (strEql(scratch_names[i], scratch_names[j])) return true;
                j += 1;
            }
            i += 1;
        }
        return false;
    }

    fn hasDefaultRec(id: u32, depth: u32): JsBindingError!bool {
        if (depth > JS_BINDING_MAX_DEPTH) return JsBindingError.NestingTooDeep;
        const node = _.getNode(id)?;
        if (node.tag == JsBindingTag.b_array) {
            const start: usize = @as(usize, node.first_child);
            const n: usize = @as(usize, node.child_count);
            let i: usize = 0;
            while (i < n) {
                const it = _.array_items[start + i];
                if (it.default_expr.len > 0) return true;
                if (_.hasDefaultRec(it.binding, depth + 1)?) return true;
                i += 1;
            }
            return false;
        }
        if (node.tag == JsBindingTag.b_object) {
            const start: usize = @as(usize, node.first_child);
            const n: usize = @as(usize, node.child_count);
            let i: usize = 0;
            while (i < n) {
                const pr = _.object_props[start + i];
                if (pr.default_expr.len > 0) return true;
                if (_.hasDefaultRec(pr.value, depth + 1)?) return true;
                i += 1;
            }
            return false;
        }
        return false;
    }

    fn hasDefaultValues(root_id: u32): JsBindingError!bool {
        return _.hasDefaultRec(root_id, 0);
    }

    fn hasRestRec(id: u32, depth: u32): JsBindingError!bool {
        if (depth > JS_BINDING_MAX_DEPTH) return JsBindingError.NestingTooDeep;
        const node = _.getNode(id)?;
        if (node.has_spread) return true;
        if (node.tag == JsBindingTag.b_array) {
            const start: usize = @as(usize, node.first_child);
            const n: usize = @as(usize, node.child_count);
            let i: usize = 0;
            while (i < n) {
                if (_.hasRestRec(_.array_items[start + i].binding, depth + 1)?) return true;
                i += 1;
            }
        } else if (node.tag == JsBindingTag.b_object) {
            const start: usize = @as(usize, node.first_child);
            const n: usize = @as(usize, node.child_count);
            let i: usize = 0;
            while (i < n) {
                if (_.hasRestRec(_.object_props[start + i].value, depth + 1)?) return true;
                i += 1;
            }
        }
        return false;
    }

    fn hasRestElement(root_id: u32): JsBindingError!bool {
        return _.hasRestRec(root_id, 0);
    }

    fn formatRec(id: u32, out: []u8, pos_in: usize, minify: bool, depth: u32): JsBindingError!usize {
        if (depth > JS_BINDING_MAX_DEPTH) return JsBindingError.NestingTooDeep;
        const node = _.getNode(id)?;
        let pos = pos_in;

        if (node.tag == JsBindingTag.b_missing) {
            return pos;
        }
        if (node.tag == JsBindingTag.b_identifier) {
            return appendSlice(out, pos, node.ident_name);
        }
        if (node.tag == JsBindingTag.b_array) {
            pos = appendByte(out, pos, '[')?;
            const start: usize = @as(usize, node.first_child);
            const n: usize = @as(usize, node.child_count);
            let i: usize = 0;
            while (i < n) {
                if (i > 0) {
                    pos = appendByte(out, pos, ',')?;
                    if (!minify) {
                        pos = appendByte(out, pos, ' ')?;
                    }
                }
                const it = _.array_items[start + i];
                if (it.is_rest) {
                    pos = appendSlice(out, pos, "...")?;
                }
                pos = _.formatRec(it.binding, out, pos, minify, depth + 1)?;
                if (it.default_expr.len > 0) {
                    if (minify) {
                        pos = appendByte(out, pos, '=')?;
                    } else {
                        pos = appendSlice(out, pos, " = ")?;
                    }
                    pos = appendSlice(out, pos, it.default_expr)?;
                }
                // Trailing hole in JS array pattern requires trailing comma: `[a,]` -> `[a, ,]`
                if (i + 1 == n) {
                    const last_child = _.getNode(it.binding)?;
                    if (last_child.tag == JsBindingTag.b_missing) {
                        pos = appendByte(out, pos, ',')?;
                    }
                }
                i += 1;
            }
            return appendByte(out, pos, ']');
        }

        // b_object
        pos = appendByte(out, pos, '{')?;
        const start: usize = @as(usize, node.first_child);
        const n: usize = @as(usize, node.child_count);
        if (!minify && n > 0) {
            pos = appendByte(out, pos, ' ')?;
        }
        let i: usize = 0;
        while (i < n) {
            if (i > 0) {
                pos = appendByte(out, pos, ',')?;
                if (!minify) {
                    pos = appendByte(out, pos, ' ')?;
                }
            }
            const pr = _.object_props[start + i];
            if (pr.isSpread()) {
                pos = appendSlice(out, pos, "...")?;
                pos = _.formatRec(pr.value, out, pos, minify, depth + 1)?;
            } else if (pr.isShorthand()) {
                pos = _.formatRec(pr.value, out, pos, minify, depth + 1)?;
                if (pr.default_expr.len > 0) {
                    if (minify) {
                        pos = appendByte(out, pos, '=')?;
                    } else {
                        pos = appendSlice(out, pos, " = ")?;
                    }
                    pos = appendSlice(out, pos, pr.default_expr)?;
                }
            } else {
                if (pr.isComputed()) {
                    pos = appendByte(out, pos, '[')?;
                    pos = appendSlice(out, pos, pr.key)?;
                    pos = appendByte(out, pos, ']')?;
                } else {
                    pos = appendSlice(out, pos, pr.key)?;
                }
                if (minify) {
                    pos = appendByte(out, pos, ':')?;
                } else {
                    pos = appendSlice(out, pos, ": ")?;
                }
                pos = _.formatRec(pr.value, out, pos, minify, depth + 1)?;
                if (pr.default_expr.len > 0) {
                    if (minify) {
                        pos = appendByte(out, pos, '=')?;
                    } else {
                        pos = appendSlice(out, pos, " = ")?;
                    }
                    pos = appendSlice(out, pos, pr.default_expr)?;
                }
            }
            i += 1;
        }
        if (!minify && n > 0) {
            pos = appendByte(out, pos, ' ')?;
        }
        return appendByte(out, pos, '}');
    }

    /// Formats the destructuring binding pattern into caller-provided `out`.
    fn formatPattern(root_id: u32, out: []u8, minify: bool): JsBindingError!usize {
        return _.formatRec(root_id, out, 0, minify, 0);
    }

    /// Lowers a binding pattern into a valid JS assignment expression (`toExpr` semantics).
    /// Automatically parenthesizes top-level object destructuring: `({ a, b } = rhs)`.
    fn formatAsAssignmentTarget(root_id: u32, rhs_expr: []const u8, out: []u8, minify: bool): JsBindingError!usize {
        const root = _.getNode(root_id)?;
        const needs_parens = root.tag == JsBindingTag.b_object;
        let pos: usize = 0;
        if (needs_parens) {
            pos = appendByte(out, pos, '(')?;
        }
        pos = _.formatRec(root_id, out, pos, minify, 0)?;
        if (minify) {
            pos = appendByte(out, pos, '=')?;
        } else {
            pos = appendSlice(out, pos, " = ")?;
        }
        pos = appendSlice(out, pos, rhs_expr)?;
        if (needs_parens) {
            pos = appendByte(out, pos, ')')?;
        }
        return pos;
    }
}
