// Source: Absorbed from /data/dev/bun/src/collections/array_list.cp | Synced: 2026-11-22T10:57:41Z
// Pure-CP zero-allocation external-slice-backed generic SliceList<T>
// (no hardcoded capacity limits; full ArrayList semantics: append, insert, orderedRemove,
// swapRemove, replaceRange, appendNTimes, resize, shrinkRetainingCapacity, reverse).

/// `SliceList<T>` 边界与容量错误集
pub err SliceListError {
    Overflow,
    OutOfBounds,
    InvalidRange,
}

/// 外部切片驱动的泛型零堆分配动态视图列表（对齐 Bun `collections/array_list.cp`）
///
/// 由调用方提供任意长度的后备切片 `backing: []T`，无任何硬编码上限；
/// 所有写操作均执行前置边界/容量检查，发生错误时保持列表原子不变（无半截写入）。
pub struct SliceList<T> {
    buf: []T,
    count: usize = 0,

    /// 绑定调用方提供的后备切片，初始长度为 0
    static fn init(backing: []T): SliceList<T> {
        return SliceList<T>{
            buf: backing,
            count: 0,
        };
    }

    /// 绑定已有前 `initial_len` 个有效元素的切片
    static fn fromInitial(backing: []T, initial_len: usize): SliceList<T>! {
        if (initial_len > backing.len) {
            return err.OutOfBounds;
        }
        return SliceList<T>{
            buf: backing,
            count: initial_len,
        };
    }

    fn len(): usize {
        return _.count;
    }

    fn capacity(): usize {
        return _.buf.len;
    }

    fn remaining(): usize {
        return _.buf.len - _.count;
    }

    fn isEmpty(): bool {
        return _.count == 0;
    }

    fn isFull(): bool {
        return _.count == _.buf.len;
    }

    fn items(): []const T {
        return _.buf[0.._.count];
    }

    fn itemsMut(*_): []T {
        return _.buf[0.._.count];
    }

    fn unusedCapacitySlice(*_): []T {
        return _.buf[_.count.._.buf.len];
    }

    fn get(index: usize): T! {
        if (index >= _.count) {
            return err.OutOfBounds;
        }
        return _.buf[index];
    }

    fn set(*_, index: usize, value: T): bool! {
        if (index >= _.count) {
            return err.OutOfBounds;
        }
        _.buf[index] = value;
        return true;
    }

    fn first(): T? {
        if (_.count == 0) {
            return null;
        }
        return _.buf[0];
    }

    fn last(): T? {
        if (_.count == 0) {
            return null;
        }
        return _.buf[_.count - 1];
    }

    /// 尾部 O(1) 追加单个元素
    fn append(*_, item: T): bool! {
        if (_.count >= _.buf.len) {
            return err.Overflow;
        }
        _.buf[_.count] = item;
        _.count += 1;
        return true;
    }

    /// 尾部批量追加切片（前置容量检查，原子失败不写半截）
    fn appendSlice(*_, src: []const T): bool! {
        if (_.buf.len - _.count < src.len) {
            return err.Overflow;
        }
        let i: usize = 0;
        while (i < src.len) {
            _.buf[_.count + i] = src[i];
            i += 1;
        }
        _.count += src.len;
        return true;
    }

    /// 尾部重复追加 `value` 共 `n` 次
    fn appendNTimes(*_, value: T, n: usize): bool! {
        if (_.buf.len - _.count < n) {
            return err.Overflow;
        }
        let i: usize = 0;
        while (i < n) {
            _.buf[_.count + i] = value;
            i += 1;
        }
        _.count += n;
        return true;
    }

    /// 尾部 O(1) 弹出元素
    fn pop(*_): T? {
        if (_.count == 0) {
            return null;
        }
        _.count -= 1;
        return _.buf[_.count];
    }

    /// 在 `index` 处保序插入单个元素（允许 `index == len()`）
    fn insert(*_, index: usize, item: T): bool! {
        if (index > _.count) {
            return err.OutOfBounds;
        }
        if (_.count >= _.buf.len) {
            return err.Overflow;
        }
        let i: usize = _.count;
        while (i > index) {
            _.buf[i] = _.buf[i - 1];
            i -= 1;
        }
        _.buf[index] = item;
        _.count += 1;
        return true;
    }

