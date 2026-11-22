# `cap:nginx` — 纯 CP 零分配 Nginx `.conf` 词法扫描与块解析校验库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:compiler`，导入请使用 `import { ... } from "cap:compiler"`。

## 概述

`cap:nginx` 提供纯 CP、零外部依赖、零堆分配的 Nginx `.conf` 配置文件词法扫描器（`NginxIter`）、层级指令与块迭代器（`NginxBlockIter`）、单层指令查找（`findNginxDirective`）及配置完整性校验器（`validateNginxConfig`），支持 `server` / `location` / `upstream` / `http` 等任意嵌套块递归遍历。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `NginxTokenKind` | 词法 Token 枚举：`word`, `string`, `lbrace`, `rbrace`, `semicolon`, `invalid` |
| `NginxToken` | 词法 Token：`kind`, `lexeme: []const u8`（零拷贝指向原始输入） |
| `NginxIter` | 词法扫描迭代器：`init(input)`, `next(*_): NginxToken?`（跳过 `#` 行注释，支持单/双引号转义、`$var`/`${var}` 与路径/正则 `word`，未闭合引号产出 `invalid`） |
| `NginxDirective` | 指令/块结构体：`name`, `args`, `is_block`, `body: ([]const u8)?`, `depth: u16` |
| `NginxBlockIter` | 层级指令与块迭代器：`init(input)`, `initWithDepth(input, depth)`, `next(*_): NginxDirective?`（支持对 `body` 递归遍历） |
| `findNginxDirective(block, name)` | 在当前块层级按指令名查找首个匹配参数切片 |
| `validateNginxConfig(input)` | 字符串/注释/`${var}` 感知校验 `{}` 平衡、64 层最大深度上限与无未闭合引号 |

## 示例

```cp
import {
    NginxBlockIter,
    findNginxDirective,
    validateNginxConfig,
} from "cap:compiler";

fn inspectServerListen(conf: []const u8): ([]const u8)? {
    if (!validateNginxConfig(conf)) {
        return null;
    }
    let it = NginxBlockIter.init(conf);
    while (d := it.next()) {
        if (d.is_block) {
            if (body := d.body) {
                if (listen := findNginxDirective(body, "listen")) {
                    return listen;
                }
            }
        }
    }
    return null;
}
```
