// Source: Absorbed from /data/dev/bun/src/js_parser/ast/Symbol.cp | Synced: 2026-11-22T19:04:08Z
// ============================================================================
// lib/js/symbolTable.cp — Zero-Alloc JS/TS Symbol Link Table & Union-Find Map
// ============================================================================
// Implements Bun's `js_parser/ast/Symbol.cp` linker symbol table in 100% pure
// CP with zero dynamic heap allocation:
// - 24-variant `JsSymbolEntryKind` with `isPrivate`/`isHoisted`/`isFunction`
// - 5-bucket `JsSlotNamespace` classification for minifier identifier renaming
// - Iterative path-compressing Union-Find `merge()` / `follow()` / `followAll()`
//   aggregating `use_count_estimate` and `must_not_be_renamed` invariants across
//   merged cross-module bindings.

pub const JS_SYMBOL_MAX_ENTRIES: usize = 256;
pub const JS_SYMBOL_INVALID_INDEX: u32 = 0xFFFFFFFF;

pub err JsSymbolTableError {
    TableFull,
    InvalidRef,
}

/// Renaming namespace bucket assigned to a symbol (aligned with Bun's `SlotNamespace`).
pub enum JsSlotNamespace: u8 {
    must_not_be_renamed = 0,
    default_ns = 1,
    label_ns = 2,
    private_name = 3,
    mangled_prop = 4,
}

/// Status of an imported symbol during linking (aligned with Bun's `ImportItemStatus`).
pub enum JsImportItemStatus: u8 {
    none = 0,
    generated = 1,
    missing = 2,
    dynamic_fallback = 3,
}

/// Full 24-variant JS/TS linker symbol kind (1:1 port of Bun's `Symbol.Kind`).
pub enum JsSymbolEntryKind: u8 {
    unbound = 0,
    hoisted = 1,
    hoisted_function = 2,
    catch_identifier = 3,
    generator_or_async_function = 4,
    arguments_sym = 5,
    class_sym = 6,
    private_field = 7,
    private_method = 8,
    private_get = 9,
    private_set = 10,
    private_get_set_pair = 11,
    private_static_field = 12,
    private_static_method = 13,
    private_static_get = 14,
    private_static_set = 15,
    private_static_get_set_pair = 16,
    label = 17,
    ts_enum = 18,
    ts_namespace = 19,
    import_sym = 20,
    constant = 21,
    local_css = 22,
    other = 23,

    fn isPrivate(): bool {
        const v = @intFromEnum(_);
        return v >= @intFromEnum(JsSymbolEntryKind.private_field) &&
            v <= @intFromEnum(JsSymbolEntryKind.private_static_get_set_pair);
    }

    fn isHoisted(): bool {
        return _ == JsSymbolEntryKind.hoisted || _ == JsSymbolEntryKind.hoisted_function;
    }

    fn isHoistedOrFunction(): bool {
        return _ == JsSymbolEntryKind.hoisted ||
            _ == JsSymbolEntryKind.hoisted_function ||
            _ == JsSymbolEntryKind.generator_or_async_function;
    }

    fn isFunction(): bool {
        return _ == JsSymbolEntryKind.hoisted_function ||
            _ == JsSymbolEntryKind.generator_or_async_function;
    }
}

/// Compact handle identifying a symbol inside `JsSymbolTable`.
pub struct JsSymbolRef {
    source_index: u32 = JS_SYMBOL_INVALID_INDEX,
    inner_index: u32 = JS_SYMBOL_INVALID_INDEX,

    static fn none(): JsSymbolRef {
        return JsSymbolRef{
            source_index: JS_SYMBOL_INVALID_INDEX,
            inner_index: JS_SYMBOL_INVALID_INDEX,
        };
    }

    static fn init(source_index: u32, inner_index: u32): JsSymbolRef {
        return JsSymbolRef{
            source_index: source_index,
            inner_index: inner_index,
        };
    }

    fn isValid(): bool {
        return _.inner_index != JS_SYMBOL_INVALID_INDEX;
    }

    fn eql(other: JsSymbolRef): bool {
        return _.source_index == other.source_index && _.inner_index == other.inner_index;
    }
}

