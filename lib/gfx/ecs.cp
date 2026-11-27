// Source: cp/lib/ecs/ecs.cp (Pure CP Sparse-Set Entity Component System)
// Pure CP, zero-C-dependency Sparse-Set ECS providing generational EntityId (ABA-safe),
// EntityRegistry with free-list slot recycling, O(1) ComponentPool<T> with Swap-and-Pop
// dense storage, and EcsQuery multi-component intersection views.

const std = @import("std");

const EMPTY_SPARSE: u32 = std.math.maxInt(u32);
const INVALID_INDEX: u32 = std.math.maxInt(u32);

/// Error set for ECS registry, component pool, and query operations.
pub err EcsError {
    DeadEntity,
    ComponentNotFound,
    OutOfMemory,
}

/// Generational entity identifier preventing ABA / dangling-ID reuse bugs.
pub struct EntityId {
    index: u32;
    generation: u32;

    pub const INVALID: EntityId = EntityId{
        index: INVALID_INDEX,
        generation: 0,
    };

    /// Compare two EntityId values for exact index and generation equality.
    pub fn eql(other: EntityId): bool {
        return _.index == other.index && _.generation == other.generation;
    }

    /// Return true if this EntityId does not carry the INVALID sentinel index.
    pub fn isValid(): bool {
        return _.index != INVALID_INDEX && _.generation != 0;
    }

    /// Pack index (low 32 bits) and generation (high 32 bits) into a single u64.
    pub fn raw(): u64 {
        return (u64(_.generation) << 32) | u64(_.index);
    }

    /// Unpack a 64-bit raw identifier back into an EntityId.
    pub static fn fromRaw(raw_val: u64): EntityId {
        return EntityId{
            index: u32(raw_val & 0xFFFF_FFFF),
            generation: u32(raw_val >> 32),
        };
    }
}

/// Manages entity lifecycle, free-list slot recycling, and generational validation.
pub struct EntityRegistry {
    alloc: std.mem.Allocator;
    generations: std.ArrayList(u32) = .empty;
    free_list: std.ArrayList(u32) = .empty;
    alive_count: usize = 0;

    /// Initialize an empty EntityRegistry backed by the given allocator.
    pub static fn init(alloc: std.mem.Allocator): EntityRegistry {
        return EntityRegistry{
            alloc: alloc,
            generations: .empty,
            free_list: .empty,
            alive_count: 0,
        };
    }

    /// Release all internal generation and free-list buffers.
    pub fn deinit(*_) {
        _.generations.deinit(_.alloc);
        _.free_list.deinit(_.alloc);
        _.alive_count = 0;
    }

    /// Spawn a new entity, reusing a recycled slot from free_list when available.
    pub fn spawn(*_): EntityId ! EcsError {
        if (_.free_list.items.len > 0) {
            const last_pos = _.free_list.items.len - 1;
            const idx = _.free_list.items[last_pos];
            _.free_list.items.len = last_pos;
            const gen = _.generations.items[usize(idx)];
            _.alive_count += 1;
            return EntityId{
                index: idx,
                generation: gen,
            };
        }

        if (_.generations.items.len >= usize(INVALID_INDEX)) {
            return EcsError.OutOfMemory;
        }

        const idx = u32(_.generations.items.len);
        _.generations.append(_.alloc, 1) catch return EcsError.OutOfMemory;
        _.alive_count += 1;
        return EntityId{
            index: idx,
            generation: 1,
        };
    }

    /// Despawn a live entity, incrementing its slot generation and queuing the slot for reuse.
    /// Returns false if the entity is already dead or invalid.
    pub fn despawn(*_, entity: EntityId): bool {
        if (!_.isAlive(entity)) return false;

        const idx = usize(entity.index);
        let next_gen = _.generations.items[idx] +% 1;
        if (next_gen == 0) {
            next_gen = 1;
        }
        _.generations.items[idx] = next_gen;
        _.free_list.append(_.alloc, entity.index) catch {};
        _.alive_count -= 1;
        return true;
    }

    /// Return true if entity refers to a currently live slot with matching generation.
    pub fn isAlive(entity: EntityId): bool {
        if (!entity.isValid()) return false;
        const idx = usize(entity.index);
        if (idx >= _.generations.items.len) return false;
        return _.generations.items[idx] == entity.generation;
    }

    /// Number of currently alive entities.
    pub fn aliveCount(): usize {
        return _.alive_count;
    }

    /// Total number of entity slots allocated so far.
    pub fn capacity(): usize {
        return _.generations.items.len;
    }
}

