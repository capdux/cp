// Source: Absorbed from /data/dev/bun/src/collections/pool.cp

/// 侵入式单链表对象池节点
pub struct PoolNode<T> {
    const Self = @This();
    next: (*Self!)? = null,
    data: T = std.mem.zeroes(T),
}

/// 实例级侵入式单链表对象空闲池（支持上限控制、自动 reset 钩子与 `~init()` 确定性全量回收）
pub struct ObjectPool<T> {
    type Node = PoolNode<T>;
    allocator: std.mem.Allocator,
    head: (*Node!)? = null,
    pooled_count: usize = 0,
    max_count: usize = 0,
    total_acquired: usize = 0,
    cache_hits: usize = 0,
    total_allocated: usize = 0,
    total_freed: usize = 0,

    ~init() {
        _.clear();
    }

    /// 构造指定缓存上限的对象池（max_count == 0 表示无上限缓存）
    static fn init(allocator: std.mem.Allocator, max_count: usize): ObjectPool<T> {
        return ObjectPool<T>{
            allocator: allocator,
            max_count: max_count,
        };
    }

    /// 构造无上限对象池
    static fn initUnbounded(allocator: std.mem.Allocator): ObjectPool<T> {
        return ObjectPool<T>{
            allocator: allocator,
            max_count: 0,
        };
    }

    /// 借出一个池节点：优先复用空闲链表头节点（若 T 定义了 `reset(*_)` 则自动调用），否则新分配
    fn acquire(*_): (*Node!)! {
        if (node := _.head) {
            unsafe {
                _.head = node.next;
                node.next = null;
                if (comptime @typeInfo(T) == .@"struct") {
                    if (comptime @hasDecl(T, "reset")) {
                        node.data.reset();
                    }
                }
            }
            _.pooled_count -= 1;
            _.cache_hits += 1;
            _.total_acquired += 1;
            return node;
        }

        let new_node = _.allocator.create(Node)?;
        unsafe {
            new_node.* = Node{
                next: null,
                data: std.mem.zeroes(T),
            };
        }
        _.total_allocated += 1;
        _.total_acquired += 1;
        return new_node;
    }

    /// 借出一个池节点并用指定初值覆盖 data
    fn acquireWith(*_, initial: T): (*Node!)! {
        let node = _.acquire()?;
        unsafe {
            node.data = initial;
        }
        return node;
    }

    /// 仅当池中存在空闲缓存时借出，否则返回 null（不触发堆分配）
    fn acquireIfCached(*_): (*Node!)? {
        if (node := _.head) {
            unsafe {
                _.head = node.next;
                node.next = null;
                if (comptime @typeInfo(T) == .@"struct") {
                    if (comptime @hasDecl(T, "reset")) {
                        node.data.reset();
                    }
                }
            }
            _.pooled_count -= 1;
            _.cache_hits += 1;
            _.total_acquired += 1;
            return node;
        }
        return null;
    }

    /// 归还节点到对象池；若超出 max_count 上限则直接销毁释放不缓存
    fn recycle(*_, node: *Node!) {
        if (_.max_count > 0 && _.pooled_count >= _.max_count) {
            _.allocator.destroy(node);
            _.total_freed += 1;
            return;
        }
        unsafe {
            node.next = _.head;
        }
        _.head = node;
        _.pooled_count += 1;
    }

    /// 直接销毁不再归还的节点
    fn destroy(*_, node: *Node!) {
        _.allocator.destroy(node);
        _.total_freed += 1;
    }

    /// 释放空闲链表中全部缓存节点（幂等安全）
    fn clear(*_) {
        let cur = _.head;
        _.head = null;
        _.pooled_count = 0;
        while (node := cur) {
            unsafe {
                cur = node.next;
            }
            _.allocator.destroy(node);
            _.total_freed += 1;
        }
    }

    /// 当前池内空闲缓存节点数
    fn count(): usize {
        return _.pooled_count;
    }

    /// 池是否已达 max_count 上限（无上限模式下恒返回 false）
    fn isFull(): bool {
        if (_.max_count == 0) {
            return false;
        }
        return _.pooled_count >= _.max_count;
    }

    /// 池内当前是否无空闲缓存节点
    fn isEmpty(): bool {
        return _.pooled_count == 0;
    }

    /// 累计复用命中次数
    fn hits(): usize {
        return _.cache_hits;
    }

    /// 累计堆分配次数
    fn allocated(): usize {
        return _.total_allocated;
    }

    /// 累计堆销毁次数
    fn freed(): usize {
        return _.total_freed;
    }

    /// 返回复用命中率千分比（0..=1000）
    fn hitRatePermille(): u32 {
        if (_.total_acquired == 0) {
            return 0;
        }
        return ((_.cache_hits * 1000) / _.total_acquired) as u32;
    }
}
