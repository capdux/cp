import { test, expect } from "cap:test";
import {
    TouchTimestamp,
    TouchOpts,
} from "cap:shell";

fn eqBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("shell touch_opts: short flags, combined clusters and effectiveTarget semantics") {
    const a1: [2][]const u8 = ["-a", "foo.txt"];
    const o1 = TouchOpts.parse(a1[0..])?;
    expect(o1.effectiveTarget() == .access_only);
    expect(o1.shouldCreateMissing());
    expect(o1.path_count == 1);
    if (p := o1.pathAt(0)) {
        expect(eqBytes(p, "foo.txt"));
    } else {
        expect(false);
    }

    const a2: [3][]const u8 = ["-cm", "a.txt", "b.txt"];
    const o2 = TouchOpts.parse(a2[0..])?;
    expect(o2.effectiveTarget() == .modify_only);
    expect(!o2.shouldCreateMissing());
    expect(o2.path_count == 2);

    // -a 与 -m 同设时等效 .both
    const a3: [2][]const u8 = ["-achm", "c.txt"];
    const o3 = TouchOpts.parse(a3[0..])?;
    expect(o3.effectiveTarget() == .both);
    expect(o3.no_create);
    expect(o3.no_dereference);
    expect(o3.pathAt(1) == null);
}

test("shell touch_opts: long options, equals syntax and --time aliases") {
    const a1: [4][]const u8 = ["--no-create", "--no-dereference", "--date=2026-11-22T08:00:00Z", "log.txt"];
    const o1 = TouchOpts.parse(a1[0..])?;
    expect(o1.no_create);
    expect(o1.no_dereference);
    expect(o1.has_date);
    expect(eqBytes(o1.date, "2026-11-22T08:00:00Z"));
    expect(o1.effectiveTarget() == .both);

    const a2: [4][]const u8 = ["--reference", "/tmp/ref.txt", "--time=use", "target.txt"];
    const o2 = TouchOpts.parse(a2[0..])?;
    expect(o2.has_reference);
    expect(eqBytes(o2.reference, "/tmp/ref.txt"));
    expect(o2.effectiveTarget() == .access_only);

    const a3: [3][]const u8 = ["--time", "mtime", "target.txt"];
    const o3 = TouchOpts.parse(a3[0..])?;
    expect(o3.effectiveTarget() == .modify_only);
}

test("shell touch_opts: POSIX -t timestamp parsing, leap year, leap second and inline clusters") {
    // 12 位 CCYYMMDDhhmm.ss（闰年 2024-02-29 + 闰秒 60）
    const ts1 = TouchTimestamp.parse("202402292359.60")?;
    expect(ts1.has_year);
    expect(ts1.year == 2024);
    expect(ts1.month == 2);
    expect(ts1.day == 29);
    expect(ts1.hour == 23);
    expect(ts1.minute == 59);
    expect(ts1.second == 60);

    // 10 位 YYMMDDhhmm：POSIX 69..99 -> 1969..1999, 00..68 -> 2000..2068
    const ts_1999 = TouchTimestamp.parse("9912312359")?;
    expect(ts_1999.year == 1999);
    expect(ts_1999.second == 0);

    const ts_2068 = TouchTimestamp.parse("6801010000")?;
    expect(ts_2068.year == 2068);

    // 8 位 MMDDhhmm.ss（无年份）
    const ts_noyear = TouchTimestamp.parse("05011230.15")?;
    expect(!ts_noyear.has_year);
    expect(ts_noyear.year == 0);
    expect(ts_noyear.month == 5);
    expect(ts_noyear.day == 1);
    expect(ts_noyear.hour == 12);
    expect(ts_noyear.minute == 30);
    expect(ts_noyear.second == 15);

    // 内联短参数 -ct202605010830
    const a_inline: [2][]const u8 = ["-ct202605010830", "file.txt"];
    const o_inline = TouchOpts.parse(a_inline[0..])?;
    expect(o_inline.no_create);
    if (ts := o_inline.timestamp) {
        expect(ts.year == 2026 && ts.month == 5 && ts.day == 1 && ts.hour == 8 && ts.minute == 30);
    } else {
        expect(false);
    }
}

