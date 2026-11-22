# `cap:wat` — 零分配 WebAssembly Text (`.wat`) 词法器、导出提取器与结构校验器

> [!NOTE]
> 本子模块已归并至统一主包 `cap:compiler`，导入请使用 `import { ... } from "cap:compiler"`。

> 来源：`/data/dev/bun/src/`（纯 CP 零堆分配实现）

`cap:wat` 提供 WebAssembly 文本格式（`.wat`）的零分配词法扫描、模块导出声明提取与 S-表达式结构校验能力。

## 核心特性

- **零分配词法器 `WatIter`**：自动跳过 UTF-8 BOM、`;;` 单行注释与任意层嵌套 `(; ... ;)` 块注释，识别 `(` / `)`、`$ident`、双引号转义字符串（含 `\HH` 字节转义与 `\u{...}`）、十进制/十六进制/浮点/下划线数字（含 `inf`/`nan`/`nan:0x...`）与 WAT 指令关键字。
- **导出声明提取器 `WatExportIter`**：同时支持顶层 `(export "name" (func $id))` 独立导出与内联 `(func $id (export "name") ...)` / `(memory (export "mem") 1)` 导出，并自动维护匿名实体的默认索引编号。
- **结构校验器 `validateWatStructure`**：校验顶层 `(module ...)` 结构、全部括号平衡、嵌套块注释与字符串闭合，并具备 64 层最大嵌套深度防护。

## API 参考

- `WatTokenKind`：`lparen`, `rparen`, `keyword`, `id`, `string`, `number`, `invalid`
- `WatToken`：`kind: WatTokenKind`, `lexeme: []const u8`
- `WatIter`：`init(input: []const u8): WatIter`、`next(*_): WatToken?`
- `WatExport`：`name: []const u8`, `kind: []const u8`, `target: []const u8`
- `WatExportIter`：`init(input: []const u8): WatExportIter`、`next(*_): WatExport?`
- `validateWatStructure(input: []const u8): bool`

## 使用示例

```cp
import { WatExportIter, validateWatStructure } from "cap:compiler";

const wat = "(module (func $add (export \"add\") (param i32 i32) (result i32) (i32.add (local.get 0) (local.get 1))))";
if (validateWatStructure(wat)) {
    let it = WatExportIter.init(wat);
    while (ex := it.next()) {
        // ex.name == "add", ex.kind == "func", ex.target == "$add"
        * = ex;
    }
}
```
