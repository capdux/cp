// ============================================================================
// cap/test/test_col_full.cp — cap:col 全家桶 12 大容器与编码结构端到端回归测试套件
// 覆盖:
//   1. BabyList<T> & OffsetByteList
//   2. HiveArray<T, CAP>
//   3. ObjectPool<T, CAP>
//   4. BoundedArray<T, CAP>
//   5. LinearFifo<T>
//   6. PriorityQueue<T>
//   7. BitSet64 / BitSet128 / BitSet256 / BitSet512 / BitSet1024 & DynamicBitSet
//   8. Treap<K, V>
//   9. StaticHashMap<K, V, CAP> & FastStringMap<V, CAP>
//  10. Soa<T> (Struct of Arrays)
//  11. Leb128 (ULEB128 & SLEB128)
// ============================================================================

import { test, expect_eq } from "cap:test";
import {
    BabyList,
    OffsetByteList,
    HiveArray,
    ObjectPool,
    BoundedArray,
    LinearFifo,
    PriorityQueue,
    Range,
    BitSet64,
    BitSet128,
    BitSet256,
    BitSet512,
    BitSet1024,
    DynamicBitSet,
    Treap,
    StaticHashMap,
    FastStringMap,
    Soa,
    Leb128,
    Uleb128Result,
    Sleb128Result,
} from "cap:col";

pub struct Vec3Record {
    x: i32 = 0,
    y: i32 = 0,
    weight: i64 = 0,
}

