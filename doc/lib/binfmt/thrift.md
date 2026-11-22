# `cap:thrift` — 零分配 Apache Thrift (`.thrift`) IDL 词法扫描、声明提取与括号校验器

> [!NOTE]
> 本子模块已归并至统一主包 `cap:binfmt`，导入请使用 `import { ... } from "cap:binfmt"`。

> 来源：`/data/dev/bun/src/`（纯 CP 零堆分配实现）

`cap:thrift` 提供 Apache Thrift IDL 文件的零分配顶层声明迭代、结构体字段提取（支持嵌套泛型容器类型）与字符串/注释感知括号平衡校验能力。

## 核心特性

- **顶层声明迭代器 `ThriftDeclIter`**：自动跳过 UTF-8 BOM 与 `#` / `//` / `/* */` 三种注释，支持提取 `namespace`、`include`、`typedef`、`const`、`enum`、`struct`、`union`、`exception` 与 `service [extends Base]` 声明及其 `{ ... }` 主体切片。
- **结构体字段迭代器 `ThriftFieldIter`**：解析 `field_id`（可选整数编号）、`required`/`optional` 修饰符、嵌套泛型类型（如 `map<string, list<i64>>`）、字段名、默认值及注解。
- **结构校验器 `validateThriftBraces`**：字符串与注释感知校验 `{}`、`()`、`<>`、`[]` 严格匹配平衡，具备 64 层最大嵌套深度防护。

## API 参考

- `ThriftDeclKind`：`namespace_decl`, `include_decl`, `typedef_decl`, `enum_decl`, `struct_decl`, `union_decl`, `exception_decl`, `service_decl`, `const_decl`
- `ThriftDecl`：`kind: ThriftDeclKind`, `name: []const u8`, `extends_name: ([]const u8)?`, `body: ([]const u8)?`
- `ThriftField`：`field_id: i32?`, `is_required: bool`, `is_optional: bool`, `ty: []const u8`, `name: []const u8`
- `ThriftDeclIter`：`init(input: []const u8): ThriftDeclIter`、`next(*_): ThriftDecl?`
- `ThriftFieldIter`：`init(struct_body: []const u8): ThriftFieldIter`、`next(*_): ThriftField?`
- `validateThriftBraces(input: []const u8): bool`

## 使用示例

```cp
import { ThriftDeclIter, ThriftFieldIter, validateThriftBraces } from "cap:binfmt";

const idl = "struct User { 1: required i64 id, 2: optional map<string, list<i32>> attrs; }";
if (validateThriftBraces(idl)) {
    let decls = ThriftDeclIter.init(idl);
    if (d := decls.next()) {
        if (body := d.body) {
            let fields = ThriftFieldIter.init(body);
            while (f := fields.next()) {
                * = f;
            }
        }
    }
}
```
