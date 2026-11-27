[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `utf8parse`

面向 CP 语言的零分配 RFC 3629 严格 UTF-8 校验器、16-lane SIMD ASCII 快路径扫描器、WHATWG 有损修复器（`U+FFFD`）与 UTF-16 码元长度计算库——完整对标 **crates.io `utf8parse`** 与 **`utf8_iter`**。

## 源包溯源与零成本迁移

| 项目 | 说明 |
| :--- | :--- |
| **源包平台与对标** | `crates.io:utf8parse` ([alacritty/vte](https://github.com/alacritty/vte), v0.2 对齐) & `crates.io:utf8_iter` ([hsivonen/utf8_iter](https://github.com/hsivonen/utf8_iter), v1 对齐) |
| **CP 包名与版本** | `utf8parse@1.0.0` (`MIT`) |
| **外部依赖** | 无（纯 CP，零外部依赖） |

保持原包名导入，零改名成本直接迁移：

```rust
// Rust (std / utf8parse)
let ok = std::str::from_utf8(bytes).is_ok();
let repaired = String::from_utf8_lossy(bytes);
```

```cp
// CP — 保持原包名，16-lane SIMD 加速
import { validateUtf8, isValidUtf8, toLossyUtf8 } from "utf8parse";

const ok = isValidUtf8("Hello, 世界 🚀");  // true
const res = validateUtf8("Hello, 世界 🚀"); // res.valid == true, res.codepoint_count == 11
```

## ReleaseFast 实测性能亮点

在 Intel Xeon Platinum 8175M 平台下使用 `cpc 1.0.0` + `zig 0.17.0 -OReleaseFast -fllvm` 实测（详见 [`bench/ECOSYSTEM_PERF_REPORT.md`](../../bench/ECOSYSTEM_PERF_REPORT.md)）：

| 操作 | 测试载荷 | 延迟 | 吞吐量 | 带宽 | 核心优化架构 |
| :--- | :--- | --: | --: | --: | :--- |
| `validateUtf8` | 64 KB ASCII 缓冲区 | **3,508.13 ns/op** | 285,052 ops/s | **17,815.77 MB/s (17.82 GB/s)** | 16-lane SIMD ASCII 快路径、零堆分配 |

- **17.82 GB/s 极限吞吐**：16-lane SIMD 向量化每轮并行校验 16 字节 ASCII，逼近 L1/L2 缓存带宽上限。
- **严格遵循 RFC 3629**：拒绝过长编码（overlong）、UTF-16 代理项（`U+D800..U+DFFF`）、超过 `U+10FFFF` 的码点以及尾部截断序列。

## 实战示例

### 1. 严格 UTF-8 校验、码点计数与 UTF-16 长度计算

```cp
import { validateUtf8, isValidUtf8, isAscii, utf16Len } from "utf8parse";

fn demoValidation() {
    const ascii_ok = isAscii("GET /api/v1/health HTTP/1.1"); // true
    const utf8_ok = isValidUtf8("你好 CP 🚀");                // true

    const res = validateUtf8("你好 CP 🚀");
    if (res.valid) {
        const cp_count = res.codepoint_count; // 7 个 Unicode 标量值
        // UTF-16 码元长度（🚀 占代理对 2 个码元）
        const u16_units = utf16Len("你好 CP 🚀"); // 8
        _ = cp_count; _ = u16_units;
    }
    _ = ascii_ok; _ = utf8_ok;
}
```

### 2. 非法字节定位（`valid_up_to`、`error_len`）与 WHATWG 有损修复

```cp
import { validateUtf8, toLossyUtf8 } from "utf8parse";

fn demoRepair() {
    // 0xFF 在 UTF-8 中永远非法
    const bad = "ok\xFFtail";
    const res = validateUtf8(bad);
    if (!res.valid) {
        const good_prefix = res.valid_up_to; // 2 ("ok")
        const err_len = res.error_len;       // 1（若为末尾截断则为 0）
        _ = good_prefix; _ = err_len;
    }

    // 按 WHATWG 规范将非法最大子段替换为 U+FFFD (EF BF BD)，零堆写入调用方栈缓冲
    let out: [64]u8 = undefined;
    const repaired = toLossyUtf8(bad, out[0..]); // "ok\u{FFFD}tail"
    _ = repaired;
}
```

### 3. 逐序列步进解析（`checkSeqAt`）与续字节判定

```cp
import { checkSeqAt, isCont } from "utf8parse";

fn demoStepping() {
    const s = "Aé🚀";
    let pos: usize = 0;
    while (pos < s.len) {
        const step = checkSeqAt(s, pos);
        if (!step.valid) { break; }
        pos += step.step_len; // 1 ('A'), 2 ('é'), 4 ('🚀')
    }
    const cont = isCont(0x80); // true (10xxxxxx)
    _ = cont;
}
```

## 核心 API 速览

- **核心类型**：`Utf8CheckResult`（`valid`、`valid_up_to`、`error_len`、`codepoint_count`）、`StepResult`（`valid`、`step_len`、`error_len`）、`Utf8Error`
- **核心函数**：`validateUtf8(data)`、`isValidUtf8(data)`、`isAscii(data)`、`toLossyUtf8(data, out)`、`utf16Len(data)`、`checkSeqAt(data, pos)`、`isCont(b)`

完整类型签名请查阅 [`src/index.d.cp`](src/index.d.cp)。
