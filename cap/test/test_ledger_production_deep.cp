// Source: cp/cap/test/test_ledger_production_deep.cp
// Deep Production Verification Suite for cap:ledger & cap.ledger
// Covers:
//   1. Full business cycle bookkeeping (Equity->Cash, Expense<-Cash, Liability, Cash<-Revenue) & TrialBalance
//   2. Strict constraint interception (UnbalancedEntry, CurrencyMismatch, InsufficientFunds, InvalidAmount) & 100% atomic rollback
//   3. idempotency_key deduplication, reverseTransaction, and AlreadyReversed guard
//   4. statementForAccount running balances & std.testing.allocator zero memory leak check
//   5. `import cap from "cap"; cap.ledger` dynamic mounting

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    LedgerError,
    AccountType,
    EntrySide,
    Account,
    JournalLine,
    JournalEntry,
    StatementLine,
    TrialBalance,
    Ledger,
} from "cap:ledger";

test("cap:ledger - full business cycle, accounting equation & verifyTrialBalance") {
    const alloc = std.testing.allocator;
    let ledger = Ledger.init(alloc);
    defer ledger.deinit();

    const usd: [3]u8 = ['U', 'S', 'D'];
    ledger.createAccount("cash", "Operating Cash", AccountType.Asset, usd, false) catch unreachable;
    ledger.createAccount("ap", "Accounts Payable", AccountType.Liability, usd, false) catch unreachable;
    ledger.createAccount("equity", "Founder Equity", AccountType.Equity, usd, false) catch unreachable;
    ledger.createAccount("rev", "AI Token Revenue", AccountType.Revenue, usd, false) catch unreachable;
    ledger.createAccount("exp", "GPU Server Expense", AccountType.Expense, usd, false) catch unreachable;

    // 1. Capital injection: Debit Cash 100_000, Credit Equity 100_000
    const l1 = [
        JournalLine{ account_id: "cash", side: EntrySide.Debit, amount: 100_000 },
        JournalLine{ account_id: "equity", side: EntrySide.Credit, amount: 100_000 },
    ];
    const tx1 = ledger.post(1000, "tx-seed", "Initial capital", &l1) catch unreachable;
    expect_eq(tx1, 1);

    // 2. Purchase servers: Debit Expense 35_000, Credit Cash 20_000 & Credit AP 15_000
    const l2 = [
        JournalLine{ account_id: "exp", side: EntrySide.Debit, amount: 35_000 },
        JournalLine{ account_id: "cash", side: EntrySide.Credit, amount: 20_000 },
        JournalLine{ account_id: "ap", side: EntrySide.Credit, amount: 15_000 },
    ];
    const tx2 = ledger.post(2000, "tx-gpu", "GPU cluster purchase", &l2) catch unreachable;
    expect_eq(tx2, 2);

    // 3. Collect customer service fee: Debit Cash 60_000, Credit Revenue 60_000
    const l3 = [
        JournalLine{ account_id: "cash", side: EntrySide.Debit, amount: 60_000 },
        JournalLine{ account_id: "rev", side: EntrySide.Credit, amount: 60_000 },
    ];
    const tx3 = ledger.post(3000, "tx-fee", "Customer AI billing", &l3) catch unreachable;
    expect_eq(tx3, 3);

    // Check individual balances
    expect_eq(ledger.balanceOf("cash") catch unreachable, 140_000);
    expect_eq(ledger.balanceOf("ap") catch unreachable, 15_000);
    expect_eq(ledger.balanceOf("equity") catch unreachable, 100_000);
    expect_eq(ledger.balanceOf("rev") catch unreachable, 60_000);
    expect_eq(ledger.balanceOf("exp") catch unreachable, 35_000);

    // Assets (140_000) == Liabilities (15_000) + Equity (100_000) + NetIncome (60_000 - 35_000 = 25_000)
    const tb = ledger.verifyTrialBalance(usd);
    expect_eq(tb.balanced, true);
    expect_eq(tb.total_debits, 195_000);
    expect_eq(tb.total_credits, 195_000);
    expect_eq(tb.assets_eq_liab_equity_plus_net_income, true);
}

