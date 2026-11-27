[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# strip-bom

Pure CP zero-allocation UTF-8/UTF-16/UTF-32 BOM detection, zero-copy stripping and idempotent prepending (npm strip-bom parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:strip-bom` ([https://github.com/sindresorhus/strip-bom](https://github.com/sindresorhus/strip-bom)) / `npm:strip-bom-buf` ([https://github.com/sindresorhus/strip-bom-buf](https://github.com/sindresorhus/strip-bom-buf))

| 元信息 | 值 |
| --- | --- |
| 包名 | `strip-bom` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add strip-bom
```

```cp
import {
    BomInfo,
    detectBom,
    hasUtf8Bom,
    stripUtf8Bom,
    stripAnyBomOffset,
    prependUtf8Bom,
} from "strip-bom";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BomInfo` | `class` | 字段: `kind: u8, byte_len: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `detectBom` | `(data: []u8): BomInfo` | 调用 `detectBom` 并返回 `BomInfo`。 |
| `hasUtf8Bom` | `(s: string): bool` | 执行 `hasUtf8Bom` 条件判定，返回 `bool`。 |
| `stripUtf8Bom` | `(s: string): string` | 调用 `stripUtf8Bom` 并返回 `string`。 |
| `stripAnyBomOffset` | `(data: []u8): usize` | 调用 `stripAnyBomOffset` 并返回 `usize`。 |
| `prependUtf8Bom` | `(s: string, out: []u8): usize` | 调用 `prependUtf8Bom` 并返回 `usize`。 |

## 最小调用示例

```cp
import { hasUtf8Bom } from "strip-bom";

pub fn main() {
    let result = hasUtf8Bom("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
