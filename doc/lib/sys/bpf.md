# `cap:bpf` — RFC 9669 eBPF 指令集零分配编解码与程序校验库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:sys`，导入请使用 `import { ... } from "cap:sys"`。

纯 CP、零堆分配的 RFC 9669 eBPF 指令集（64-bit 标准指令与 128-bit `LD_IMM64` 宽指令）编解码、流式反汇编迭代与程序结构校验库。

## 导入

```cp
import {
    BPF_INSN_SIZE,
    BPF_WIDE_INSN_SIZE,
    BPF_MAX_REG,
    BPF_LD,
    BPF_LDX,
    BPF_ST,
    BPF_STX,
    BPF_ALU,
    BPF_JMP,
    BPF_JMP32,
    BPF_ALU64,
    BPF_K,
    BPF_X,
    BPF_W,
    BPF_H,
    BPF_B,
    BPF_DW,
    BPF_IMM,
    BPF_MEM,
    BPF_ATOMIC,
    ALU_ADD,
    ALU_SUB,
    ALU_MOV,
    JMP_JEQ,
    JMP_CALL,
    JMP_EXIT,
    OP_LD_IMM64,
    OP_CALL,
    OP_EXIT,
    BpfInsn,
    decodeBpfInsn,
    encodeBpfInsn,
    encodeLdImm64,
    insnMov64Imm,
    insnMov64Reg,
    insnAlu64Reg,
    insnCall,
    insnExit,
    BpfProgIter,
    validateBpfProgram,
} from "cap:sys";
```

## 核心 API

- **指令编解码**：
  - `decodeBpfInsn(bytes8: []const u8): BpfInsn?`
  - `encodeBpfInsn(opcode: u8, dst_reg: u8, src_reg: u8, offset: i16, imm: i32, out: []u8): usize?`
  - `encodeLdImm64(dst_reg: u8, src_reg: u8, imm64: u64, out: []u8): usize?`（输出标准 16B 宽指令）
- **指令模型与便捷构造器**：
  - `BpfInsn` — 附 `insnClass()`, `aluOp()`, `jmpOp()`, `isRegSource()`, `isExit()`, `isCall()`, `isLdImm64()`, `isValidRegs()`, `encode(out)`
  - `insnMov64Imm(dst, imm)`、`insnMov64Reg(dst, src)`、`insnAlu64Reg(op, dst, src)`、`insnCall(helper_id)`、`insnExit()`
- **程序流式迭代与校验**：
  - `BpfProgIter.init(code)` / `next(*_)` / `hasError()` — 自动合并 `OP_LD_IMM64 (0x18)` 16B 宽指令并按 `pc + 2` 步进。
  - `validateBpfProgram(code: []const u8): bool` — 校验非空、8B 对齐、所有寄存器 `<= 10`、宽指令完整且含 `EXIT` 指令。

## 示例

```cp
import { insnMov64Imm, insnExit, validateBpfProgram } from "cap:sys";

fn buildXdpPass(buf: []u8): bool {
    * = insnMov64Imm(0, 2).encode(buf[0..8]); // r0 = XDP_PASS (2)
    * = insnExit().encode(buf[8..16]);       // exit
    return validateBpfProgram(buf[0..16]);
}
```
