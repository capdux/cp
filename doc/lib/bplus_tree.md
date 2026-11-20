# `cap:bplus_tree` — 工业级 B+ 树索引与 4KB Slotted Page 标准库

## 1. 概述

`lib/bplus_tree/`（协议 `cap:bplus_tree` / 动态挂载 `cap.bplus_tree`）提供纯 CP 实现的可变阶内存 B+ 树引擎与数据库标准 4KB 定长 Slotted Page 二进制页编解码器：

- **可变阶 B+ 树引擎 (`BPlusTree`)**：支持任意阶数（`order >= 4`，如 4 到 64+）与变长 `[]const u8` 键值存储，完整实现叶子/内部节点分裂、兄弟左/右借位、节点合并与根收缩，并维护叶子双向链表以支持高效范围扫描与 `validateInvariants()` 全量不变量自检。
- **4KB 定长页编解码 (`SlottedPage`)**：采用标准数据库 Slotted Page 布局（16B Header + 向下增长的 Slot Array + Free Space + 向上增长的 Cell Payload + 尾部 4B CRC32C 校验），支持页内碎片整理 `compact()` 及魔数/校验和篡改拦截。

## 2. 核心类型

- `BPlusError`：统一错误集（`InvalidOrder`, `OutOfMemory`, `KeyNotFound`, `CorruptedTree`, `PageFull`, `InvalidSlot`, `SlotDeleted`, `InvalidMagic`, `CorruptedChecksum`, `InvalidPageSize`, `CellTooLarge`）。
- `KvEntry`：键值对视图 `{ key: []const u8, val: []const u8 }`。
- `RangeIter`：基于叶子双向链表的惰性范围迭代器，支持 `next(): KvEntry?`。
- `BPlusTree`：可变阶 B+ 树索引引擎。
- `SlottedPage`：4096 字节定长页结构体（`PAGE_SIZE = 4096`, `PAGE_MAGIC = 0x43504250`）。

## 3. API 速查

### `BPlusTree`
- `BPlusTree.init(alloc: std.mem.Allocator, order: usize): BPlusError!BPlusTree`
- `deinit(*_)`
- `insert(*_, key: []const u8, val: []const u8): BPlusError!void`（存在则覆盖更新）
- `get(key: []const u8): ([]const u8)?`
- `remove(*_, key: []const u8): bool`
- `first(): KvEntry?` / `last(): KvEntry?`
- `len(): usize` / `height(): usize`
- `rangeScan(alloc, start_key, end_key): BPlusError![]KvEntry`（闭区间）
- `rangeScanEx(alloc, start_key, end_key, inclusive_end): BPlusError![]KvEntry`（支持半开区间）
- `rangeIter(start_key, end_key, inclusive_end): RangeIter`
- `validateInvariants(): BPlusError!void`

### `SlottedPage`
- `SlottedPage.init(): SlottedPage`
- `insertCell(*_, payload: []const u8): BPlusError!u16`
- `getCell(slot_id: u16): BPlusError![]const u8`
- `deleteCell(*_, slot_id: u16): BPlusError!void`
- `compact(*_)`
- `freeSpace(): usize` / `slotCount(): u16` / `activeCount(): u16`
- `encodeSlottedPage(page: *SlottedPage, out: *[PAGE_SIZE]u8)`
- `decodeSlottedPage(raw: []const u8): BPlusError!SlottedPage`

## 4. 示例

```cp
import { BPlusTree, SlottedPage, encodeSlottedPage, decodeSlottedPage, PAGE_SIZE } from "cap:bplus_tree";

let tree = try BPlusTree.init(alloc, 16);
defer tree.deinit();

try tree.insert("user:1001", "alice");
try tree.insert("user:1002", "bob");
try tree.validateInvariants();

const entries = try tree.rangeScan(alloc, "user:1000", "user:1999");
defer alloc.free(entries);

let page = SlottedPage.init();
const slot = try page.insertCell("record-payload");
let raw: [PAGE_SIZE]u8 = undefined;
encodeSlottedPage(&page, &raw);
const decoded = try decodeSlottedPage(&raw);
```
