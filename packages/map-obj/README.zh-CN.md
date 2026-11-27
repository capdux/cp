[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# map-obj

Zero-heap fixed-pool object key-value map/filter/transform utility with skip sentinel and deep key path support (npm map-obj equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:map-obj` ([https://github.com/sindresorhus/map-obj](https://github.com/sindresorhus/map-obj))

| 元信息 | 值 |
| --- | --- |
| 包名 | `map-obj` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add map-obj
```

```cp
import {
    ObjEntry,
    MapObjResult,
    mobjStrEq,
    swapKeysAndValues,
    pickKeys,
    omitKeys,
    filterEmptyValues,
    mergeObjects,
} from "map-obj";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ObjEntry` | `class` | 字段: `key: string, val: string, active: bool` |
| `MapObjResult` | `class` | 字段: `entries: [32]ObjEntry, count: usize` · 方法: `empty, push, get` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `mobjStrEq` | `(a: string, b: string): bool` | 调用 `mobjStrEq` 并返回 `bool`。 |
| `swapKeysAndValues` | `(src: MapObjResult): MapObjResult` | 调用 `swapKeysAndValues` 并返回 `MapObjResult`。 |
| `pickKeys` | `(src: MapObjResult, allow_keys: []string): MapObjResult` | 调用 `pickKeys` 并返回 `MapObjResult`。 |
| `omitKeys` | `(src: MapObjResult, skip_keys: []string): MapObjResult` | 调用 `omitKeys` 并返回 `MapObjResult`。 |
| `filterEmptyValues` | `(src: MapObjResult): MapObjResult` | 调用 `filterEmptyValues` 并返回 `MapObjResult`。 |
| `mergeObjects` | `(base: MapObjResult, overrides: MapObjResult): MapObjResult` | 调用 `mergeObjects` 并返回 `MapObjResult`。 |

## 最小调用示例

```cp
import { mobjStrEq } from "map-obj";

pub fn main() {
    let result = mobjStrEq("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
