// Source: cp/lib/ledger/ledger.cp (Double-Entry Bookkeeping Accounting Ledger)
// Pure CP zero-C-dependency double-entry bookkeeping engine.
// Enforces Debits == Credits with exact i128 minor currency units, atomic multi-line
// posting with rollback on error, idempotency-key deduplication, immutable journal
// history with reversal entries, account statements, and trial balance verification.

const std = @import("std");

pub err LedgerError {
    UnbalancedEntry,
    DuplicateAccount,
    AccountNotFound,
    CurrencyMismatch,
    InvalidAmount,
    DuplicateIdempotencyKey,
    TransactionNotFound,
    AlreadyReversed,
    InsufficientFunds,
    OutOfMemory,
}

pub enum AccountType {
    Asset,
    Liability,
    Equity,
    Revenue,
    Expense,

    fn isDebitNormal(): bool {
        switch (_) {
            .Asset | .Expense => return true,
            .Liability | .Equity | .Revenue => return false,
        }
    }

    fn toString(): string {
        switch (_) {
            .Asset => return "Asset",
            .Liability => return "Liability",
            .Equity => return "Equity",
            .Revenue => return "Revenue",
            .Expense => return "Expense",
        }
    }
}

pub enum EntrySide {
    Debit,
    Credit,

    fn opposite(): EntrySide {
        switch (_) {
            .Debit => return EntrySide.Credit,
            .Credit => return EntrySide.Debit,
        }
    }

    fn toString(): string {
        switch (_) {
            .Debit => return "Debit",
            .Credit => return "Credit",
        }
    }
}

pub struct Account {
    id: string;
    name: string;
    account_type: AccountType;
    currency: [3]u8;
    allow_negative: bool;
    debit_total: i128;
    credit_total: i128;

    fn netBalance(): i128 {
        if (_.account_type.isDebitNormal()) {
            return _.debit_total - _.credit_total;
        }
        return _.credit_total - _.debit_total;
    }
}

pub struct JournalLine {
    account_id: string;
    side: EntrySide;
    amount: i128;
}

pub struct JournalEntry {
    tx_id: u64;
    timestamp_ms: u64;
    idempotency_key: string;
    memo: string;
    lines: []const JournalLine;
    reversed_by_tx_id: u64?;
}

pub struct StatementLine {
    tx_id: u64;
    timestamp_ms: u64;
    memo: string;
    side: EntrySide;
    amount: i128;
    running_balance: i128;
    reversed_by_tx_id: u64?;
}

pub struct TrialBalance {
    balanced: bool;
    total_debits: i128;
    total_credits: i128;
    assets_eq_liab_equity_plus_net_income: bool;
}

pub struct Ledger {
    alloc: std.mem.Allocator;
    accounts: std.ArrayListUnmanaged(Account) = .empty;
    account_index: std.StringHashMapUnmanaged(usize) = .empty;
    entries: std.ArrayListUnmanaged(JournalEntry) = .empty;
    tx_index: std.AutoHashMapUnmanaged(u64, usize) = .empty;
    idempotency_index: std.StringHashMapUnmanaged(u64) = .empty;
    owned_strs: std.ArrayListUnmanaged([]u8) = .empty;
    owned_line_slices: std.ArrayListUnmanaged([]JournalLine) = .empty;
    next_tx_id: u64 = 1;

    static fn init(alloc: std.mem.Allocator): Ledger {
        return Ledger{
            alloc: alloc,
            accounts: .empty,
            account_index: .empty,
            entries: .empty,
            tx_index: .empty,
            idempotency_index: .empty,
            owned_strs: .empty,
            owned_line_slices: .empty,
            next_tx_id: 1,
        };
    }

    fn deinit(*_) {
        for (slice of _.owned_line_slices.items) {
            _.alloc.free(slice);
        }
        _.owned_line_slices.deinit(_.alloc);

        for (s of _.owned_strs.items) {
            _.alloc.free(s);
        }
        _.owned_strs.deinit(_.alloc);

        _.idempotency_index.deinit(_.alloc);
        _.tx_index.deinit(_.alloc);
        _.entries.deinit(_.alloc);
        _.account_index.deinit(_.alloc);
        _.accounts.deinit(_.alloc);
    }

    fn ownSlice(*_, s: string): string ! LedgerError {
        const dup = _.alloc.dupe(u8, s) catch return LedgerError.OutOfMemory;
        errdefer _.alloc.free(dup);
        _.owned_strs.append(_.alloc, dup) catch return LedgerError.OutOfMemory;
        return dup;
    }

