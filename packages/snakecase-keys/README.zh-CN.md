[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# snakecase-keys

Zero-heap object key converter to snake_case with exclude list and fixed key buffer pool, composing map-obj and heck-case (clean-room port of npm snakecase-keys)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:snakecase-keys` ([https://github.com/bendrucker/snakecase-keys](https://github.com/bendrucker/snakecase-keys))

| 元信息 | 值 |
| --- | --- |
| 包名 | `snakecase-keys` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add snakecase-keys
```

```cp
import {
    SnakecaseKeysResult,
    sckStrEq,
    sckIsExcluded,
    snakecaseKeyInto,
    snakecaseKeys,
} from "snakecase-keys";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SnakecaseKeysResult` | `class` | 字段: `key_buf: [1024]u8, key_buf_len: usize, key_offs: [32]usize, key_lens: [32]usize, values: [32]string, active: [32]bool, +1 项` · 方法: `empty, len, get, pushRaw, pushSnake` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `sckStrEq` | `(a: string, b: string): bool` | 调用 `sckStrEq` 并返回 `bool`。 |
| `sckIsExcluded` | `(k: string, exclude: []string): bool` | 调用 `sckIsExcluded` 并返回 `bool`。 |
| `snakecaseKeyInto` | `(key: string, out: []u8): usize` | 调用 `snakecaseKeyInto` 并返回 `usize`。 |
| `snakecaseKeys` | `(src: MapObjResult, exclude: []string): SnakecaseKeysResult` | 调用 `snakecaseKeys` 并返回 `SnakecaseKeysResult`。 |

## 最小调用示例

```cp
import { sckStrEq } from "snakecase-keys";

pub fn main() {
    let result = sckStrEq("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
