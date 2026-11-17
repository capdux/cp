// ============================================================================
// lib/sort/index.cp — High-Performance Sorting & Slice Query Primitives (cap:sort)
// Absorbed from Zig Standard Library: std.sort (PDQSort, BlockSort, HeapSort)
// ============================================================================

const std = @import("std");

/// Generic typed slice operations (min, max, etc.)
pub struct SortOf<T> {
    pub static fn min(slice: []const T): T? {
        if (slice.len == 0) return null;
        let mut_min: T = slice[0];
        zig {
            for (slice[1..]) |v| {
                if (v < mut_min) mut_min = v;
            }
        }
        return mut_min;
    }

    pub static fn max(slice: []const T): T? {
        if (slice.len == 0) return null;
        let mut_max: T = slice[0];
        zig {
            for (slice[1..]) |v| {
                if (v > mut_max) mut_max = v;
            }
        }
        return mut_max;
    }
}

/// Unified public sorting and query interface with zero-allocation in-place algorithms.
pub struct Sort {
    /// In-place Pattern-defeating Quicksort (Ascending).
    /// O(N) best case on sorted/equal inputs, O(N log N) guaranteed worst case.
    pub static fn asc(slice: auto) {
        zig {
            const SliceT = @TypeOf(slice);
            const ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => |a| a.child,
                    else => p.child,
                },
                .array => |a| a.child,
                else => @compileError("Sort.asc expects a slice or array"),
            };
            const items: []ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => slice[0..],
                    else => slice,
                },
                .array => slice[0..],
                else => unreachable,
            };
            const lessThan = struct {
                fn order(_: void, a: ChildT, b: ChildT) bool {
                    return a < b;
                }
            }.order;
            @import("std").sort.pdq(ChildT, items, {}, lessThan);
        }
    }

    /// In-place Pattern-defeating Quicksort (Descending).
    pub static fn desc(slice: auto) {
        zig {
            const SliceT = @TypeOf(slice);
            const ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => |a| a.child,
                    else => p.child,
                },
                .array => |a| a.child,
                else => @compileError("Sort.desc expects a slice or array"),
            };
            const items: []ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => slice[0..],
                    else => slice,
                },
                .array => slice[0..],
                else => unreachable,
            };
            const greaterThan = struct {
                fn order(_: void, a: ChildT, b: ChildT) bool {
                    return a > b;
                }
            }.order;
            @import("std").sort.pdq(ChildT, items, {}, greaterThan);
        }
    }

    /// In-place Pattern-defeating Quicksort with custom comparator `cmp(a, b) -> bool`.
    pub static fn sort(slice: auto, cmp: auto) {
        zig {
            const SliceT = @TypeOf(slice);
            const ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => |a| a.child,
                    else => p.child,
                },
                .array => |a| a.child,
                else => @compileError("Sort.sort expects a slice or array"),
            };
            const items: []ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => slice[0..],
                    else => slice,
                },
                .array => slice[0..],
                else => unreachable,
            };
            const FnPtr = fn (ChildT, ChildT) bool;
            const Context = struct {
                cmp_fn: FnPtr,
            };
            const ctx = Context{ .cmp_fn = cmp };
            const wrapper = struct {
                fn less(c: Context, a: ChildT, b: ChildT) bool {
                    return c.cmp_fn(a, b);
                }
            }.less;
            @import("std").sort.pdq(ChildT, items, ctx, wrapper);
        }
    }

    /// Stable in-place block sort with custom comparator `cmp(a, b) -> bool`.
    /// Preserves original relative order of equal elements.
    pub static fn stable(slice: auto, cmp: auto) {
        zig {
            const SliceT = @TypeOf(slice);
            const ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => |a| a.child,
                    else => p.child,
                },
                .array => |a| a.child,
                else => @compileError("Sort.stable expects a slice or array"),
            };
            const items: []ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => slice[0..],
                    else => slice,
                },
                .array => slice[0..],
                else => unreachable,
            };
            const FnPtr = fn (ChildT, ChildT) bool;
            const Context = struct {
                cmp_fn: FnPtr,
            };
            const ctx = Context{ .cmp_fn = cmp };
            const wrapper = struct {
                fn less(c: Context, a: ChildT, b: ChildT) bool {
                    return c.cmp_fn(a, b);
                }
            }.less;
            @import("std").sort.block(ChildT, items, ctx, wrapper);
        }
    }

    /// Stable in-place block sort (Ascending).
    pub static fn stableAsc(slice: auto) {
        zig {
            const SliceT = @TypeOf(slice);
            const ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => |a| a.child,
                    else => p.child,
                },
                .array => |a| a.child,
                else => @compileError("Sort.stableAsc expects a slice or array"),
            };
            const items: []ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => slice[0..],
                    else => slice,
                },
                .array => slice[0..],
                else => unreachable,
            };
            const lessThan = struct {
                fn order(_: void, a: ChildT, b: ChildT) bool {
                    return a < b;
                }
            }.order;
            @import("std").sort.block(ChildT, items, {}, lessThan);
        }
    }

    /// In-place HeapSort with custom comparator `cmp(a, b) -> bool`.
    /// Strictly guaranteed O(N log N) worst-case with O(1) auxiliary memory.
    pub static fn heap(slice: auto, cmp: auto) {
        zig {
            const SliceT = @TypeOf(slice);
            const ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => |a| a.child,
                    else => p.child,
                },
                .array => |a| a.child,
                else => @compileError("Sort.heap expects a slice or array"),
            };
            const items: []ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => slice[0..],
                    else => slice,
                },
                .array => slice[0..],
                else => unreachable,
            };
            const FnPtr = fn (ChildT, ChildT) bool;
            const Context = struct {
                cmp_fn: FnPtr,
            };
            const ctx = Context{ .cmp_fn = cmp };
            const wrapper = struct {
                fn less(c: Context, a: ChildT, b: ChildT) bool {
                    return c.cmp_fn(a, b);
                }
            }.less;
            @import("std").sort.heap(ChildT, items, ctx, wrapper);
        }
    }

    /// In-place HeapSort (Ascending).
    pub static fn heapAsc(slice: auto) {
        zig {
            const SliceT = @TypeOf(slice);
            const ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => |a| a.child,
                    else => p.child,
                },
                .array => |a| a.child,
                else => @compileError("Sort.heapAsc expects a slice or array"),
            };
            const items: []ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => slice[0..],
                    else => slice,
                },
                .array => slice[0..],
                else => unreachable,
            };
            const lessThan = struct {
                fn order(_: void, a: ChildT, b: ChildT) bool {
                    return a < b;
                }
            }.order;
            @import("std").sort.heap(ChildT, items, {}, lessThan);
        }
    }

    /// Checks if a slice is sorted with respect to `cmp(a, b) -> bool`.
    pub static fn isSorted(slice: auto, cmp: auto): bool {
        let sorted: bool = false;
        zig {
            const SliceT = @TypeOf(slice);
            const ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => |a| a.child,
                    else => p.child,
                },
                .array => |a| a.child,
                else => @compileError("Sort.isSorted expects a slice or array"),
            };
            const items: []const ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => slice[0..],
                    else => slice,
                },
                .array => slice[0..],
                else => unreachable,
            };
            const FnPtr = fn (ChildT, ChildT) bool;
            const Context = struct {
                cmp_fn: FnPtr,
            };
            const ctx = Context{ .cmp_fn = cmp };
            const wrapper = struct {
                fn less(c: Context, a: ChildT, b: ChildT) bool {
                    return c.cmp_fn(a, b);
                }
            }.less;
            sorted = @import("std").sort.isSorted(ChildT, items, ctx, wrapper);
        }
        return sorted;
    }

    /// Checks if a slice is sorted in ascending order.
    pub static fn isSortedAsc(slice: auto): bool {
        let sorted: bool = false;
        zig {
            const SliceT = @TypeOf(slice);
            const ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => |a| a.child,
                    else => p.child,
                },
                .array => |a| a.child,
                else => @compileError("Sort.isSortedAsc expects a slice or array"),
            };
            const items: []const ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => slice[0..],
                    else => slice,
                },
                .array => slice[0..],
                else => unreachable,
            };
            const lessThan = struct {
                fn order(_: void, a: ChildT, b: ChildT) bool {
                    return a < b;
                }
            }.order;
            sorted = @import("std").sort.isSorted(ChildT, items, {}, lessThan);
        }
        return sorted;
    }

    /// Binary search in a sorted slice. Returns the index if found, or null.
    pub static fn binarySearch(slice: auto, key: auto): usize? {
        let idx: usize = 0;
        let found: bool = false;
        zig {
            const SliceT = @TypeOf(slice);
            const ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => |a| a.child,
                    else => p.child,
                },
                .array => |a| a.child,
                else => @compileError("Sort.binarySearch expects a slice or array"),
            };
            const items: []const ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => slice[0..],
                    else => slice,
                },
                .array => slice[0..],
                else => unreachable,
            };
            var left: usize = 0;
            var right: usize = items.len;
            while (left < right) {
                const mid = left + (right - left) / 2;
                if (items[mid] < key) {
                    left = mid + 1;
                } else if (items[mid] > key) {
                    right = mid;
                } else {
                    idx = mid;
                    found = true;
                    break;
                }
            }
        }
        if (found) {
            return idx;
        }
        return null;
    }

    /// Returns the index of the first element not less than `key` in a sorted slice.
    pub static fn lowerBound(slice: auto, key: auto): usize {
        let res: usize = 0;
        zig {
            const SliceT = @TypeOf(slice);
            const ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => |a| a.child,
                    else => p.child,
                },
                .array => |a| a.child,
                else => @compileError("Sort.lowerBound expects a slice or array"),
            };
            const items: []const ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => slice[0..],
                    else => slice,
                },
                .array => slice[0..],
                else => unreachable,
            };
            var left: usize = 0;
            var right: usize = items.len;
            while (left < right) {
                const mid = left + (right - left) / 2;
                if (items[mid] < key) {
                    left = mid + 1;
                } else {
                    right = mid;
                }
            }
            res = left;
        }
        return res;
    }

    /// Returns the index of the first element greater than `key` in a sorted slice.
    pub static fn upperBound(slice: auto, key: auto): usize {
        let res: usize = 0;
        zig {
            const SliceT = @TypeOf(slice);
            const ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => |a| a.child,
                    else => p.child,
                },
                .array => |a| a.child,
                else => @compileError("Sort.upperBound expects a slice or array"),
            };
            const items: []const ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => slice[0..],
                    else => slice,
                },
                .array => slice[0..],
                else => unreachable,
            };
            var left: usize = 0;
            var right: usize = items.len;
            while (left < right) {
                const mid = left + (right - left) / 2;
                if (items[mid] <= key) {
                    left = mid + 1;
                } else {
                    right = mid;
                }
            }
            res = left;
        }
        return res;
    }

    /// Returns the index of the minimum element, or null if empty.
    pub static fn argMin(slice: auto): usize? {
        let idx: usize = 0;
        let found: bool = false;
        zig {
            const SliceT = @TypeOf(slice);
            const ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => |a| a.child,
                    else => p.child,
                },
                .array => |a| a.child,
                else => @compileError("Sort.argMin expects a slice or array"),
            };
            const items: []const ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => slice[0..],
                    else => slice,
                },
                .array => slice[0..],
                else => unreachable,
            };
            if (items.len > 0) {
                var best_val = items[0];
                var best_idx: usize = 0;
                for (items[1..], 1..) |v, i| {
                    if (v < best_val) {
                        best_val = v;
                        best_idx = i;
                    }
                }
                idx = best_idx;
                found = true;
            }
        }
        if (found) {
            return idx;
        }
        return null;
    }

    /// Returns the index of the maximum element, or null if empty.
    pub static fn argMax(slice: auto): usize? {
        let idx: usize = 0;
        let found: bool = false;
        zig {
            const SliceT = @TypeOf(slice);
            const ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => |a| a.child,
                    else => p.child,
                },
                .array => |a| a.child,
                else => @compileError("Sort.argMax expects a slice or array"),
            };
            const items: []const ChildT = switch (@typeInfo(SliceT)) {
                .pointer => |p| switch (@typeInfo(p.child)) {
                    .array => slice[0..],
                    else => slice,
                },
                .array => slice[0..],
                else => unreachable,
            };
            if (items.len > 0) {
                var best_val = items[0];
                var best_idx: usize = 0;
                for (items[1..], 1..) |v, i| {
                    if (v > best_val) {
                        best_val = v;
                        best_idx = i;
                    }
                }
                idx = best_idx;
                found = true;
            }
        }
        if (found) {
            return idx;
        }
        return null;
    }

    // Typed min/max convenience helpers
    pub static fn minI32(slice: []const i32): i32? {
        return SortOf(i32).min(slice);
    }

    pub static fn maxI32(slice: []const i32): i32? {
        return SortOf(i32).max(slice);
    }

    pub static fn minI64(slice: []const i64): i64? {
        return SortOf(i64).min(slice);
    }

    pub static fn maxI64(slice: []const i64): i64? {
        return SortOf(i64).max(slice);
    }

    pub static fn minF64(slice: []const f64): f64? {
        return SortOf(f64).min(slice);
    }

    pub static fn maxF64(slice: []const f64): f64? {
        return SortOf(f64).max(slice);
    }
}
