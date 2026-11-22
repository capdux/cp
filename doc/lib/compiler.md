# CP 标准库：语言前端、词法解析、DSL、着色器、IDL 与虚拟机套件手册 (`cap:compiler`)

> 来源：Pure-CP 零 C 依赖编译器前端、表达式引擎、SAT 求解器、着色器/字节码与 IDL 协议栈收敛（16 个子模块统一聚合） | 创建日期：2026-11-22

## 一、 架构总览

`cap:compiler` 是 CP 语言官方内置的纯原生、零外部 C 依赖语言前端与虚拟机标准库，统一覆盖 **安全表达式求值、DPLL 布尔可满足性 (SAT) 求解、Anti-ReDoS Pike VM 正则引擎、S-Expression / WAT / WGSL / GLSL / SPIR-V / WebIDL 词法与字节码扫描、Cypher / SPARQL 图查询语言前端、Nginx / Caddyfile DSL 词法分析以及 Protobuf / FlatBuffers / FlexBuffers 二进制模式解析**。

---

## 二、 模块文件拓扑与核心 API 索引

### 1. 表达式引擎、SAT 求解器与正则虚拟机（3 个子模块）

| 子模块文件 | 领域分类 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/compiler/expr.cp` | 安全确定性表达式编译与求值 | `compile`, `compileWithMaxDepth`, `eval`, `CompiledExpr`, `ExprContext`, `ExprValue`, `ExprError`, `DEFAULT_MAX_DEPTH` |
| `lib/compiler/sat.cp` | DPLL 布尔可满足性求解器与 DIMACS CNF | `SatSolver`, `ConstraintModel`, `SatSolution`, `NamedSolution`, `SatStatus`, `SatError` |
| `lib/compiler/regexCompiler.cp` / `regexVm.cp` | Anti-ReDoS Pike VM / Thompson NFA 正则引擎 | `Regex`, `Compiler`, `Program`, `Inst`, `Opcode`, `Captures`, `MatchSpan`, `RegexError` |

### 2. 语言前端、WebAssembly、着色器与 IDL（6 个子模块）

| 子模块文件 | 领域分类 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/compiler/sexp.cp` | S-Expression (Lisp) 词法与格式化器 | `SexpIter`, `validateSexp`, `maxSexpDepth`, `formatSexpCompact`, `SexpToken`, `SexpTokenKind` |
| `lib/compiler/wat.cp` | WebAssembly Text (WAT) 词法与导出扫描器 | `WatIter`, `WatExportIter`, `validateWatStructure`, `WatToken`, `WatExport` |
| `lib/compiler/wgsl.cp` | WebGPU WGSL 词法、入口点与绑定扫描器 | `WgslIter`, `WgslEntryPointIter`, `WgslBindingIter`, `validateWgslDelimiters`, `WgslStage`, `WgslEntryPoint` |
| `lib/compiler/glsl.cp` | GLSL 着色器词法与声明扫描器 | `GlslIter`, `GlslDeclIter`, `parseGlslVersion`, `validateGlslBraces`, `GlslVersion`, `GlslDecl` |
| `lib/compiler/spv.cp` | Khronos SPIR-V 二进制着色器解析器 | `isSpv`, `parseSpvHeader`, `SpvInstIter`, `parseSpvEntryPoint`, `summarizeSpv`, `SpvHeader`, `SpvInst` |
| `lib/compiler/idl.cp` | W3C WebIDL 词法与声明扫描器 | `WebIdlIter`, `WebIdlDeclIter`, `validateWebIdlBraces`, `WebIdlDecl`, `WebIdlToken` |

### 3. 查询语言与配置 DSL 词法分析器（4 个子模块）

| 子模块文件 | 领域分类 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/compiler/cypher.cp` | openCypher 图查询语言词法与只读检测 | `CypherIter`, `CypherParamIter`, `classifyCypherKeyword`, `validateCypherDelimiters`, `isReadOnlyCypher` |
| `lib/compiler/sparql.cp` | W3C SPARQL 1.1 词法与前缀扫描 | `SparqlIter`, `SparqlPrefixIter`, `detectSparqlQueryForm`, `isReadOnlySparql`, `validateSparqlDelimiters` |
| `lib/compiler/nginx.cp` | Nginx 配置 DSL 词法与块扫描器 | `NginxIter`, `NginxBlockIter`, `findNginxDirective`, `validateNginxConfig`, `NginxDirective` |
| `lib/compiler/caddy.cp` | Caddyfile DSL 词法与站点块扫描器 | `CaddyIter`, `CaddySiteIter`, `validateCaddyfile`, `CaddySiteBlock` |

### 4. IDL 线格式与二进制模式解析（3 个子模块）

| 子模块文件 | 领域分类 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/compiler/protobuf.cp` / `protobufGrpc.cp` | Protocol Buffers Wire & gRPC Framing | `ProtoWriter`, `ProtoReader`, `ProtoField`, `WireType`, `PROTO_DEFAULT_MAX_DEPTH`, `GrpcFrameCodec`, `GrpcFrameStream`, `GrpcStatus` |
| `lib/compiler/flatbuf.cp` | Google FlatBuffers 零分配二进制解析 | `fbRootTable`, `fbFileIdentifier`, `fbHasIdentifier`, `fbStripSizePrefix`, `FbTable`, `FbVector` |
| `lib/compiler/flexbuf.cp` | Google FlexBuffers 无模式二进制解析 | `parseFlexRoot`, `FlexRef`, `FlexType`, `unpackFlexType`, `packFlexType` |

---

## 三、 使用示例

```typescript
import {
    validateSexp,
    validateWatStructure,
    isReadOnlyCypher,
    isSpv,
} from "cap:compiler";

pub fn demoCompiler(buf: []const u8) {
    * = validateSexp("(define (square x) (* x x))");
    * = validateWatStructure("(module (func (export \"add\") (result i32) i32.const 42))");
    * = isReadOnlyCypher("MATCH (n:Person) RETURN n.name LIMIT 10");
    * = isSpv(buf);
}
```

---

## 收敛子模块深度文档

- [caddy.md](compiler/caddy.md)
- [cypher.md](compiler/cypher.md)
- [expr.md](compiler/expr.md)
- [flatbuf.md](compiler/flatbuf.md)
- [flexbuf.md](compiler/flexbuf.md)
- [glsl.md](compiler/glsl.md)
- [idl.md](compiler/idl.md)
- [nginx.md](compiler/nginx.md)
- [protobuf.md](compiler/protobuf.md)
- [regex.md](compiler/regex.md)
- [sat.md](compiler/sat.md)
- [sexp.md](compiler/sexp.md)
- [sparql.md](compiler/sparql.md)
- [spv.md](compiler/spv.md)
- [wat.md](compiler/wat.md)
- [wgsl.md](compiler/wgsl.md)
