# `cap:nix` — 纯 CP Nix 表达式与 `flake.nix` 零分配词法扫描与属性提取库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:conf`，导入请使用 `import { ... } from "cap:conf"`。

## 概述

`cap:nix` 提供纯 CP、零外部依赖、零堆分配的 Nix 表达式语言（`.nix` / `flake.nix`）词法扫描器、属性集顶层 `key = value;` 绑定迭代器与定界符深度校验器，完整支持 `#` 与 `/* */` 注释、双引号 `${...}` 嵌套插值、`''...''` 缩进字符串、`./foo.nix` 与 `<nixpkgs>` 路径及 URI 识别。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `NIX_MAX_DEPTH` | 定界符与插值最大允许嵌套深度常量（`64`） |
| `NixTokenKind` | 词法记号枚举：`ident`, `string`, `indented_string` (`''...''`), `path`, `uri`, `number`, `lbrace`, `rbrace`, `lbracket`, `rbracket`, `lparen`, `rparen`, `equals`, `semicolon`, `colon`, `comma`, `dot`, `ellipsis` (`...`), `punct`, `invalid` |
| `NixToken` | 零拷贝词法记号：`kind: NixTokenKind`, `lexeme: []const u8` |
| `NixAttrBinding` | 零拷贝属性绑定：`path: []const u8`（如 `"inputs.nixpkgs.url"`）、`value: []const u8`（`=` 至 `;` 前去空白切片） |
| `NixIter` | 流式词法扫描器：`init(input)`, `next(*_): NixToken?`；自动跳过 `#` 与 `/* */` 注释，识别双引号字符串与 `${...}` 插值、`''...''` 缩进字符串、路径、URI、数字与 `...` |
| `NixAttrIter` | 属性集顶层绑定迭代器：`init(attrset_body)`, `next(*_): NixAttrBinding?`；兼容 `{ ... }`、`rec { ... }` 或裸属性集主体，自动跳过嵌套 `{}`/`[]`/`()` 内分号与 `inherit` 语句 |
| `validateNixDelimiters(input)` | 字符串/插值/注释感知校验 `{}`、`[]`、`()` 平衡性、词法闭合性与 64 层嵌套深度上限 |

## 示例

```cp
import { NixAttrIter, validateNixDelimiters } from "cap:conf";

fn scanFlake(src: []const u8): bool {
    if (!validateNixDelimiters(src)) return false;
    let it = NixAttrIter.init(src);
    let count: usize = 0;
    while (b := it.next()) {
        if (b.path.len > 0 && b.value.len > 0) count += 1;
    }
    return count > 0;
}
```