/// Classic Sparse-Set component storage with O(1) set/get/has/remove and dense iteration.
pub fn ComponentPool(comptime T: type): type {
    return struct {
        const Self = @This();

        alloc: std.mem.Allocator;
        sparse: std.ArrayList(u32) = .empty;
        dense_entities: std.ArrayList(EntityId) = .empty;
        dense_data: std.ArrayList(T) = .empty;

        /// Initialize an empty ComponentPool<T>.
        pub static fn init(alloc: std.mem.Allocator): Self {
            return Self{
                alloc: alloc,
                sparse: .empty,
                dense_entities: .empty,
                dense_data: .empty,
            };
        }

        /// Release sparse and dense backing storage.
        pub fn deinit(*_) {
            _.sparse.deinit(_.alloc);
            _.dense_entities.deinit(_.alloc);
            _.dense_data.deinit(_.alloc);
        }

        /// Remove all components while retaining allocated capacity.
        pub fn clear(*_) {
            _.dense_entities.clearRetainingCapacity();
            _.dense_data.clearRetainingCapacity();
            @memset(_.sparse.items, EMPTY_SPARSE);
        }

        /// Attach or overwrite component `val` for a live `entity`.
        /// Returns EcsError.DeadEntity if `entity` is not alive in `reg`.
        pub fn set(*_, entity: EntityId, reg: *const EntityRegistry, val: T): void ! EcsError {
            if (!reg.isAlive(entity)) {
                return EcsError.DeadEntity;
            }

            const e_idx = usize(entity.index);
            if (e_idx >= _.sparse.items.len) {
                const old_len = _.sparse.items.len;
                const new_len = e_idx + 1;
                _.sparse.resize(_.alloc, new_len) catch return EcsError.OutOfMemory;
                @memset(_.sparse.items[old_len..new_len], EMPTY_SPARSE);
            }

            const d_idx = _.sparse.items[e_idx];
            if (d_idx != EMPTY_SPARSE) {
                const u_d_idx = usize(d_idx);
                // Guard against stale entry if entity index was recycled without prior pool removal
                _.dense_entities.items[u_d_idx] = entity;
                _.dense_data.items[u_d_idx] = val;
            } else {
                const new_d_idx = u32(_.dense_entities.items.len);
                _.dense_entities.append(_.alloc, entity) catch return EcsError.OutOfMemory;
                _.dense_data.append(_.alloc, val) catch {
                    _.dense_entities.items.len -= 1;
                    return EcsError.OutOfMemory;
                };
                _.sparse.items[e_idx] = new_d_idx;
            }
        }

        /// Return true if `entity` (with matching generation) currently has a component in this pool.
        pub fn has(entity: EntityId): bool {
            if (!entity.isValid()) return false;
            const e_idx = usize(entity.index);
            if (e_idx >= _.sparse.items.len) return false;
            const d_idx = _.sparse.items[e_idx];
            if (d_idx == EMPTY_SPARSE) return false;
            const u_d_idx = usize(d_idx);
            if (u_d_idx >= _.dense_entities.items.len) return false;
            return _.dense_entities.items[u_d_idx].eql(entity);
        }

        /// Get a mutable pointer to `entity`'s component, or null if absent or generation mismatched.
        pub fn get(*_, entity: EntityId): *T!? {
            if (!_.has(entity)) return null;
            const d_idx = usize(_.sparse.items[usize(entity.index)]);
            return &_.dense_data.items[d_idx];
        }

        /// Get a read-only pointer to `entity`'s component, or null if absent or generation mismatched.
        pub fn getConst(entity: EntityId): *const T!? {
            if (!_.has(entity)) return null;
            const d_idx = usize(_.sparse.items[usize(entity.index)]);
            return &_.dense_data.items[d_idx];
        }

        /// O(1) Swap-and-Pop removal of `entity`'s component while keeping dense arrays contiguous.
        pub fn remove(*_, entity: EntityId): bool {
            if (!_.has(entity)) return false;

            const e_idx = usize(entity.index);
            const d_idx = usize(_.sparse.items[e_idx]);
            const last_idx = _.dense_entities.items.len - 1;

            if (d_idx != last_idx) {
                const moved_entity = _.dense_entities.items[last_idx];
                _.dense_entities.items[d_idx] = moved_entity;
                _.dense_data.items[d_idx] = _.dense_data.items[last_idx];
                _.sparse.items[usize(moved_entity.index)] = u32(d_idx);
            }

            _.dense_entities.items.len -= 1;
            _.dense_data.items.len -= 1;
            _.sparse.items[e_idx] = EMPTY_SPARSE;
            return true;
        }

        /// Number of components currently stored in this pool.
        pub fn len(): usize {
            return _.dense_data.items.len;
        }

        /// Return true if the pool contains zero components.
        pub fn isEmpty(): bool {
            return _.dense_data.items.len == 0;
        }

        /// Contiguous read-only slice of EntityIds matching `items()` 1-to-1 by index.
        pub fn entities(): []const EntityId {
            return _.dense_entities.items;
        }

        /// Contiguous mutable slice of component values for cache-friendly linear iteration.
        pub fn items(*_): []T {
            return _.dense_data.items;
        }

        /// Contiguous read-only slice of component values.
        pub fn itemsConst(): []const T {
            return _.dense_data.items;
        }
    };
}

/// Multi-component query utilities driven by the smaller component pool.
pub static class EcsQuery {
    /// Collect all EntityIds present in both `pool_a` and `pool_b` by driving from the smaller pool.
    /// Caller owns the returned slice and must free it with `alloc.free(...)`.
    pub fn collectJoined2(
        alloc: std.mem.Allocator,
        comptime A: type,
        comptime B: type,
        pool_a: *const ComponentPool(A),
        pool_b: *const ComponentPool(B),
    ): []EntityId ! EcsError {
        let out: std.ArrayList(EntityId) = .empty;
        errdefer out.deinit(alloc);

        if (pool_a.len() <= pool_b.len()) {
            for (e of pool_a.entities()) {
                if (pool_b.has(e)) {
                    out.append(alloc, e) catch return EcsError.OutOfMemory;
                }
            }
        } else {
            for (e of pool_b.entities()) {
                if (pool_a.has(e)) {
                    out.append(alloc, e) catch return EcsError.OutOfMemory;
                }
            }
        }

        return out.toOwnedSlice(alloc) catch return EcsError.OutOfMemory;
    }
}
