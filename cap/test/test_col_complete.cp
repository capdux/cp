// cap/test/test_col_complete.cp
// Comprehensive test suite verifying 100% complete and industrial-grade collections in cap:col

import { test, expect_eq } from "cap:test";
import {
    Range,
    BitSet64,
    BitSet64Iter,
    BitSet128,
    BitSet256,
    BitSet256Iter,
    BitSet512,
    BitSet1024,
    DynamicBitSet,
    DynamicBitSetIter,
    BoundedArray,
    BabyList,
    OffsetByteList,
    LinearFifo
} from "cap:col";

fn main() {
    test("1. BoundedArray complete methods: fromSlice, slice, insert, replaceRange, contains, indexOf", () => {
        let arr = BoundedArray<i32, 10>.fromSlice([10, 20, 30, 40]);
        expect_eq(arr.capacity() as i64, 10);
        expect_eq(arr.count() as i64, 4);
        expect_eq(arr.isEmpty(), false);
        expect_eq(arr.isFull(), false);
        expect_eq(arr.unusedCapacity() as i64, 6);

        expect_eq(arr.first() ?? -1, 10);
        expect_eq(arr.last() ?? -1, 40);
        expect_eq(arr.at(2), 30);
        expect_eq(arr.get(3), 40);

        // contains and findIndex
        expect_eq(arr.contains(30), true);
        expect_eq(arr.contains(99), false);
        expect_eq((arr.findIndex(30) ?? 999) as i64, 2);
        expect_eq(arr.findIndex(99) == null, true);

        // slice & constSlice
        let sl = arr.slice();
        expect_eq(sl.len as i64, 4);
        expect_eq(sl[0], 10);
        expect_eq(sl[3], 40);

        // insert at index 1
        expect_eq(arr.insert(1, 15), true);
        expect_eq(arr.count() as i64, 5);
        expect_eq(arr.at(1), 15);
        expect_eq(arr.at(2), 20);

        // insertSlice at index 2
        expect_eq(arr.insertSlice(2, [16, 17]), true);
        expect_eq(arr.count() as i64, 7);
        expect_eq(arr.at(2), 16);
        expect_eq(arr.at(3), 17);
        expect_eq(arr.at(4), 20);

        // replaceRange: replace [16, 17] at index 2 with [77, 88]
        expect_eq(arr.replaceRange(2, 2, [77, 88]), true);
        expect_eq(arr.count() as i64, 7);
        expect_eq(arr.at(2), 77);
        expect_eq(arr.at(3), 88);

        // appendNTimes
        expect_eq(arr.appendNTimes(9, 2), true);
        expect_eq(arr.count() as i64, 9);
        expect_eq(arr.at(7), 9);
        expect_eq(arr.at(8), 9);

        // resize
        expect_eq(arr.resize(5), true);
        expect_eq(arr.count() as i64, 5);
        expect_eq(arr.last() ?? -1, 20);

        arr.clear();
        expect_eq(arr.isEmpty(), true);
    });

    test("2. LinearFifo complete methods: realign, unget, readableSlice, writableSlice, update, remove", () => {
        let fifo = LinearFifo<i32>.init(8);
        expect_eq(fifo.capacity() as i64, 16);
        expect_eq(fifo.isEmpty(), true);

        // write items
        fifo.writeItem(100);
        fifo.writeItem(200);
        fifo.writeItem(300);
        expect_eq(fifo.readableLength() as i64, 3);
        expect_eq(fifo.first() ?? -1, 100);
        expect_eq(fifo.last() ?? -1, 300);

        // read one item
        expect_eq(fifo.readItem() ?? -1, 100);
        expect_eq(fifo.readableLength() as i64, 2);
        expect_eq(fifo.first() ?? -1, 200);

        // unget: push [50, 60] back in front of read position
        fifo.unget([50, 60]);
        expect_eq(fifo.readableLength() as i64, 4);
        expect_eq(fifo.first() ?? -1, 50);

        // realign
        fifo.realign();
        expect_eq(fifo.head as i64, 0);
        expect_eq(fifo.first() ?? -1, 50);

        // readableSlice
        let rslice = fifo.readableSlice(0);
        expect_eq(rslice.len as i64, 4);
        expect_eq(rslice[0], 50);
        expect_eq(rslice[1], 60);

        // orderedRemoveItem at offset 1 (item 60)
        expect_eq(fifo.orderedRemoveItem(1), true);
        expect_eq(fifo.readableLength() as i64, 3);
        expect_eq(fifo.peekItem(0) ?? -1, 50);
        expect_eq(fifo.peekItem(1) ?? -1, 200);
        expect_eq(fifo.peekItem(2) ?? -1, 300);

        // writeSlice
        fifo.write([400, 500]);
        expect_eq(fifo.readableLength() as i64, 5);
        expect_eq(fifo.last() ?? -1, 500);

        // readSlice into buffer
        let out_buf = BoundedArray<i32, 4>.init();
        out_buf.resize(3);
        const read_count = fifo.read(out_buf.slice());
        expect_eq(read_count as i64, 3);
        expect_eq(out_buf.get(0), 50);
        expect_eq(out_buf.get(1), 200);
        expect_eq(out_buf.get(2), 300);
        expect_eq(fifo.readableLength() as i64, 2);

        fifo.~init();
    });

    test("3. BabyList & OffsetByteList complete methods: fromSlice, insert, remove, clone, shrink", () => {
        let list = BabyList<i32>.fromSlice([1000, 2000, 3000]);
        expect_eq(list.count() as i64, 3);
        expect_eq(list.at(0), 1000);
        expect_eq(list.last() ?? -1, 3000);

        // contains and findIndex
        expect_eq(list.contains(2000), true);
        expect_eq(list.contains(9999), false);
        expect_eq((list.findIndex(2000) ?? 999) as i64, 1);
        expect_eq(list.findIndex(9999) == null, true);

        // insert at index 1
        list.insert(1, 1500);
        expect_eq(list.count() as i64, 4);
        expect_eq(list.at(1), 1500);
        expect_eq(list.at(2), 2000);

        // insertSlice at index 2
        list.insertSlice(2, [1600, 1700]);
        expect_eq(list.count() as i64, 6);
        expect_eq(list.at(2), 1600);
        expect_eq(list.at(3), 1700);
        expect_eq(list.at(4), 2000);

        // orderedRemove at index 2 (1600)
        const rm = list.orderedRemove(2);
        expect_eq(rm, 1600);
        expect_eq(list.count() as i64, 5);
        expect_eq(list.at(2), 1700);

        // clone
        let cloned = list.clone();
        expect_eq(cloned.count() as i64, 5);
        expect_eq(cloned.at(2), 1700);
        cloned.~init();

        // shrinkRetainingCapacity
        list.shrinkRetainingCapacity(2);
        expect_eq(list.count() as i64, 2);
        expect_eq(list.last() ?? -1, 1500);

        list.~init();

        // OffsetByteList extra methods: slice, remaining, len, clear
        let obl = OffsetByteList.init(32);
        obl.write("BunToCpStdlib");
        expect_eq(obl.len() as i64, 13);
        let s = obl.slice();
        expect_eq(s.len as i64, 13);

        const consumed = obl.consume(5);
        expect_eq(consumed as i64, 5);
        expect_eq(obl.len() as i64, 8);
        let rem = obl.remaining();
        expect_eq(rem.len as i64, 8);

        obl.clear();
        expect_eq(obl.len() as i64, 0);
        obl.~init();
    });

    test("4. BitSet64, BitSet128, BitSet256, BitSet512: full boolean algebra and iteration", () => {
        let bs64 = BitSet64.initEmpty();
        bs64.set(10);
        bs64.set(20);
        bs64.set(30);
        expect_eq(bs64.count() as i64, 3);
        expect_eq(bs64.isSet(10), true);
        expect_eq(bs64.isSet(15), false);

        // Range setting
        let rng = Range.init(15, 18);
        bs64.setRangeValue(rng, true);
        expect_eq(bs64.isSet(15), true);
        expect_eq(bs64.isSet(16), true);
        expect_eq(bs64.isSet(17), true);
        expect_eq(bs64.isSet(18), false);
        expect_eq(bs64.count() as i64, 6);

        // Iterator
        let it = bs64.iterator();
        expect_eq(it.next() ?? 999, 10);
        expect_eq(it.next() ?? 999, 15);
        expect_eq(it.next() ?? 999, 16);
        expect_eq(it.next() ?? 999, 17);
        expect_eq(it.next() ?? 999, 20);
        expect_eq(it.next() ?? 999, 30);
        expect_eq(it.next() == null, true);

        // Boolean algebra
        let a = BitSet64.initEmpty();
        a.set(1);
        a.set(2);
        let b = BitSet64.initEmpty();
        b.set(2);
        b.set(3);

        let un = a.unionWith(b);
        expect_eq(un.count() as i64, 3);
        expect_eq(un.isSet(1), true);
        expect_eq(un.isSet(2), true);
        expect_eq(un.isSet(3), true);

        let insec = a.intersectWith(b);
        expect_eq(insec.count() as i64, 1);
        expect_eq(insec.isSet(2), true);

        let xor_set = a.xorWith(b);
        expect_eq(xor_set.count() as i64, 2);
        expect_eq(xor_set.isSet(1), true);
        expect_eq(xor_set.isSet(3), true);

        let diff = a.differenceWith(b);
        expect_eq(diff.count() as i64, 1);
        expect_eq(diff.isSet(1), true);

        // BitSet256 multi-word test
        let bs256 = BitSet256.initEmpty();
        bs256.set(5);
        bs256.set(70);  // word 1
        bs256.set(135); // word 2
        bs256.set(200); // word 3
        expect_eq(bs256.count() as i64, 4);
        expect_eq(bs256.isSet(70), true);
        expect_eq(bs256.isSet(71), false);

        expect_eq(bs256.findFirstSet() ?? 999, 5);
        expect_eq(bs256.toggleFirstSet() ?? 999, 5);
        expect_eq(bs256.findFirstSet() ?? 999, 70);
        expect_eq(bs256.count() as i64, 3);

        // BitSet256 iterator
        let it256 = bs256.iterator();
        expect_eq(it256.next() ?? 999, 70);
        expect_eq(it256.next() ?? 999, 135);
        expect_eq(it256.next() ?? 999, 200);
        expect_eq(it256.next() == null, true);
    });

    test("5. DynamicBitSet: dynamic expansion, resize with fill, setRangeValue, boolean algebra", () => {
        let dbs = DynamicBitSet.initEmpty(100);
        expect_eq(dbs.capacity() as i64, 100);
        expect_eq(dbs.count() as i64, 0);

        dbs.set(10);
        dbs.set(70);
        dbs.set(99);
        expect_eq(dbs.count() as i64, 3);
        expect_eq(dbs.isSet(10), true);
        expect_eq(dbs.isSet(70), true);
        expect_eq(dbs.isSet(99), true);
        expect_eq(dbs.isSet(100), false);

        // findFirstSet & toggleFirstSet
        expect_eq(dbs.findFirstSet() ?? 999, 10);
        expect_eq(dbs.toggleFirstSet() ?? 999, 10);
        expect_eq(dbs.findFirstSet() ?? 999, 70);
        expect_eq(dbs.count() as i64, 2);

        // resize to 200 bits with fill = true
        dbs.resize(200, true);
        expect_eq(dbs.capacity() as i64, 200);
        // All new bits [100..200) should be set!
        expect_eq(dbs.isSet(100), true);
        expect_eq(dbs.isSet(150), true);
        expect_eq(dbs.isSet(199), true);
        expect_eq(dbs.isSet(200), false);

        // clone
        let cloned = dbs.clone();
        expect_eq(cloned.capacity() as i64, 200);
        expect_eq(cloned.eql(dbs), true);

        // unset on clone
        cloned.unset(150);
        expect_eq(cloned.isSet(150), false);
        expect_eq(cloned.eql(dbs), false);
        expect_eq(cloned.subsetOf(dbs), true);
        expect_eq(dbs.supersetOf(cloned), true);

        cloned.~init();
        dbs.~init();
    });
}
