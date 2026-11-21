# `cap:unicode_lut` — Unicode 三级压缩查找表生成器与 O(1) 查询引擎

> 溯源：从 `/data/dev/bun/src/unicode/uucode/lut.cp` 吸收并规范化为纯 CP 标准库模块。

`cap:unicode_lut` 实现经典的三级压缩查找表（3-Level Compressed Lookup Table，Stage 1 -> Stage 2 -> Stage 3）：

- 每 256 个连续码点合并为一个块（`BLOCK_SIZE = 256`）；
- 所有唯一属性值去重存入 `stage3`（最多 256 种唯一值，以 `u8` 索引）；
- 所有 256 字节索引块经哈希去重存入 `stage2`（`[]const u8`，以 `u16` 偏移索引）；
- `stage1`（`[]const u16`）按 `cp >> 8` 直接索引对应块在 `stage2` 中的偏移；
- 查询复杂度严格为 **O(1)**：`stage3[stage2[stage1[cp >> 8] + (cp & 0xFF)]]`，且自带 `~init()` 确定性 RAII 内存释放。

---

## 快速导入

```cp
import {
    Generator,
    Tables,
    Error,
    BLOCK_SIZE,
    MAX_UNICODE_CODEPOINT,
    MAX_UNIQUE_ELEMENTS,
} from "cap:unicode_lut";
```

---

## API 参考

### 1. 常量与错误集

| 符号 | 值 / 成员 | 说明 |
| :--- | :--- | :--- |
| `BLOCK_SIZE` | `256` | 每个 Stage-2 块覆盖的码点数量 |
| `MAX_UNICODE_CODEPOINT` | `0x10FFFF` | Unicode 最大合法码点 |
| `MAX_UNIQUE_ELEMENTS` | `256` | Stage-3 最大唯一元素数（`u8` 索引上限） |
| `Error` | `TooManyUniqueElements`, `Stage2Overflow`, `CodepointOutOfRange` | 构建期与范围错误集 |

### 2. `Tables<Elem>`

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `~init` | `~init()` | 确定性 RAII 析构，自动释放 `stage1` / `stage2` / `stage3` |
| `get` | `fn get(cp: u21): Elem?` | 安全 O(1) 查询，超过 `max_codepoint` 返回 `null` |
| `getUnchecked` | `fn getUnchecked(cp: u21): Elem` | 零分支 O(1) 热路径查询 |
| `totalBytes` | `fn totalBytes(): usize` | 返回压缩后三级表总字节数 |
| `uncompressedBytes` | `fn uncompressedBytes(): usize` | 返回未压缩直查表字节数 |
| `uniqueCount` | `fn uniqueCount(): usize` | 返回去重后的唯一元素数（`stage3.len`） |

### 3. `Generator<Elem, Context>`

`Context` 需实现 `fn get(cp: u21): Elem` 与 `fn eql(a: Elem, b: Elem): bool`。

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `init` | `static fn init(context: Context): Generator<Elem, Context>` | 构造生成器 |
| `generate` | `fn generate(alloc: std.mem.Allocator): Tables<Elem>!` | 构建覆盖全量 `0..=0x10FFFF` 的三级压缩查找表 |
| `generateUpTo` | `fn generateUpTo(alloc: std.mem.Allocator, max_cp: u21): Tables<Elem>!` | 构建覆盖 `0..=max_cp` 的三级压缩查找表 |

---

## 使用示例

```cp
import { Generator } from "cap:unicode_lut";

struct AsciiClassCtx {
    fn get(cp: u21): u8 {
        if (cp >= '0' && cp <= '9') { return 1; }
        if ((cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z')) { return 2; }
        return 0;
    }

    fn eql(a: u8, b: u8): bool {
        return a == b;
    }
}

fn demo(alloc: std.mem.Allocator)! {
    const gen = Generator<u8, AsciiClassCtx>.init(AsciiClassCtx{});
    const lut = gen.generateUpTo(alloc, 0xFFFF)?;
    if (cls := lut.get('A')) {
        * = cls; // 2
    }
} // 离开作用域自动触发 lut.~init() 释放内存
```
