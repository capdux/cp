[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ora

Zero-heap CLI terminal spinner animation and status line renderer (clean-room port of ora v8)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ora` ([https://github.com/sindresorhus/ora](https://github.com/sindresorhus/ora))（原库名 `ora` → CP 包名 `ora`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `ora` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ora
```

```cp
import {
    Spinner,
    oraWriteSlice,
    oraFrameCount,
    oraFrameAt,
    oraRenderStatus,
} from "ora";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Spinner` | `class` | 字段: `style: u8, text: string, frame_index: usize` · 方法: `new, frameCount, currentFrame, tick, setText, renderFrame, +6 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `oraWriteSlice` | `(out: []u8, pos: usize, s: string): usize` | 调用 `oraWriteSlice` 并返回 `usize`。 |
| `oraFrameCount` | `(style: u8): usize` | 调用 `oraFrameCount` 并返回 `usize`。 |
| `oraFrameAt` | `(style: u8, index: usize): string` | 调用 `oraFrameAt` 并返回 `string`。 |
| `oraRenderStatus` | `(symbol: string, msg: string, fallback: string, out: []u8): usize` | 调用 `oraRenderStatus` 并返回 `usize`。 |

## 最小调用示例

```cp
import { oraWriteSlice } from "ora";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = oraWriteSlice(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
