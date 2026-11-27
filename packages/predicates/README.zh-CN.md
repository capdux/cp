[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# predicates

适用于 CP 语言的零堆分配可组合布尔谓词库，对齐 Rust [`predicates`](https://github.com/assert-rs/predicates-rs) crate（MIT OR Apache-2.0，CP v1.0.0）。

提供基于固定 16 槽后缀指令栈（RPN）的 `IntPredicate`（`i64`）、`StrPredicate`（`string`）与 `FloatPredicate`（`f64`）组合匹配器，适用于单元测试断言、参数校验和规则引擎，全程零堆分配、无动态派发开销。

## 安装与导入

```bash
cap add predicates
```

```cp
import { IntPredicate, StrPredicate, FloatPredicate } from "predicates";
```

## 核心 API 参考

### `IntPredicate`（`i64`）

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `eq` / `ne` / `lt` / `le` / `gt` / `ge` | `(v: i64): IntPredicate` | 标准整数比较谓词。 |
| `inRange(lo, hi)` | `(lo: i64, hi: i64): IntPredicate` | 闭区间范围校验 `lo <= x && x <= hi`。 |
| `always()` / `never()` | `(): IntPredicate` | 恒真 / 恒假谓词。 |
| `andPred(other)` / `orPred(other)` / `notPred()` | `(...): IntPredicate` | 逻辑与、或、非组合子，返回组合后的新谓词值。 |
| `eval(x)` | `(x: i64): bool` | 对目标整数 `x` 求值。 |
| `fromOp(op)` | `(op: IntPredOp): IntPredicate` | 从单个 `IntPredOp` 构造谓词。 |

### `StrPredicate`（`string`）

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `eq(target)` / `eqIgnoreCase(target)` | `(target: string): StrPredicate` | 精确匹配或忽略 ASCII 大小写相等。 |
| `contains(needle)` | `(needle: string): StrPredicate` | 子串包含匹配。 |
| `hasPrefix(prefix)` / `hasSuffix(suffix)` | `(string): StrPredicate` | 前缀 / 后缀匹配。 |
| `isEmpty()` | `(): StrPredicate` | 空串匹配（`s.len == 0`）。 |
| `lenEq(n)` / `lenBetween(lo, hi)` | `(usize)` / `(usize, usize)` | 精确字节长度或闭区间长度匹配。 |
| `always()` / `never()` | `(): StrPredicate` | 恒真 / 恒假谓词。 |
| `andPred(other)` / `orPred(other)` / `notPred()` | `(...): StrPredicate` | 布尔组合子。 |
| `eval(s)` | `(s: string): bool` | 对目标字符串 `s` 求值。 |

### `FloatPredicate`（`f64`）

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `approxEq(target, epsilon)` | `(target: f64, epsilon: f64): FloatPredicate` | 浮点误差近似相等 `|x - target| <= epsilon`（拒绝 NaN）。 |
| `inRange(lo, hi)` | `(lo: f64, hi: f64): FloatPredicate` | 闭区间范围校验 `lo <= x && x <= hi`。 |
| `isFinite()` | `(): FloatPredicate` | 匹配有限 IEEE-754 浮点数（排除 NaN 与 ±Inf）。 |
| `always()` / `never()` | `(): FloatPredicate` | 恒真 / 恒假谓词。 |
| `andPred(other)` / `orPred(other)` / `notPred()` | `(...): FloatPredicate` | 布尔组合子。 |
| `eval(x)` | `(x: f64): bool` | 对目标浮点数 `x` 求值。 |

### 辅助导出

同时导出 `prdEqIgnoreCase(a: string, b: string): bool`、`prdIsFinite(x: f64): bool`，以及底层操作码类型 `IntPredKind`、`IntPredOp`、`StrPredKind`、`StrPredOp`、`FloatPredKind`、`FloatPredOp`。

## 代码示例

### 1. 组合整数端口范围规则

```cp
import { IntPredicate } from "predicates";

fn checkPort(port: i64): bool {
    let valid_user_port = IntPredicate.inRange(1024, 65535)
        .andPred(IntPredicate.ne(8080));
    let allow_http = valid_user_port.orPred(IntPredicate.eq(80));
    return allow_http.eval(port);
}
```

### 2. 多条件字符串格式校验

```cp
import { StrPredicate } from "predicates";

fn validateApiKey(key: string): bool {
    let rule = StrPredicate.hasPrefix("cp_")
        .andPred(StrPredicate.lenBetween(12, 64))
        .andPred(StrPredicate.contains(" ").notPred());
    return rule.eval(key);
}
```

### 3. 浮点精度容差断言

```cp
import { FloatPredicate } from "predicates";

fn checkMetric(v: f64): bool {
    let rule = FloatPredicate.isFinite()
        .andPred(FloatPredicate.approxEq(1.0, 0.001));
    return rule.eval(v);
}
```

## 与源包差异与零拷贝安全亮点

- **零堆分配、无 Trait Object**：Rust 版通常依赖深层泛型嵌套或 `Box<dyn Predicate<T>>`；CP 版将组合树编译为扁平的 16 槽栈上 RPN 指令数组，无堆分配、无虚表间接调用。
- **零拷贝字符串匹配**：`StrPredicate` 直接保存 `string` 切片引用而不复制底层字节。
- **确定性 IEEE-754 检测**：`prdIsFinite` 通过 `@bitCast` 直接检查指数位，确保 NaN/Inf 处理跨平台一致。
