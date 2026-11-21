# CP 标准库：INI / Systemd / Gitconfig 无损解析与增量编辑手册 (`cap:ini`)

## 一、 架构总览与设计准则

`cap:ini` 是 CP 语言官方内置的纯 CP INI / Systemd Unit (`.service`) / `.gitconfig` 解析与增量编辑引擎，**零外部依赖、零 `std.posix`、零 `@import("bun")`**。通过保留原始行序列与注释/空行 AST 节点（`IniLineKind`），实现“修改任意键值或删除节后，未改动的 `#` / `;` 注释、空行、原始分隔符与节顺序 100% 无损回写”。既可通过 `import { ... } from "cap:ini"` 导入，也支持 `import cap from "cap"; cap.ini.*` 动态懒挂载调用。

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/ini/ini.cp` | `// Source: cp/lib/ini/ini.cp (...)` | `IniError`、`IniOptions`、`IniLineKind`、`IniLine`、`IniDoc` 解析、类型安全读取、无损编辑、回写与 JSON 导出 |
| `lib/ini/index.cp` | `// Source: cp/lib/ini/index.cp (Unified Exports for cap:ini)` | `cap:ini` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合与解析选项

- **`IniError`**：`UnterminatedSection`、`UnterminatedQuote`、`MissingEquals`、`InvalidInteger`、`InvalidBool`、`SectionNotFound`、`KeyNotFound`、`OutOfMemory`。
- **`IniOptions`**：
  - `allow_duplicate_keys: bool = true`：允许同节内同名多值键（如 Systemd `Environment=` / `ExecStart=`）；为 `false` 时后者覆盖前者；
  - `case_sensitive_sections: bool = true`：是否区分节名大小写（设为 `false` 兼容大小写不敏感的传统 INI）；
  - `allow_hash_comments: bool = true`：是否启用 `#` 注释；
  - `allow_semicolon_comments: bool = true`：是否启用 `;` 注释。
- **`IniLineKind`**：`Blank`、`Comment`、`SectionHeader`、`KeyValue`。

### 2. 文档解析与生命周期（`IniDoc`）

- **`IniDoc.init(alloc): IniDoc`**：创建空文档；
- **`IniDoc.parse(alloc, text): IniDoc!`** / **`IniDoc.parseWithOptions(alloc, text, opts): IniDoc!`**：
  - 支持 UTF-8 BOM 自动剥离、LF/CRLF；
  - 支持无节名全局键值（节名为 `""`）、`[section]` 与 Gitconfig 风格 `[remote "origin"]`；
  - 支持 `=` 或 `:` 分隔符、单引号 `'...'` 与双引号 `"..."` 值（引号内 `#` / `;` 保留字面值不截断，双引号支持 `\n`/`\r`/`\t`/`\\`/`\"` 转义）；
  - 支持行尾 `\` 多行续行拼接、全行注释与行内注释；
  - 严格拦截未闭合 `[sec`（`UnterminatedSection`）、缺失分隔符（`MissingEquals`）及未闭合引号（`UnterminatedQuote`）；
- **`doc.deinit()`**：确定性释放全部行节点与字符串切片内存。

### 3. 类型安全读取 API

- `hasSection(sec): bool` / `hasKey(sec, key): bool`
- `get(sec, key): ([]const u8)?`：返回最后一次出现的键值；
- `getAll(alloc, sec, key): ([][]const u8)!`：返回同节同键全部值切片（调用方仅需 `alloc.free(res)` 释放外层切片）；
- `getInt(sec, key): i64!`（支持十进制/`0x`/`0o`/`0b`）、`getFloat(sec, key): f64!`、`getBool(sec, key): bool!`（识别 `true/yes/on/1` 与 `false/no/off/0`，大小写无关）；
- `sections(alloc): ([][]const u8)!` / `keys(alloc, sec): ([][]const u8)!`。

### 4. 无损增量编辑与序列化

- `set(sec, key, val): !`：原地更新已有键（标记为 `dirty`），若键不存在则插入对应节末尾，若节不存在则自动追加 `[sec]` 与键值；
- `removeKey(sec, key): bool` / `removeSection(sec): bool`；
- `stringify(): ([]u8)!`：未改动行按原始文本无损输出，已修改/新增行规范格式化输出（调用方需 `alloc.free`）；
- `toJson(): ([]u8)!`：导出为严格 RFC 8259 JSON 对象（多值同键自动导出为 JSON 数组），可与 `cap:json` 无缝联动。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { IniDoc } from "cap:ini";
import cap from "cap";

pub fn demoIni(): ! {
    const alloc = std.heap.page_allocator;
    const raw =
        "# Global comment\n" ++
        "[Service]\n" ++
        "Environment=FOO=1\n" ++
        "Environment=BAR=2\n";

    let doc = IniDoc.parse(alloc, raw)?;
    defer doc.deinit();

    const envs = doc.getAll(alloc, "Service", "Environment")?;
    defer alloc.free(envs);

    doc.set("Service", "Restart", "always")?;
    const out = doc.stringify()?;
    defer alloc.free(out);

    let doc2 = cap.ini.IniDoc.parse(alloc, out)?;
    defer doc2.deinit();
}
```

---

## 附加零分配子模块

- **`lib/ini/writer.cp`**（INI 安全写入与转义）：`isValidIniKey`、`needsIniQuotes`、`appendIniSection`、`appendIniSubsection`、`appendIniPair`、`appendIniComment`
- **`lib/ini/properties.cp`**（Java `.properties` 零分配流式解析）：`PropertyEntry`、`PropertiesIter`、`getProperty`
