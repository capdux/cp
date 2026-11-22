# cap:hive_array

> 参考来源：/data/dev/bun/src/collections/hive_array.cp | 吸收日期：2026-11-22

提供纯 CP、零外部依赖、零堆分配的固定容量位图追踪 Slab 槽池。

## 导入方式

```cp
import { HiveArray } from "cap:hive_array";
```

## 核心类型与 API

### `HiveArray(comptime T: type, comptime capacity: u16): type`

基于 64-bit 分块位图（`[WORD_COUNT]u64`）管理 `capacity` 个连续槽位，支持任意容量（如 64、128、150、256、512），具备稳定槽索引与 O(1) 位图空闲槽检索能力。

| 方法 / 常量 | 签名 | 说明 |
| --- | --- | --- |
| `CAP` | `const CAP: u16` | 编译期容量常量 |
| `init` | `fn init(): Self` | 创建空槽池（全部位图清零，`len = 0`） |
| `alloc` | `fn alloc(*_): u16?` | 使用 `@ctz` 查找首个空闲槽并置位，返回槽索引；满池返回 `null` |
| `allocWith` | `fn allocWith(*_, value: T): u16?` | 分配槽位并写入初始值；满池返回 `null` |
| `get` | `fn get(index: u16): T?` | 按值读取已占用槽位；未占用或越界返回 `null` |
| `getPtr` | `fn getPtr(*_, index: u16): (*T)?` | 获取已占用槽位的可变指针；未占用或越界返回 `null` |
| `set` | `fn set(*_, index: u16, value: T): bool` | 更新已占用槽位的值；未占用或越界返回 `false` |
| `free` | `fn free(*_, index: u16): bool` | 归还槽位；越界或重复释放返回 `false`（防 double-free） |
| `isOccupied` | `fn isOccupied(index: u16): bool` | 查询指定槽位是否已占用 |
| `count` | `fn count(): u16` | 返回当前已占用槽数量 |
| `freeCount` | `fn freeCount(): u16` | 返回当前空闲槽数量 |
| `isEmpty` | `fn isEmpty(): bool` | 是否为空 |
| `isFull` | `fn isFull(): bool` | 是否已满 |
| `clear` | `fn clear(*_)` | 清空全部位图与计数 |
| `nextOccupied` | `fn nextOccupied(start_idx: u16): u16?` | 从 `start_idx` 起使用 `@ctz` 跳跃查找下一个已占用槽位 |

## 使用示例

```cp
import { HiveArray } from "cap:hive_array";

struct Session {
    id: u32;
    active: bool;
}

const Pool = HiveArray(Session, 128);

fn example() {
    let pool = Pool.init();
    if (slot := pool.allocWith(Session{ id: 1001, active: true })) {
        if (ptr := pool.getPtr(slot)) {
            ptr.active = false;
        }
        let cursor: u16 = 0;
        while (pool.nextOccupied(cursor) != null) {
            if (idx := pool.nextOccupied(cursor)) {
                * = pool.get(idx);
                cursor = idx + 1;
            }
        }
        * = pool.free(slot);
    }
}
```

## 上游行为契约与性能/边界特性

1. **指针/索引稳定性（对标 C++ P0447 `std::hive` 与 Bun `HiveArray`）**：
   - 底层存储为编译期固定大小连续数组，元素分配与释放永不触发 `realloc` 搬迁，已分配槽索引与 `getPtr` 返回指针在 `free` 之前恒定有效，适合高并发连接句柄、定时器与异步上下文槽池。
2. **硬件指令级 O(1) 位图检索**：
   - 以 64-bit 位图数组追踪占用状态，`alloc` 与 `nextOccupied` 直接利用 `@ctz` 计数尾随零指令快速跳过整批已满/空闲 64 槽位，避免线性逐槽扫描。
3. **防 Double-Free 与尾字掩码安全**：
   - `free(index)` 对越界索引或未占用槽位安全返回 `false`（不破坏 `count`）；非 64 倍数容量（如 `capacity = 150`）严格校验 `index < CAP`，杜绝尾字高位越界分配。
