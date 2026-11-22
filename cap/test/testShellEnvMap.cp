import { test, expect } from "cap:test";
import {
    ENV_MAP_MAX_ENTRIES,
    EnvStr,
    EnvStrPool,
    ShellEnvMap,
} from "cap:shell";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("POSIX case-sensitive insert, overwrite, and self-overwrite refcount safety", () => {
    let pool = EnvStrPool.init();
    let env = ShellEnvMap.init(false);

    const k_upper = pool.dupeRefCounted("PATH") catch unreachable;
    const k_mixed = pool.dupeRefCounted("Path") catch unreachable;
    const v1 = pool.dupeRefCounted("/usr/bin") catch unreachable;
    const v2 = pool.dupeRefCounted("/opt/bin") catch unreachable;

    expect(env.insert(&pool, k_upper, v1) catch unreachable);
    expect(env.insert(&pool, k_mixed, v2) catch unreachable);
    expect(env.len() == 2);

    // Caller drops its own refs; map holds rc == 1 for each
    * = pool.deref(k_upper) catch unreachable;
    * = pool.deref(k_mixed) catch unreachable;
    * = pool.deref(v1) catch unreachable;
    * = pool.deref(v2) catch unreachable;
    expect((pool.refCount(v1) catch unreachable) == 1);

    // Self-overwrite (same value handle) must ref before deref so rc stays 1
    expect(!(env.insert(&pool, k_upper, v1) catch unreachable));
    expect((pool.refCount(v1) catch unreachable) == 1);

    // Overwrite PATH with v2 -> v1 drops to rc 0, v2 rises to rc 2
    expect(!(env.insert(&pool, k_upper, v2) catch unreachable));
    expect((pool.refCount(v1) catch unreachable) == 0);
    expect((pool.refCount(v2) catch unreachable) == 2);

    if (got := env.getSlice(&pool, "PATH") catch unreachable) {
        expect(strEq(got, "/opt/bin"));
    } else {
        expect(false);
    }

    * = env.clear(&pool) catch unreachable;
    expect(pool.live_slots == 0);
});

test("Windows ASCII case-insensitive mode matches, updates, and removes regardless of case", () => {
    let pool = EnvStrPool.init();
    let env = ShellEnvMap.init(true);

    const k1 = EnvStr.initSlice("PATH");
    const v1 = EnvStr.initSlice("C:\\Windows\\System32");
    expect(env.insert(&pool, k1, v1) catch unreachable);

    if (got := env.getSlice(&pool, "path") catch unreachable) {
        expect(strEq(got, "C:\\Windows\\System32"));
    } else {
        expect(false);
    }

    // Overwrite via "Path" updates existing slot without growing map
    expect(!(env.insert(&pool, EnvStr.initSlice("Path"), EnvStr.initSlice("C:\\Bun")) catch unreachable));
    expect(env.len() == 1);
    if (got2 := env.getSlice(&pool, "PATH") catch unreachable) {
        expect(strEq(got2, "C:\\Bun"));
    } else {
        expect(false);
    }

    expect(env.remove(&pool, "pAtH") catch unreachable);
    expect(env.len() == 0);
    expect(!(env.remove(&pool, "PATH") catch unreachable));
});

test("clone isolates subshell mutations and achieves zero-leak refcount closure", () => {
    let pool = EnvStrPool.init();
    let parent = ShellEnvMap.init(false);

    const k_a = pool.dupeRefCounted("APP_ENV") catch unreachable;
    const v_prod = pool.dupeRefCounted("prod") catch unreachable;
    * = parent.insert(&pool, k_a, v_prod) catch unreachable;
    * = pool.deref(k_a) catch unreachable;
    * = pool.deref(v_prod) catch unreachable;

    let child = parent.clone(&pool) catch unreachable;
    expect((pool.refCount(k_a) catch unreachable) == 2);
    expect((pool.refCount(v_prod) catch unreachable) == 2);

    // Child overrides APP_ENV without affecting parent
    const v_dev = pool.dupeRefCounted("dev") catch unreachable;
    * = child.insert(&pool, EnvStr.initSlice("APP_ENV"), v_dev) catch unreachable;
    * = pool.deref(v_dev) catch unreachable;

    if (p_val := parent.getSlice(&pool, "APP_ENV") catch unreachable) {
        expect(strEq(p_val, "prod"));
    } else {
        expect(false);
    }
    if (c_val := child.getSlice(&pool, "APP_ENV") catch unreachable) {
        expect(strEq(c_val, "dev"));
    } else {
        expect(false);
    }

    * = child.clear(&pool) catch unreachable;
    * = parent.clear(&pool) catch unreachable;
    expect(pool.live_slots == 0);
});

