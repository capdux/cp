// Source: Absorbed from /data/dev/bun/src/collections/hive_array.cp | Synced: 2026-11-24T07:05:00Z

import std from "std";

/// 固定容量位图追踪 Slab 槽池（纯 CP、零外部依赖、零堆分配）
/// 基于 64-bit 分块位图支持任意 capacity（如 64 / 128 / 150 / 256 / 512）
export fn HiveArray(comptime T: type, comptime capacity: u16): type {
    const WORD_COUNT: usize = if (capacity == 0) 1 else (usize(capacity) + 63) / 64;

    return struct {
        const Self = @This();

        const CAP: u16 = capacity;
        const WORDS: usize = WORD_COUNT;

        slots: [capacity]T;
        bitmap: [WORD_COUNT]u64;
        len: u16;

        /// 创建一个空的固定容量槽池
        fn init(): Self {
            return Self{
                slots: undefined,
                bitmap: std.mem.zeroes([WORD_COUNT]u64),
                len: 0,
            };
        }

        /// O(1) 使用 @ctz 查找首个空闲槽并置位，返回稳定槽索引
        fn alloc(*_): u16? {
            if (_.len >= capacity) {
                return null;
            }
            let w: usize = 0;
            while (w < WORD_COUNT) {
                const free_bits: u64 = ~_.bitmap[w];
                if (free_bits != 0) {
                    const bit: usize = usize(@ctz(free_bits));
                    const idx: usize = w * 64 + bit;
                    if (idx >= usize(capacity)) {
                        return null;
                    }
                    const shift: u6 = @intCast(bit);
                    _.bitmap[w] |= u64(1) << shift;
                    _.len += 1;
                    return u16(idx);
                }
                w += 1;
            }
            return null;
        }

        /// 分配槽位并写入初始值，返回槽索引
        fn allocWith(*_, value: T): u16? {
            if (idx := _.alloc()) {
                _.slots[usize(idx)] = value;
                return idx;
            }
            return null;
        }

        /// 判断指定槽位是否处于已占用状态
        fn isOccupied(index: u16): bool {
            if (index >= capacity) {
                return false;
            }
            const w: usize = usize(index) / 64;
            const shift: u6 = @intCast(usize(index) % 64);
            return (_.bitmap[w] & (u64(1) << shift)) != 0;
        }

        /// 按值读取已占用槽位内容，未占用或越界返回 null
        fn get(index: u16): T? {
            if (!_.isOccupied(index)) {
                return null;
            }
            return _.slots[usize(index)];
        }

        /// 获取已占用槽位的可变指针，未占用或越界返回 null
        fn getPtr(*_, index: u16): (*T)? {
            if (!_.isOccupied(index)) {
                return null;
            }
            return &_.slots[usize(index)];
        }

        /// 更新已占用槽位的值，未占用或越界返回 false
        fn set(*_, index: u16, value: T): bool {
            if (!_.isOccupied(index)) {
                return false;
            }
            _.slots[usize(index)] = value;
            return true;
        }

        /// 释放指定槽位；越界或重复释放安全返回 false
        fn free(*_, index: u16): bool {
            if (index >= capacity) {
                return false;
            }
            const w: usize = usize(index) / 64;
            const shift: u6 = @intCast(usize(index) % 64);
            const mask: u64 = u64(1) << shift;
            if ((_.bitmap[w] & mask) == 0) {
                return false;
            }
            _.bitmap[w] &= ~mask;
            _.len -= 1;
            return true;
        }

        /// 返回当前已占用槽数量
        fn count(): u16 {
            return _.len;
        }

        /// 返回当前空闲槽数量
        fn freeCount(): u16 {
            return capacity - _.len;
        }

        /// 判断槽池是否为空
        fn isEmpty(): bool {
            return _.len == 0;
        }

        /// 判断槽池是否已满
        fn isFull(): bool {
            return _.len >= capacity;
        }

        /// 清空所有占用标记并重置计数
        fn clear(*_) {
            let w: usize = 0;
            while (w < WORD_COUNT) {
                _.bitmap[w] = 0;
                w += 1;
            }
            _.len = 0;
        }

        /// 从 start_idx（含）起基于位图 @ctz 跳跃查找下一个已占用槽位
        fn nextOccupied(start_idx: u16): u16? {
            if (start_idx >= capacity || _.len == 0) {
                return null;
            }
            let w: usize = usize(start_idx) / 64;
            const start_bit: u6 = @intCast(usize(start_idx) % 64);
            const first_mask: u64 = (~u64(0)) << start_bit;
            let word: u64 = _.bitmap[w] & first_mask;

            while (w < WORD_COUNT) {
                if (word != 0) {
                    const bit: usize = usize(@ctz(word));
                    const idx: usize = w * 64 + bit;
                    if (idx < usize(capacity)) {
                        return u16(idx);
                    }
                    return null;
                }
                w += 1;
                if (w < WORD_COUNT) {
                    word = _.bitmap[w];
                }
            }
            return null;
        }
    };
}
