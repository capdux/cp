# `cap:kdl` — 零分配 KDL 配置文档词法扫描、节点头迭代与结构校验器

> [!NOTE]
> 本子模块已归并至统一主包 `cap:conf`，导入请使用 `import { ... } from "cap:conf"`。

> 来源：`/data/dev/bun/src/`（纯 CP 零堆分配实现）

`cap:kdl` 提供 KDL（KDL Document Language）配置文档的零分配词法扫描、节点摘要迭代与结构合法性校验，支持嵌套块注释、`/-` slashdash 节点/参数注释、原始字符串 `r#"..."#`、带下划线多进制数字及 `\` 行续接。

## 核心特性

- **零堆分配词法器 `KdlIter`**：自动跳过 UTF-8 BOM、水平空白与 NBSP、`//` 行注释、任意层嵌套 `/* ... */` 块注释及 `\` 跨行续接；支持双引号转义字符串（含 `\u{...}`）、原始字符串 `r#"..."#`、下划线分隔多进制数值（`1_000`、`0xff`、`0o755`、`0b1010_0101`）与 `/-` slashdash 记号。
- **节点头迭代器 `KdlNodeIter`**：按文档顺序产出 `KdlNodeSummary`（节点名、位置参数数、键值对属性数、是否含子块及嵌套深度），自动跳过 `/-` 注释掉的整条节点、单个参数、单个属性或子块。
- **结构校验器 `validateKdl`**：校验全部 `{ ... }` 括号平衡、最大嵌套深度（64 层）、块注释与字符串闭合、合法属性赋值与无非法 token。

## API 参考

- `KdlTokenKind`：`ident`, `string`, `number`, `bool_lit`, `null_lit`, `equals`, `lbrace`, `rbrace`, `semicolon`, `newline`, `slashdash`, `invalid`
- `KdlToken`：`kind: KdlTokenKind`, `lexeme: []const u8`
- `KdlIter`：`init(input: []const u8): KdlIter`、`next(*_): KdlToken?`
- `KdlNodeSummary`：`name: []const u8`, `arg_count: u16`, `prop_count: u16`, `has_children: bool`, `depth: u16`
- `KdlNodeIter`：`init(input: []const u8): KdlNodeIter`、`next(*_): KdlNodeSummary?`
- `validateKdl(input: []const u8): bool`

## 使用示例

```cp
import { KdlNodeIter, validateKdl } from "cap:conf";

const doc = "package \"cp\" version=\"0.1.0\" {\n  author \"Alice\"\n  /- disabled-node 123\n}\n";
if (validateKdl(doc)) {
    let nodes = KdlNodeIter.init(doc);
    while (n := nodes.next()) {
        // n.name, n.arg_count, n.prop_count, n.has_children, n.depth
        * = n;
    }
}
```
