import { test, expect } from "cap:test";
import { StaticLinearFifo } from "cap:linear_fifo";

test("linear_fifo: wrap-around byte ring buffer writeSlice/readSlice/unget/discard", () => {
    let fifo = StaticLinearFifo(u8, 8).init();
    expect(fifo.isEmpty() && fifo.writableLength() == 8);

    const w1 = fifo.writeSlice("hello");
    expect(w1 == 5 && fifo.readableLength() == 5 && fifo.writableLength() == 3);
    expect(fifo.peekItem(0).? == 'h' && fifo.peekItem(4).? == 'o');
    expect(fifo.peekItem(5) == null);

    let out: [8]u8 = [0; 8];
    const r1 = fifo.readSlice(out[0..3]); // "hel"
    expect(r1 == 3 && out[0] == 'h' && out[2] == 'l');
    expect(fifo.readableLength() == 2);

    // Trigger wrap-around: write 6 bytes into capacity 8 (remaining 2 + 6 = 8 full)
    const w2 = fifo.writeSlice("12345678");
    expect(w2 == 6 && fifo.isFull());
    expect(!fifo.writeItem('X'));

    // Read all 8 bytes: "lo123456"
    const r2 = fifo.readSlice(out[0..8]);
    expect(r2 == 8);
    expect(out[0] == 'l' && out[1] == 'o' && out[2] == '1' && out[7] == '6');
    expect(fifo.isEmpty());

    // Unget and discard
    expect(fifo.writeItem('B'));
    expect(fifo.unget('A'));
    expect(fifo.readItem().? == 'A');
    expect(fifo.discard(5) == 1);
    expect(fifo.readItem() == null);
});

test("linear_fifo: generic u32 FIFO", () => {
    let q = StaticLinearFifo(u32, 4).init();
    expect(q.writeItem(10) && q.writeItem(20) && q.writeItem(30));
    expect(q.readItem().? == 10);
    expect(q.writeItem(40) && q.writeItem(50));
    expect(q.isFull());
    expect(q.readItem().? == 20 && q.readItem().? == 30 && q.readItem().? == 40 && q.readItem().? == 50);
    expect(q.isEmpty());
});
