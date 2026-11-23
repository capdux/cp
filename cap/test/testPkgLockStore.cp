import { test, assert, assertEq } from "cap:test";
import {
    LOCK_STORE_SNAPSHOT_VERSION,
    LockPackageStoreError,
    computeLockStoreCrc32,
    LockResolutionKind,
    lockResolutionKindName,
    LockBinKind,
    LockPackageMeta,
    LockPackageRecord,
    LockPackageInput,
    LockPackageStore,
    pruneUnreachablePackages,
    DuplicatePkgVersionEntry,
    findDuplicatePackageVersions,
    encodeLockStoreSnapshot,
    decodeLockStoreSnapshot,
} from "cap:pkg";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("LockPackageStore interns strings, deduplicates identical packages, and resolves multi-version names in O(1)", () => {
    let recs: [16]LockPackageRecord = [LockPackageRecord.empty(); 16];
    let edges: [32]u32 = [0; 32];
    let sbuf: [1024]u8 = [0; 1024];
    let buckets: [32]u32 = [0; 32];
    let store = LockPackageStore.init(recs[0..], edges[0..], sbuf[0..], buckets[0..]);

    const no_deps: [0]u32 = [];
    const id0 = store.addPackage(LockPackageInput{
        name: "semver",
        version: "7.6.3",
        tarball_url: "https://reg/semver-7.6.3.tgz",
        integrity: "sha512-v7",
        resolution: LockResolutionKind.Npm,
        meta: LockPackageMeta{ bin_kind: LockBinKind.SingleFile, has_scripts: false, is_trusted: true },
    }, no_deps[0..]) catch unreachable;

    // Duplicate (name, version, resolution) returns existing ID without growing store
    const id0_dup = store.addPackage(LockPackageInput{
        name: "semver",
        version: "7.6.3",
        resolution: LockResolutionKind.Npm,
    }, no_deps[0..]) catch unreachable;
    assertEq(id0, id0_dup);
    assertEq(store.record_count, @as(usize, 1));

    // Co-installed second version of semver
    const id1 = store.addPackage(LockPackageInput{
        name: "semver",
        version: "6.3.1",
        tarball_url: "https://reg/semver-6.3.1.tgz",
        integrity: "sha512-v6",
        resolution: LockResolutionKind.Npm,
    }, no_deps[0..]) catch unreachable;
    assertEq(id1, @as(u32, 1));

    let found: [4]u32 = [0; 4];
    const n = store.findAllByName("semver", found[0..]);
    assertEq(n, @as(usize, 2));
    assertEq(found[0], @as(u32, 0));
    assertEq(found[1], @as(u32, 1));
    assert(strEql(store.getVersion(found[0]), "7.6.3"));
    assert(strEql(store.getVersion(found[1]), "6.3.1"));
    assert(strEql(lockResolutionKindName(LockResolutionKind.GitHub), "github"));
});

test("pruneUnreachablePackages marks BFS closure from roots and collects orphan packages", () => {
    let recs: [8]LockPackageRecord = [LockPackageRecord.empty(); 8];
    let edges: [16]u32 = [0; 16];
    let sbuf: [512]u8 = [0; 512];
    let buckets: [16]u32 = [0; 16];
    let store = LockPackageStore.init(recs[0..], edges[0..], sbuf[0..], buckets[0..]);

    const empty_d: [0]u32 = [];
    const root_id = store.addPackage(LockPackageInput{ name: "app", version: "1.0.0", resolution: LockResolutionKind.Root }, empty_d[0..]) catch unreachable;
    const dep_a = store.addPackage(LockPackageInput{ name: "pkg-a", version: "1.0.0", resolution: LockResolutionKind.Npm }, empty_d[0..]) catch unreachable;
    const dep_b = store.addPackage(LockPackageInput{ name: "pkg-b", version: "2.0.0", resolution: LockResolutionKind.Npm }, empty_d[0..]) catch unreachable;
    const orphan1 = store.addPackage(LockPackageInput{ name: "dead-1", version: "0.1.0", resolution: LockResolutionKind.Npm }, empty_d[0..]) catch unreachable;
    const orphan2 = store.addPackage(LockPackageInput{ name: "dead-2", version: "0.2.0", resolution: LockResolutionKind.Git }, empty_d[0..]) catch unreachable;

    // Graph: root -> dep_a -> dep_b; orphan1 -> orphan2 (unreachable)
    const root_deps: [1]u32 = [dep_a];
    const a_deps: [1]u32 = [dep_b];
    const o1_deps: [1]u32 = [orphan2];
    store.setPackageDeps(root_id, root_deps[0..]) catch unreachable;
    store.setPackageDeps(dep_a, a_deps[0..]) catch unreachable;
    store.setPackageDeps(orphan1, o1_deps[0..]) catch unreachable;

    const roots: [1]u32 = [root_id];
    let reach: [8]u8 = [0; 8];
    let queue: [8]u32 = [0; 8];
    let orphans: [8]u32 = [0; 8];
    const sum = pruneUnreachablePackages(store, roots[0..], reach[0..], queue[0..], orphans[0..]) catch unreachable;

    assertEq(sum.total_packages, @as(usize, 5));
    assertEq(sum.reachable_count, @as(usize, 3));
    assertEq(sum.unreachable_count, @as(usize, 2));
    assertEq(orphans[0], orphan1);
    assertEq(orphans[1], orphan2);
});