test("cap:ledger - strict constraints & 100% atomic rollback on failure") {
    const alloc = std.testing.allocator;
    let ledger = Ledger.init(alloc);
    defer ledger.deinit();

    const usd: [3]u8 = ['U', 'S', 'D'];
    const eur: [3]u8 = ['E', 'U', 'R'];
    ledger.createAccount("cash_usd", "USD Cash", AccountType.Asset, usd, false) catch unreachable;
    ledger.createAccount("equity_usd", "USD Equity", AccountType.Equity, usd, false) catch unreachable;
    ledger.createAccount("exp_usd", "USD Expense", AccountType.Expense, usd, false) catch unreachable;
    ledger.createAccount("cash_eur", "EUR Cash", AccountType.Asset, eur, false) catch unreachable;

    // DuplicateAccount check
    let dup_err = false;
    * = ledger.createAccount("cash_usd", "Dup", AccountType.Asset, usd, false) catch |e| {
        if (e == LedgerError.DuplicateAccount) dup_err = true;
    };
    expect_eq(dup_err, true);

    // Seed 5_000 USD
    const seed = [
        JournalLine{ account_id: "cash_usd", side: EntrySide.Debit, amount: 5_000 },
        JournalLine{ account_id: "equity_usd", side: EntrySide.Credit, amount: 5_000 },
    ];
    * = ledger.post(100, "seed", "Seed", &seed) catch unreachable;

    // 1. UnbalancedEntry
    const unbal = [
        JournalLine{ account_id: "exp_usd", side: EntrySide.Debit, amount: 1_000 },
        JournalLine{ account_id: "cash_usd", side: EntrySide.Credit, amount: 900 },
    ];
    let unbal_err = false;
    * = ledger.post(200, "bad-unbal", "Unbalanced", &unbal) catch |e| {
        if (e == LedgerError.UnbalancedEntry) unbal_err = true;
    };
    expect_eq(unbal_err, true);

    // 2. CurrencyMismatch
    const fx = [
        JournalLine{ account_id: "cash_eur", side: EntrySide.Debit, amount: 1_000 },
        JournalLine{ account_id: "cash_usd", side: EntrySide.Credit, amount: 1_000 },
    ];
    let fx_err = false;
    * = ledger.post(300, "bad-fx", "Mixed currency", &fx) catch |e| {
        if (e == LedgerError.CurrencyMismatch) fx_err = true;
    };
    expect_eq(fx_err, true);

    // 3. InvalidAmount (<= 0)
    const zero_amt = [
        JournalLine{ account_id: "exp_usd", side: EntrySide.Debit, amount: 0 },
        JournalLine{ account_id: "cash_usd", side: EntrySide.Credit, amount: 0 },
    ];
    let amt_err = false;
    * = ledger.post(400, "bad-amt", "Zero amount", &zero_amt) catch |e| {
        if (e == LedgerError.InvalidAmount) amt_err = true;
    };
    expect_eq(amt_err, true);

    // 4. InsufficientFunds (attempting to spend 6_000 when cash_usd == 5_000)
    const overdraft = [
        JournalLine{ account_id: "exp_usd", side: EntrySide.Debit, amount: 6_000 },
        JournalLine{ account_id: "cash_usd", side: EntrySide.Credit, amount: 6_000 },
    ];
    let funds_err = false;
    * = ledger.post(500, "bad-funds", "Overdraft", &overdraft) catch |e| {
        if (e == LedgerError.InsufficientFunds) funds_err = true;
    };
    expect_eq(funds_err, true);

    // Verify 100% atomic rollback: balances and trial balance remain untouched
    expect_eq(ledger.balanceOf("cash_usd") catch unreachable, 5_000);
    expect_eq(ledger.balanceOf("exp_usd") catch unreachable, 0);
    expect_eq(ledger.balanceOf("cash_eur") catch unreachable, 0);
    const tb = ledger.verifyTrialBalance(usd);
    expect_eq(tb.balanced, true);
    expect_eq(tb.total_debits, 5_000);
    expect_eq(tb.assets_eq_liab_equity_plus_net_income, true);
}

