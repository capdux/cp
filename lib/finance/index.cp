// Source: cp/lib/finance/index.cp (Unified Exports for cap:finance)
// Unified Public Exports for Quantitative Billing, Multi-Currency FX, Double-Entry Ledger, Spreadsheet DAG, Audit Trail, Base58Check, Rule Engine & Time-Series Analytics (cap:finance & cap.finance)

// --- Billing, Tiered Pricing, Proration & Invoicing ---

export {
    BPS_DENOMINATOR,
    MICROS_PER_CENT,
    BillingError,
    TierMode,
    CouponKind,
    PriceTier,
    Coupon,
    InvoiceLineItem,
    Invoice,
    BillingCalculator,
} from "./billing.cp";

// --- Multi-Currency Money Arithmetic (ISO 4217) & FX Triangulation ---

export {
    FxError,
    RoundingMode,
    Currency,
    Money,
} from "./money.cp";

export {
    RATE_SCALE_E9,
    FxRate,
    FxBook,
} from "./rates.cp";

// --- Double-Entry Bookkeeping Accounting Ledger ---

export {
    LedgerError,
    AccountType,
    EntrySide,
    Account,
    JournalLine,
    JournalEntry,
    StatementLine,
    TrialBalance,
    Ledger,
} from "./ledger.cp";

// --- Spreadsheet Formula DAG & Recalculation Engine ---

export {
    MAX_FORMULA_DEPTH,
    MAX_ROW_COUNT,
    MAX_COL_COUNT,
    SheetError,
    CellCoord,
    CellValue,
} from "./formula.cp";

export {
    Spreadsheet,
} from "./sheet.cp";

// --- Tamper-Evident Hash-Chained Audit Trail ---

export {
    GENESIS_HASH,
    AuditError,
    AuditOutcome,
    AuditSeverity,
    AuditEntry,
    ChainVerifyResult,
    AuditTrail,
} from "./audit.cp";

// --- Bitcoin Base58 & Base58Check Codec ---

export {
    Base58Error,
    encode,
    decode,
    encodeCheck,
    CheckDecoded,
    decodeCheck,
} from "./base58_check.cp";

// --- Declarative Business & Risk Rule Engine ---

export {
    MAX_CONDITION_DEPTH,
    RuleError,
    FactValue,
    FactMap,
    CompareOp,
    CompareSpec,
    InListSpec,
    Condition,
    RuleSpec,
    FiredRule,
    RuleEvalReport,
    RuleEngine,
} from "./rule.cp";