test("findDuplicatePackageVersions audits multi-version package distribution accurately", () => {
    let recs: [8]LockPackageRecord = [LockPackageRecord.empty(); 8];
    let edges: [8]u32 = [0; 8];
    let sbuf: [512]u8 = [0; 512];
    let buckets: [16]u32 = [0; 16];
    let store = LockPackageStore.init(recs[0..], edges[0..], sbuf[0..], buckets[0..]);

    const empty_d: [0]u32 = [];
    * = store.addPackage(LockPackageInput{ name: "chalk", version: "4.1.2" }, empty_d[0..]) catch unreachable;
    * = store.addPackage(LockPackageInput{ name: "chalk", version: "5.3.0" }, empty_d[0..]) catch unreachable;
    * = store.addPackage(LockPackageInput{ name: "chalk", version: "2.4.2" }, empty_d[0..]) catch unreachable;
    * = store.addPackage(LockPackageInput{ name: "zod", version: "3.23.8" }, empty_d[0..]) catch unreachable;
    * = store.addPackage(LockPackageInput{ name: "debug", version: "3.2.7" }, empty_d[0..]) catch unreachable;
    * = store.addPackage(LockPackageInput{ name: "debug", version: "4.3.7" }, empty_d[0..]) catch unreachable;

    let visited: [8]u8 = [0; 8];
    let dups: [8]DuplicatePkgVersionEntry = [DuplicatePkgVersionEntry.empty(); 8];
    const audit = findDuplicatePackageVersions(store, visited[0..], dups[0..]) catch unreachable;

    assertEq(audit.unique_names, @as(usize, 3));
    assertEq(audit.duplicated_names, @as(usize, 2));
    assertEq(audit.extra_version_instances, @as(u32, 3)); // 2 extra chalk + 1 extra debug
    assert(strEql(dups[0].name_ref.slice(store.string_buf[0..store.string_used]), "chalk"));
    assertEq(dups[0].version_count, @as(u32, 3));
    assert(strEql(dups[1].name_ref.slice(store.string_buf[0..store.string_used]), "debug"));
    assertEq(dups[1].version_count, @as(u32, 2));
});

