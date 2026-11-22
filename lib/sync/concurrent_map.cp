// Source: Absorbed from /data/dev/bun/src/threading/threading.cp | Synced: 2026-11-22T11:14:16Z
// ============================================================================
// Absorbed from Bun: bun/src/threading/concurrent_map.cp
// Upstream Sync: Refer to bun/src/threading/concurrent_map.zig for future upstream updates.
// ============================================================================
// Striped concurrent hash map in CP
// Lock contention partitioned across stripes using Wyhash
import { Mutex } from "./mutex";
import { Wyhash } from "./wyhash";

pub fn ConcurrentMap(comptime K: type, comptime V: type): type {
    const StripeCount: usize = 32;
    const is_string = @comptime (@typeInfo(K) == .pointer and @typeInfo(K).pointer.size == .slice and @typeInfo(K).pointer.child == u8);
    const MapType = @if (is_string) std.StringHashMap(V) else std.AutoHashMap(K, V);

    const Stripe = struct {
        mutex: Mutex = .{},
        map: MapType,
    };

    return struct {
        const Self = @This();

        stripes: [StripeCount]Stripe = undefined,
        allocator: std.mem.Allocator = undefined,

        pub static fn init(allocator: std.mem.Allocator): Self {
            let map: Self = .{
                .stripes = undefined,
                .allocator = allocator,
            };
            let i: usize = 0;
            while (i < StripeCount) {
                map.stripes[i].mutex = .{};
                map.stripes[i].map = MapType.init(allocator);
                i += 1;
            }
            return map;
        }

        pub fn deinit(*_) {
            let i: usize = 0;
            while (i < StripeCount) {
                _.stripes[i].mutex.lock();
                _.stripes[i].map.deinit();
                _.stripes[i].mutex.unlock();
                _.stripes[i].mutex.deinit();
                i += 1;
            }
        }

        inline fn getStripeIndex(key: K): usize {
            const hash_val = if (comptime is_string)
                Wyhash.hash(0x12345678, key)
            else
                std.hash.Wyhash.hash(0x12345678, std.mem.asBytes(&key));
            return @as(usize, @intCast(hash_val % StripeCount));
        }

        pub fn get(*_, key: K): V? {
            const idx = getStripeIndex(key);
            const s = &_.stripes[idx];
            s.mutex.lock();
            defer s.mutex.unlock();
            return s.map.get(key);
        }

        pub fn put(*_, key: K, value: V): !void {
            const idx = getStripeIndex(key);
            const s = &_.stripes[idx];
            s.mutex.lock();
            defer s.mutex.unlock();
            try s.map.put(key, value);
        }

        pub fn remove(*_, key: K): bool {
            const idx = getStripeIndex(key);
            const s = &_.stripes[idx];
            s.mutex.lock();
            defer s.mutex.unlock();
            return s.map.remove(key);
        }

        pub fn contains(*_, key: K): bool {
            const idx = getStripeIndex(key);
            const s = &_.stripes[idx];
            s.mutex.lock();
            defer s.mutex.unlock();
            return s.map.contains(key);
        }

        pub fn count(*_): usize {
            let total: usize = 0;
            let i: usize = 0;
            while (i < StripeCount) {
                _.stripes[i].mutex.lock();
                total += _.stripes[i].map.count();
                _.stripes[i].mutex.unlock();
                i += 1;
            }
            return total;
        }

        pub fn clear(*_) {
            let i: usize = 0;
            while (i < StripeCount) {
                _.stripes[i].mutex.lock();
                _.stripes[i].map.clearRetainingCapacity();
                _.stripes[i].mutex.unlock();
                i += 1;
            }
        }
    };
}
