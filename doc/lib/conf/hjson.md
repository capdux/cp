# `cap:hjson` — 零分配 Hjson (Human JSON) 解析器、注释剥离器与标准 JSON 转换器

> [!NOTE]
> 本子模块已归并至统一主包 `cap:conf`，导入请使用 `import { ... } from "cap:conf"`。

> 来源：`/data/dev/bun/src/`（纯 CP 零堆分配实现）

`cap:hjson` 提供人类可读的 Hjson 配置文件零分配解析、字符串感知注释剥离及严格 RFC 8259 JSON 转换能力。

## 核心特性

- **三种注释**：支持 `#` 行注释、`//` 行注释与 `/* ... */` 块注释，并在 `stripHjsonComments` 中完整保护 `"..."`、`'...'`、`'''...'''` 字符串字面量。
- **可选逗号与尾随逗号**：支持换行分隔的对象键值对和数组元素，以及末尾尾随逗号。
- **无引号键名与无引号单行值**：自动识别 `true`/`false`/`null`/严格 JSON 数值，其余裸行自动作为无引号字符串并规范转义输出。
- **三单引号多行字符串 `'''...'''`**：自动根据起始列对齐剥离公共缩进，转换为标准 JSON 转义字符串。
- **零分配安全防护**：全部转换在调用方 `[]u8` 缓冲区内完成，具备缓冲溢出检查与 64 层最大嵌套深度防护。

## API 参考

- `HjsonTokenKind`：`lbrace`, `rbrace`, `lbracket`, `rbracket`, `colon`, `comma`, `string`, `quoteless_string`, `number`, `bool_lit`, `null_lit`, `key`, `invalid`
- `HjsonToken`：`kind: HjsonTokenKind`, `lexeme: []const u8`
- `hjsonToJson(input: []const u8, out: []u8): ([]const u8)?`
- `isValidHjson(input: []const u8): bool`
- `stripHjsonComments(input: []const u8, out: []u8): ([]const u8)?`

## 使用示例

```cp
import { hjsonToJson, isValidHjson, stripHjsonComments } from "cap:conf";

const src = "{\n  # config\n  name: cp-lang\n  port: 8080\n  tags: [\n    fast\n    safe\n  ]\n}";
let buf: [256]u8 = [0; 256];
if (json := hjsonToJson(src, buf[0..])) {
    // {"name":"cp-lang","port":8080,"tags":["fast","safe"]}
    * = json;
}
```