test("encodeLockStoreSnapshot and decodeLockStoreSnapshot roundtrip all fields with IEEE CRC32", () => {
    // Standard CRC32 test vector: CRC32("123456789") == 0xCBF43926
    assertEq(computeLockStoreCrc32("123456789"), @as(u32, 0xCBF43926));

    let recs1: [8]LockPackageRecord = [LockPackageRecord.empty(); 8];
    let edges1: [16]u32 = [0; 16];
    let sbuf1: [512]u8 = [0; 512];
    let buckets1: [16]u32 = [0; 16];
    let store1 = LockPackageStore.init(recs1[0..], edges1[0..], sbuf1[0..], buckets1[0..]);

    const empty_d: [0]u32 = [];
    const p0 = store1.addPackage(LockPackageInput{
        name: "root",
        version: "0.0.0",
        resolution: LockResolutionKind.Root,
    }, empty_d[0..]) catch unreachable;
    const p1 = store1.addPackage(LockPackageInput{
        name: "esbuild",
        version: "0.24.0",
        tarball_url: "https://registry.npmjs.org/esbuild/-/esbuild-0.24.0.tgz",
        integrity: "sha512-esbuildhash",
        resolution: LockResolutionKind.Npm,
        meta: LockPackageMeta{
            os_mask: 0x0003,
            cpu_mask: 0x0005,
            bin_kind: LockBinKind.NamedMap,
            has_scripts: true,
            is_trusted: true,
            is_patched: true,
        },
    }, empty_d[0..]) catch unreachable;
    const rdeps: [1]u32 = [p1];
    store1.setPackageDeps(p0, rdeps[0..]) catch unreachable;

    let wire: [2048]u8 = [0; 2048];
    const wire_len = encodeLockStoreSnapshot(store1, wire[0..]) catch unreachable;
    assert(wire_len > 100);

    // Decode into fresh buffers
    let recs2: [8]LockPackageRecord = [LockPackageRecord.empty(); 8];
    let edges2: [16]u32 = [0; 16];
    let sbuf2: [512]u8 = [0; 512];
    let buckets2: [16]u32 = [0; 16];
    const store2 = decodeLockStoreSnapshot(wire[0..wire_len], recs2[0..], edges2[0..], sbuf2[0..], buckets2[0..]) catch unreachable;

    assertEq(store2.record_count, @as(usize, 2));
    assertEq(store2.edge_count, @as(usize, 1));
    assert(strEql(store2.getName(1), "esbuild"));
    assert(strEql(store2.getVersion(1), "0.24.0"));
    assert(strEql(store2.getIntegrity(1), "sha512-esbuildhash"));
    assertEq(store2.records[1].meta.os_mask, @as(u16, 0x0003));
    assertEq(store2.records[1].meta.cpu_mask, @as(u16, 0x0005));
    assert(store2.records[1].meta.has_scripts);
    assert(store2.records[1].meta.is_trusted);
    assert(store2.records[1].meta.is_patched);
    assert(store2.records[1].meta.bin_kind == LockBinKind.NamedMap);

    // Rebuilt hash table must find "esbuild"
    let lookup: [2]u32 = [0; 2];
    assertEq(store2.findAllByName("esbuild", lookup[0..]), @as(usize, 1));
    assertEq(lookup[0], @as(u32, 1));
});

test("decodeLockStoreSnapshot detects corrupted magic, version, and single-bit CRC32 flips", () => {
    let recs1: [4]LockPackageRecord = [LockPackageRecord.empty(); 4];
    let edges1: [4]u32 = [0; 4];
    let sbuf1: [128]u8 = [0; 128];
    let buckets1: [8]u32 = [0; 8];
    let store1 = LockPackageStore.init(recs1[0..], edges1[0..], sbuf1[0..], buckets1[0..]);
    const empty_d: [0]u32 = [];
    * = store1.addPackage(LockPackageInput{ name: "zod", version: "3.23.8" }, empty_d[0..]) catch unreachable;

    let wire: [512]u8 = [0; 512];
    const wlen = encodeLockStoreSnapshot(store1, wire[0..]) catch unreachable;

    let recs2: [4]LockPackageRecord = [LockPackageRecord.empty(); 4];
    let edges2: [4]u32 = [0; 4];
    let sbuf2: [128]u8 = [0; 128];
    let buckets2: [8]u32 = [0; 8];

    // 1. Flip 1 bit in payload -> CRC32 mismatch
    wire[30] = wire[30] ^ 0x01;
    let crc_err = false;
    * = decodeLockStoreSnapshot(wire[0..wlen], recs2[0..], edges2[0..], sbuf2[0..], buckets2[0..]) catch {
        crc_err = true;
    };
    assert(crc_err);
    wire[30] = wire[30] ^ 0x01;

    // 2. Corrupt version
    wire[4] = @as(u8, @intCast(LOCK_STORE_SNAPSHOT_VERSION + 5));
    let ver_err = false;
    * = decodeLockStoreSnapshot(wire[0..wlen], recs2[0..], edges2[0..], sbuf2[0..], buckets2[0..]) catch {
        ver_err = true;
    };
    assert(ver_err);

    // 3. Corrupt magic
    wire[0] = 'X';
    let magic_err = false;
    * = decodeLockStoreSnapshot(wire[0..wlen], recs2[0..], edges2[0..], sbuf2[0..], buckets2[0..]) catch {
        magic_err = true;
    };
    assert(magic_err);
});

