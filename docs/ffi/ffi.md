# `cap:ffi` — 动态库加载、TinyCC 内存编译绑定与 C-ABI 结构体对齐推演标准库

> **上游溯源**: `/data/dev/bun/src/runtime/ffi/` (`ffi.cp`, `FFIObject.cp`) & `/data/dev/bun/src/tcc_sys/tcc.cp`  
> **模块路径**: `lib/ffi/`  
> **导入协议**: `import { FFIType, DlFlags, FFISignature, DynLib, CStructField, CStructLayout, TCCState, TccOutputFormat } from "cap:ffi";`

---

## 一、 架构概述与设计亮点

`cap:ffi` 吸收自 Bun 的 `bun:ffi` 运行时与 TinyCC 内存编译器绑定（`bun/src/runtime/ffi` 与 `bun/src/tcc_sys`），提供三大核心能力：

1. **FFI 类型系统与函数签名描述符 (`FFIType` & `FFISignature`)**：
   - 覆盖 C ABI 全套基础标量与指针类型（`Void`, `Bool`, `I8`, `U8`, `I16`, `U16`, `I32`, `U32`, `I64`, `U64`, `F32`, `F64`, `Ptr`, `CString`），精确计算每种类型的字节宽度（`FFIType.byteSize`）与浮点寄存器分类（`FFIType.isFloatingPoint`）。
   - `FFISignature` 支持最多 16 个参数的签名编排与调用栈总字节数统计（`totalArgBytes`）。
2. **C-ABI 结构体自然对齐与偏移推演引擎 (`CStructLayout`)**：
   - 在跨语言调用 C 动态库时，精确模拟 C 编译器（System V AMD64 ABI / AAPCS64）的结构体字段自然对齐规则：
     - 每个字段的起始偏移自动向上对齐至其自身对齐要求的整数倍：$\text{offset}_i = \text{alignUp}(\text{current\_offset}, \text{align}_i)$。
     - 结构体的最大对齐边界 $\text{max\_align} = \max_i(\text{align}_i)$。
     - 调用 `finalize()` 时，自动在末尾补齐尾随填充（Trailing Padding），使结构体总大小 `total_size` 成为 `max_align` 的整数倍，确保在 C 结构体数组中步进（Array Stride）100% 内存对齐。
3. **动态链接库管理 (`DynLib`) 与 TinyCC 内存 C 编译器声明 (`_tcc.cp`)**：
   - 提供 `DynLib` 与 `DlFlags`（`RTLD_LAZY`, `RTLD_NOW`, `RTLD_GLOBAL`, `RTLD_LOCAL`）生命周期封装。
   - 在 `_tcc.cp` 中通过原生 `pub @c { ... }` 块声明了 `libtcc` 全套 C-FFI 接口（`tcc_new`, `tcc_compile_string`, `tcc_relocate`, `tcc_get_symbol` 等），支持运行时 JIT 编译内联 C 代码。

---

## 二、 `FFIType` 类型矩阵与 `bun:ffi` 对照表

| `cap:ffi` `FFIType` | `bun:ffi` `FFIType` 对照 | C 语言类型对应 | 字节大小 (`byteSize`) | 自然对齐 (`typeAlign`) |
| :--- | :--- | :--- | :---: | :---: |
| `FFIType.Void` | `FFIType.void` | `void` | 0 | 1 |
| `FFIType.Bool` | `FFIType.bool` | `bool` / `_Bool` | 1 | 1 |
| `FFIType.I8` / `U8` | `FFIType.i8` / `u8` | `int8_t` / `uint8_t` | 1 | 1 |
| `FFIType.I16` / `U16` | `FFIType.i16` / `u16` | `int16_t` / `uint16_t` | 2 | 2 |
| `FFIType.I32` / `U32` | `FFIType.i32` / `u32` | `int32_t` / `uint32_t` | 4 | 4 |
| `FFIType.I64` / `U64` | `FFIType.i64` / `u64` | `int64_t` / `uint64_t` | 8 | 8 |
| `FFIType.F32` | `FFIType.f32` | `float` | 4 | 4 |
| `FFIType.F64` | `FFIType.f64` | `double` | 8 | 8 |
| `FFIType.Ptr` | `FFIType.ptr` | `void*` | 8 | 8 |
| `FFIType.CString` | `FFIType.cstring` | `const char*` | 8 | 8 |

---

## 三、 代码示例

### 1. `CStructLayout` C-ABI 结构体内存对齐与偏移推演

假设 C 头文件定义了如下结构体：
```c
struct PacketHeader {
    uint8_t  tag;    // 1B, offset 0 (+ 3B padding)
    uint32_t id;     // 4B, offset 4
    double   score;  // 8B, offset 8
    uint8_t  flags;  // 1B, offset 16 (+ 7B trailing padding -> total 24B)
};
```
使用 `CStructLayout` 自动计算每个字段的内存偏移与数组步长：

```typescript
import { FFIType, CStructLayout } from "cap:ffi";

fn demoStructLayout(): void {
    let layout = CStructLayout.init();

    const off_tag   = layout.addField(FFIType.U8);  // 0
    const off_id    = layout.addField(FFIType.U32); // 4
    const off_score = layout.addField(FFIType.F64); // 8
    const off_flags = layout.addField(FFIType.U8);  // 16

    const total_bytes = layout.finalize();
    // total_bytes => 24
    // layout.max_align => 8
    // layout.fieldOffset(2).? => 8
}
```

### 2. `FFISignature` 函数签名构造与 `DynLib` 动态库管理

```typescript
import { FFIType, FFISignature, DynLib, DlFlags } from "cap:ffi";

fn demoSignatureAndDynLib(): void {
    // 构造 int32_t write_record(void* ctx, uint64_t id, double val) 的 FFI 签名
    let sig = FFISignature.init(FFIType.I32);
    * = sig.addArg(FFIType.Ptr);
    * = sig.addArg(FFIType.U64);
    * = sig.addArg(FFIType.F64);
    // sig.arg_count => 3
    // sig.totalArgBytes() => 24

    // 打开与关闭动态链接库句柄
    let lib = DynLib.open("libcrypto.so", DlFlags.RTLD_LAZY);
    // lib.is_open => true
    lib.close();
    // lib.is_open => false
}
```
