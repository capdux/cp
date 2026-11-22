// Source: cp/lib/billing/billing.cp (Quantitative Billing, Tiered Pricing, Proration & Invoicing)
// Pure CP, zero-C-dependency billing engine using integer minor units (cents / micros)
// for exact metered pricing, subscription proration, coupon discounts, taxes, and JSON invoicing.

const std = @import("std");

/// Basis points denominator (10,000 bps = 100.00%).
pub const BPS_DENOMINATOR: i128 = 10000;

/// Micro-units per cent (1 cent = 10,000 micros; 1 USD = 1,000,000 micros).
pub const MICROS_PER_CENT: i128 = 10000;

/// Error set for billing validation, pricing, proration, and invoice generation.
pub err BillingError {
    InvalidTiers,
    InvalidPeriod,
    InvalidDiscount,
    NegativeQuantity,
    InvalidTaxRate,
    OutOfMemory,
}

/// Tiered pricing evaluation mode.
pub enum TierMode {
    /// Each tier applies only to the units falling within that tier's bracket.
    Graduated,
    /// All units are priced at the single tier matched by total volume.
    Volume,
}

/// Discount coupon calculation strategy.
pub enum CouponKind {
    /// Percentage discount expressed in basis points (0..=10000, e.g., 1500 = 15.00%).
    PercentBps,
    /// Fixed monetary discount in integer cents.
    FixedAmount,
}

/// Single pricing tier bracket for metered usage.
/// `up_to_units == null` marks the uncapped final tier.
pub struct PriceTier {
    up_to_units: u64? = null;
    unit_price_micros: i64 = 0;
    flat_fee_micros: i64 = 0;
}

/// Promotional or contractual discount coupon.
pub struct Coupon {
    code: []const u8;
    kind: CouponKind;
    value: i64;
}

/// Single billable line item on an invoice.
pub struct InvoiceLineItem {
    description: []const u8;
    quantity: u64;
    amount_cents: i64;
}

// Symmetric half-up integer division for exact monetary rounding.
fn divRoundHalfUp(num: i128, den: i128): i128 {
    if (num == 0) return 0;
    const is_neg = (num < 0) != (den < 0);
    const abs_num: i128 = if (num < 0) -num else num;
    const abs_den: i128 = if (den < 0) -den else den;
    const rounded: i128 = @divTrunc(abs_num * 2 + abs_den, 2 * abs_den);
    if (is_neg) return -rounded;
    return rounded;
}

fn validateTiers(tiers: []const PriceTier): void ! BillingError {
    if (tiers.len == 0) return BillingError.InvalidTiers;

    let prev_up_to: u64 = 0;
    for (i of 0..tiers.len) {
        const tier = tiers[i];
        if (tier.unit_price_micros < 0 || tier.flat_fee_micros < 0) {
            return BillingError.InvalidTiers;
        }

        const is_last = (i + 1 == tiers.len);
        if (is_last) {
            // Final tier must be uncapped (`up_to_units == null`).
            if (tier.up_to_units != null) {
                return BillingError.InvalidTiers;
            }
        } else {
            // Intermediate tiers must have strictly increasing finite upper bounds.
            if (upper := tier.up_to_units) {
                if (upper <= prev_up_to) {
                    return BillingError.InvalidTiers;
                }
                prev_up_to = upper;
            } else {
                return BillingError.InvalidTiers;
            }
        }
    }
}

fn appendJsonString(out: *std.ArrayList(u8), alloc: std.mem.Allocator, s: []const u8): void ! BillingError {
    out.append(alloc, '"') catch return BillingError.OutOfMemory;
    for (ch of s) {
        switch (ch) {
            '"' => out.appendSlice(alloc, "\\\"") catch return BillingError.OutOfMemory,
            '\\' => out.appendSlice(alloc, "\\\\") catch return BillingError.OutOfMemory,
            '\n' => out.appendSlice(alloc, "\\n") catch return BillingError.OutOfMemory,
            '\r' => out.appendSlice(alloc, "\\r") catch return BillingError.OutOfMemory,
            '\t' => out.appendSlice(alloc, "\\t") catch return BillingError.OutOfMemory,
            else => {
                if (ch < 0x20) {
                    let esc_buf: [6]u8 = undefined;
                    const esc = std.fmt.bufPrint(&esc_buf, "\\u{x:0>4}", .{ch}) catch return BillingError.OutOfMemory;
                    out.appendSlice(alloc, esc) catch return BillingError.OutOfMemory;
                } else {
                    out.append(alloc, ch) catch return BillingError.OutOfMemory;
                }
            },
        }
    }
    out.append(alloc, '"') catch return BillingError.OutOfMemory;
}

