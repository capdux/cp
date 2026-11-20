# `cap:ffi` — POSIX 动态库加载、TinyCC 内存编译与 C-ABI 布局计算标准库

> **上游溯源**: `/data/dev/bun/src/runtime/ffi/ffi.cp`, `/data/dev/bun/src/runtime/ffi/FFIObject.cp`  
> **模块路径**: `lib/ffi/`  
> **导入协议**: `import { FFIType, DlFlags, FFISignature, FfiCallbackDesc, DynLib, CString, CStructField, CStructLayout, TCCState, TccOutputFormat } from "cap:ffi";`

---

## 一、 架构概述与核心能力

`cap:ffi` 对标 Bun 的 `bun:ffi` 运行时与 TinyCC 内存编译器绑定，提供四大工业级基础能力：

1. **完整 16 种 C-ABI 类型矩阵 (`FFIType`) 与签名/回调描述符 (`FFISignature`, `FfiCallbackDesc`)**：
   - 覆盖 `Void`, `Bool`, `I8`, `U8`, `I16`, `U16`, `I32`, `U32`, `I64`, `U64`, `F32`, `F64`, `Ptr`, `CString`, `Buffer`, `Function`。
   - 提供 `sizeOf`（兼容 `byteSize`）、`alignOf`、`isInteger`、`isFloat`（兼容 `isFloatingPoint`）、`isSigned`、`isUnsigned`、`isPointer`、`typeName` 与 `fromName`。
2. **真实 POSIX 动态链接库加载 (`DynLib`)**：
   - 通过 `extern "c"` 直通 libc/libdl 的 `dlopen` / `dlsym` / `dlclose` / `dlerror`。
   - 支持打开当前进程全局符号表（`DynLib.openSelf(DlFlags.RTLD_LAZY)`，等价 `dlopen(null, RTLD_LAZY)`）或指定共享库（如 `"libc.so.6"`），直接解析真实 C 函数地址并调用。
3. **C-ABI 结构体自然对齐与布局推演引擎 (`CStructLayout`)**：
   - 支持任意数量字段的偏移、对齐与填充计算（前 64 个字段保留完整元数据缓存）；
   - 支持 `#packed` 紧密布局（`CStructLayout.initPacked()`，无对齐填充与尾填充）；
   - 支持嵌套子结构体（`addStruct` / `addNamedStruct`）、自定义字节块（`addBytes`）、命名字段查询（`fieldByName` / `offsetOf`）及填充统计（`paddingBytes` / `trailingPadding`）。
4. **零拷贝 `CString` 辅助 & TinyCC 声明 (`_tcc.cp`)**：
   - `CString` 提供 `fromPtr` / `fromOptionalPtr` / `fromRawPtr` / `fromSlice` 与零拷贝 `asSlice` / `len` / `eql`。

---

## 二、 `FFIType` 完整 C-ABI 类型矩阵

| `cap:ffi` 类型 | `bun:ffi` 对照 | `sizeOf` | `alignOf` | `isInteger` | `isFloat` | `isSigned` | `isPointer` |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| `FFIType.Void` | `void` | 0 | 1 | `false` | `false` | `false` | `false` |
| `FFIType.Bool` | `bool` | 1 | 1 | `false` | `false` | `false` | `false` |
| `FFIType.I8` / `U8` | `i8` / `u8` | 1 | 1 | `true` | `false` | `true` / `false` | `false` |
| `FFIType.I16` / `U16` | `i16` / `u16` | 2 | 2 | `true` | `false` | `true` / `false` | `false` |
| `FFIType.I32` / `U32` | `i32` / `u32` | 4 | 4 | `true` | `false` | `true` / `false` | `false` |
| `FFIType.I64` / `U64` | `i64` / `u64` | 8 | 8 | `true` | `false` | `true` / `false` | `false` |
| `FFIType.F32` / `F64` | `f32` / `f64` | 4 / 8 | 4 / 8 | `false` | `true` | `false` | `false` |
| `FFIType.Ptr` | `ptr` | 8 | 8 | `false` | `false` | `false` | `true` |
| `FFIType.CString` | `cstring` | 8 | 8 | `false` | `false` | `false` | `true` |
| `FFIType.Buffer` | `buffer` | 8 | 8 | `false` | `false` | `false` | `true` |
| `FFIType.Function` | `function` | 8 | 8 | `false` | `false` | `false` | `true` |

---

## 三、 实战代码示例

### 1. 真实 POSIX `dlopen` / `dlsym` 解析并调用 `libc` 符号

```typescript
import { DynLib, DlFlags } from "cap:ffi";

fn demoPosixDlopen() {
    let lib = DynLib.openSelf(DlFlags.RTLD_LAZY);
    defer lib.close();

    if (sym := lib.symbol("strlen")) {
        unsafe {
            type StrlenFn = @c ([*:0]const u8): usize;
            const c_strlen: StrlenFn = @ptrCast(@alignCast(sym));
            const len = c_strlen("hello cp"); // 8
        }
    }
}
```

### 2. `CStructLayout`：标准对齐、`#packed` 与嵌套子结构体

```typescript
import { FFIType, CStructLayout } from "cap:ffi";

fn demoNestedAndPacked() {
    let inner = CStructLayout.init();
    * = inner.addNamedField("tag", FFIType.U8);  // offset 0
    * = inner.addNamedField("val", FFIType.U64); // offset 8
    * = inner.finalize();                        // size 16, align 8

    let outer = CStructLayout.init();
    * = outer.addNamedField("head", FFIType.U8);      // offset 0
    * = outer.addNamedStruct("inner", &inner);        // offset 8
    * = outer.addNamedField("tail", FFIType.U8);      // offset 24
    const total = outer.finalize();                   // size 32, trailingPadding 7

    let packed = CStructLayout.initPacked();
    * = packed.addField(FFIType.U8);  // 0
    * = packed.addField(FFIType.U32); // 1
    * = packed.addField(FFIType.F64); // 5
    const packed_total = packed.finalize(); // 13, paddingBytes == 0
}
```
