[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# kebabcase-keys

纯 CP 实现的零堆分配对象键 `kebab-case` 转换库，支持排除列表（`exclude`）、嵌套点分路径转换与 `stop_paths`、单键/路径转换以及批量对象转换。

- **包名**：`kebabcase-keys`（`v1.0.0`）
- **上游对齐**：洁净室移植自 `npm/kebabcase-keys`（`MIT`，`sindresorhus/kebabcase-keys`），组合复用 `map-obj`（`MapObjResult`）与 `decamelize-keys`（`DecamelizeKeysResult`）。

## 安装与导入

```bash
cap add kebabcase-keys
```

```typescript
import { MapObjResult } from "map-obj";
import { DecamelizeKeysResult } from "decamelize-keys";
import {
    kebabcaseString, isKebabCase,
    kebabcaseKeys, kebabcaseKeysAll, kebabcaseEntries,
    kebabcaseKeysDeep, kebabcaseKeyInto, kebabcasePathInto,
    kebabcaseKeysBatch,
} from "kebabcase-keys";
```

## 核心 API

### 对象与批量键转换

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `kebabcaseKeys` | `(src: MapObjResult, exclude: []string): DecamelizeKeysResult` | 将 `src` 的键转为 `kebab-case`，跳过 `exclude` 中的键。 |
| `kebabcaseKeysAll` | `(src: MapObjResult): DecamelizeKeysResult` | 将 `src` 的全部键转为 `kebab-case`（无排除项）。 |
| `kebabcaseEntries` | `(keys: &[]string, vals: &[]string, exclude: &[]string): DecamelizeKeysResult` | 将并行键值切片转换并构建为 `DecamelizeKeysResult`。 |
| `kebabcaseKeysDeep` | `(src: MapObjResult, exclude: &[]string, stop_paths: &[]string): DecamelizeKeysResult` | 转换点分嵌套路径键（如 `"fooBar.bazQux"` -> `"foo-bar.baz-qux"`），支持 `exclude` 与 `stop_paths` 前缀阻断。 |
| `kebabcaseKeysBatch` | `(items: []MapObjResult, exclude: []string, out: []DecamelizeKeysResult): usize` | 批量转换对象数组到 `out`，返回写入条数。 |

### 字符串、单键与点分路径工具

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `kebabcaseString` / `kebabcaseKeyInto` | `(input: string, out: []u8): usize` | 将单个 camelCase / PascalCase / snake_case 字符串转为 `kebab-case` 写入 `out`。 |
| `kebabcasePathInto` | `(dot_path: string, out: []u8): usize` | 将点分路径的每一段分别转为 `kebab-case` 写入 `out`。 |
| `isKebabCase` | `(s: string): bool` | 校验字符串是否符合小写 `kebab-case` 规范。 |

## 使用示例

### 1. 带排除列表的对象键转换

```typescript
import { MapObjResult } from "map-obj";
import { kebabcaseKeys } from "kebabcase-keys";

fn main() {
    let obj = MapObjResult.empty();
    obj.push("fooBar", "1");
    obj.push("userId", "42");
    obj.push("keepMe", "raw");

    let excl: [1]string = ["keepMe"];
    let res = kebabcaseKeys(obj, excl[0..]);
    // res.get("foo-bar") == "1", res.get("user-id") == "42", res.get("keepMe") == "raw"
}
```

### 2. 点分嵌套路径与 `stop_paths`

```typescript
import { MapObjResult } from "map-obj";
import { kebabcaseKeysDeep } from "kebabcase-keys";

fn main() {
    let obj = MapObjResult.empty();
    obj.push("userProfile.firstName", "Ada");
    obj.push("rawMeta.camelKey", "preserve");

    let excl: [0]string = [];
    let stops: [1]string = ["rawMeta"];
    let res = kebabcaseKeysDeep(obj, excl[0..], stops[0..]);
    // "user-profile.first-name" 已转换；"rawMeta.camelKey" 保持原样
}
```

### 3. 直接字符串与点分路径格式化

```typescript
import { kebabcaseString, kebabcasePathInto, isKebabCase } from "kebabcase-keys";

fn main() {
    let buf: [64]u8 = undefined;
    let n = kebabcaseString("XMLHttpRequest", buf[0..]); // "xml-http-request"
    let ok = isKebabCase(buf[0..n]);                     // true

    let pbuf: [64]u8 = undefined;
    let m = kebabcasePathInto("apiConfig.baseUrl", pbuf[0..]); // "api-config.base-url"
}
```

## 与源包差异及零拷贝安全亮点

- **零堆分配**：基于 `map-obj`（`MapObjResult`，最多 32 项）与 `decamelize-keys`（`DecamelizeKeysResult`，最多 32 项 / 1024 字节键缓冲区）全栈结构运行。
- **扁平点分路径深层转换**：用点分路径表达嵌套键并支持 `stop_paths` 前缀保护，无需递归动态对象树。
- **无正则开销**：纯单趟 ASCII 状态机完成大小写拆分与缩略词处理。