    fn createAccount(
        *_,
        id: string,
        name: string,
        account_type: AccountType,
        currency: [3]u8,
        allow_negative: bool,
    ): !LedgerError {
        if (_.account_index.contains(id)) {
            return LedgerError.DuplicateAccount;
        }

        const owned_id = _.ownSlice(id)?;
        const owned_name = _.ownSlice(name)?;
        const idx = _.accounts.items.len;

        _.accounts.append(_.alloc, Account{
            id: owned_id,
            name: owned_name,
            account_type: account_type,
            currency: currency,
            allow_negative: allow_negative,
            debit_total: 0,
            credit_total: 0,
        }) catch return LedgerError.OutOfMemory;

        _.account_index.put(_.alloc, owned_id, idx) catch {
            * = _.accounts.pop();
            return LedgerError.OutOfMemory;
        };
    }

    static fn linesMatch(existing: []const JournalLine, incoming: []const JournalLine): bool {
        if (existing.len != incoming.len) return false;
        let i: usize = 0;
        while (i < existing.len) : (i += 1) {
            const a = existing[i];
            const b = incoming[i];
            if (a.side != b.side) return false;
            if (a.amount != b.amount) return false;
            if (!std.mem.eql(u8, a.account_id, b.account_id)) return false;
        }
        return true;
    }

    fn post(
        *_,
        timestamp_ms: u64,
        idempotency_key: string,
        memo: string,
        lines: []const JournalLine,
    ): u64 ! LedgerError {
        // 1. Idempotency check
        if (idempotency_key.len > 0) {
            if (existing_tx_id := _.idempotency_index.get(idempotency_key)) {
                const entry_idx = _.tx_index.get(existing_tx_id) ?? return LedgerError.TransactionNotFound;
                const existing = _.entries.items[entry_idx];
                if (Ledger.linesMatch(existing.lines, lines)) {
                    return existing_tx_id;
                }
                return LedgerError.DuplicateIdempotencyKey;
            }
        }

        // 2. Structural & currency & balance validation (zero mutation)
        if (lines.len == 0) {
            return LedgerError.UnbalancedEntry;
        }

        let debit_sum: i128 = 0;
        let credit_sum: i128 = 0;
        let ref_currency: [3]u8 = undefined;
        let has_ref_currency = false;

        for (line of lines) {
            if (line.amount <= 0) {
                return LedgerError.InvalidAmount;
            }
            const acc_idx = _.account_index.get(line.account_id) ?? return LedgerError.AccountNotFound;
            const acc = _.accounts.items[acc_idx];
            if (!has_ref_currency) {
                ref_currency = acc.currency;
                has_ref_currency = true;
            } else {
                if (!std.mem.eql(u8, &acc.currency, &ref_currency)) {
                    return LedgerError.CurrencyMismatch;
                }
            }
            switch (line.side) {
                .Debit => debit_sum += line.amount,
                .Credit => credit_sum += line.amount,
            }
        }

        if (debit_sum != credit_sum) {
            return LedgerError.UnbalancedEntry;
        }

        // 3. Projected balance check for accounts with allow_negative == false (zero mutation)
        let i: usize = 0;
        while (i < lines.len) : (i += 1) {
            const line = lines[i];
            const acc_idx = _.account_index.get(line.account_id).?;
            const acc = _.accounts.items[acc_idx];
            if (!acc.allow_negative) {
                let d_delta: i128 = 0;
                let c_delta: i128 = 0;
                for (other of lines) {
                    if (std.mem.eql(u8, other.account_id, acc.id)) {
                        switch (other.side) {
                            .Debit => d_delta += other.amount,
                            .Credit => c_delta += other.amount,
                        }
                    }
                }
                const proj_d = acc.debit_total + d_delta;
                const proj_c = acc.credit_total + c_delta;
                const proj_net = if (acc.account_type.isDebitNormal()) (proj_d - proj_c) else (proj_c - proj_d);
                if (proj_net < 0) {
                    return LedgerError.InsufficientFunds;
                }
            }
        }

        // 4. Commit atomically
        const tx_id = _.next_tx_id;
        const owned_key = _.ownSlice(idempotency_key)?;
        const owned_memo = _.ownSlice(memo)?;

        let lines_copy = _.alloc.alloc(JournalLine, lines.len) catch return LedgerError.OutOfMemory;
        errdefer _.alloc.free(lines_copy);

        let k: usize = 0;
        while (k < lines.len) : (k += 1) {
            const acc_idx = _.account_index.get(lines[k].account_id).?;
            lines_copy[k] = JournalLine{
                account_id: _.accounts.items[acc_idx].id,
                side: lines[k].side,
                amount: lines[k].amount,
            };
        }

        _.owned_line_slices.append(_.alloc, lines_copy) catch return LedgerError.OutOfMemory;

        const entry_idx = _.entries.items.len;
        _.entries.append(_.alloc, JournalEntry{
            tx_id: tx_id,
            timestamp_ms: timestamp_ms,
            idempotency_key: owned_key,
            memo: owned_memo,
            lines: lines_copy,
            reversed_by_tx_id: null,
        }) catch return LedgerError.OutOfMemory;

        _.tx_index.put(_.alloc, tx_id, entry_idx) catch {
            * = _.entries.pop();
            return LedgerError.OutOfMemory;
        };

        if (owned_key.len > 0) {
            _.idempotency_index.put(_.alloc, owned_key, tx_id) catch {
                * = _.tx_index.remove(tx_id);
                * = _.entries.pop();
                return LedgerError.OutOfMemory;
            };
        }

        for (line of lines_copy) {
            const acc_idx = _.account_index.get(line.account_id).?;
            switch (line.side) {
                .Debit => _.accounts.items[acc_idx].debit_total += line.amount,
                .Credit => _.accounts.items[acc_idx].credit_total += line.amount,
            }
        }

        _.next_tx_id += 1;
        return tx_id;
    }