/// Linker symbol record (aligned with Bun's `js_parser/ast/Symbol`).
pub struct JsSymbolRecord {
    in_use: bool = false,
    source_index: u32 = 0,
    original_name: []const u8 = "",
    link: JsSymbolRef = JsSymbolRef.none(),
    use_count_estimate: u32 = 0,
    chunk_index: u32 = JS_SYMBOL_INVALID_INDEX,
    nested_scope_slot: u32 = JS_SYMBOL_INVALID_INDEX,
    did_keep_name: bool = true,
    must_start_with_capital_letter_for_jsx: bool = false,
    kind: JsSymbolEntryKind = JsSymbolEntryKind.other,
    must_not_be_renamed: bool = false,
    import_item_status: JsImportItemStatus = JsImportItemStatus.none,
    private_symbol_must_be_lowered: bool = false,
    has_been_assigned_to: bool = false,

    fn chunkIndex(): u32? {
        if (_.chunk_index == JS_SYMBOL_INVALID_INDEX) return null;
        return _.chunk_index;
    }

    fn nestedScopeSlot(): u32? {
        if (_.nested_scope_slot == JS_SYMBOL_INVALID_INDEX) return null;
        return _.nested_scope_slot;
    }

    fn hasLink(): bool {
        return _.link.isValid();
    }

    fn slotNamespace(): JsSlotNamespace {
        if (_.kind == JsSymbolEntryKind.unbound || _.must_not_be_renamed) {
            return JsSlotNamespace.must_not_be_renamed;
        }
        if (_.kind.isPrivate()) {
            return JsSlotNamespace.private_name;
        }
        if (_.kind == JsSymbolEntryKind.label) {
            return JsSlotNamespace.label_ns;
        }
        return JsSlotNamespace.default_ns;
    }

    fn mergeContentsWith(*_, old: JsSymbolRecord) {
        _.use_count_estimate += old.use_count_estimate;
        if (old.must_not_be_renamed) {
            _.original_name = old.original_name;
            _.must_not_be_renamed = true;
        }
        if (old.must_start_with_capital_letter_for_jsx) {
            _.must_start_with_capital_letter_for_jsx = true;
        }
        if (old.has_been_assigned_to) {
            _.has_been_assigned_to = true;
        }
    }
}

/// Fixed-capacity zero-allocation linker symbol table with iterative path-compressing
/// Union-Find symbol merging (`merge` / `follow` / `followAll`).
pub struct JsSymbolTable {
    entries: [JS_SYMBOL_MAX_ENTRIES]JsSymbolRecord = [JsSymbolRecord{}; JS_SYMBOL_MAX_ENTRIES],
    count: usize = 0,

    static fn init(): JsSymbolTable {
        return JsSymbolTable{};
    }

    fn len(): usize {
        return _.count;
    }

    fn addSymbol(
        *_,
        source_index: u32,
        original_name: []const u8,
        kind: JsSymbolEntryKind,
    ): JsSymbolTableError!JsSymbolRef {
        if (_.count >= JS_SYMBOL_MAX_ENTRIES) return JsSymbolTableError.TableFull;
        const idx = @as(u32, @intCast(_.count));
        _.entries[_.count] = JsSymbolRecord{
            in_use: true,
            source_index: source_index,
            original_name: original_name,
            link: JsSymbolRef.none(),
            use_count_estimate: 0,
            chunk_index: JS_SYMBOL_INVALID_INDEX,
            nested_scope_slot: JS_SYMBOL_INVALID_INDEX,
            did_keep_name: true,
            must_start_with_capital_letter_for_jsx: false,
            kind: kind,
            must_not_be_renamed: false,
            import_item_status: JsImportItemStatus.none,
            private_symbol_must_be_lowered: false,
            has_been_assigned_to: false,
        };
        _.count += 1;
        return JsSymbolRef.init(source_index, idx);
    }

    fn get(target: JsSymbolRef): JsSymbolRecord? {
        if (!target.isValid()) return null;
        const idx = @as(usize, target.inner_index);
        if (idx >= _.count || !_.entries[idx].in_use) return null;
        return _.entries[idx];
    }

    /// Follows symbol links to the canonical root with iterative path compression.
    fn follow(*_, target: JsSymbolRef): JsSymbolRef {
        if (!target.isValid()) return target;
        let cur = target;
        let steps: usize = 0;
        while (steps < JS_SYMBOL_MAX_ENTRIES) {
            const idx = @as(usize, cur.inner_index);
            if (idx >= _.count || !_.entries[idx].in_use) break;
            if (!_.entries[idx].hasLink()) break;
            cur = _.entries[idx].link;
            steps += 1;
        }
        const root = cur;

        // Path compression pass
        let walk = target;
        steps = 0;
        while (steps < JS_SYMBOL_MAX_ENTRIES) {
            const idx = @as(usize, walk.inner_index);
            if (idx >= _.count || !_.entries[idx].in_use) break;
            if (!_.entries[idx].hasLink()) break;
            const next = _.entries[idx].link;
            _.entries[idx].link = root;
            walk = next;
            steps += 1;
        }
        return root;
    }

