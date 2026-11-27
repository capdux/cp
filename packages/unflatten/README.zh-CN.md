[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# unflatten

适用于 CP 语言的零堆分配嵌套 JSON 扁平化与还原库，对齐 npm [`flat` / `unflatten`](https://github.com/hughsk/flat)（MIT 协议，CP v1.0.0）。

支持自定义键分隔符（`delimiter`）、最大展开深度（`max_depth`）、数组保护模式（`safe`）、结构化键值表提取（`FlatTable`）以及扁平 JSON 到嵌套 JSON 对象/数组的无损往返还原（`unflattenJson`），全程零堆分配。

## 安装与导入

```bash
cap add unflatten
```

```cp
import {
    FlatOptions,
    FlatTable,
    flattenJson,
    flattenToJson,
    unflattenJson,
} from "unflatten";
```

## 核心 API 参考

### 配置与数据结构

| 类型 | 字段 / 方法 | 说明 |
| :--- | :--- | :--- |
| `FlatOptions` | `delimiter: u8`, `max_depth: usize`, `safe: bool` | 控制扁平化与还原行为（`delimiter == 0` 时默认使用 `.`）。 |
| `FlatOptions.defaults()` | `(): FlatOptions` | 返回默认配置（`delimiter: 46` 即 `'.'`，`max_depth: 0` 不限深度，`safe: false`）。 |
| `FlatEntry` | `key: [96]u8`, `key_len: usize`, `val: [128]u8`, `val_len: usize` | 定长扁平键值对条目，存储原始 JSON 值字节。 |
| `FlatTable` | `entries: [48]FlatEntry`, `len: usize` | 固定容量键值表，最多容纳 48 条扁平化 JSON 条目。 |
| `FlatTable.new()` | `(): FlatTable` | 创建空的 `FlatTable`。 |
| `FlatTable.set(key, raw_json_val)` | `(key: string, raw_json_val: string): bool` | 写入或更新键及原始 JSON 值；表满或超长时返回 `false`。 |
| `FlatTable.get(key, out)` | `(key: string, out: []u8): i64` | 将原始 JSON 值拷贝到 `out`，返回字节长度（`>= 0`），未找到或缓冲区不足返回 `-1`。 |
| `FlatTable.count()` | `(): usize` | 返回当前已存储条目数。 |
| `UfPool` / `UfNode` | `nodes: [96]UfNode`, `count: usize` | 还原阶段使用的固定容量树节点池（最多 96 节点，单段键名上限 48 字节）。 |

### 高层核心函数

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `flattenJson` | `(json_in: string, opts: FlatOptions, out_table: &mut FlatTable): i64` | 将嵌套 JSON 扁平化写入 `out_table`，返回条目数（`>= 0`）或负错误码。 |
| `flattenToJson` | `(json_in: string, opts: FlatOptions, out: []u8): i64` | 直接将嵌套 JSON 扁平化为扁平 JSON 对象写入 `out`，返回写入字节数或 `< 0`。 |
| `unflattenJson` | `(flat_json: string, opts: FlatOptions, out: []u8): i64` | 将扁平 JSON 对象还原为嵌套 JSON 对象与数组写入 `out`，返回写入字节数或 `< 0`。 |

### 底层扫描与树构建辅助函数

包同时导出零分配扫描与建树原语供自定义管线复用：`ufDecodeString`、`ufSkipString`、`ufSkipNumber`、`ufSkipValue`、`ufJoinKey`、`ufFlattenValue`、`ufEmitByte`、`ufEmitBytes`、`ufEmitJsonString`、`ufAllocNode`、`ufFindChild`、`ufAppendChild`、`ufInsertPath`、`ufIsSequentialArray`、`ufEmitNode`、`ufStrEq`、`ufCopyBuf`、`ufIsWs`、`ufIsDigit`、`ufIsHex`、`ufSkipWs`、`ufFormatUsize`、`ufEmptyEntry`。

## 代码示例

### 1. 扁平化与还原完整往返

```cp
import { FlatOptions, flattenToJson, unflattenJson } from "unflatten";

fn roundTripExample() {
    let opts = FlatOptions.defaults();
    let input = "{\"user\":{\"name\":\"Alice\",\"roles\":[\"admin\",\"dev\"]}}";

    let flat_buf: [512]u8 = undefined;
    let flat_len = flattenToJson(input, opts, flat_buf[0..512]);
    if (flat_len >= 0) {
        let flat_str = flat_buf[0..(flat_len as usize)];
        // {"user.name":"Alice","user.roles.0":"admin","user.roles.1":"dev"}

        let nested_buf: [512]u8 = undefined;
        let nested_len = unflattenJson(flat_str, opts, nested_buf[0..512]);
        if (nested_len >= 0) {
            let restored = nested_buf[0..(nested_len as usize)];
            // {"user":{"name":"Alice","roles":["admin","dev"]}}
        }
    }
}
```

### 2. 自定义分隔符 + `safe` 数组模式写入 `FlatTable`

```cp
import { FlatOptions, FlatTable, flattenJson } from "unflatten";

fn tableExample() {
    let opts = FlatOptions {
        delimiter: 47, // '/'
        max_depth: 0,
        safe: true,    // 保留嵌套数组整体，不按索引拆分
    };
    let mut table = FlatTable.new();
    let count = flattenJson("{\"cfg\":{\"ports\":[80,443],\"tls\":true}}", opts, &mut table);

    if (count > 0) {
        let val_buf: [128]u8 = undefined;
        let n = table.get("cfg/ports", val_buf[0..128]);
        // n > 0 -> val_buf[0..n] 为 "[80,443]"
    }
}
```

### 3. 限制最大展开深度（`max_depth`）

```cp
import { FlatOptions, flattenToJson } from "unflatten";

fn maxDepthExample() {
    let opts = FlatOptions {
        delimiter: 46, // '.'
        max_depth: 1,
        safe: false,
    };
    let out: [256]u8 = undefined;
    let n = flattenToJson("{\"a\":{\"b\":{\"c\":1}}}", opts, out[0..256]);
    // 超过 max_depth 的深层子树保持原始 JSON 形态
}
```

## 与源包差异与零拷贝安全亮点

- **零堆分配确定性内存**：区别于 JS 版动态构造对象，CP 版所有解析状态、`FlatTable`（最多 48 条、单键 ≤ 96B、单值 ≤ 128B）与 `UfPool`（最多 96 节点）均在栈上或调用方提供的 `[]u8` 缓冲区内完成，无 GC 停顿。
- **显式错误返回**：所有主函数返回 `i64`（`>= 0` 表示条目数或字节数，`< 0` 表示 JSON 畸形或容量/缓冲区不足），不抛出异常。
- **连续索引数组自动识别**：`unflattenJson` 内部通过 `ufIsSequentialArray` 自动检测键名为连续非负整数（`0, 1, 2, ...`）的节点并将其还原为 JSON 数组。
