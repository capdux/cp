// Source: Absorbed from /data/dev/bun/src/bake/DevServer/PackedMap.cp | Synced: 2026-11-22T18:56:41Z
// ============================================================================
// lib/sourcemap/packedMap.cp — Zero-Alloc Packed Incremental SourceMap Arena
// ============================================================================
// Implements Bun's DevServer `PackedMap` & `SharedPackedMap` architecture in
// 100% pure CP with zero dynamic heap allocation:
// - Files without source maps (HTML/CSS/Assets) use `.line_count` or `.none`
//   handles without allocating any entry or arena bytes.
// - JavaScript/TypeScript files store packed VLQ bytes, pre-escaped JSON
//   source contents, and `PackedMapEndState` in a fixed-capacity arena with
//   explicit strong reference counting and amortized memory-cost accounting.

pub const PACKED_MAP_MAX_SLOTS: usize = 64;
pub const PACKED_MAP_ARENA_BYTES: usize = 8192;
pub const PACKED_MAP_ENTRY_OVERHEAD: usize = 40;

pub err PackedMapError {
    EmptyVlq,
    SlotsFull,
    ArenaFull,
    InvalidHandle,
    BufferTooSmall,
}

/// Last original source line/column tracked at the end of a source map chunk.
pub struct PackedMapEndState {
    original_line: i32 = 0,
    original_column: i32 = 0,

    fn eql(other: PackedMapEndState): bool {
        return _.original_line == other.original_line &&
            _.original_column == other.original_column;
    }
}

/// Tag for `SharedPackedMapRef` (mirrors Bun's `SharedPackedMap`).
pub enum SharedPackedMapKind {
    none,
    line_count,
    some,
}

/// Zero-allocation handle representing either no map, an inline line count
/// (for HTML/CSS/asset files), or a reference-counted packed map slot.
pub struct SharedPackedMapRef {
    kind: SharedPackedMapKind = SharedPackedMapKind.none,
    slot_or_lines: u32 = 0,

    static fn none(): SharedPackedMapRef {
        return SharedPackedMapRef{
            kind: SharedPackedMapKind.none,
            slot_or_lines: 0,
        };
    }

    static fn fromLineCount(lines: u32): SharedPackedMapRef {
        return SharedPackedMapRef{
            kind: SharedPackedMapKind.line_count,
            slot_or_lines: lines,
        };
    }

    static fn fromSlot(slot: u32): SharedPackedMapRef {
        return SharedPackedMapRef{
            kind: SharedPackedMapKind.some,
            slot_or_lines: slot,
        };
    }

    fn isNone(): bool {
        return _.kind == SharedPackedMapKind.none;
    }

    fn isLineCount(): bool {
        return _.kind == SharedPackedMapKind.line_count;
    }

    fn hasMap(): bool {
        return _.kind == SharedPackedMapKind.some;
    }

    fn slotIndex(): u32? {
        if (_.kind != SharedPackedMapKind.some) return null;
        return _.slot_or_lines;
    }

    fn lineCount(): u32? {
        if (_.kind != SharedPackedMapKind.line_count) return null;
        return _.slot_or_lines;
    }

    /// Moves out the current handle and resets this variable to `.none`
    /// (aligned with Bun's `SharedPackedMap.take`).
    fn take(*_): SharedPackedMapRef {
        const out = SharedPackedMapRef{
            kind: _.kind,
            slot_or_lines: _.slot_or_lines,
        };
        _.kind = SharedPackedMapKind.none;
        _.slot_or_lines = 0;
        return out;
    }
}

/// Metadata for a single packed source map entry stored inside `PackedMapArena`.
pub struct PackedMapEntry {
    in_use: bool = false,
    strong_count: u32 = 0,
    vlq_off: u32 = 0,
    vlq_len: u32 = 0,
    escaped_off: u32 = 0,
    escaped_len: u32 = 0,
    end_state: PackedMapEndState = PackedMapEndState{},