    fn reverseTransaction(*_, tx_id: u64, timestamp_ms: u64, memo: string): u64 ! LedgerError {
        const entry_idx = _.tx_index.get(tx_id) ?? return LedgerError.TransactionNotFound;
        const orig = _.entries.items[entry_idx];
        if (orig.reversed_by_tx_id != null) {
            return LedgerError.AlreadyReversed;
        }

        let rev_lines = _.alloc.alloc(JournalLine, orig.lines.len) catch return LedgerError.OutOfMemory;
        defer _.alloc.free(rev_lines);

        let i: usize = 0;
        while (i < orig.lines.len) : (i += 1) {
            rev_lines[i] = JournalLine{
                account_id: orig.lines[i].account_id,
                side: orig.lines[i].side.opposite(),
                amount: orig.lines[i].amount,
            };
        }

        const rev_tx_id = _.post(timestamp_ms, "", memo, rev_lines)?;
        _.entries.items[entry_idx].reversed_by_tx_id = rev_tx_id;
        return rev_tx_id;
    }

    fn balanceOf(account_id: string): i128 ! LedgerError {
        const acc_idx = _.account_index.get(account_id) ?? return LedgerError.AccountNotFound;
        return _.accounts.items[acc_idx].netBalance();
    }

    fn getAccount(account_id: string): Account ! LedgerError {
        const acc_idx = _.account_index.get(account_id) ?? return LedgerError.AccountNotFound;
        return _.accounts.items[acc_idx];
    }

    fn getEntry(tx_id: u64): JournalEntry ! LedgerError {
        const entry_idx = _.tx_index.get(tx_id) ?? return LedgerError.TransactionNotFound;
        return _.entries.items[entry_idx];
    }

    fn statementForAccount(alloc: std.mem.Allocator, account_id: string): []StatementLine ! LedgerError {
        const acc_idx = _.account_index.get(account_id) ?? return LedgerError.AccountNotFound;
        const acc = _.accounts.items[acc_idx];
        const is_debit_normal = acc.account_type.isDebitNormal();

        let out: std.ArrayListUnmanaged(StatementLine) = .empty;
        errdefer out.deinit(alloc);

        let running: i128 = 0;
        for (entry of _.entries.items) {
            for (line of entry.lines) {
                if (std.mem.eql(u8, line.account_id, acc.id)) {
                    const delta: i128 = switch (line.side) {
                        .Debit => if (is_debit_normal) line.amount else -line.amount,
                        .Credit => if (is_debit_normal) -line.amount else line.amount,
                    };
                    running += delta;
                    out.append(alloc, StatementLine{
                        tx_id: entry.tx_id,
                        timestamp_ms: entry.timestamp_ms,
                        memo: entry.memo,
                        side: line.side,
                        amount: line.amount,
                        running_balance: running,
                        reversed_by_tx_id: entry.reversed_by_tx_id,
                    }) catch return LedgerError.OutOfMemory;
                }
            }
        }

        return out.toOwnedSlice(alloc) catch return LedgerError.OutOfMemory;
    }

    fn verifyTrialBalance(currency: [3]u8): TrialBalance {
        let total_debits: i128 = 0;
        let total_credits: i128 = 0;
        let assets: i128 = 0;
        let liabilities: i128 = 0;
        let equity: i128 = 0;
        let revenue: i128 = 0;
        let expenses: i128 = 0;

        for (acc of _.accounts.items) {
            if (!std.mem.eql(u8, &acc.currency, &currency)) {
                continue;
            }
            total_debits += acc.debit_total;
            total_credits += acc.credit_total;
            const net = acc.netBalance();
            switch (acc.account_type) {
                .Asset => assets += net,
                .Liability => liabilities += net,
                .Equity => equity += net,
                .Revenue => revenue += net,
                .Expense => expenses += net,
            }
        }

        const net_income = revenue - expenses;
        const balanced = (total_debits == total_credits);
        const eq_ok = (assets == liabilities + equity + net_income);

        return TrialBalance{
            balanced: balanced,
            total_debits: total_debits,
            total_credits: total_credits,
            assets_eq_liab_equity_plus_net_income: eq_ok,
        };
    }
}