test("LockPackageStore handles cyclic package graphs safely during BFS pruning", () => {
    let recs: [4]LockPackageRecord = [LockPackageRecord.empty(); 4];
    let edges: [8]u32 = [0; 8];
    let sbuf: [128]u8 = [0; 128];
    let buckets: [8]u32 = [0; 8];
    let store = LockPackageStore.init(recs[0..], edges[0..], sbuf[0..], buckets[0..]);

    const empty_d: [0]u32 = [];
    const a = store.addPackage(LockPackageInput{ name: "a", version: "1.0.0" }, empty_d[0..]) catch unreachable;
    const b = store.addPackage(LockPackageInput{ name: "b", version: "1.0.0" }, empty_d[0..]) catch unreachable;
    const da: [1]u32 = [b];
    const db: [1]u32 = [a];
    store.setPackageDeps(a, da[0..]) catch unreachable;
    store.setPackageDeps(b, db[0..]) catch unreachable;

    const roots: [1]u32 = [a];
    let reach: [4]u8 = [0; 4];
    let queue: [4]u32 = [0; 4];
    let orphans: [4]u32 = [0; 4];
    const sum = pruneUnreachablePackages(store, roots[0..], reach[0..], queue[0..], orphans[0..]) catch unreachable;
    assertEq(sum.reachable_count, @as(usize, 2));
    assertEq(sum.unreachable_count, @as(usize, 0));
});

test("deterministic 500-iteration fuzz over corrupted and truncated binary snapshots causes 0 panics", () => {
    let seed: u64 = 0x6c62272e07bb0142;

    let recs1: [4]LockPackageRecord = [LockPackageRecord.empty(); 4];
    let edges1: [4]u32 = [0; 4];
    let sbuf1: [128]u8 = [0; 128];
    let buckets1: [8]u32 = [0; 8];
    let store1 = LockPackageStore.init(recs1[0..], edges1[0..], sbuf1[0..], buckets1[0..]);
    const empty_d: [0]u32 = [];
    const p0 = store1.addPackage(LockPackageInput{ name: "root", version: "1.0.0", resolution: LockResolutionKind.Root }, empty_d[0..]) catch unreachable;
    const p1 = store1.addPackage(LockPackageInput{ name: "dep", version: "2.0.0", integrity: "sha512-xyz" }, empty_d[0..]) catch unreachable;
    const d0: [1]u32 = [p1];
    store1.setPackageDeps(p0, d0[0..]) catch unreachable;

    let base_wire: [512]u8 = [0; 512];
    const base_len = encodeLockStoreSnapshot(store1, base_wire[0..]) catch unreachable;

    let mut_wire: [512]u8 = [0; 512];
    let recs2: [8]LockPackageRecord = [LockPackageRecord.empty(); 8];
    let edges2: [8]u32 = [0; 8];
    let sbuf2: [256]u8 = [0; 256];
    let buckets2: [16]u32 = [0; 16];

    let iter: u32 = 0;
    let decoded_ok: u32 = 0;
    let decoded_err: u32 = 0;

    while (iter < 500) {
        seed = seed ^ (seed >> 12);
        seed = seed ^ (seed << 25);
        seed = seed ^ (seed >> 27);
        let r = seed *% 2685821657736338717;

        const cut_len: usize = @as(usize, @intCast(r % (base_len + 1)));
        let i: usize = 0;
        while (i < cut_len) {
            mut_wire[i] = base_wire[i];
            i += 1;
        }
        if (cut_len > 0 && (r & 7) != 0) {
            let m: u32 = 0;
            while (m < 3) {
                r = r *% 6364136223846793005 +% 1442695040888963407;
                const idx = @as(usize, @intCast((r >> 16) % cut_len));
                mut_wire[idx] = @as(u8, @truncate(r >> 8));
                m += 1;
            }
        }

        const dec_opt = decodeLockStoreSnapshot(mut_wire[0..cut_len], recs2[0..], edges2[0..], sbuf2[0..], buckets2[0..]) catch null;
        if (dec_opt != null) {
            decoded_ok += 1;
        } else {
            decoded_err += 1;
        }
        iter += 1;
    }

    assert(decoded_err > 450);
    assertEq(decoded_ok + decoded_err, @as(u32, 500));
});