fn main() {
    test("1. BabyList<T> & OffsetByteList dynamic u32-indexed list & cursor stream", () => {
        let list = BabyList<i32>.fromSlice([10, 20, 30]);
        defer list.~init();

        expect_eq(list.count(), 3 as usize);
        list.append(40);
        list.insert(1, 15); // [10, 15, 20, 30, 40]
        expect_eq(list.count(), 5 as usize);
        expect_eq(list.at(1), 15);
        expect_eq(list.contains(30), true);
        expect_eq(list.findIndex(30).?, 3 as usize);

        expect_eq(list.orderedRemove(1), 15); // [10, 20, 30, 40]
        expect_eq(list.swapRemove(0), 10);    // [40, 20, 30]
        expect_eq(list.pop().?, 30);          // [40, 20]
        expect_eq(list.first().?, 40);
        expect_eq(list.last().?, 20);

        let obl = OffsetByteList.init(16);
        defer obl.~init();
        obl.write("ABCDEF");
        expect_eq(obl.remainingLen(), 6 as u32);
        expect_eq(obl.consume(2), 2 as u32);
        expect_eq(obl.remaining() == "CDEF", true);
        expect_eq(obl.consume(4), 4 as u32);
        expect_eq(obl.remainingLen(), 0 as u32);
    });

    test("2. HiveArray<T, CAP> & ObjectPool<T, CAP> O(1) zero-heap slot pools", () => {
        let hive = HiveArray<i64, 8>.init();
        expect_eq(hive.isEmpty(), true);
        expect_eq(hive.capacity(), 8 as usize);

        const s0 = hive.acquire(100).?;
        const s1 = hive.acquire(200).?;
        const s2 = hive.acquire(300).?;
        expect_eq(hive.usedCount(), 3 as usize);
        expect_eq(hive.get(s1).?, 200 as i64);

        expect_eq(hive.set(s1, 250), true);
        expect_eq(hive.get(s1).?, 250 as i64);

        expect_eq(hive.releaseSlot(s1), true);
        expect_eq(hive.get(s1) == null, true);
        expect_eq(hive.usedCount(), 2 as usize);

        const s_reused = hive.acquire(999).?;
        expect_eq(s_reused, s1);
        expect_eq(hive.get(s0).?, 100 as i64);
        expect_eq(hive.get(s2).?, 300 as i64);

        // ObjectPool<T, CAP>
        let pool = ObjectPool<i64, 4>.init();
        const p0 = pool.acquire(11).?;
        const p1 = pool.acquire(22).?;
        const p2 = pool.acquire(33).?;
        const p3 = pool.acquire(44).?;
        expect_eq(pool.isFull(), true);
        expect_eq(pool.acquire(55) == null, true);

        expect_eq(pool.releaseSlot(p2), true);
        expect_eq(pool.availableCount(), 1 as usize);
        const p_new = pool.acquire(77).?;
        expect_eq(p_new, p2);
        expect_eq(pool.get(p_new).?, 77 as i64);
        expect_eq(pool.get(p0).?, 11 as i64);
        expect_eq(pool.get(p1).?, 22 as i64);
        expect_eq(pool.get(p3).?, 44 as i64);
    });

    test("3. BoundedArray<T, CAP> & LinearFifo<T> stack bounded vector & ring FIFO", () => {
        let ba = BoundedArray<i32, 6>.fromSlice([1, 2, 3]);
        expect_eq(ba.append(4), true);
        expect_eq(ba.insert(1, 99), true); // [1, 99, 2, 3, 4]
        expect_eq(ba.count(), 5 as usize);
        expect_eq(ba.replaceRange(1, 2, [20, 30]), true); // [1, 20, 30, 3, 4]
        expect_eq(ba.get(1), 20);
        expect_eq(ba.get(2), 30);
        expect_eq(ba.pop().?, 4);

        let fifo = LinearFifo<i32>.init(4);
        defer fifo.~init();
        fifo.writeSlice([10, 20, 30]);
        fifo.unget([5]); // [5, 10, 20, 30]
        expect_eq(fifo.readableLength(), 4 as usize);
        expect_eq(fifo.first().?, 5);
        expect_eq(fifo.last().?, 30);
        expect_eq(fifo.readItem().?, 5);
        expect_eq(fifo.readItem().?, 10);

        // Force wrap-around and dynamic growth
        fifo.writeSlice([40, 50, 60, 70]);
        expect_eq(fifo.readableLength(), 6 as usize);
        expect_eq(fifo.readItem().?, 20);
        expect_eq(fifo.readItem().?, 30);
        expect_eq(fifo.readItem().?, 40);
        expect_eq(fifo.readItem().?, 50);
        expect_eq(fifo.readItem().?, 60);
        expect_eq(fifo.readItem().?, 70);
        expect_eq(fifo.isEmpty(), true);
    });

    test("4. PriorityQueue<T> binary min-heap ordering & peek/push/pop", () => {
        let pq = PriorityQueue<i32>.init();
        defer pq.deinit();

        expect_eq(pq.isEmpty(), true);
        pq.push(42);
        pq.push(7);
        pq.push(19);
        pq.push(3);
        pq.push(25);

        expect_eq(pq.len(), 5 as usize);
        expect_eq(pq.peek().?, 3);

        expect_eq(pq.pop().?, 3);
        expect_eq(pq.pop().?, 7);
        expect_eq(pq.pop().?, 19);
        expect_eq(pq.pop().?, 25);
        expect_eq(pq.pop().?, 42);
        expect_eq(pq.pop() == null, true);
    });

    test("5. BitSet suite (BitSet64, BitSet128, BitSet256, BitSet512, BitSet1024, DynamicBitSet)", () => {
        let b64 = BitSet64.initEmpty();
        b64.set(0);
        b64.set(31);
        b64.set(63);
        expect_eq(b64.count(), 3 as usize);
        expect_eq(b64.findFirst().?, 0 as usize);

        let b128 = BitSet128.initEmpty();
        b128.set(100);
        expect_eq(b128.isSet(100), true);
        expect_eq(b128.findFirstSet().?, 100 as usize);

        let b256 = BitSet256.initEmpty();
        b256.setRangeValue(Range.init(60, 70), true);
        expect_eq(b256.count(), 10 as usize);
        expect_eq(b256.isSet(60), true);
        expect_eq(b256.isSet(69), true);
        expect_eq(b256.isSet(70), false);

        let b512 = BitSet512.initEmpty();
        b512.set(400);
        expect_eq(b512.findFirstSet().?, 400 as usize);

        let b1024 = BitSet1024.initEmpty();
        b1024.set(999);
        expect_eq(b1024.findFirstSet().?, 999 as usize);

        let dbs = DynamicBitSet.initEmpty(128);
        defer dbs.~init();
        dbs.set(10);
        dbs.set(120);
        expect_eq(dbs.count(), 2 as usize);
        dbs.resize(512, false);
        dbs.set(450);
        expect_eq(dbs.count(), 3 as usize);
        expect_eq(dbs.isSet(450), true);
    });

    test("6. Treap<K, V> randomized balanced BST insert, lookup, min/max & deletion", () => {
        let treap = Treap<i64, i64>.init();
        expect_eq(treap.isEmpty(), true);

        treap.insert(50, 500);
        treap.insert(20, 200);
        treap.insert(80, 800);
        treap.insert(10, 100);
        treap.insert(30, 300);
        treap.insert(70, 700);
        treap.insert(90, 900);

        expect_eq(treap.len(), 7 as usize);
        expect_eq(treap.get(30).?, 300 as i64);
        expect_eq(treap.get(70).?, 700 as i64);
        expect_eq(treap.get(999) == null, true);
        expect_eq(treap.minKey().?, 10 as i64);
        expect_eq(treap.maxKey().?, 90 as i64);

        // Update existing key
        treap.insert(30, 333);
        expect_eq(treap.len(), 7 as usize);
        expect_eq(treap.get(30).?, 333 as i64);

        // Remove root, min, and internal nodes
        expect_eq(treap.remove(10), true);
        expect_eq(treap.minKey().?, 20 as i64);
        expect_eq(treap.remove(50), true);
        expect_eq(treap.contains(50), false);
        expect_eq(treap.len(), 5 as usize);
        expect_eq(treap.get(20).?, 200 as i64);
        expect_eq(treap.get(30).?, 333 as i64);
        expect_eq(treap.get(70).?, 700 as i64);
        expect_eq(treap.get(80).?, 800 as i64);
        expect_eq(treap.get(90).?, 900 as i64);
    });

    test("7. StaticHashMap<K, V, CAP> & FastStringMap<V, CAP> open-addressing & string lookup", () => {
        let hmap = StaticHashMap<i64, i64, 16>.init();
        expect_eq(hmap.put(1, 100), true);
        expect_eq(hmap.put(17, 1700), true); // Collides mod 16 with 1
        expect_eq(hmap.put(33, 3300), true); // Collides mod 16 with 1 & 17
        expect_eq(hmap.count(), 3 as usize);

        expect_eq(hmap.get(1).?, 100 as i64);
        expect_eq(hmap.get(17).?, 1700 as i64);
        expect_eq(hmap.get(33).?, 3300 as i64);

        // Delete middle element in probe chain and verify cluster rehashing preserves 33
        expect_eq(hmap.delete(17), true);
        expect_eq(hmap.has(17), false);
        expect_eq(hmap.get(33).?, 3300 as i64);

        let smap = FastStringMap<i32, 8>.init();
        expect_eq(smap.put("Accept-Encoding", 10), true);
        expect_eq(smap.put("Content-Length", 20), true);
        expect_eq(smap.count(), 2 as usize);
        expect_eq(smap.get("Accept-Encoding").?, 10);
        expect_eq(smap.getCaseInsensitive("accept-encoding").?, 10);
        expect_eq(smap.getCaseInsensitive("CONTENT-LENGTH").?, 20);
        expect_eq(smap.has("Content-Length"), true);
    });

    test("8. Soa<T> (Struct of Arrays) contiguous column slices & row operations", () => {
        let soa = Soa<Vec3Record>.initCapacity(8);
        defer soa.deinit();

        soa.append(Vec3Record{ x: 10, y: 20, weight: 100 });
        soa.append(Vec3Record{ x: 30, y: 40, weight: 200 });
        soa.append(Vec3Record{ x: 50, y: 60, weight: 300 });

        expect_eq(soa.len(), 3 as usize);
        const r1 = soa.get(1);
        expect_eq(r1.x, 30);
        expect_eq(r1.y, 40);
        expect_eq(r1.weight, 200 as i64);

        let xs: []i32 = soa.items(.x);
        let weights: []i64 = soa.items(.weight);
        expect_eq(xs[0] + xs[1] + xs[2], 90);
        expect_eq(weights[0] + weights[1] + weights[2], 600 as i64);

        soa.swapRemove(0);
        expect_eq(soa.len(), 2 as usize);
        expect_eq(soa.get(0).x, 50);
    });

    test("9. Leb128 (ULEB128 & SLEB128) variable-length integer encoding & decoding", () => {
        let buf: [16]u8 = [0; 16];

        // ULEB128: DWARF standard vector 624485 -> [0xE5, 0x8E, 0x26] (3 bytes)
        expect_eq(Leb128.uleb128Size(0), 1 as usize);
        expect_eq(Leb128.uleb128Size(127), 1 as usize);
        expect_eq(Leb128.uleb128Size(128), 2 as usize);
        expect_eq(Leb128.uleb128Size(624485), 3 as usize);

        const u_len = Leb128.encodeUleb128(624485, buf[0..16]);
        expect_eq(u_len, 3 as usize);
        expect_eq(buf[0], 0xE5 as u8);
        expect_eq(buf[1], 0x8E as u8);
        expect_eq(buf[2], 0x26 as u8);

        const u_dec = Leb128.decodeUleb128(buf[0..u_len]);
        expect_eq(u_dec != null, true);
        expect_eq(u_dec.?.value, 624485 as u64);
        expect_eq(u_dec.?.len, 3 as usize);

        // SLEB128: negative -123456 -> [0xC0, 0xBB, 0x78] (3 bytes)
        expect_eq(Leb128.sleb128Size(-1), 1 as usize);
        expect_eq(Leb128.sleb128Size(-64), 1 as usize);
        expect_eq(Leb128.sleb128Size(-65), 2 as usize);
        expect_eq(Leb128.sleb128Size(-123456), 3 as usize);

        const s_len = Leb128.encodeSleb128(-123456, buf[0..16]);
        expect_eq(s_len, 3 as usize);
        expect_eq(buf[0], 0xC0 as u8);
        expect_eq(buf[1], 0xBB as u8);
        expect_eq(buf[2], 0x78 as u8);

        const s_dec = Leb128.decodeSleb128(buf[0..s_len]);
        expect_eq(s_dec != null, true);
        expect_eq(s_dec.?.value, -123456 as i64);
        expect_eq(s_dec.?.len, 3 as usize);

        // SLEB128 positive roundtrip
        const sp_len = Leb128.encodeSleb128(987654321, buf[0..16]);
        const sp_dec = Leb128.decodeSleb128(buf[0..sp_len]);
        expect_eq(sp_dec != null, true);
        expect_eq(sp_dec.?.value, 987654321 as i64);
        expect_eq(sp_dec.?.len, sp_len);
    });
}