fn appendFmt(out: *std.ArrayList(u8), alloc: std.mem.Allocator, comptime fmt: []const u8, args: auto): void ! BillingError {
    let buf: [64]u8 = undefined;
    const slice = std.fmt.bufPrint(&buf, fmt, args) catch return BillingError.OutOfMemory;
    out.appendSlice(alloc, slice) catch return BillingError.OutOfMemory;
}

/// Structured invoice containing line items, subtotal, discount, tax, and total in cents.
pub class Invoice {
    alloc: std.mem.Allocator;
    pub customer_id: []const u8;
    pub currency: []const u8;
    pub lines: []InvoiceLineItem;
    pub subtotal_cents: i64;
    pub discount_cents: i64;
    pub tax_cents: i64;
    pub total_cents: i64;
    pub coupon_code: ([]const u8)? = null;
    pub tax_bps: u32 = 0;

    /// Releases heap-allocated invoice line items.
    pub fn deinit(*_) {
        _.alloc.free(_.lines);
    }

    /// Serializes the invoice to an RFC 8259 JSON string allocated via `alloc`.
    pub fn toJson(alloc: std.mem.Allocator): ([]u8) ! BillingError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        out.appendSlice(alloc, "{\"customer_id\":") catch return BillingError.OutOfMemory;
        appendJsonString(&out, alloc, _.customer_id)?;
        out.appendSlice(alloc, ",\"currency\":") catch return BillingError.OutOfMemory;
        appendJsonString(&out, alloc, _.currency)?;

        out.appendSlice(alloc, ",\"subtotal_cents\":") catch return BillingError.OutOfMemory;
        appendFmt(&out, alloc, "{d}", .{_.subtotal_cents})?;

        out.appendSlice(alloc, ",\"discount_cents\":") catch return BillingError.OutOfMemory;
        appendFmt(&out, alloc, "{d}", .{_.discount_cents})?;

        out.appendSlice(alloc, ",\"tax_bps\":") catch return BillingError.OutOfMemory;
        appendFmt(&out, alloc, "{d}", .{_.tax_bps})?;

        out.appendSlice(alloc, ",\"tax_cents\":") catch return BillingError.OutOfMemory;
        appendFmt(&out, alloc, "{d}", .{_.tax_cents})?;

        out.appendSlice(alloc, ",\"total_cents\":") catch return BillingError.OutOfMemory;
        appendFmt(&out, alloc, "{d}", .{_.total_cents})?;

        if (code := _.coupon_code) {
            out.appendSlice(alloc, ",\"coupon_code\":") catch return BillingError.OutOfMemory;
            appendJsonString(&out, alloc, code)?;
        }

        out.appendSlice(alloc, ",\"lines\":[") catch return BillingError.OutOfMemory;
        for (i of 0.._.lines.len) {
            if (i > 0) {
                out.append(alloc, ',') catch return BillingError.OutOfMemory;
            }
            const item = _.lines[i];
            out.appendSlice(alloc, "{\"description\":") catch return BillingError.OutOfMemory;
            appendJsonString(&out, alloc, item.description)?;
            out.appendSlice(alloc, ",\"quantity\":") catch return BillingError.OutOfMemory;
            appendFmt(&out, alloc, "{d}", .{item.quantity})?;
            out.appendSlice(alloc, ",\"amount_cents\":") catch return BillingError.OutOfMemory;
            appendFmt(&out, alloc, "{d}", .{item.amount_cents})?;
            out.append(alloc, '}') catch return BillingError.OutOfMemory;
        }
        out.appendSlice(alloc, "]}") catch return BillingError.OutOfMemory;

        return out.toOwnedSlice(alloc) catch return BillingError.OutOfMemory;
    }
}

/// Stateless quantitative billing, tiered pricing, proration, and invoice builder.
pub class BillingCalculator {
    /// Calculates metered usage cost in micro-units (`i64`) under `Graduated` or `Volume` tier mode.
    /// Validates that `tiers` is non-empty, non-negative, strictly increasing, and uncapped on the final tier.
    pub static fn priceMeteredUsage(units: u64, tiers: []const PriceTier, mode: TierMode): i64 ! BillingError {
        validateTiers(tiers)?;
        if (units == 0) return 0;

        switch (mode) {
            .Graduated => {
                let total_micros: i128 = 0;
                let prev_limit: u64 = 0;
                for (tier of tiers) {
                    if (units <= prev_limit) break;

                    const upper: u64 = tier.up_to_units ?? units;
                    const cap_units: u64 = if (units < upper) units else upper;
                    const bracket_units: u64 = cap_units - prev_limit;

                    total_micros += (tier.flat_fee_micros as i128) + (bracket_units as i128) * (tier.unit_price_micros as i128);

                    if (tier.up_to_units == null || units <= upper) break;
                    prev_limit = upper;
                }
                return total_micros as i64;
            },
            .Volume => {
                for (tier of tiers) {
                    let matched = true;
                    if (upper := tier.up_to_units) {
                        matched = (units <= upper);
                    }
                    if (matched) {
                        const total_micros: i128 = (tier.flat_fee_micros as i128) + (units as i128) * (tier.unit_price_micros as i128);
                        return total_micros as i64;
                    }
                }
                return BillingError.InvalidTiers;
            },
        }
    }

