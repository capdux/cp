import {
    ProgressSpinnerStyle,
    spinnerFrame,
    ProgressMeter,
} from "cap:cli";
import { test, expect } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("ProgressMeter rendering, clamping, percentage, and ETA", () => {
    let pb = ProgressMeter.init(100, 10);
    let buf: [64]u8 = [0; 64];

    expect(pb.percent() == 0);
    expect(pb.etaMs(1000) == null);
    expect(strEq(pb.render(buf[0..]).?, "[----------] 0% (0/100)"));

    pb.inc(25);
    expect(pb.percent() == 25);
    expect(pb.etaMs(1000).? == 3000);
    expect(strEq(pb.render(buf[0..]).?, "[##--------] 25% (25/100)"));

    pb.set(50);
    expect(strEq(pb.render(buf[0..]).?, "[#####-----] 50% (50/100)"));

    pb.inc(999); // clamps to total
    expect(pb.isComplete());
    expect(pb.etaMs(2000).? == 0);
    expect(strEq(pb.render(buf[0..]).?, "[##########] 100% (100/100)"));
});

test("spinnerFrame cycles through ASCII and Braille dot styles", () => {
    expect(strEq(spinnerFrame(ProgressSpinnerStyle.ascii, 0), "|"));
    expect(strEq(spinnerFrame(ProgressSpinnerStyle.ascii, 1), "/"));
    expect(strEq(spinnerFrame(ProgressSpinnerStyle.ascii, 4), "|"));
    expect(strEq(spinnerFrame(ProgressSpinnerStyle.dots, 0), "⠋"));
    expect(strEq(spinnerFrame(ProgressSpinnerStyle.dots, 10), "⠋"));
});
