# CP 标准库：WebAssembly 1.0 二进制模块解析与构造引擎 (`cap:wasm`)

## 一、 架构总览与设计准则

`cap:wasm` 是 CP 语言官方内置的 **WebAssembly 1.0 二进制模块解析、校验与构造标准库**，专为沙盒插件内省、WASM 工具模块验证与二进制编码打造。**零外部依赖、零 `std.posix`、零 `@import("bun")`**，既支持 `import { ... } from "cap:wasm"`，也支持 `import cap from "cap"; cap.wasm.*` 动态懒挂载调用。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│             CP WebAssembly 1.0 二进制引擎 (cap:wasm)                     │
├───────────────────────┬─────────────────────────┬───────────────────────┤
│ LEB128 整数编解码      │ WasmModule 解析校验器    │ WasmBuilder 模块构造器 │
│ Leb128 (u32/u64/i32/64)│ 魔数/版本/节序严格验证  │ Type/Import/Func/Mem  │
│ 5B/10B 溢出与截断防护  │ ArenaAllocator O(1)释放 │ Export/Start/Code/Cust│
└───────────────────────┴─────────────────────────┴───────────────────────┘
```

### 模块文件拓扑

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/wasm/leb128.cp` | `// Source: cp/lib/wasm/leb128.cp (WebAssembly Unsigned & Signed LEB128 Codec)` | `WasmError` 错误集与 `Leb128` 无符号/有符号 32/64 位编解码（严格 5B/10B 溢出与符号扩展检查） |
| `lib/wasm/module.cp` | `// Source: cp/lib/wasm/module.cp (WebAssembly 1.0 Binary Module Parser & Validator)` | `WasmValType`、`WasmExternKind`、`WasmModule.parse` 全节解析与 `findExport` / `findCustomSection` |
| `lib/wasm/builder.cp` | `// Source: cp/lib/wasm/builder.cp (Minimal WebAssembly 1.0 Binary Module Builder)` | `WasmBuilder` 增量式 `.wasm` 二进制生成引擎（100% 可被 `WasmModule.parse` 无损读回） |
| `lib/wasm/index.cp` | `// Source: cp/lib/wasm/index.cp (Unified Exports for cap:wasm)` | `cap:wasm` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与 LEB128 编解码器（`leb128.cp`）

- **`WasmError`**：`InvalidMagic`、`UnsupportedVersion`、`InvalidSectionOrder`、`UnexpectedEof`、`Leb128Overflow`、`InvalidValType`、`InvalidExportKind`、`MalformedSection`、`OutOfMemory`。
- **`Leb128`**：
  - 编码：`encodeU32(alloc, value)`、`encodeU64(alloc, value)`、`encodeI32(alloc, value)`、`encodeI64(alloc, value)`（返回调用方所有的 `[]u8`），以及零中间分配写入器 `writeU32`/`writeU64`/`writeI32`/`writeI64`；
  - 解码：`decodeU32(bytes, &pos)`、`decodeU64(bytes, &pos)`、`decodeI32(bytes, &pos)`、`decodeI64(bytes, &pos)`，严格校验 5 字节（32 位）/ 10 字节（64 位）溢出、末字节高位未清零或符号扩展位错误（`Leb128Overflow`）以及截断边界（`UnexpectedEof`）。

### 2. WebAssembly 1.0 模块解析与内省（`module.cp`）

- **`WasmValType`**：`I32 = 0x7F`、`I64 = 0x7E`、`F32 = 0x7D`、`F64 = 0x7C`、`FuncRef = 0x70`、`ExternRef = 0x6F`。
- **`WasmExternKind`**：`Func = 0`、`Table = 1`、`Memory = 2`、`Global = 3`。
- **`WasmLimits`** / **`WasmFuncType`** / **`WasmImport`** / **`WasmExport`** / **`WasmCustomSection`** / **`WasmCodeBody`**：完整建模函数签名、导入表、线性内存限制、导出表、自定义段与解码后的函数局部变量/指令字节。
- **`WasmModule`**：
  - `WasmModule.parse(alloc, bytes): WasmModule!`：校验 `\0asm` 魔数与版本 `1`，强制已知节 ID（`1..12`）严格单调递增（`Custom(0)` 可出现在任意位置），完整解析 `Type(1)`/`Import(2)`/`Function(3)`/`Memory(5)`/`Export(7)`/`Start(8)`/`Code(10)`/`Custom(0)`；
  - `deinit(*_)`：通过内部 `ArenaAllocator` O(1) 一次性释放全部解码切片与元数据；
  - `findExport(name): WasmExport?` / `findCustomSection(name): WasmCustomSection?` / `getFuncType(func_idx): WasmFuncType?` / `getCodeBody(func_idx): WasmCodeBody?`。

### 3. 二进制模块构造器（`builder.cp`）

- **`WasmBuilder`**：
  - `init(alloc)` / `deinit(*_)`：内部 Arena 托管全部增量添加的签名、名称与字节码；
  - `addType(params, results): u32!`：追加函数签名并返回类型索引；
  - `addImportFunc(module, name, type_idx): u32!`：追加导入函数（按 Wasm 索引空间约定，请在 `addFunction` 之前调用）；
  - `addFunction(type_idx, body_bytes): u32!` / `addFunctionWithLocals(type_idx, locals, body_bytes): u32!`：追加本地函数并返回全局函数索引；
  - `addMemory(min_pages, max_pages): u32!`、`addExport(name, kind, index)`、`setStart(func_idx)`、`addCustomSection(name, data)`；
  - `build(alloc): ([]u8)!`：按标准节序输出完整 `.wasm` 二进制缓冲区（调用方负责 `alloc.free`）。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { WasmValType, WasmExternKind, WasmBuilder, WasmModule } from "cap:wasm";
import cap from "cap";

pub fn buildAndInspectAddModule()! {
    const alloc = std.heap.page_allocator;

    let b = WasmBuilder.init(alloc);
    defer b.deinit();

    const params: [2]WasmValType = [.I32, .I32];
    const results: [1]WasmValType = [.I32];
    const t_add = b.addType(&params, &results)?;

    // local.get 0; local.get 1; i32.add; end
    const add_idx = b.addFunction(t_add, "\x20\x00\x20\x01\x6A\x0B")?;
    b.addExport("add", .Func, add_idx)?;
    b.addCustomSection("name", "add_module")?;

    const wasm_bytes = b.build(alloc)?;
    defer alloc.free(wasm_bytes);

    let mod = cap.wasm.WasmModule.parse(alloc, wasm_bytes)?;
    defer mod.deinit();

    const exp = mod.findExport("add").?;
    const body = mod.getCodeBody(exp.index).?;
    * = body.code;
}
```