    /// Converts micro-units (1/1,000,000) to cents (1/100) using symmetric half-up rounding.
    pub static fn microsToCents(micros: i64): i64 {
        return divRoundHalfUp(micros as i128, MICROS_PER_CENT) as i64;
    }

    /// Calculates subscription proration in cents for switching plans at `switch_ms`
    /// within `[period_start_ms, period_end_ms]`.
    /// Returns positive charge for upgrades and negative credit for downgrades.
    pub static fn calculateProration(
        old_plan_cents: i64,
        new_plan_cents: i64,
        period_start_ms: i64,
        period_end_ms: i64,
        switch_ms: i64,
    ): i64 ! BillingError {
        if (old_plan_cents < 0 || new_plan_cents < 0) {
            return BillingError.NegativeQuantity;
        }
        if (period_end_ms <= period_start_ms) {
            return BillingError.InvalidPeriod;
        }
        if (switch_ms < period_start_ms || switch_ms > period_end_ms) {
            return BillingError.InvalidPeriod;
        }

        const total_duration: i128 = (period_end_ms - period_start_ms) as i128;
        const remaining_duration: i128 = (period_end_ms - switch_ms) as i128;
        const plan_delta: i128 = (new_plan_cents as i128) - (old_plan_cents as i128);

        return divRoundHalfUp(plan_delta * remaining_duration, total_duration) as i64;
    }

    /// Builds a structured `Invoice`, applying optional coupon discount (clamped so payable amount never drops below 0)
    /// and basis-point tax (`tax_bps <= 10000`, e.g., `1000` = 10%).
    pub static fn buildInvoice(
        alloc: std.mem.Allocator,
        customer_id: []const u8,
        currency: []const u8,
        items: []const InvoiceLineItem,
        coupon: Coupon?,
        tax_bps: u32,
    ): Invoice ! BillingError {
        if (tax_bps > 10000) {
            return BillingError.InvalidTaxRate;
        }

        let subtotal_i128: i128 = 0;
        for (item of items) {
            subtotal_i128 += item.amount_cents as i128;
        }

        let discount_i128: i128 = 0;
        let coupon_code: ([]const u8)? = null;
        if (c := coupon) {
            if (c.value < 0) {
                return BillingError.InvalidDiscount;
            }
            coupon_code = c.code;
            switch (c.kind) {
                .PercentBps => {
                    if (c.value > 10000) {
                        return BillingError.InvalidDiscount;
                    }
                    if (subtotal_i128 > 0) {
                        discount_i128 = divRoundHalfUp(subtotal_i128 * (c.value as i128), BPS_DENOMINATOR);
                    }
                },
                .FixedAmount => {
                    if (subtotal_i128 > 0) {
                        const raw_discount: i128 = c.value as i128;
                        discount_i128 = if (raw_discount > subtotal_i128) subtotal_i128 else raw_discount;
                    }
                },
            }
        }

        const taxable_i128: i128 = subtotal_i128 - discount_i128;
        let tax_i128: i128 = 0;
        if (taxable_i128 > 0 && tax_bps > 0) {
            tax_i128 = divRoundHalfUp(taxable_i128 * (tax_bps as i128), BPS_DENOMINATOR);
        }

        const total_i128: i128 = taxable_i128 + tax_i128;

        let owned_lines = alloc.alloc(InvoiceLineItem, items.len) catch return BillingError.OutOfMemory;
        for (i of 0..items.len) {
            owned_lines[i] = items[i];
        }

        return Invoice{
            alloc: alloc,
            customer_id: customer_id,
            currency: currency,
            lines: owned_lines,
            subtotal_cents: subtotal_i128 as i64,
            discount_cents: discount_i128 as i64,
            tax_cents: tax_i128 as i64,
            total_cents: total_i128 as i64,
            coupon_code: coupon_code,
            tax_bps: tax_bps,
        };
    }
}
