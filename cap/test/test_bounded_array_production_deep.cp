import { test, expect } from "cap:test";
import { BoundedArray } from "cap:bounded_array";

test("bounded_array: append, appendSlice, pop, capacity and overflow", () => {
    const Arr8 = BoundedArray(u32, 8);
    let a = Arr8.empty();
    expect(a.isEmpty()).toBeTrue();
    expect(a.capacity()).toBe(8);
    expect(a.remainingCapacity()).toBe(8);

    a.append(10) catch unreachable;
    a.append(20) catch unreachable;
    const more: [3]u32 = [30, 40, 50];
    a.appendSlice(more[0..]) catch unreachable;
    expect(a.len).toBe(5);
    expect(a.get(0)).toBe(10);
    expect(a.get(4)).toBe(50);
    expect(a.getChecked(5) == null).toBeTrue();

    expect(a.pop().?).toBe(50);
    expect(a.len).toBe(4);

    a.appendNTimes(99, 4) catch unreachable;
    expect(a.isFull()).toBeTrue();

    // Overflow rejected
    let overflowed = false;
    a.append(1) catch { overflowed = true; };
    expect(overflowed).toBeTrue();
});

test("bounded_array: insert, orderedRemove, swapRemove, fromSlice, clear", () => {
    const Arr6 = BoundedArray(i32, 6);
    const seed: [3]i32 = [1, 3, 4];
    let a = Arr6.fromSlice(seed[0..]) catch unreachable;

    a.insert(1, 2) catch unreachable; // [1, 2, 3, 4]
    expect(a.len).toBe(4);
    expect(a.get(0)).toBe(1);
    expect(a.get(1)).toBe(2);
    expect(a.get(2)).toBe(3);
    expect(a.get(3)).toBe(4);

    const r1 = a.orderedRemove(1) catch unreachable; // removes 2 -> [1, 3, 4]
    expect(r1).toBe(2);
    expect(a.len).toBe(3);
    expect(a.get(1)).toBe(3);

    const r2 = a.swapRemove(0) catch unreachable; // removes 1, swaps 4 -> [4, 3]
    expect(r2).toBe(1);
    expect(a.len).toBe(2);
    expect(a.get(0)).toBe(4);
    expect(a.get(1)).toBe(3);

    a.clear();
    expect(a.isEmpty()).toBeTrue();
    expect(a.pop() == null).toBeTrue();
});