test("shell touch_opts: double-dash terminator and single-dash stdout operand") {
    const a1: [4][]const u8 = ["-c", "--", "-weird.txt", "--other"];
    const o1 = TouchOpts.parse(a1[0..])?;
    expect(o1.no_create);
    expect(o1.path_count == 2);
    if (p0 := o1.pathAt(0)) { expect(eqBytes(p0, "-weird.txt")); } else { expect(false); }
    if (p1 := o1.pathAt(1)) { expect(eqBytes(p1, "--other")); } else { expect(false); }

    const a2: [2][]const u8 = ["-m", "-"];
    const o2 = TouchOpts.parse(a2[0..])?;
    expect(o2.path_count == 1);
    if (p0 := o2.pathAt(0)) { expect(eqBytes(p0, "-")); } else { expect(false); }
}

test("shell touch_opts: malformed flags, conflicting time sources and invalid calendar guards") {
    let rejected: usize = 0;

    // 缺少操作数
    const e1: [1][]const u8 = ["-c"];
    * = TouchOpts.parse(e1[0..]) catch { rejected += 1; TouchOpts.defaults(); };
    // 缺少选项值
    const e2: [1][]const u8 = ["-t"];
    * = TouchOpts.parse(e2[0..]) catch { rejected += 1; TouchOpts.defaults(); };
    const e3: [1][]const u8 = ["--reference"];
    * = TouchOpts.parse(e3[0..]) catch { rejected += 1; TouchOpts.defaults(); };
    // 未知选项 / 非法 --time 关键字
    const e4: [2][]const u8 = ["-Z", "a.txt"];
    * = TouchOpts.parse(e4[0..]) catch { rejected += 1; TouchOpts.defaults(); };
    const e5: [2][]const u8 = ["--unknown", "a.txt"];
    * = TouchOpts.parse(e5[0..]) catch { rejected += 1; TouchOpts.defaults(); };
    const e6: [2][]const u8 = ["--time=ctime", "a.txt"];
    * = TouchOpts.parse(e6[0..]) catch { rejected += 1; TouchOpts.defaults(); };
    // 互斥时间源冲突
    const e7: [5][]const u8 = ["-t", "202601010000", "-r", "ref.txt", "a.txt"];
    * = TouchOpts.parse(e7[0..]) catch { rejected += 1; TouchOpts.defaults(); };
    const e8: [3][]const u8 = ["--date=now", "--reference=ref.txt", "a.txt"];
    * = TouchOpts.parse(e8[0..]) catch { rejected += 1; TouchOpts.defaults(); };

    // 畸形 POSIX 时间戳（平年 2/29、月份 13、日期 32、小时 24、分钟 60、秒 61、长度非法）
    * = TouchTimestamp.parse("202502290000") catch { rejected += 1; TouchTimestamp.parse("01010000")?; };
    * = TouchTimestamp.parse("202613010000") catch { rejected += 1; TouchTimestamp.parse("01010000")?; };
    * = TouchTimestamp.parse("202604310000") catch { rejected += 1; TouchTimestamp.parse("01010000")?; };
    * = TouchTimestamp.parse("202601012400") catch { rejected += 1; TouchTimestamp.parse("01010000")?; };
    * = TouchTimestamp.parse("202601011260") catch { rejected += 1; TouchTimestamp.parse("01010000")?; };
    * = TouchTimestamp.parse("202601011200.61") catch { rejected += 1; TouchTimestamp.parse("01010000")?; };
    * = TouchTimestamp.parse("12345") catch { rejected += 1; TouchTimestamp.parse("01010000")?; };

    expect(rejected == 15);
}

test("shell touch_opts: TouchTimestamp formatPosix round-trip and buffer guard") {
    const ts1 = TouchTimestamp.parse("202611220755.30")?;
    let buf: [32]u8 = [0; 32];
    const n1 = ts1.formatPosix(buf[0..])?;
    expect(eqBytes(buf[0..n1], "202611220755.30"));

    const ts2 = TouchTimestamp.parse(buf[0..n1])?;
    expect(ts1.eql(ts2));

    // 无年份往返
    const ny1 = TouchTimestamp.parse("07041830.05")?;
    const n2 = ny1.formatPosix(buf[0..])?;
    expect(eqBytes(buf[0..n2], "07041830.05"));
    expect(ny1.eql(TouchTimestamp.parse(buf[0..n2])?));

    // 缓冲区过小防护
    let tiny: [6]u8 = [0; 6];
    const small_res = ts1.formatPosix(tiny[0..]) catch {
        expect(true);
        return;
    };
    * = small_res;
    expect(false);
}