    /// 在 `index` 处保序批量插入切片
    fn insertSlice(*_, index: usize, src: []const T): bool! {
        if (index > _.count) {
            return err.OutOfBounds;
        }
        if (_.buf.len - _.count < src.len) {
            return err.Overflow;
        }
        if (src.len == 0) {
            return true;
        }
        const tail_len = _.count - index;
        let k: usize = tail_len;
        while (k > 0) {
            k -= 1;
            _.buf[index + src.len + k] = _.buf[index + k];
        }
        let i: usize = 0;
        while (i < src.len) {
            _.buf[index + i] = src[i];
            i += 1;
        }
        _.count += src.len;
        return true;
    }

    /// 保序删除 `index` 处元素并返回其值（后续元素前移）
    fn orderedRemove(*_, index: usize): T! {
        if (index >= _.count) {
            return err.OutOfBounds;
        }
        const removed = _.buf[index];
        let i: usize = index;
        while (i + 1 < _.count) {
            _.buf[i] = _.buf[i + 1];
            i += 1;
        }
        _.count -= 1;
        return removed;
    }

    /// O(1) 删除 `index` 处元素（以末尾元素填补空位）并返回其值
    fn swapRemove(*_, index: usize): T! {
        if (index >= _.count) {
            return err.OutOfBounds;
        }
        const removed = _.buf[index];
        _.count -= 1;
        if (index < _.count) {
            _.buf[index] = _.buf[_.count];
        }
        return removed;
    }

    /// 将 `[start, start + del_len)` 区间原子替换为 `new_items`（支持等长/缩容/扩容）
    fn replaceRange(*_, start: usize, del_len: usize, new_items: []const T): bool! {
        if (start > _.count || del_len > _.count - start) {
            return err.InvalidRange;
        }
        const after_end = start + del_len;
        const tail_len = _.count - after_end;
        const base_len = _.count - del_len;
        if (_.buf.len - base_len < new_items.len) {
            return err.Overflow;
        }
        const new_len = base_len + new_items.len;
        if (new_items.len > del_len) {
            // 扩容：自右向左搬移尾部
            let k: usize = tail_len;
            while (k > 0) {
                k -= 1;
                _.buf[start + new_items.len + k] = _.buf[after_end + k];
            }
        } else if (new_items.len < del_len) {
            // 缩容：自左向右搬移尾部
            let k: usize = 0;
            while (k < tail_len) {
                _.buf[start + new_items.len + k] = _.buf[after_end + k];
                k += 1;
            }
        }
        let i: usize = 0;
        while (i < new_items.len) {
            _.buf[start + i] = new_items[i];
            i += 1;
        }
        _.count = new_len;
        return true;
    }

    /// 调整列表长度：变长时用 `fill` 填充，变短时截断
    fn resize(*_, new_len: usize, fill: T): bool! {
        if (new_len > _.buf.len) {
            return err.Overflow;
        }
        let i: usize = _.count;
        while (i < new_len) {
            _.buf[i] = fill;
            i += 1;
        }
        _.count = new_len;
        return true;
    }

    /// 仅允许缩容截断（`new_len > len()` 报错）
    fn shrinkRetainingCapacity(*_, new_len: usize): bool! {
        if (new_len > _.count) {
            return err.InvalidRange;
        }
        _.count = new_len;
        return true;
    }

    /// O(1) 清空逻辑长度并保留后备切片容量
    fn clearRetainingCapacity(*_) {
        _.count = 0;
    }

    /// 原地反转当前有效元素顺序
    fn reverse(*_) {
        if (_.count <= 1) {
            return;
        }
        let left: usize = 0;
        let right: usize = _.count - 1;
        while (left < right) {
            const tmp = _.buf[left];
            _.buf[left] = _.buf[right];
            _.buf[right] = tmp;
            left += 1;
            right -= 1;
        }
    }
}
