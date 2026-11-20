# CP 标准库：复式记账会计总账引擎手册 (`cap:ledger`)

## 一、 架构总览与设计准则

`cap:ledger` 是 CP 语言官方内置的纯 CP **复式记账（Double-Entry Bookkeeping）会计总账引擎**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，专为金融支付系统、SaaS 账单结算与 AI Agent Token 计费账本设计。既可通过 `import { ... } from "cap:ledger"` 导入，也支持 `import cap from "cap"; cap.ledger.*` 动态懒挂载调用。

### 1. 核心会计不变式

1. **精确 `i128` 最小货币单位**：所有金额均以 `i128` 整数最小单位（如分、聪、微美元、Token）存储与运算，零浮点舍入误差。
2. **有借必有贷，借贷必相等**：每笔凭证 `JournalEntry` 必须满足至少 1 条分录、所有分录金额 `> 0`、同一凭证内所有科目币种一致，且 `∑Debit == ∑Credit`。
3. **原子过账与零污染回滚**：任何校验失败（借贷不平、异币种、账户不存在、透支拦截 `InsufficientFunds`、幂等键冲突）均在修改状态前拦截，确保账户余额与凭证列表 100% 原子回滚。
4. **不可变凭证历史与红字冲正**：已过账凭证不可就地修改或删除；纠错统一通过 `reverseTransaction` 生成借贷方向完全相反的冲正凭证并双向关联，且禁止重复冲正。
5. **会计恒等式自洽**：
   - 资产类（`Asset`）与费用类（`Expense`）为借方余额：`netBalance = debit_total - credit_total`
   - 负债类（`Liability`）、权益类（`Equity`）与收入类（`Revenue`）为贷方余额：`netBalance = credit_total - debit_total`
   - 全账本试算平衡校验 `verifyTrialBalance(currency)` 恒满足：
     - `total_debits == total_credits`
     - `Assets == Liabilities + Equity + (Revenue - Expense)`

### 2. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/ledger/ledger.cp` | `// Source: cp/lib/ledger/ledger.cp (Double-Entry Bookkeeping Accounting Ledger)` | `LedgerError`、`AccountType`、`EntrySide`、`Account`、`JournalLine`、`JournalEntry`、`StatementLine`、`TrialBalance`、`Ledger` |
| `lib/ledger/index.cp` | `// Source: cp/lib/ledger/index.cp (Unified Exports for cap:ledger)` | `cap:ledger` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误与基础枚举

- **`LedgerError`**：
  - `UnbalancedEntry`：凭证为空或借方总额不等于贷方总额。
  - `DuplicateAccount`：重复创建相同 `id` 的科目。
  - `AccountNotFound`：引用了不存在的科目 `account_id`。
  - `CurrencyMismatch`：同一凭证内混用了不同币种的科目。
  - `InvalidAmount`：分录金额 `<= 0`。
  - `DuplicateIdempotencyKey`：相同 `idempotency_key` 但分录载荷与原凭证不一致。
  - `TransactionNotFound`：指定的 `tx_id` 不存在。
  - `AlreadyReversed`：尝试对已冲正的凭证再次冲正。
  - `InsufficientFunds`：`allow_negative == false` 的科目过账后净余额将变为负数。
  - `OutOfMemory`：内存分配失败。
- **`AccountType`**：`Asset`、`Liability`、`Equity`、`Revenue`、`Expense`（提供 `isDebitNormal()` 与 `toString()`）。
- **`EntrySide`**：`Debit`、`Credit`（提供 `opposite()` 与 `toString()`）。

### 2. 核心数据结构

- **`Account`**：`id: string`、`name: string`、`account_type: AccountType`、`currency: [3]u8`、`allow_negative: bool`、`debit_total: i128`、`credit_total: i128`，方法 `netBalance(): i128`。
- **`JournalLine`**：`account_id: string`、`side: EntrySide`、`amount: i128`。
- **`JournalEntry`**：`tx_id: u64`、`timestamp_ms: u64`、`idempotency_key: string`、`memo: string`、`lines: []const JournalLine`、`reversed_by_tx_id: u64?`。
- **`StatementLine`**：`tx_id: u64`、`timestamp_ms: u64`、`memo: string`、`side: EntrySide`、`amount: i128`、`running_balance: i128`、`reversed_by_tx_id: u64?`。
- **`TrialBalance`**：`balanced: bool`、`total_debits: i128`、`total_credits: i128`、`assets_eq_liab_equity_plus_net_income: bool`。

### 3. `Ledger` 核心方法

- **`Ledger.init(alloc)` / `deinit()`**：初始化与确定性释放所有内部托管内存。
- **`createAccount(id, name, account_type, currency, allow_negative): !LedgerError`**：注册会计科目。
- **`post(timestamp_ms, idempotency_key, memo, lines): u64 ! LedgerError`**：原子过账多行凭证并返回 `tx_id`。若 `idempotency_key` 非空且已存在：分录完全一致时幂等返回原 `tx_id`，分录不一致时返回 `DuplicateIdempotencyKey`。
- **`reverseTransaction(tx_id, timestamp_ms, memo): u64 ! LedgerError`**：生成借贷方向完全相反的红字冲正凭证并标记原凭证 `reversed_by_tx_id`。
- **`balanceOf(account_id): i128 ! LedgerError`**：查询科目当前净余额。
- **`getAccount(account_id): Account ! LedgerError`** / **`getEntry(tx_id): JournalEntry ! LedgerError`**：查询科目或凭证详情。
- **`statementForAccount(alloc, account_id): []StatementLine ! LedgerError`**：按时间顺序生成科目流水对账单（含每笔过账后的 `running_balance`；调用方使用 `alloc.free(stmt)` 释放切片）。
- **`verifyTrialBalance(currency): TrialBalance`**：校验指定币种全账本借贷总平衡及会计恒等式 `Assets == Liabilities + Equity + (Revenue - Expense)`。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { Ledger, AccountType, EntrySide, JournalLine } from "cap:ledger";
import cap from "cap";

pub fn demoLedger(): ! {
    const alloc = std.heap.page_allocator;
    let ledger = cap.ledger.Ledger.init(alloc);
    defer ledger.deinit();

    const usd: [3]u8 = ['U', 'S', 'D'];
    ledger.createAccount("cash", "Operating Cash", AccountType.Asset, usd, false)?;
    ledger.createAccount("equity", "Paid-In Capital", AccountType.Equity, usd, false)?;
    ledger.createAccount("rev", "API Revenue", AccountType.Revenue, usd, false)?;

    * = ledger.post(1700000000000, "seed-1", "Capital injection", &[2]JournalLine{
        JournalLine{ account_id: "cash", side: EntrySide.Debit, amount: 1000_00 },
        JournalLine{ account_id: "equity", side: EntrySide.Credit, amount: 1000_00 },
    })?;

    const tb = ledger.verifyTrialBalance(usd);
    if (!tb.balanced || !tb.assets_eq_liab_equity_plus_net_income) {
        return;
    }
}
```
