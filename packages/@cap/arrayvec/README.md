# @cap/arrayvec

零堆分配的 CP 栈上定容字符串与定容向量容器 — 洁净室 MIT 实现，对标 Rust crates.io #214 `arrayvec`（7.30 亿+ 总下载），专为无 GC 高并发热路径与协议拼装打造。

Zero-allocation stack-backed fixed-capacity containers (`ArrayString64`, `ArrayString256`, `ArrayVecI64`, `ArrayVecU64`) for CP — clean-room MIT parity with Rust `arrayvec` (730M+ downloads).

## 安装 / Install

```bash
cap add @cap/arrayvec
```

## API 参考 / API Reference

- **`ArrayString64`**（64B）与 **`ArrayString256`**（256B）：
  - `init()`, `fromStr(s)`, `len()`, `capacity()`, `remaining()`, `isEmpty()`, `isFull()`
  - `tryPushByte(*_, b)`, `tryPushStr(*_, s)`, `pushStrTruncating(*_, s)`, `popByte(*_)`, `truncate(*_, n)`, `clear(*_)`, `asStr()`
- **`ArrayVecI64`**（64 项）与 **`ArrayVecU64`**（64 项）：
  - `init()`, `len()`, `isEmpty()`, `isFull()`
  - `tryPush(*_, v)`, `pop(*_)`, `get(idx)`, `set(*_, idx, v)`, `tryInsert(*_, idx, v)`, `remove(*_, idx)`, `swapRemove(*_, idx)`（O(1)）, `truncate(*_, n)`, `clear(*_)`

## 使用示例 / Usage

```cp
import { ArrayString64, ArrayVecU64 } from "@cap/arrayvec";

let s = ArrayString64.fromStr("GET ").?;
* = s.tryPushStr("/api/v1");
const req = s.asStr(); // "GET /api/v1"

let v = ArrayVecU64.init();
* = v.tryPush(10 as u64);
* = v.tryPush(20 as u64);
* = v.tryPush(30 as u64);
const removed = v.swapRemove(0 as usize).?; // 10, remaining: [30, 20]
```

## License

MIT
