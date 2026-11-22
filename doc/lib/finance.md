# CP 标准库：金融、多币种外汇、复式记账、计费与商业规则手册 (`cap:finance` / `cap.finance`)

## 一、 架构总览与设计准则

`cap:finance` 是 CP 语言官方内置的纯原生零 C 依赖金融、计费、复式记账、商业规则与合规审计标准库，覆盖：
1. **阶梯计费、按比例折算与开票**（`billing.cp`）
2. **定点数多币种资金算术（ISO 4217）与 FX 汇率三角换算**（`money.cp`, `rates.cp`）
3. **复式记账总账与试算平衡表**（`ledger.cp`）
4. **电子表格公式 DAG 依赖图与拓扑重算引擎**（`formula.cp`, `sheet.cp`）
5. **防篡改哈希链合规审计日志**（`audit.cp`）
6. **Bitcoin Base58 / Base58Check 双重 SHA-256 编解码**（`base58_check.cp`）
7. **声明式业务与风控规则引擎**（`rule.cp`）

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:finance"` 与 `import cap from "cap"; cap.finance.*` 编译期自动惰性挂载。

### 模块文件结构

| 文件路径 | 核心职责 |
| :--- | :--- |
| `lib/finance/billing.cp` | 阶梯/分档计量计费、订阅按比例折算（Proration）、优惠券与开票引擎 |
| `lib/finance/money.cp` | 精确整数最小单位多币种 `Money`、ISO 4217 货币注册表与银行家舍入 |
| `lib/finance/rates.cp` | `FxBook` 汇率账本（直查、自动逆汇率、单跳枢纽货币三角换算） |
| `lib/finance/ledger.cp` | 复式记账总账（借贷平衡校验、会计科目、分录过账与试算平衡表） |
| `lib/finance/formula.cp` | 电子表格单元格坐标解析、值类型与公式求值器 |
| `lib/finance/sheet.cp` | 双向依赖图、Kahn 拓扑排序环检测与级联重算 `Spreadsheet` |
| `lib/finance/audit.cp` | SHA-256 哈希链防篡改合规审计日志 `AuditTrail` 与整链校验 |
| `lib/finance/base58_check.cp` | Bitcoin Base58 与 Base58Check（版本号 + 双重 SHA-256 校验和） |
| `lib/finance/rule.cp` | 声明式生产/风控规则引擎（`FactMap`、`Condition` AST、`RuleEngine`） |
| `lib/finance/index.cp` | `cap:finance` 与 `cap.finance` 统一零冲突公开导出入口 |

---

## 二、 核心 API 索引

- **计费与开票（`billing.cp`）**：`BPS_DENOMINATOR`, `MICROS_PER_CENT`, `BillingError`, `TierMode`, `CouponKind`, `PriceTier`, `Coupon`, `InvoiceLineItem`, `Invoice`, `BillingCalculator`
- **多币种与外汇（`money.cp`, `rates.cp`）**：`FxError`, `RoundingMode`, `Currency`, `Money`, `RATE_SCALE_E9`, `FxRate`, `FxBook`
- **复式记账（`ledger.cp`）**：`LedgerError`, `AccountType`, `EntrySide`, `Account`, `JournalLine`, `JournalEntry`, `StatementLine`, `TrialBalance`, `Ledger`
- **电子表格（`formula.cp`, `sheet.cp`）**：`MAX_FORMULA_DEPTH`, `MAX_ROW_COUNT`, `MAX_COL_COUNT`, `SheetError`, `CellCoord`, `CellValue`, `Spreadsheet`
- **合规审计（`audit.cp`）**：`GENESIS_HASH`, `AuditError`, `AuditOutcome`, `AuditSeverity`, `AuditEntry`, `ChainVerifyResult`, `AuditTrail`
- **Base58Check（`base58_check.cp`）**：`Base58Error`, `encode`, `decode`, `encodeCheck`, `CheckDecoded`, `decodeCheck`
- **风控规则引擎（`rule.cp`）**：`MAX_CONDITION_DEPTH`, `RuleError`, `FactValue`, `FactMap`, `CompareOp`, `CompareSpec`, `InListSpec`, `Condition`, `RuleSpec`, `FiredRule`, `RuleEvalReport`, `RuleEngine`

---

## 三、 使用示例

```typescript
import cap from "cap";
import {
    Money,
    FxBook,
    RoundingMode,
    Ledger,
    AccountType,
    EntrySide,
    JournalLine,
} from "cap:finance";

pub fn demoFinance(alloc: std.mem.Allocator): !void {
    // 1. 多币种资金与 FX 换算
    const a = Money.parse("25.50", "SGD")?;
    const b = Money.parse("4.50", "SGD")?;
    const total = a.add(b)?;
    let book = FxBook.init(alloc);
    defer book.deinit();
    book.setRateFloat("SGD", "USD", 0.75)?;
    const usd = book.convert(total, "USD", RoundingMode.HalfEven)?;
    * = usd;

    // 2. 复式记账总账（cap.finance 动态挂载等价）
    let ledger = cap.finance.Ledger.init(alloc);
    defer ledger.deinit();
    ledger.createAccount("cash", "Cash", AccountType.Asset, "USD", false)?;
    ledger.createAccount("rev", "Revenue", AccountType.Revenue, "USD", false)?;
    const lines = [
        JournalLine{ account_id: "cash", side: EntrySide.Debit, amount: 1000 },
        JournalLine{ account_id: "rev", side: EntrySide.Credit, amount: 1000 },
    ];
    * = ledger.post("tx-1", "Payment received", &lines)?;
}
```

---

## 收敛子模块深度文档

- [audit.md](finance/audit.md)
- [base58_check.md](finance/base58_check.md)
- [billing.md](finance/billing.md)
- [fx.md](finance/fx.md)
- [ledger.md](finance/ledger.md)
- [rule.md](finance/rule.md)
- [sheet.md](finance/sheet.md)
