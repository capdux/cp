# RFC: 面向 AI 与高性能计算的 CP 编译器原生迭代计划（SIMD 向量原语 & 非重叠借用优化）

> **状态**：已排期（Planned） | **创建日期**：2026-11-22  
> **目标模块**：`cpc/src/sema/type_infer.rs`、`cpc/src/codegen/expr/builtin.rs`、`cpc/src/codegen/runtime_emit.rs`、`cpc/src/codegen/fn_gen.rs`、`lib/vector/`、`lib/attention/`

---

## 一、 迭代背景与核心动机

在构建纯 CP 零堆分配标准库（如 `cap:vector` 的混合检索/量化/IVF/PQ、`cap:attention` 的 RoPE/RMSNorm/Q8_0/Q4_0 GEMV、`cap:svd`/`cap:lu`/`cap:qr` 线性代数库）的实战中，有两类底层性能与开发体验瓶颈最为突出：

1. **向量/张量热循环标量化**：点积、乘加（FMA）、L2 距离与归一化目前需手写标量 `for` 循环，既冗长又依赖后端自动向量化猜测，无法稳定吃满 AVX2 / AVX-512 / NEON 硬件 SIMD 吞吐。
2. **多切片工作区别名阻碍与切分繁琐**：矩阵分块与双缓冲算法频繁对同一连续数组切分不重叠可变子切片，且生成的 Zig 函数参数缺少 `noalias` 提示，导致 LLVM 保守假定输入/输出切片可能重叠而放弃向量化与寄存器提升。

---

## 二、 Phase 1：`@dot` / `@fma` / `@l2sq` 一等 SIMD 向量内建原语

### 1. CP 源码语法（TS 级简洁，零学习成本）

```typescript
// 1. 向量点积：支持 []const f64 / []const f32 / []const i32（长度取 min(a.len, b.len)）
const score: f64 = @dot(query, doc_vec);

// 2. 硬件融合乘加（Fused Multiply-Add）：单指令精度更高，无中间舍入误差
const acc: f64 = @fma(a, b, c); // 等价于 Zig @mulAdd(@TypeOf(a), a, b, c)

// 3. 向量平方欧氏距离：直接映射 SIMD 向量减法 + 平方归约（用于 KNN/IVF/PQ/KMeans）
const d2: f64 = @l2sq(a, b);
```

### 2. `cpc` 编译器实现路径

| 编译器阶段 | 目标文件 | 改动要点 |
| :--- | :--- | :--- |
| **类型推导** | `cpc/src/sema/type_infer.rs` | 在 `Expr::Builtin(call)` 分支注册 `@dot`（返回切片元素标量类型 `f64`/`f32`/`i64`）、`@fma`（返回操作数浮点类型）、`@l2sq`（返回 `f64`/`f32`），并在参数类型不匹配时给出 AI-First 三段式自愈建议 |
| **IR/Zig 降级** | `cpc/src/codegen/expr/builtin.rs` | `@fma(a, b, c)` 直接降级为 `@mulAdd(@TypeOf(a), a, b, c)`；`@dot(a, b)` 与 `@l2sq(a, b)` 降级为调用编译期泛型 SIMD 辅助函数 `__cp_simd_dot(a, b)` / `__cp_simd_l2sq(a, b)` |
| **SIMD 运行时核** | `cpc/src/codegen/runtime_emit.rs` | 按需发射（仅当源码使用 `@dot`/`@l2sq` 时注入，0 额外体积）：主循环使用 `@Vector(V, T)`（如 `f64` 取 `V=4`、`f32` 取 `V=8`）配合 `@mulAdd` 与 `@reduce(.Add, acc)`，尾部标量收尾，兼容任意未对齐与非 4/8 倍数长度 |

### 3. 验收标准与回归测试
- 新增正式测试 `cap/test/test_builtin_simd_dot_fma.cp`：覆盖空切片、1/3/7/8/16/128/1536 维向量、`f32`/`f64` 精度（与标量参考误差 `< 1e-12`）及 `@fma` 硬件精度验证；
- 升级 `lib/vector/` 与 `lib/attention/` 热路径接入 `@dot`/`@l2sq`/`@fma`，全部既有回归测试 100% PASS。

---

## 三、 Phase 2：`@splitAtMut` 非重叠可变切分 + `noalias` 借用代码生成优化

### 1. CP 源码语法与借用安全契约

```typescript
// 将可变切片安全切分为两个物理不相交的可变子切片 [0..mid) 与 [mid..len)
// 编译期借用检查器识别二者互不别名，可同时可变传入矩阵分块/双缓冲函数
let [left, right] = @splitAtMut(buf, mid);
```

### 2. `cpc` 借用检查与 `noalias` 代码生成协同

- **借用检查器保障（`cpc/src/sema/`）**：CP 严格执行“同一时刻至多一个可变借用 `*[]T`，或任意多个只读借用 `[]const T`”。
- **`noalias` 自动注入（`cpc/src/codegen/`）**：
  - 当纯 CP 函数参数同时包含 `*[]T`（独占可变借用）与 `[]const T`（只读借用）且已通过 Sema 借用独占性校验时，在生成的 Zig 函数签名中为对应指针/切片安全注入 `noalias` 语义（或局部 `noalias` 指针别名），让 LLVM 放心展开 SIMD 向量化与循环不变量外提（LICM），无需担心输出写回污染输入数组。
- **`@splitAtMut(slice, mid)` 降级**：
  - 边界检查 `mid <= slice.len`（越界触发清晰诊断或返回可选元组），零拷贝返回 `.{ slice[0..mid], slice[mid..] }`，配合解构 `let [a, b] = @splitAtMut(buf, mid)` 实现零样板双缓冲操作。

### 3. 验收标准
- 新增正式测试 `cap/test/test_borrow_split_at_mut_noalias.cp`：覆盖正常切分、边界 `mid == 0` / `mid == len`、两半独立并发修改无冲突、以及与借用检查器协同拦截非法重叠借用。

---

## 四、 执行排期与并发安全约束

1. **当前前置门禁**：等待正在执行的 `CPC Hotfix Engineer` 完成 2 条 `bun_agy` RAII 紧急工单并回收槽位（避免 `cpc/src/` 并发写冲突）。
2. **迭代顺序**：
   - **Step 1**：实施 Phase 1（`@fma` / `@dot` / `@l2sq`） -> `bash scripts/sync_cpc.sh release` -> 全量回归与测试验收 -> 更新 `cp_agent_ref.md`（打 `[SYNC_PENDING: 2026-11-22]` 标记）。
   - **Step 2**：实施 Phase 2（`@splitAtMut` + `noalias` 借用优化） -> `bash scripts/sync_cpc.sh release` -> 全量回归与测试验收 -> 更新 `cp_agent_ref.md`。
