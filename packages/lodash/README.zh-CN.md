[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# lodash

面向 CP 的零堆分配点路径（Dot-Path）解析器与深层 JSON 路径读取引擎（兼容 `lodash.get` / `dot-prop` / `dlv` 语义）。

无需构建 JSON DOM 树、零堆内存分配，直接从原始 JSON 字符串中提取深层嵌套对象字段与数组元素。支持方括号索引（`a.b[0].c`）、反斜杠转义特殊键名（`a\.b.c`）及路径段安全转义。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源生态平台 | **npm** |
| 原包名与版本 | `lodash`（`lodash.get`）/ `dot-prop`（CP 生态版 `v1.0.0`） |
| 上游仓库 | `https://github.com/lodash/lodash` · `https://github.com/sindresorhus/dot-prop` |
| 开源协议 | MIT |

从 TypeScript / JavaScript 迁移到 CP 时无需修改包名：

```ts
// TypeScript / Node.js (npm: lodash / dot-prop)
import get from "lodash/get";
const host = get(JSON.parse(raw), "servers[0].host");
```

```cp
// CP (cap: lodash) — 零堆分配，无需先解析整棵 JSON DOM
import { getJsonString, getJsonPath, hasJsonPath } from "lodash";
const host = getJsonString(raw, "servers[0].host");
```

## 安装

```bash
cap add lodash
```

## 实战示例

### 基础用法：深层 JSON 提取与路径存在性校验

```cp
import { getJsonPath, getJsonString, hasJsonPath } from "lodash";

const payload = "{\"user\":{\"name\":\"Alice\",\"roles\":[\"admin\",\"dev\"],\"age\":30}}";

// 提取去掉双引号的字符串值
if (name := getJsonString(payload, "user.name")) {
    // name == "Alice"
}

// 提取原始 JSON 值切片（适用于数字、布尔值、对象、数组与字符串）
if (age := getJsonPath(payload, "user.age")) {
    // age == "30"
}

// 支持数组下标访问
if (role := getJsonString(payload, "user.roles[1]")) {
    // role == "dev"
}

const exists = hasJsonPath(payload, "user.roles[0]"); // true
const missing = hasJsonPath(payload, "user.email");   // false
```

### 进阶与边界用法：含点号键名转义与预解析路径

```cp
import { parseDotPath, escapePathSegment, getJsonPath } from "lodash";

// 1. 将动态键名中的 . [ ] \ 安全转义到栈缓冲区
let escBuf: [128]u8 = undefined;
const escaped = escapePathSegment("api.v1[beta]", escBuf[0..128]);
// escaped == "api\\.v1\\[beta\\]"

// 2. 通过反斜杠转义读取键名中包含字面点号的字段
const cfg = "{\"api.v1\":{\"timeout\":5000}}";
const rawTimeout = getJsonPath(cfg, "api\\.v1.timeout"); // "5000"

// 3. 将路径预解析到栈上 ParsedDotPath（最多支持 32 段）
let keyBuf: [512]u8 = undefined;
const parsed = parseDotPath("items[2].meta\\.id", keyBuf[0..512]);
if (parsed.valid) {
    let i: usize = 0;
    while (i < parsed.count) : (i += 1) {
        if (seg := parsed.get(i)) {
            // seg.is_index, seg.key, seg.index
        }
    }
}
```

## API 速查表

| 导出符号 | 函数/类型签名 | 说明 |
| --- | --- | --- |
| `PathSegment` | `class { is_index: bool, key: string, index: usize }` | 单个路径段（对象键名或数组下标）。 |
| `ParsedDotPath` | `class { segs: [32]PathSegment, count: usize, valid: bool, fn get(idx: usize): PathSegment? }` | 栈上已解析路径对象（上限 32 段）。 |
| `parseDotPath` | `fn(path: string, key_buf: []u8): ParsedDotPath` | 解析点/方括号路径，反转义后的键写入 `key_buf`。 |
| `getJsonPath` | `fn(json: string, path: string): string?` | 返回路径对应的原始 JSON 切片；不存在或非法返回 `null`。 |
| `getJsonString` | `fn(json: string, path: string): string?` | 返回去除首尾引号的字符串值；非字符串或不存在返回 `null`。 |
| `hasJsonPath` | `fn(json: string, path: string): bool` | 判断路径是否存在有效 JSON 值。 |
| `escapePathSegment` | `fn(seg: string, out: []u8): string` | 转义 `.`、`[`、`]`、`\` 并返回写入后的切片。 |

## 性能与边界约束

- **零堆分配**：全程基于输入切片与调用方栈缓冲区运行，无 GC/堆开销。
- **固定上限**：最多支持 `32` 个路径段，`getJsonPath` 内部键缓冲区固定为 `512` 字节。
- **单遍扫描**：直接跳过无关兄弟节点，适合高频 JSON 字段抽取。

## 协议

MIT — 受 `lodash` 与 `dot-prop` 启发的纯 CP 净室实现。
