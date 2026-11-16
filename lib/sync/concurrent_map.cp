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
            for (*s of &map.stripes) {
                s.mutex = .{};
                s.map = MapType.init(allocator);
            }
            return map;
        }

        pub fn deinit(*_) {
            for (*s of &_.stripes) {
                s.mutex.lock();
                s.map.deinit();
                s.mutex.unlock();
                s.mutex.deinit();
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
            for (*s of &_.stripes) {
                s.mutex.lock();
                total += s.map.count();
                s.mutex.unlock();
            }
            return total;
        }

        pub fn clear(*_) {
            for (*s of &_.stripes) {
                s.mutex.lock();
                s.map.clearRetainingCapacity();
                s.mutex.unlock();
            }
        }
    };
}
