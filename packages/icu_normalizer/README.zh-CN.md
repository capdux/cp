[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# `icu_normalizer`

CP 零堆分配 UAX #15 Unicode NFC / NFD / NFKC / NFKD 正规化库 —— 对齐 ICU4X [`icu_normalizer`](https://github.com/unicode-org/icu4x)（v1.5）的纯 CP MIT 净室实现，支持规范组合类（CCC）排序、谚文（Hangul）算法拆合以及拉丁/兼容字符映射。

## 安装与导入

```bash
cap add icu_normalizer
```

```cp
import {
    HANGUL_SBASE, HANGUL_LBASE, HANGUL_VBASE, HANGUL_TBASE,
    HANGUL_LCOUNT, HANGUL_VCOUNT, HANGUL_TCOUNT, HANGUL_NCOUNT, HANGUL_SCOUNT,
    canonicalCombiningClass,
    hangulDecompose,
    hangulCompose,
    latinCanonicalDecompose,
    latinCanonicalCompose,
    compatibilityDecompose,
    canonicalOrder,
    normalizeNfd,
    normalizeNfc,
    normalizeNfkd,
    normalizeNfkc,
    isNfc,
    isNfd,
    eqNfc,
} from "icu_normalizer";
```

## 核心 API

### 高层 UTF-8 正规化与等价断言

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `normalizeNfc` | `(input: string, out: []u8): usize` | 将 UTF-8 字符串规范化为 NFC 写入 `out`，返回写入字节数 |
| `normalizeNfd` | `(input: string, out: []u8): usize` | 将 UTF-8 字符串规范化为 NFD 写入 `out` |
| `normalizeNfkc` | `(input: string, out: []u8): usize` | 将 UTF-8 字符串规范化为 NFKC（兼容组合）写入 `out` |
| `normalizeNfkd` | `(input: string, out: []u8): usize` | 将 UTF-8 字符串规范化为 NFKD（兼容分解）写入 `out` |
| `isNfc` / `isNfd` | `(input: string): bool` | 判断输入是否已处于 NFC / NFD 形式（纯 ASCII 走零解码快路径） |
| `eqNfc` | `(a: string, b: string): bool` | 零堆分配判断两个字符串在 Unicode NFC 下是否规范等价 |

### 底层 UAX #15 原语与谚文常量

| 符号 | 签名 / 取值 | 说明 |
| :--- | :--- | :--- |
| `HANGUL_*` | `SBASE=0xAC00`, `LBASE=0x1100`, `VBASE=0x1161`, `TBASE=0x11A7`, `LCOUNT=19`, `VCOUNT=21`, `TCOUNT=28`, `NCOUNT=588`, `SCOUNT=11172` | Unicode 谚文音节算法常量 |
| `canonicalCombiningClass` | `(cp: u32): u8` | 查询码点的规范组合类 CCC（`0..=240`） |
| `canonicalOrder` | `(cps: []u32)` | 就地稳定排序连续非零 CCC 组合记号 |
| `hangulDecompose` / `hangulCompose` | `(cp: u32, out: []u32): usize` / `(l: u32, v: u32, t: u32): u32` | 谚文音节算法分解（`LV` / `LVT`）与重组 |
| `latinCanonicalDecompose` / `latinCanonicalCompose` | `(cp: u32, out: []u32): usize` / `(base: u32, comb: u32): u32` | 预组合拉丁字母的规范分解与组合 |
| `compatibilityDecompose` | `(cp: u32, out: []u32): usize` | 兼容字符分解（全角 ASCII、连字、上下标、分数等） |

## 代码示例

### 1. 判断规范等价字符串与 NFC 正规化

```cp
import { print } from "cap:io";
import { normalizeNfc, normalizeNfd, eqNfc, isNfc } from "icu_normalizer";

fn main() {
    const composed = "caf\u{00E9}";       // "café" (U+00E9)
    const decomposed = "cafe\u{0301}";     // "cafe" + U+0301

    print("canonical eq:", eqNfc(composed, decomposed)); // true
    print("composed isNfc:", isNfc(composed));           // true
    print("decomposed isNfc:", isNfc(decomposed));       // false

    let buf: [128]u8 = undefined;
    const n = normalizeNfc(decomposed, buf[0..128]);
    print("normalized:", buf[0..n]);
}
```

### 2. NFKC 全角折叠与谚文音节算法拆合

```cp
import { print } from "cap:io";
import { normalizeNfkc, hangulDecompose, hangulCompose } from "icu_normalizer";

fn main() {
    // 全角 "ＡＢＣ" -> ASCII "ABC"
    let out: [64]u8 = undefined;
    const n = normalizeNfkc("\u{FF21}\u{FF22}\u{FF23}", out[0..64]);
    print("NFKC:", out[0..n]); // "ABC"

    // 谚文音节 '한' (U+D55C) 拆分为 L/V/T 字母再重组
    let jamo: [3]u32 = [0, 0, 0];
    const k = hangulDecompose(0xD55C, jamo[0..3]);
    if (k == 3) {
        const recomposed = hangulCompose(jamo[0], jamo[1], jamo[2]);
        print("recomposed:", recomposed == 0xD55C); // true
    }
}
```

## 与源包差异及性能亮点

- **零堆内存分配**：所有正规化接口直接写入调用方 `[]u8` / `[]u32` 缓冲，适合标识符清洗、路径规范化与安全比对热路径。
- **ASCII 零解码快路径**：`isNfc`、`isNfd` 与 `eqNfc` 对纯 ASCII 输入直接走 `cap:ascii.isAscii` 与 `cap:bin.bytesEq` 短路返回。
- **零外部数据表包体积**：采用纯算法覆盖全部 11,172 个谚文音节并内联常用拉丁/兼容映射，无需携带数 MB 的 ICU 数据文件。
