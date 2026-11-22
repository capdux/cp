# `cap:cue` — 零分配 CUE 配置与数据约束语言词法扫描与校验标准库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:conf`，导入请使用 `import { ... } from "cap:conf"`。

`cap:cue` 提供零堆分配的 CUE 语言词法扫描器、括号平衡与深度校验器以及文件头 `package` 声明提取器，原生支持 `#Def` 定义、`_hidden` 隐藏字段、`_|_` bottom、`@attr(...)` 属性注解、SI/IEC 数字后缀以及双/单引号多行字符串。

## 核心 API

### 词法类型与迭代器
- `CueTokenKind`：`ident`, `def_ident`, `hidden_ident`, `string`, `bytes_lit`, `number`, `bool_lit`, `null_lit`, `bottom`, `lbrace`, `rbrace`, `lbracket`, `rbracket`, `lparen`, `rparen`, `colon`, `comma`, `pipe`, `amp`, `ellipsis`, `attr`, `invalid`
- `CueToken`：`kind: CueTokenKind`, `lexeme: []const u8`
- `CueIter`：`init(input: []const u8)` / `next(*_): CueToken?`

### 校验与包名提取
- `validateCue(input: []const u8): bool` — 栈式校验 `{}`/`[]`/`()` 匹配平衡、最大 64 层深度、字符串/属性闭合与无非法 token
- `extractCuePackage(input: []const u8): ([]const u8)?` — 提取文件头 `package <name>` 声明

## 使用示例

```cp
import { validateCue, extractCuePackage, CueIter } from "cap:conf";

fn demo(src: []const u8) {
    if (!validateCue(src)) {
        return;
    }
    if (pkg := extractCuePackage(src)) {
        * = pkg;
    }
    let it = CueIter.init(src);
    while (tok := it.next()) {
        * = tok.lexeme;
    }
}
```