test("cap:ledger - idempotency_key deduplication, reverseTransaction & AlreadyReversed") {
    const alloc = std.testing.allocator;
    let ledger = Ledger.init(alloc);
    defer ledger.deinit();

    const usd: [3]u8 = ['U', 'S', 'D'];
    ledger.createAccount("cash", "Cash", AccountType.Asset, usd, false) catch unreachable;
    ledger.createAccount("rev", "Revenue", AccountType.Revenue, usd, false) catch unreachable;

    const sale = [
        JournalLine{ account_id: "cash", side: EntrySide.Debit, amount: 25_000 },
        JournalLine{ account_id: "rev", side: EntrySide.Credit, amount: 25_000 },
    ];
    const tx1 = ledger.post(1000, "order-42", "Order #42", &sale) catch unreachable;

    // Idempotent retry with identical key & payload returns same tx_id without double-posting
    const tx1_retry = ledger.post(1001, "order-42", "Order #42 retry", &sale) catch unreachable;
    expect_eq(tx1_retry, tx1);
    expect_eq(ledger.balanceOf("cash") catch unreachable, 25_000);

    // Reusing same idempotency_key with different payload returns DuplicateIdempotencyKey
    const different_sale = [
        JournalLine{ account_id: "cash", side: EntrySide.Debit, amount: 99_999 },
        JournalLine{ account_id: "rev", side: EntrySide.Credit, amount: 99_999 },
    ];
    let dup_key_err = false;
    * = ledger.post(1002, "order-42", "Conflicting payload", &different_sale) catch |e| {
        if (e == LedgerError.DuplicateIdempotencyKey) dup_key_err = true;
    };
    expect_eq(dup_key_err, true);
    expect_eq(ledger.balanceOf("cash") catch unreachable, 25_000);

    // Reverse transaction
    const rev_tx = ledger.reverseTransaction(tx1, 2000, "Refund Order #42") catch unreachable;
    expect_eq(rev_tx, 2);
    expect_eq(ledger.balanceOf("cash") catch unreachable, 0);
    expect_eq(ledger.balanceOf("rev") catch unreachable, 0);

    const orig_entry = ledger.getEntry(tx1) catch unreachable;
    expect_eq(orig_entry.reversed_by_tx_id.?, rev_tx);

    // Double reversal must be rejected with AlreadyReversed
    let already_rev = false;
    * = ledger.reverseTransaction(tx1, 2001, "Second refund") catch |e| {
        if (e == LedgerError.AlreadyReversed) already_rev = true;
    };
    expect_eq(already_rev, true);

    const tb = ledger.verifyTrialBalance(usd);
    expect_eq(tb.balanced, true);
    expect_eq(tb.assets_eq_liab_equity_plus_net_income, true);
}

test("cap:ledger - statementForAccount running balance & zero memory leaks") {
    const alloc = std.testing.allocator;
    let ledger = Ledger.init(alloc);
    defer ledger.deinit();

    const usd: [3]u8 = ['U', 'S', 'D'];
    ledger.createAccount("cash", "Cash", AccountType.Asset, usd, false) catch unreachable;
    ledger.createAccount("equity", "Equity", AccountType.Equity, usd, false) catch unreachable;
    ledger.createAccount("exp", "Expense", AccountType.Expense, usd, false) catch unreachable;

    const l1 = [
        JournalLine{ account_id: "cash", side: EntrySide.Debit, amount: 10_000 },
        JournalLine{ account_id: "equity", side: EntrySide.Credit, amount: 10_000 },
    ];
    const tx1 = ledger.post(100, "k1", "Deposit", &l1) catch unreachable;

    const l2 = [
        JournalLine{ account_id: "exp", side: EntrySide.Debit, amount: 3_000 },
        JournalLine{ account_id: "cash", side: EntrySide.Credit, amount: 3_000 },
    ];
    const tx2 = ledger.post(200, "k2", "Cloud bill", &l2) catch unreachable;

    const rev_tx2 = ledger.reverseTransaction(tx2, 300, "Cloud credit") catch unreachable;

    const stmt = ledger.statementForAccount(alloc, "cash") catch unreachable;
    defer alloc.free(stmt);

    expect_eq(stmt.len, 3);
    expect_eq(stmt[0].tx_id, tx1);
    expect_eq(stmt[0].side == EntrySide.Debit, true);
    expect_eq(stmt[0].running_balance, 10_000);

    expect_eq(stmt[1].tx_id, tx2);
    expect_eq(stmt[1].side == EntrySide.Credit, true);
    expect_eq(stmt[1].running_balance, 7_000);
    expect_eq(stmt[1].reversed_by_tx_id.?, rev_tx2);

    expect_eq(stmt[2].tx_id, rev_tx2);
    expect_eq(stmt[2].side == EntrySide.Debit, true);
    expect_eq(stmt[2].running_balance, 10_000);
}

test("cap:ledger - dynamic mount via import cap from 'cap'; cap.ledger") {
    const alloc = std.testing.allocator;
    let ledger = cap.ledger.Ledger.init(alloc);
    defer ledger.deinit();

    const jpy: [3]u8 = ['J', 'P', 'Y'];
    ledger.createAccount("wallet", "Agent Wallet", cap.ledger.AccountType.Asset, jpy, false) catch unreachable;
    ledger.createAccount("tokens", "Token Revenue", cap.ledger.AccountType.Revenue, jpy, false) catch unreachable;

    const lines = [
        cap.ledger.JournalLine{ account_id: "wallet", side: cap.ledger.EntrySide.Debit, amount: 50_000 },
        cap.ledger.JournalLine{ account_id: "tokens", side: cap.ledger.EntrySide.Credit, amount: 50_000 },
    ];
    const tx = ledger.post(9000, "cap-dyn-1", "Dynamic mount test", &lines) catch unreachable;
    expect_eq(tx, 1);
    expect_eq(ledger.balanceOf("wallet") catch unreachable, 50_000);

    const tb = ledger.verifyTrialBalance(jpy);
    expect_eq(tb.balanced, true);
    expect_eq(tb.assets_eq_liab_equity_plus_net_income, true);
}
