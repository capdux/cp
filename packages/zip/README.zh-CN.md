[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# zip

Zero-heap PKZIP Stored archive writer, local header & EOCD parser, and CRC-32 verifier (crates.io zip equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:zip` ([https://github.com/zip-rs/zip2](https://github.com/zip-rs/zip2))（原库名 `zip` → CP 包名 `zip`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `zip` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add zip
```

```cp
import {
    ZipLocalHeader,
    ZipEocd,
    zipEmptyLocalHeader,
    zipEmptyEocd,
    zipCopyStr,
    zipCopyBytes,
    writeStoredZipSingle,
    parseLocalHeader,
} from "zip";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ZipLocalHeader` | `class` | 字段: `version_needed: u16, flags: u16, method: u16, crc32: u32, compressed_size: u32, uncompressed_size: u32, +4 项` |
| `ZipEocd` | `class` | 字段: `disk_num: u16, total_entries: u16, cd_size: u32, cd_offset: u32, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `zipEmptyLocalHeader` | `(): ZipLocalHeader` | 调用 `zipEmptyLocalHeader` 并返回 `ZipLocalHeader`。 |
| `zipEmptyEocd` | `(): ZipEocd` | 调用 `zipEmptyEocd` 并返回 `ZipEocd`。 |
| `zipCopyStr` | `(dst: []u8, offset: usize, s: string)` | 就地执行 `zipCopyStr` 操作。 |
| `zipCopyBytes` | `(dst: []u8, offset: usize, src: []u8)` | 就地执行 `zipCopyBytes` 操作。 |
| `writeStoredZipSingle` | `(filename: string, payload: []u8, out: []u8): usize?` | 调用 `writeStoredZipSingle` 并返回 `usize?`。 |
| `parseLocalHeader` | `(buf: []u8, offset: usize): ZipLocalHeader` | 通过 `parseLocalHeader` 解析输入数据，返回 `ZipLocalHeader`。 |
| `findAndParseEocd` | `(buf: []u8): ZipEocd` | 查询或读取目标数据，返回 `ZipEocd`。 |
| `verifyStoredEntry` | `(buf: []u8, hdr: ZipLocalHeader): bool` | 校验输入合法性，返回 `bool`。 |

## 最小调用示例

```cp
import { writeStoredZipSingle } from "zip";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = writeStoredZipSingle("example", out_buf[0..256], buf_1[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
