import { test, expect } from "cap:test";
import {
    ENV_STR_MAX_SLOTS,
    ENV_STR_ARENA_CAP,
    ENV_STR_INVALID_SLOT,
    EnvStrTag,
    EnvStr,
    EnvStrPool,
} from "cap:shell";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("Empty strings collapse to Empty tag without consuming pool slots", () => {
    let pool = EnvStrPool.init();

    const e1 = EnvStr.empty();
    const e2 = EnvStr.initSlice("");
    const e3 = pool.dupeRefCounted("") catch unreachable;

    expect(e1.tag == .Empty && e1.isEmpty() && !e1.isRefCounted());
    expect(e2.tag == .Empty && e2.isEmpty());
    expect(e3.tag == .Empty && e3.isEmpty());
    expect(pool.live_slots == 0);
    expect(pool.arena_used == 0);
    expect(strEq(pool.slice(e3) catch unreachable, ""));
    expect(pool.memoryCost(e3) == 0);
    expect((pool.ref(e3) catch unreachable) == 0);
    expect((pool.deref(e3) catch unreachable) == 0);
});

test("Borrowed Slice and RefCounted EnvStr interoperate with eql and slice", () => {
    let pool = EnvStrPool.init();

    const s_bor = EnvStr.initSlice("/usr/local/bin:/usr/bin");
    const s_rc = pool.dupeRefCounted("/usr/local/bin:/usr/bin") catch unreachable;
    const s_diff = pool.dupeRefCounted("/bin") catch unreachable;

    expect(s_bor.tag == .Slice && !s_bor.isRefCounted());
    expect(s_rc.tag == .RefCounted && s_rc.isRefCounted());
    expect(pool.live_slots == 2);

    expect(strEq(pool.slice(s_bor) catch unreachable, "/usr/local/bin:/usr/bin"));
    expect(strEq(pool.slice(s_rc) catch unreachable, "/usr/local/bin:/usr/bin"));
    expect(pool.eql(s_bor, s_rc) catch unreachable);
    expect(!(pool.eql(s_rc, s_diff) catch unreachable));
    expect((pool.ref(s_bor) catch unreachable) == 0);
    expect((pool.deref(s_bor) catch unreachable) == 0);
    expect(pool.memoryCost(s_bor) == 23);
});

test("Refcount lifecycle and amortized memoryCost division", () => {
    let pool = EnvStrPool.init();

    // 24-byte string: divisible cleanly by 1, 2, 3, 4
    const s = pool.dupeRefCounted("0123456789abcdef01234567") catch unreachable;
    expect(s.len == 24);
    expect((pool.refCount(s) catch unreachable) == 1);
    expect(pool.memoryCost(s) == 24);

    expect((pool.ref(s) catch unreachable) == 2);
    expect(pool.memoryCost(s) == 12);

    expect((pool.ref(s) catch unreachable) == 3);
    expect(pool.memoryCost(s) == 8);

    expect((pool.ref(s) catch unreachable) == 4);
    expect(pool.memoryCost(s) == 6);

    expect((pool.deref(s) catch unreachable) == 3);
    expect((pool.deref(s) catch unreachable) == 2);
    expect((pool.deref(s) catch unreachable) == 1);
    expect(pool.live_slots == 1);

    expect((pool.deref(s) catch unreachable) == 0);
    expect(pool.live_slots == 0);
    expect(pool.memoryCost(s) == 0);
});

test("Multiple refcounted strings coexist in arena without aliasing", () => {
    let pool = EnvStrPool.init();
    const k1 = pool.dupeRefCounted("NODE_ENV=production") catch unreachable;
    const k2 = pool.dupeRefCounted("BUN_INSTALL=/root/.bun") catch unreachable;
    const k3 = pool.dupeRefCounted("CI=true") catch unreachable;

    expect(strEq(pool.slice(k1) catch unreachable, "NODE_ENV=production"));
    expect(strEq(pool.slice(k2) catch unreachable, "BUN_INSTALL=/root/.bun"));
    expect(strEq(pool.slice(k3) catch unreachable, "CI=true"));
    expect(pool.arena_used == 19 + 22 + 7);
});

test("Released handles reject double deref, ref, and slice reads", () => {
    let pool = EnvStrPool.init();
    const s = pool.dupeRefCounted("secret_token") catch unreachable;
    * = pool.deref(s) catch unreachable;

    let saw_double_deref = false;
    * = pool.deref(s) catch {
        saw_double_deref = true;
    };
    expect(saw_double_deref);

    let saw_ref_after = false;
    * = pool.ref(s) catch {
        saw_ref_after = true;
    };
    expect(saw_ref_after);

    let saw_slice_after = false;
    * = pool.slice(s) catch {
        saw_slice_after = true;
    };
    expect(saw_slice_after);
});

test("Boundary guards: InvalidSlot, SlotTableFull, and ArenaFull", () => {
    let pool = EnvStrPool.init();

    const bogus = EnvStr{ tag: .RefCounted, slot_id: ENV_STR_INVALID_SLOT, len: 4 };
    let saw_invalid = false;
    * = pool.slice(bogus) catch {
        saw_invalid = true;
    };
    expect(saw_invalid);
    expect(pool.memoryCost(bogus) == 0);

    // Exhaust slot table
    for (i of 0..ENV_STR_MAX_SLOTS) {
        * = i;
        * = pool.dupeRefCounted("a") catch unreachable;
    }
    let saw_slot_full = false;
    * = pool.dupeRefCounted("b") catch {
        saw_slot_full = true;
    };
    expect(saw_slot_full);

    // Exhaust arena bytes
    let pool2 = EnvStrPool.init();
    const chunk: [256]u8 = ['x'; 256];
    const blocks = ENV_STR_ARENA_CAP / 256;
    for (b of 0..blocks) {
        * = b;
        * = pool2.dupeRefCounted(chunk[0..]) catch unreachable;
    }
    let saw_arena_full = false;
    * = pool2.dupeRefCounted("y") catch {
        saw_arena_full = true;
    };
    expect(saw_arena_full);
});
