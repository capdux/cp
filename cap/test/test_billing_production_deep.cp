// Source: cp/cap/test/test_billing_production_deep.cp
// Deep Production Test Suite for cap:billing
// Covers:
//   1. Graduated vs Volume tiered pricing (AI tokens + flat fees) & non-monotonic InvalidTiers guard
//   2. calculateProration midpoint upgrade/downgrade exact symmetry & InvalidPeriod guard
//   3. buildInvoice with subscription + metered usage + PercentBps/FixedAmount coupons (no negative total) + tax_bps
//   4. Invoice.toJson RFC 8259 validation via cap:json with DebugAllocator zero-leak verification
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.billing`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { JSON } from "cap:json";
import {
    BillingError,
    TierMode,
    CouponKind,
    PriceTier,
    Coupon,
    InvoiceLineItem,
    Invoice,
    BillingCalculator,
} from "cap:billing";
import cap from "cap";

fn testGraduatedVsVolumeAndInvalidTiers()! {
    const tiers: []const PriceTier = [
        PriceTier{ up_to_units: 1000, unit_price_micros: 2000, flat_fee_micros: 0 },
        PriceTier{ up_to_units: 5000, unit_price_micros: 1500, flat_fee_micros: 50000 },
        PriceTier{ up_to_units: null, unit_price_micros: 1000, flat_fee_micros: 100000 },
    ];

    // Zero usage must always cost 0 after tier validation.
    const zero_grad = BillingCalculator.priceMeteredUsage(0, tiers, .Graduated)?;
    const zero_vol = BillingCalculator.priceMeteredUsage(0, tiers, .Volume)?;
    expect_eq(zero_grad, 0 as i64);
    expect_eq(zero_vol, 0 as i64);

    // 6,000 AI tokens:
    // Graduated = 1000*2000 + (4000*1500 + 50000) + (1000*1000 + 100000)
    //           = 2_000_000 + 6_050_000 + 1_100_000 = 9_150_000 micros ($9.15)
    const grad_cost = BillingCalculator.priceMeteredUsage(6000, tiers, .Graduated)?;
    expect_eq(grad_cost, 9150000 as i64);
    expect_eq(BillingCalculator.microsToCents(grad_cost), 915 as i64);

    // Volume = 6000*1000 + 100000 = 6_100_000 micros ($6.10)
    const vol_cost = BillingCalculator.priceMeteredUsage(6000, tiers, .Volume)?;
    expect_eq(vol_cost, 6100000 as i64);
    expect_eq(BillingCalculator.microsToCents(vol_cost), 610 as i64);

    // Guard: empty tiers
    const empty_tiers: []const PriceTier = [];
    let hit_empty = false;
    * = BillingCalculator.priceMeteredUsage(100, empty_tiers, .Graduated) catch e => {
        if (e == BillingError.InvalidTiers) hit_empty = true;
    };
    expect_eq(hit_empty, true);

    // Guard: non-monotonic tiers (5000 -> 3000)
    const non_mono: []const PriceTier = [
        PriceTier{ up_to_units: 5000, unit_price_micros: 100, flat_fee_micros: 0 },
        PriceTier{ up_to_units: 3000, unit_price_micros: 50, flat_fee_micros: 0 },
        PriceTier{ up_to_units: null, unit_price_micros: 20, flat_fee_micros: 0 },
    ];
    let hit_non_mono = false;
    * = BillingCalculator.priceMeteredUsage(4000, non_mono, .Graduated) catch e => {
        if (e == BillingError.InvalidTiers) hit_non_mono = true;
    };
    expect_eq(hit_non_mono, true);

    // Guard: final tier not uncapped (last up_to_units != null)
    const capped_end: []const PriceTier = [
        PriceTier{ up_to_units: 1000, unit_price_micros: 100, flat_fee_micros: 0 },
        PriceTier{ up_to_units: 2000, unit_price_micros: 50, flat_fee_micros: 0 },
    ];
    let hit_capped = false;
    * = BillingCalculator.priceMeteredUsage(500, capped_end, .Volume) catch e => {
        if (e == BillingError.InvalidTiers) hit_capped = true;
    };
    expect_eq(hit_capped, true);

    // Guard: uncapped tier in middle
    const mid_null: []const PriceTier = [
        PriceTier{ up_to_units: null, unit_price_micros: 100, flat_fee_micros: 0 },
        PriceTier{ up_to_units: null, unit_price_micros: 50, flat_fee_micros: 0 },
    ];
    let hit_mid_null = false;
    * = BillingCalculator.priceMeteredUsage(500, mid_null, .Graduated) catch e => {
        if (e == BillingError.InvalidTiers) hit_mid_null = true;
    };
    expect_eq(hit_mid_null, true);
}

fn testProrationSymmetryAndGuards()! {
    const start_ms: i64 = 1700000000000;
    const end_ms: i64 = start_ms + 30 * 24 * 60 * 60 * 1000;
    const mid_ms: i64 = start_ms + 15 * 24 * 60 * 60 * 1000;

    // Midpoint upgrade ($19.00 -> $49.00) = +$15.00 (+1500 cents)
    const upgrade = BillingCalculator.calculateProration(1900, 4900, start_ms, end_ms, mid_ms)?;
    expect_eq(upgrade, 1500 as i64);

    // Midpoint downgrade ($49.00 -> $19.00) = -$15.00 (-1500 cents)
    const downgrade = BillingCalculator.calculateProration(4900, 1900, start_ms, end_ms, mid_ms)?;
    expect_eq(downgrade, -1500 as i64);
    expect_eq(upgrade, -downgrade);

    // Odd-duration rounding symmetry check
    const odd_up = BillingCalculator.calculateProration(100, 200, 0, 3, 1)?;
    const odd_down = BillingCalculator.calculateProration(200, 100, 0, 3, 1)?;
    expect_eq(odd_up, 67 as i64);
    expect_eq(odd_down, -67 as i64);
    expect_eq(odd_up, -odd_down);

    // Boundary switches: at start -> full delta, at end -> 0
    expect_eq(BillingCalculator.calculateProration(1000, 3000, start_ms, end_ms, start_ms)?, 2000 as i64);
    expect_eq(BillingCalculator.calculateProration(1000, 3000, start_ms, end_ms, end_ms)?, 0 as i64);

    // Guards: InvalidPeriod
    let hit_reversed = false;
    * = BillingCalculator.calculateProration(1000, 2000, 200, 100, 150) catch e => {
        if (e == BillingError.InvalidPeriod) hit_reversed = true;
    };
    expect_eq(hit_reversed, true);

    let hit_before = false;
    * = BillingCalculator.calculateProration(1000, 2000, 100, 200, 99) catch e => {
        if (e == BillingError.InvalidPeriod) hit_before = true;
    };
    expect_eq(hit_before, true);

    let hit_after = false;
    * = BillingCalculator.calculateProration(1000, 2000, 100, 200, 201) catch e => {
        if (e == BillingError.InvalidPeriod) hit_after = true;
    };
    expect_eq(hit_after, true);
}

fn testBuildInvoiceDiscountsAndTaxes()! {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    const alloc = gpa.allocator();

    {
        const token_tiers: []const PriceTier = [
            PriceTier{ up_to_units: 1000, unit_price_micros: 2000, flat_fee_micros: 0 },
            PriceTier{ up_to_units: null, unit_price_micros: 1000, flat_fee_micros: 0 },
        ];
        const token_micros = BillingCalculator.priceMeteredUsage(2500, token_tiers, .Graduated)?;
        const token_cents = BillingCalculator.microsToCents(token_micros); // 2_000_000 + 1_500_000 = 350 cents
        expect_eq(token_cents, 350 as i64);

        const proration_cents = BillingCalculator.calculateProration(2900, 4900, 0, 1000, 500)?; // +1000 cents
        expect_eq(proration_cents, 1000 as i64);

        const lines: []const InvoiceLineItem = [
            InvoiceLineItem{ description: "Base Subscription", quantity: 1, amount_cents: 4900 },
            InvoiceLineItem{ description: "AI Token Usage", quantity: 2500, amount_cents: token_cents },
            InvoiceLineItem{ description: "Mid-Cycle Plan Upgrade", quantity: 1, amount_cents: proration_cents },
        ];

        // Subtotal = 4900 + 350 + 1000 = 6250 cents
        // PercentBps = 2000 bps (20%) => discount = 1250 cents
        // Taxable = 5000 cents, Tax = 1000 bps (10%) => tax = 500 cents
        // Total = 5500 cents
        const pct_coupon = Coupon{ code: "SAVE20", kind: .PercentBps, value: 2000 };
        let inv1 = BillingCalculator.buildInvoice(alloc, "cust_ai_01", "USD", lines, pct_coupon, 1000)?;
        defer inv1.deinit();

        expect_eq(inv1.subtotal_cents, 6250 as i64);
        expect_eq(inv1.discount_cents, 1250 as i64);
        expect_eq(inv1.tax_cents, 500 as i64);
        expect_eq(inv1.total_cents, 5500 as i64);

        // Oversized FixedAmount coupon must clamp discount to subtotal so total never goes negative
        const huge_coupon = Coupon{ code: "PROMO100USD", kind: .FixedAmount, value: 10000 };
        let inv2 = BillingCalculator.buildInvoice(alloc, "cust_ai_02", "USD", lines, huge_coupon, 1000)?;
        defer inv2.deinit();

        expect_eq(inv2.subtotal_cents, 6250 as i64);
        expect_eq(inv2.discount_cents, 6250 as i64);
        expect_eq(inv2.tax_cents, 0 as i64);
        expect_eq(inv2.total_cents, 0 as i64);

        // Guards: InvalidDiscount & InvalidTaxRate
        let hit_bad_disc = false;
        const bad_coupon = Coupon{ code: "BAD", kind: .PercentBps, value: 15000 };
        * = BillingCalculator.buildInvoice(alloc, "cust", "USD", lines, bad_coupon, 0) catch e => {
            if (e == BillingError.InvalidDiscount) hit_bad_disc = true;
        };
        expect_eq(hit_bad_disc, true);

        let hit_bad_tax = false;
        * = BillingCalculator.buildInvoice(alloc, "cust", "USD", lines, null, 10001) catch e => {
            if (e == BillingError.InvalidTaxRate) hit_bad_tax = true;
        };
        expect_eq(hit_bad_tax, true);
    }
    expect_eq(gpa.deinit() == .ok, true);
}

fn testInvoiceToJsonWithCapJsonAndZeroLeak()! {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    const alloc = gpa.allocator();

    {
        const lines: []const InvoiceLineItem = [
            InvoiceLineItem{ description: "Pro \"AI\" Plan\\v2", quantity: 1, amount_cents: 5000 },
            InvoiceLineItem{ description: "Vector Storage (GB)", quantity: 40, amount_cents: 800 },
        ];
        const coupon = Coupon{ code: "VIP500", kind: .FixedAmount, value: 500 };

        let inv = BillingCalculator.buildInvoice(alloc, "cust_json_42", "EUR", lines, coupon, 2000)?;
        defer inv.deinit();

        // Subtotal = 5800, discount = 500, taxable = 5300, tax (20%) = 1060, total = 6360
        expect_eq(inv.total_cents, 6360 as i64);

        const json_bytes = inv.toJson(alloc)?;
        defer alloc.free(json_bytes);

        let json_arena = std.heap.ArenaAllocator.init(alloc);
        defer json_arena.deinit();
        const parsed = JSON.parse(json_arena.allocator(), json_bytes)?;

        if (cid := parsed.get("customer_id")) {
            expect_eq(std.mem.eql(u8, cid.asString().?, "cust_json_42"), true);
        } else {
            return error.MissingCustomerId;
        }

        if (curr := parsed.get("currency")) {
            expect_eq(std.mem.eql(u8, curr.asString().?, "EUR"), true);
        } else {
            return error.MissingCurrency;
        }

        if (tot := parsed.get("total_cents")) {
            expect_eq(tot.asNumber().?, 6360.0);
        } else {
            return error.MissingTotal;
        }

        if (arr := parsed.get("lines")) {
            const items = arr.asArray().?;
            expect_eq(items.len, 2 as usize);
            const first_desc = items[0].get("description").?.asString().?;
            expect_eq(std.mem.eql(u8, first_desc, "Pro \"AI\" Plan\\v2"), true);
        } else {
            return error.MissingLines;
        }
    }
    expect_eq(gpa.deinit() == .ok, true);
}

fn testCapBillingDynamicLazyMount()! {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    const alloc = gpa.allocator();

    {
        const tiers: []const cap.billing.PriceTier = [
            cap.billing.PriceTier{ up_to_units: 100, unit_price_micros: 10000, flat_fee_micros: 0 },
            cap.billing.PriceTier{ up_to_units: null, unit_price_micros: 5000, flat_fee_micros: 0 },
        ];
        const micros = cap.billing.BillingCalculator.priceMeteredUsage(200, tiers, cap.billing.TierMode.Graduated)?;
        expect_eq(micros, 1500000 as i64); // 100*10000 + 100*5000 = 1,500,000 micros ($1.50)

        const prorated = cap.billing.BillingCalculator.calculateProration(1000, 5000, 0, 1000, 750)?;
        expect_eq(prorated, 1000 as i64); // 25% remaining * 4000 = 1000

        const items: []const cap.billing.InvoiceLineItem = [
            cap.billing.InvoiceLineItem{ description: "Metered API", quantity: 200, amount_cents: cap.billing.BillingCalculator.microsToCents(micros) },
            cap.billing.InvoiceLineItem{ description: "Proration", quantity: 1, amount_cents: prorated },
        ];
        const coupon = cap.billing.Coupon{ code: "FLAT150", kind: cap.billing.CouponKind.FixedAmount, value: 150 };
        let inv = cap.billing.BillingCalculator.buildInvoice(alloc, "cust_cap", "USD", items, coupon, 0)?;
        defer inv.deinit();

        // 150 + 1000 - 150 = 1000
        expect_eq(inv.total_cents, 1000 as i64);
    }
    expect_eq(gpa.deinit() == .ok, true);
}

pub fn main()! {
    test("1. Graduated vs Volume Tiered Pricing & Non-Monotonic InvalidTiers Guards", () => {
        testGraduatedVsVolumeAndInvalidTiers()?;
    });

    test("2. Subscription Proration Midpoint Upgrade/Downgrade Symmetry & InvalidPeriod Guards", () => {
        testProrationSymmetryAndGuards()?;
    });

    test("3. Invoice Building with Usage, Proration, PercentBps/FixedAmount Coupons & Tax", () => {
        testBuildInvoiceDiscountsAndTaxes()?;
    });

    test("4. Invoice.toJson RFC 8259 Validation via cap:json & Zero-Leak DebugAllocator", () => {
        testInvoiceToJsonWithCapJsonAndZeroLeak()?;
    });

    test("5. Dynamic Lazy Namespace Mounting via `import cap from \"cap\"; cap.billing`", () => {
        testCapBillingDynamicLazyMount()?;
    });
}
