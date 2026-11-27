[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# file-entry-cache

Zero-heap incremental build and lint file metadata cache in pure CP (covers flat-cache)

| 元信息 | 值 |
| --- | --- |
| 包名 | `file-entry-cache` |
| 版本 | `10.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add file-entry-cache
```

```cp
import {
    FileCacheEntry,
    FileEntryCache,
    fecStrEq,
    fecEmptyEntry,
    fecPathExists,
} from "file-entry-cache";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `FileCacheEntry` | `class` | 字段: `path: string, mtime_ms: i64, size: u64, content_hash: u64, payload: string, valid: bool` |
| `FileEntryCache` | `class` | 字段: `entries: [64]FileCacheEntry, count: usize` · 方法: `new, set, get, hasChanged, remove, reconcile, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `fecStrEq` | `(a: string, b: string): bool` | 调用 `fecStrEq` 并返回 `bool`。 |
| `fecEmptyEntry` | `(): FileCacheEntry` | 调用 `fecEmptyEntry` 并返回 `FileCacheEntry`。 |
| `fecPathExists` | `(path: string, existing_paths: &[]string): bool` | 调用 `fecPathExists` 并返回 `bool`。 |

## 最小调用示例

```cp
import { fecStrEq } from "file-entry-cache";

pub fn main() {
    let result = fecStrEq("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
