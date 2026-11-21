# `cap:smol_str` — 小字符串内联优化 Compact String

> 溯源：从 `/data/dev/bun/src/string/SmolStr.cp` 吸收并规范化为纯 CP 标准库模块。

`cap:smol_str` 提供带小字符串优化（SSO, Small String Optimization）与 `~init()` 确定性 RAII 释放的 `SmolStr` 类型：

- 长度 `<= 15` 字节（`INLINE_MAX_LEN = 15`）直接存储在栈上内联缓冲区，**完全零堆分配**；
- 长度 `> 15` 字节自动透明切换为堆分配，支持 `appendChar` / `appendSlice` 从内联平滑升级到堆及倍增扩容；
- 离开作用域时自动触发 `~init()` 释放堆缓冲（幂等安全，无 Double-Free）。

---

## 快速导入

```cp
import {
    SmolStr,
    INLINE_MAX_LEN,
    Error,
} from "cap:smol_str";
```

---

## API 参考

| 符号 / 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `INLINE_MAX_LEN` | `pub const INLINE_MAX_LEN: usize = 15` | 栈上内联最大字节长度 |
| `Error` | `pub err Error { StringTooLong }` | `fromInline` 超长错误 |
| `~init` | `~init()` | 确定性 RAII 析构（幂等释放堆缓冲） |
| `empty` | `static fn empty(): SmolStr` | 构造零堆分配空串 |
| `fromChar` | `static fn fromChar(ch: u8): SmolStr` | 构造单字节内联串 |
| `fromInline` | `static fn fromInline(src: string): SmolStr!` | 零堆分配构造，超过 15 字节返回 `err.StringTooLong` |
| `fromSlice` | `static fn fromSlice(allocator, src: string): SmolStr!` | `<= 15B` 内联、`> 15B` 堆分配 |
| `isInline` / `isHeap` / `isEmpty` | `fn ...(): bool` | 存储形态与判空查询 |
| `len` / `slice` | `fn len(): usize` / `fn slice(): string` | 字节长度与只读切片视图 |
| `eql` / `eqlStr` / `hash` | `fn ...(): bool / u64` | 内容比较与 FNV-1a 64-bit 哈希 |
| `clone` | `fn clone(allocator): SmolStr!` | 独立深拷贝 |
| `appendChar` / `appendSlice` | `fn append*(*_, allocator, ...)!` | 追加内容，超阈值时自动迁移至堆 |

---

## 使用示例

```cp
import { SmolStr } from "cap:smol_str";

fn buildTag(alloc: std.mem.Allocator)! {
    let s = SmolStr.fromSlice(alloc, "hello")?; // 内联存储，零堆分配
    s.appendSlice(alloc, " world, compact string!")?; // 自动迁移至堆
    const view = s.slice();
    * = view;
} // 离开作用域自动触发 s.~init() 释放堆内存
```