test("get returns a ref-incremented handle that stays alive after map removal", () => {
    let pool = EnvStrPool.init();
    let env = ShellEnvMap.init(false);

    const k = pool.dupeRefCounted("TOKEN") catch unreachable;
    const v = pool.dupeRefCounted("abc123") catch unreachable;
    * = env.insert(&pool, k, v) catch unreachable;
    * = pool.deref(k) catch unreachable;
    * = pool.deref(v) catch unreachable;

    if (fetched := env.get(&pool, EnvStr.initSlice("TOKEN")) catch unreachable) {
        expect((pool.refCount(fetched) catch unreachable) == 2);
        * = env.remove(&pool, "TOKEN") catch unreachable;
        // Still valid because caller holds a ref
        expect((pool.refCount(fetched) catch unreachable) == 1);
        expect(strEq(pool.slice(fetched) catch unreachable, "abc123"));
        * = pool.deref(fetched) catch unreachable;
    } else {
        expect(false);
    }
    expect(pool.live_slots == 0);
});

test("formatSortedExports outputs lexicographically sorted KEY=VALUE lines and tracks memoryCost", () => {
    let pool = EnvStrPool.init();
    let env = ShellEnvMap.init(false);

    * = env.insert(&pool, EnvStr.initSlice("ZETA"), EnvStr.initSlice("3")) catch unreachable;
    * = env.insert(&pool, EnvStr.initSlice("ALPHA"), EnvStr.initSlice("1")) catch unreachable;
    * = env.insert(&pool, EnvStr.initSlice("MID"), EnvStr.initSlice("2")) catch unreachable;

    let buf: [128]u8 = [0; 128];
    const n = env.formatSortedExports(&pool, buf[0..]) catch unreachable;
    expect(strEq(buf[0..n], "ALPHA=1\nMID=2\nZETA=3\n"));
    expect(env.memoryCost(&pool) > 64);
});

test("Boundary guards: EmptyKey, MapFull, and BufferTooSmall", () => {
    let pool = EnvStrPool.init();
    let env = ShellEnvMap.init(false);

    let saw_empty = false;
    * = env.insert(&pool, EnvStr.empty(), EnvStr.initSlice("v")) catch {
        saw_empty = true;
    };
    expect(saw_empty);

    const keys: [64][]const u8 = [
        "k00", "k01", "k02", "k03", "k04", "k05", "k06", "k07",
        "k08", "k09", "k10", "k11", "k12", "k13", "k14", "k15",
        "k16", "k17", "k18", "k19", "k20", "k21", "k22", "k23",
        "k24", "k25", "k26", "k27", "k28", "k29", "k30", "k31",
        "k32", "k33", "k34", "k35", "k36", "k37", "k38", "k39",
        "k40", "k41", "k42", "k43", "k44", "k45", "k46", "k47",
        "k48", "k49", "k50", "k51", "k52", "k53", "k54", "k55",
        "k56", "k57", "k58", "k59", "k60", "k61", "k62", "k63",
    ];
    for (i of 0..ENV_MAP_MAX_ENTRIES) {
        * = env.insert(&pool, EnvStr.initSlice(keys[i]), EnvStr.initSlice("1")) catch unreachable;
    }
    let saw_full = false;
    * = env.insert(&pool, EnvStr.initSlice("k64"), EnvStr.initSlice("1")) catch {
        saw_full = true;
    };
    expect(saw_full);

    let tiny: [4]u8 = [0; 4];
    let saw_small = false;
    * = env.formatSortedExports(&pool, tiny[0..]) catch {
        saw_small = true;
    };
    expect(saw_small);
});