    fn memoryCost(): usize {
        if (!_.in_use) return 0;
        return @as(usize, _.vlq_len) + @as(usize, _.escaped_len) + PACKED_MAP_ENTRY_OVERHEAD;
    }

    /// Amortized memory cost across all strong references sharing this packed map.
    fn amortizedMemoryCost(): usize {
        if (!_.in_use) return 0;
        const refs: usize = if (_.strong_count > 0) @as(usize, _.strong_count) else 1;
        return _.memoryCost() / refs;
    }
}

/// Fixed-capacity zero-allocation arena managing reference-counted `PackedMapEntry`
/// payloads (VLQ mappings + pre-escaped source contents + end state).
pub struct PackedMapArena {
    slots: [PACKED_MAP_MAX_SLOTS]PackedMapEntry = [PackedMapEntry{}; PACKED_MAP_MAX_SLOTS],
    arena: [PACKED_MAP_ARENA_BYTES]u8 = undefined,
    arena_len: usize = 0,
    active_count: usize = 0,

    static fn init(): PackedMapArena {
        return PackedMapArena{};
    }

    fn totalActiveCount(): usize {
        return _.active_count;
    }

    fn usedArenaBytes(): usize {
        return _.arena_len;
    }

    /// Allocates a new non-empty packed map entry (mirroring Bun's `PackedMap.newNonEmpty`).
    fn allocPacked(
        *_,
        vlq_bytes: []const u8,
        escaped_source: []const u8,
        end_state: PackedMapEndState,
    ): PackedMapError!SharedPackedMapRef {
        if (vlq_bytes.len == 0) return PackedMapError.EmptyVlq;

        // Find free slot
        let found_slot: usize = PACKED_MAP_MAX_SLOTS;
        let s: usize = 0;
        while (s < PACKED_MAP_MAX_SLOTS) {
            if (!_.slots[s].in_use) {
                found_slot = s;
                break;
            }
            s += 1;
        }
        if (found_slot >= PACKED_MAP_MAX_SLOTS) return PackedMapError.SlotsFull;

        const needed = vlq_bytes.len + escaped_source.len;
        if (needed > PACKED_MAP_ARENA_BYTES - _.arena_len) {
            return PackedMapError.ArenaFull;
        }

        const vlq_off = _.arena_len;
        if (vlq_bytes.len > 0) {
            @memcpy(_.arena[vlq_off .. vlq_off + vlq_bytes.len], vlq_bytes);
        }

        const escaped_off = vlq_off + vlq_bytes.len;
        if (escaped_source.len > 0) {
            @memcpy(_.arena[escaped_off .. escaped_off + escaped_source.len], escaped_source);
        }
        _.arena_len = escaped_off + escaped_source.len;

        _.slots[found_slot] = PackedMapEntry{
            in_use: true,
            strong_count: 1,
            vlq_off: @as(u32, @intCast(vlq_off)),
            vlq_len: @as(u32, @intCast(vlq_bytes.len)),
            escaped_off: @as(u32, @intCast(escaped_off)),
            escaped_len: @as(u32, @intCast(escaped_source.len)),
            end_state: end_state,
        };
        _.active_count += 1;
        return SharedPackedMapRef.fromSlot(@as(u32, @intCast(found_slot)));
    }

    /// Clones a handle: increments strong reference count for `.some`, zero-cost pass-through
    /// for `.none` and `.line_count`.
    fn cloneRef(*_, handle: SharedPackedMapRef): PackedMapError!SharedPackedMapRef {
        if (handle.kind != SharedPackedMapKind.some) return handle;
        const idx = @as(usize, handle.slot_or_lines);
        if (idx >= PACKED_MAP_MAX_SLOTS || !_.slots[idx].in_use) {
            return PackedMapError.InvalidHandle;
        }
        _.slots[idx].strong_count += 1;
        return handle;
    }

