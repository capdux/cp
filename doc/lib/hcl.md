# `cap:hcl` — 零分配 HCL / Terraform 词法扫描、块头迭代与结构校验标准库

`cap:hcl` 提供零堆分配的 HashiCorp Configuration Language (`.hcl` / `.tf`) 词法扫描器、顶层块迭代器与括号结构平衡校验器，完整支持 `#`/`//`/`/* */` 注释、`${...}` 插值保护字符串以及 `<<EOF`/`<<-EOF` Heredoc。

## 核心 API

### 词法类型与迭代器
- `HclTokenKind`：`ident`, `string`, `heredoc`, `number`, `bool_lit`, `null_lit`, `equals`, `lbrace`, `rbrace`, `lbracket`, `rbracket`, `lparen`, `rparen`, `comma`, `dot`, `invalid`
- `HclToken`：`kind: HclTokenKind`, `lexeme: []const u8`
- `HclIter`：`init(input: []const u8)` / `next(*_): HclToken?`

### 顶层块迭代与校验
- `HclBlockHeader`：`block_type: []const u8`, `label1: ([]const u8)?`, `label2: ([]const u8)?`, `body: []const u8`
- `HclBlockIter`：`init(input: []const u8)` / `next(*_): HclBlockHeader?` — 零分配迭代顶层块（自动跳过顶层属性与嵌套块）
- `validateHcl(input: []const u8): bool` — 栈式校验 `{}`/`[]`/`()` 匹配、最大 64 层深度防护、字符串/Heredoc/块注释闭合与非空校验

## 使用示例

```cp
import { validateHcl, HclBlockIter } from "cap:hcl";

fn demo(tf: []const u8) {
    if (!validateHcl(tf)) {
        return;
    }
    let blocks = HclBlockIter.init(tf);
    while (blk := blocks.next()) {
        * = blk.block_type;
        * = blk.body;
    }
}
```