    /// Resolves `target` following its link (if any) without mutating the table.
    fn getResolved(target: JsSymbolRef): JsSymbolRecord? {
        if (!target.isValid()) return null;
        let cur = target;
        let steps: usize = 0;
        while (steps < JS_SYMBOL_MAX_ENTRIES) {
            const idx = @as(usize, cur.inner_index);
            if (idx >= _.count || !_.entries[idx].in_use) return null;
            if (!_.entries[idx].hasLink()) return _.entries[idx];
            cur = _.entries[idx].link;
            steps += 1;
        }
        return null;
    }

    /// Merges `old_ref` into `new_ref` (1:1 semantics with Bun's `Symbol.Map.merge`):
    /// resolves both roots, links old root -> new root, and aggregates use counts & flags.
    fn merge(*_, old_ref: JsSymbolRef, new_ref: JsSymbolRef): JsSymbolTableError!JsSymbolRef {
        const old_root = _.follow(old_ref);
        const new_root = _.follow(new_ref);
        if (!old_root.isValid() || !new_root.isValid()) return JsSymbolTableError.InvalidRef;

        const old_idx = @as(usize, old_root.inner_index);
        const new_idx = @as(usize, new_root.inner_index);
        if (old_idx >= _.count || new_idx >= _.count) return JsSymbolTableError.InvalidRef;
        if (old_idx == new_idx) return new_root;

        const old_copy = _.entries[old_idx];
        _.entries[old_idx].link = new_root;
        _.entries[new_idx].mergeContentsWith(old_copy);
        return new_root;
    }

    /// Compresses all link chains across the entire table (mirrors `Symbol.Map.followAll`).
    /// Returns the number of linked entries compressed.
    fn followAll(*_): usize {
        let compressed: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            if (_.entries[i].in_use && _.entries[i].hasLink()) {
                const before = _.entries[i].link;
                const root = _.follow(before);
                if (!before.eql(root)) {
                    _.entries[i].link = root;
                    compressed += 1;
                }
            }
            i += 1;
        }
        return compressed;
    }

    fn assignChunkIndex(*_, target: JsSymbolRef, chunk_index: u32): JsSymbolTableError!bool {
        const root = _.follow(target);
        if (!root.isValid()) return JsSymbolTableError.InvalidRef;
        const idx = @as(usize, root.inner_index);
        if (idx >= _.count || !_.entries[idx].in_use) return JsSymbolTableError.InvalidRef;
        _.entries[idx].chunk_index = chunk_index;
        return true;
    }

    fn addUseCount(*_, target: JsSymbolRef, delta: u32): JsSymbolTableError!u32 {
        const root = _.follow(target);
        if (!root.isValid()) return JsSymbolTableError.InvalidRef;
        const idx = @as(usize, root.inner_index);
        if (idx >= _.count || !_.entries[idx].in_use) return JsSymbolTableError.InvalidRef;
        _.entries[idx].use_count_estimate += delta;
        return _.entries[idx].use_count_estimate;
    }

    fn markMustNotBeRenamed(*_, target: JsSymbolRef): JsSymbolTableError!bool {
        const root = _.follow(target);
        if (!root.isValid()) return JsSymbolTableError.InvalidRef;
        const idx = @as(usize, root.inner_index);
        if (idx >= _.count || !_.entries[idx].in_use) return JsSymbolTableError.InvalidRef;
        _.entries[idx].must_not_be_renamed = true;
        return true;
    }

    /// Counts canonical (unlinked root) symbols per `JsSlotNamespace` bucket.
    fn countCanonicalByNamespace(): [5]u32 {
        let counts: [5]u32 = [0; 5];
        let i: usize = 0;
        while (i < _.count) {
            if (_.entries[i].in_use && !_.entries[i].hasLink()) {
                const ns = @as(usize, @intFromEnum(_.entries[i].slotNamespace()));
                counts[ns] += 1;
            }
            i += 1;
        }
        return counts;
    }
}