    /// Releases a handle: decrements strong count for `.some`, freeing the slot when
    /// strong count reaches 0. Returns `true` if the underlying slot was freed.
    fn releaseRef(*_, handle: SharedPackedMapRef): PackedMapError!bool {
        if (handle.kind != SharedPackedMapKind.some) return false;
        const idx = @as(usize, handle.slot_or_lines);
        if (idx >= PACKED_MAP_MAX_SLOTS || !_.slots[idx].in_use || _.slots[idx].strong_count == 0) {
            return PackedMapError.InvalidHandle;
        }
        _.slots[idx].strong_count -= 1;
        if (_.slots[idx].strong_count == 0) {
            _.slots[idx].in_use = false;
            if (_.active_count > 0) _.active_count -= 1;
            return true;
        }
        return false;
    }

    fn vlqSlice(handle: SharedPackedMapRef): ([]const u8)? {
        if (handle.kind != SharedPackedMapKind.some) return null;
        const idx = @as(usize, handle.slot_or_lines);
        if (idx >= PACKED_MAP_MAX_SLOTS || !_.slots[idx].in_use) return null;
        const e = _.slots[idx];
        const start = @as(usize, e.vlq_off);
        const end = start + @as(usize, e.vlq_len);
        return _.arena[start..end];
    }

    fn escapedSourceSlice(handle: SharedPackedMapRef): ([]const u8)? {
        if (handle.kind != SharedPackedMapKind.some) return null;
        const idx = @as(usize, handle.slot_or_lines);
        if (idx >= PACKED_MAP_MAX_SLOTS || !_.slots[idx].in_use) return null;
        const e = _.slots[idx];
        const start = @as(usize, e.escaped_off);
        const end = start + @as(usize, e.escaped_len);
        return _.arena[start..end];
    }

    fn endState(handle: SharedPackedMapRef): PackedMapEndState? {
        if (handle.kind != SharedPackedMapKind.some) return null;
        const idx = @as(usize, handle.slot_or_lines);
        if (idx >= PACKED_MAP_MAX_SLOTS || !_.slots[idx].in_use) return null;
        return _.slots[idx].end_state;
    }

    fn strongCount(handle: SharedPackedMapRef): u32 {
        if (handle.kind != SharedPackedMapKind.some) return 0;
        const idx = @as(usize, handle.slot_or_lines);
        if (idx >= PACKED_MAP_MAX_SLOTS || !_.slots[idx].in_use) return 0;
        return _.slots[idx].strong_count;
    }

    fn memoryCost(handle: SharedPackedMapRef): usize {
        if (handle.kind != SharedPackedMapKind.some) return 0;
        const idx = @as(usize, handle.slot_or_lines);
        if (idx >= PACKED_MAP_MAX_SLOTS || !_.slots[idx].in_use) return 0;
        return _.slots[idx].memoryCost();
    }

    fn amortizedMemoryCost(handle: SharedPackedMapRef): usize {
        if (handle.kind != SharedPackedMapKind.some) return 0;
        const idx = @as(usize, handle.slot_or_lines);
        if (idx >= PACKED_MAP_MAX_SLOTS || !_.slots[idx].in_use) return 0;
        return _.slots[idx].amortizedMemoryCost();
    }

    /// Writes the quoted `sourcesContent` JSON fragment for lazy `.js.map` assembly.
    /// Returns `"null"` for `.none`/`.line_count` handles, or `"<escaped_source>"` for `.some`.
    fn writeSourcesContentItem(handle: SharedPackedMapRef, out: []u8): PackedMapError!usize {
        if (handle.kind != SharedPackedMapKind.some) {
            if (out.len < 4) return PackedMapError.BufferTooSmall;
            out[0] = 'n';
            out[1] = 'u';
            out[2] = 'l';
            out[3] = 'l';
            return 4;
        }
        const esc = _.escapedSourceSlice(handle) ?? return PackedMapError.InvalidHandle;
        if (esc.len + 2 > out.len) return PackedMapError.BufferTooSmall;
        out[0] = '"';
        if (esc.len > 0) {
            @memcpy(out[1 .. 1 + esc.len], esc);
        }
        out[1 + esc.len] = '"';
        return esc.len + 2;
    }
}
