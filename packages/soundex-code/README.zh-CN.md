[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# soundex-code

纯 CP 实现的零堆分配美式 Soundex 语音编码器、可变长/定长填充编码、同音匹配与 0–4 分语音相似度评分引擎。

- **包名**：`soundex-code`（`v1.0.0`）
- **上游对齐**：洁净室移植，同时兼容 `npm/soundex-code`（`MIT`，`words/soundex-code`）与 `crates.io/rphonetic`（`MIT`）。

## 安装与导入

```bash
cap add soundex-code
```

```typescript
import {
    DEFAULT_SOUNDEX_LEN,
    soundexCharCode, soundex, soundexCode, soundexPadded,
    soundsLike, soundsLikeWithLength, soundexDifference,
} from "soundex-code";
```

## 核心 API

| 符号 | 签名 / 值 | 说明 |
| --- | --- | --- |
| `DEFAULT_SOUNDEX_LEN` | `4`（`usize`） | 标准美式 Soundex 编码长度。 |
| `soundexCharCode` | `(ch: u8): u8` | 将 ASCII 字母映射为 Soundex 数字字符（`'1'`–`'6'`）；元音/H/W/非字母返回 `0`。 |
| `soundex` | `(word: string, out: []u8): usize` | 计算标准 4 字符填充 Soundex 编码写入 `out`（正常返回 `4`，无字母或缓冲区不足返回 `0`）。 |
| `soundexCode` | `(word: string, max_length: usize, out: []u8): usize` | 计算最长 `max_length` 的 Soundex 编码，不足 `min(4, max_length)` 时补 `'0'`。 |
| `soundexPadded` | `(word: string, length: usize, out: []u8): usize` | 计算并用 `'0'` 填充至精确长度 `length` 的 Soundex 编码。 |
| `soundsLike` | `(a: string, b: string): bool` | 判断两词的 4 字符标准 Soundex 编码是否相同。 |
| `soundsLikeWithLength` | `(a: string, b: string, max_length: usize): bool` | 判断两词在指定 `max_length` 下的 Soundex 编码是否相同。 |
| `soundexDifference` | `(a: string, b: string): u8` | 返回 SQL 风格语音相似度评分 `0..=4`（4 位编码中对应位置相同的个数）。 |

## 使用示例

### 1. 标准 4 字符 Soundex 编码

```typescript
import { soundex } from "soundex-code";

fn main() {
    let out: [4]u8 = undefined;
    let n1 = soundex("Robert", out[0..]);    // "R163" (n1 == 4)
    let n2 = soundex("Rupert", out[0..]);    // "R163" (n2 == 4)
    let n3 = soundex("Ashcraft", out[0..]);  // "A261"（遵循 H/W 合并规则）
}
```

### 2. 同音匹配与相似度评分

```typescript
import { soundsLike, soundexDifference } from "soundex-code";

fn main() {
    let same = soundsLike("Robert", "Rupert");       // true
    let score = soundexDifference("Robert", "Rubin"); // 2
}
```

### 3. 可变长与自定义填充长度

```typescript
import { soundexCode, soundexPadded, soundsLikeWithLength } from "soundex-code";

fn main() {
    let buf: [8]u8 = undefined;
    let n1 = soundexCode("Washington", 6, buf[0..]);   // "W25235"
    let n2 = soundexPadded("Lee", 6, buf[0..]);        // "L00000"
    let ok = soundsLikeWithLength("Washington", "Wasington", 6);
}
```

## 与源包差异及零拷贝安全亮点

- **全程零堆分配**：所有编码结果直接写入调用方栈切片 `out: []u8`（标准 4 字节只需 `[4]u8`）。
- **精确 NARA H/W 规则**：严格处理被 `H`/`W` 分隔的同码辅音合并（如 `Ashcraft -> A261`、`Pfister -> P236`）。
- **双生态兼容**：同时覆盖 `npm/soundex-code` 可变长编码与 `rphonetic`/SQL `soundexDifference` 评分。
