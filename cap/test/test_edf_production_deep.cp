// Source: cp/cap/test/test_edf_production_deep.cp (Production deep tests for cap:edf)

import { test, expect_eq } from "cap:test";
import {
    EDF_MAIN_HEADER_SIZE,
    EDF_SIGNAL_HEADER_SIZE,
    EdfHeader,
    EdfSignal,
    isEdf,
    parseEdfHeader,
    parseEdfSignal,
    edfRecordByteSize,
    readEdfDigitalSample,
} from "cap:edf";

fn bytesEq(a: []const u8, b: []const u8): bool {
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

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const diff = if (a >= b) a - b else b - a;
    return diff <= eps;
}

fn writePadded(buf: *[]u8, offset: usize, width: usize, text: []const u8) {
    let i: usize = 0;
    while (i < width) {
        buf.*[offset + i] = if (i < text.len) text[i] else ' ';
        i += 1;
    }
}

fn writeI16Le(buf: *[]u8, offset: usize, val: i16) {
    const uval: u16 = if (val < 0) ((val as i32) + 65536) as u16 else val as u16;
    buf.*[offset] = (uval & 0xFF) as u8;
    buf.*[offset + 1] = ((uval >> 8) & 0xFF) as u8;
}

/// 构造完整 2 通道 EDF+C 测试文件：256B 主头 + 512B 列式通道头 + 2 条数据记录（每条 12B）= 792B
fn buildTwoChannelEdf(out: *[792]u8, reserved: []const u8, num_records_str: []const u8) {
    let slice: []u8 = out.*[0..792];
    let i: usize = 0;
    while (i < 792) {
        slice[i] = ' ';
        i += 1;
    }

    // 256B 主头
    writePadded(*slice, 0, 8, "0");
    writePadded(*slice, 8, 80, "P001 M 01-JAN-1990 John_Doe");
    writePadded(*slice, 88, 80, "Startdate 21-NOV-2026 Lab_EEG_01");
    writePadded(*slice, 168, 8, "21.11.26");
    writePadded(*slice, 176, 8, "09.30.00");
    writePadded(*slice, 184, 8, "768");
    writePadded(*slice, 192, 44, reserved);
    writePadded(*slice, 236, 8, num_records_str);
    writePadded(*slice, 244, 8, "0.5");
    writePadded(*slice, 252, 4, "2");

    // 列式通道头（ns = 2，基准偏移 256）
    // 1. label (16B * 2) @ 256
    writePadded(*slice, 256 + 0, 16, "EEG Fp1-Cz");
    writePadded(*slice, 256 + 16, 16, "ECG Lead II");
    // 2. transducer (80B * 2) @ 256 + 32 = 288
    writePadded(*slice, 288 + 0, 80, "AgAgCl cup electrode");
    writePadded(*slice, 288 + 80, 80, "Disposable chest pad");
    // 3. physical_dim (8B * 2) @ 256 + 192 = 448
    writePadded(*slice, 448 + 0, 8, "uV");
    writePadded(*slice, 448 + 8, 8, "mV");
    // 4. physical_min (8B * 2) @ 256 + 208 = 464
    writePadded(*slice, 464 + 0, 8, "-500");
    writePadded(*slice, 464 + 8, 8, "-5.0");
    // 5. physical_max (8B * 2) @ 256 + 224 = 480
    writePadded(*slice, 480 + 0, 8, "500");
    writePadded(*slice, 480 + 8, 8, "5.0");
    // 6. digital_min (8B * 2) @ 256 + 240 = 496
    writePadded(*slice, 496 + 0, 8, "-2000");
    writePadded(*slice, 496 + 8, 8, "-32768");
    // 7. digital_max (8B * 2) @ 256 + 256 = 512
    writePadded(*slice, 512 + 0, 8, "2000");
    writePadded(*slice, 512 + 8, 8, "32767");
    // 8. prefiltering (80B * 2) @ 256 + 272 = 528
    writePadded(*slice, 528 + 0, 80, "HP:0.5Hz LP:70Hz N:50Hz");
    writePadded(*slice, 528 + 80, 80, "HP:0.05Hz LP:150Hz");
    // 9. samples_per_record (8B * 2) @ 256 + 432 = 688
    writePadded(*slice, 688 + 0, 8, "4");
    writePadded(*slice, 688 + 8, 8, "2");
    // 10. signal reserved (32B * 2) @ 256 + 448 = 704..768（保持空格）

    // 数据记录 0 @ 768（EEG: 4 samples + ECG: 2 samples = 12B）
    writeI16Le(*slice, 768 + 0, -2000);
    writeI16Le(*slice, 768 + 2, 0);
    writeI16Le(*slice, 768 + 4, 1000);
    writeI16Le(*slice, 768 + 6, 2000);
    writeI16Le(*slice, 768 + 8, -32768);
    writeI16Le(*slice, 768 + 10, 32767);

    // 数据记录 1 @ 780（EEG: 4 samples + ECG: 2 samples = 12B）
    writeI16Le(*slice, 780 + 0, -500);
    writeI16Le(*slice, 780 + 2, 250);
    writeI16Le(*slice, 780 + 4, -125);
    writeI16Le(*slice, 780 + 6, 1500);
    writeI16Le(*slice, 780 + 8, -16384);
    writeI16Le(*slice, 780 + 10, 16384);
}

test("1. Full 2-channel EEG/ECG EDF+ 768B header & columnar signal metadata parsing", () => {
    expect_eq(EDF_MAIN_HEADER_SIZE, 256);
    expect_eq(EDF_SIGNAL_HEADER_SIZE, 256);

    let buf: [792]u8 = [0; 792];
    buildTwoChannelEdf(*buf, "EDF+C", "2");
    const data: []const u8 = buf[0..792];

    expect_eq(isEdf(data), true);

    const hdr = parseEdfHeader(data) ?? unreachable;
    expect_eq(bytesEq(hdr.version, "0       "), true);
    expect_eq(bytesEq(hdr.patient_id, "P001 M 01-JAN-1990 John_Doe"), true);
    expect_eq(bytesEq(hdr.recording_id, "Startdate 21-NOV-2026 Lab_EEG_01"), true);
    expect_eq(bytesEq(hdr.start_date, "21.11.26"), true);
    expect_eq(bytesEq(hdr.start_time, "09.30.00"), true);
    expect_eq(hdr.header_bytes, 768);
    expect_eq(bytesEq(hdr.reserved, "EDF+C"), true);
    expect_eq(hdr.num_records, 2);
    expect_eq(approxEq(hdr.record_duration_sec, 0.5, 1e-9), true);
    expect_eq(hdr.num_signals, 2);
    expect_eq(hdr.isEdfPlus(), true);
    expect_eq(approxEq(hdr.totalDurationSec(), 1.0, 1e-9), true);

    const eeg = parseEdfSignal(data, hdr, 0) ?? unreachable;
    expect_eq(bytesEq(eeg.label, "EEG Fp1-Cz"), true);
    expect_eq(bytesEq(eeg.transducer, "AgAgCl cup electrode"), true);
    expect_eq(bytesEq(eeg.physical_dim, "uV"), true);
    expect_eq(approxEq(eeg.physical_min, -500.0, 1e-9), true);
    expect_eq(approxEq(eeg.physical_max, 500.0, 1e-9), true);
    expect_eq(eeg.digital_min, -2000);
    expect_eq(eeg.digital_max, 2000);
    expect_eq(bytesEq(eeg.prefiltering, "HP:0.5Hz LP:70Hz N:50Hz"), true);
    expect_eq(eeg.samples_per_record, 4);

    const ecg = parseEdfSignal(data, hdr, 1) ?? unreachable;
    expect_eq(bytesEq(ecg.label, "ECG Lead II"), true);
    expect_eq(bytesEq(ecg.transducer, "Disposable chest pad"), true);
    expect_eq(bytesEq(ecg.physical_dim, "mV"), true);
    expect_eq(approxEq(ecg.physical_min, -5.0, 1e-9), true);
    expect_eq(approxEq(ecg.physical_max, 5.0, 1e-9), true);
    expect_eq(ecg.digital_min, -32768);
    expect_eq(ecg.digital_max, 32767);
    expect_eq(bytesEq(ecg.prefiltering, "HP:0.05Hz LP:150Hz"), true);
    expect_eq(ecg.samples_per_record, 2);
});

test("2. sampleRateHz and digitalToPhysical linear calibration with zero-division guards", () => {
    let buf: [792]u8 = [0; 792];
    buildTwoChannelEdf(*buf, "EDF+C", "2");
    const data: []const u8 = buf[0..792];

    const hdr = parseEdfHeader(data) ?? unreachable;
    const eeg = parseEdfSignal(data, hdr, 0) ?? unreachable;
    const ecg = parseEdfSignal(data, hdr, 1) ?? unreachable;

    // EEG: 4 samples / 0.5s = 8 Hz; ECG: 2 samples / 0.5s = 4 Hz
    expect_eq(approxEq(eeg.sampleRateHz(hdr), 8.0, 1e-9), true);
    expect_eq(approxEq(ecg.sampleRateHz(hdr), 4.0, 1e-9), true);

    // EEG 线性定标 [-2000..2000] -> [-500..500] uV（0.25 uV / LSB）
    expect_eq(approxEq(eeg.digitalToPhysical(-2000), -500.0, 1e-9), true);
    expect_eq(approxEq(eeg.digitalToPhysical(0), 0.0, 1e-9), true);
    expect_eq(approxEq(eeg.digitalToPhysical(1000), 250.0, 1e-9), true);
    expect_eq(approxEq(eeg.digitalToPhysical(2000), 500.0, 1e-9), true);

    // ECG 完整 16-bit 范围 [-32768..32767] -> [-5.0..5.0] mV
    expect_eq(approxEq(ecg.digitalToPhysical(-32768), -5.0, 1e-9), true);
    expect_eq(approxEq(ecg.digitalToPhysical(32767), 5.0, 1e-9), true);

    // 除零防护：digital_min == digital_max 与 record_duration_sec == 0.0
    const degenerate_sig = EdfSignal {
        label: "FLAT",
        transducer: "",
        physical_dim: "uV",
        physical_min: -12.5,
        physical_max: 12.5,
        digital_min: 100,
        digital_max: 100,
        prefiltering: "",
        samples_per_record: 10,
    };
    const zero_dur_hdr = EdfHeader {
        version: "0       ",
        patient_id: "",
        recording_id: "",
        start_date: "",
        start_time: "",
        header_bytes: 512,
        reserved: "",
        num_records: 1,
        record_duration_sec: 0.0,
        num_signals: 1,
    };
    expect_eq(approxEq(degenerate_sig.digitalToPhysical(100), -12.5, 1e-9), true);
    expect_eq(approxEq(degenerate_sig.sampleRateHz(zero_dur_hdr), 0.0, 1e-9), true);
});

test("3. edfRecordByteSize and cross-record / cross-channel readEdfDigitalSample", () => {
    let buf: [792]u8 = [0; 792];
    buildTwoChannelEdf(*buf, "EDF+C", "2");
    const data: []const u8 = buf[0..792];

    const hdr = parseEdfHeader(data) ?? unreachable;
    const eeg = parseEdfSignal(data, hdr, 0) ?? unreachable;

    // 每条记录：(4 + 2) * 2 = 12 字节
    expect_eq(edfRecordByteSize(data, hdr) ?? 0, 12);

    // 记录 0：EEG 通道（sig 0）
    expect_eq(readEdfDigitalSample(data, hdr, 0, 0, 0) ?? 0, -2000);
    expect_eq(readEdfDigitalSample(data, hdr, 0, 0, 1) ?? 0, 0);
    expect_eq(readEdfDigitalSample(data, hdr, 0, 0, 2) ?? 0, 1000);
    expect_eq(readEdfDigitalSample(data, hdr, 0, 0, 3) ?? 0, 2000);

    // 记录 0：ECG 通道（sig 1）
    expect_eq(readEdfDigitalSample(data, hdr, 0, 1, 0) ?? 0, -32768);
    expect_eq(readEdfDigitalSample(data, hdr, 0, 1, 1) ?? 0, 32767);

    // 记录 1：跨记录随机访问 + 物理量换算
    const s1_eeg3 = readEdfDigitalSample(data, hdr, 1, 0, 3) ?? unreachable;
    expect_eq(s1_eeg3, 1500);
    expect_eq(approxEq(eeg.digitalToPhysical(s1_eeg3), 375.0, 1e-9), true);

    expect_eq(readEdfDigitalSample(data, hdr, 1, 0, 0) ?? 0, -500);
    expect_eq(readEdfDigitalSample(data, hdr, 1, 0, 2) ?? 0, -125);
    expect_eq(readEdfDigitalSample(data, hdr, 1, 1, 0) ?? 0, -16384);
    expect_eq(readEdfDigitalSample(data, hdr, 1, 1, 1) ?? 0, 16384);
});

test("4. EDF+D / EDF+C real-time streaming header with num_records == -1", () => {
    let buf: [792]u8 = [0; 792];
    buildTwoChannelEdf(*buf, "EDF+D", "-1");
    const data: []const u8 = buf[0..792];

    expect_eq(isEdf(data), true);
    const hdr = parseEdfHeader(data) ?? unreachable;
    expect_eq(hdr.isEdfPlus(), true);
    expect_eq(bytesEq(hdr.reserved, "EDF+D"), true);
    expect_eq(hdr.num_records, -1);
    expect_eq(approxEq(hdr.totalDurationSec(), -1.0, 1e-9), true);

    // 实时流中已缓冲的记录 0 与记录 1 仍可安全读取
    expect_eq(readEdfDigitalSample(data, hdr, 0, 0, 2) ?? 0, 1000);
    expect_eq(readEdfDigitalSample(data, hdr, 1, 1, 1) ?? 0, 16384);
    // 超出物理缓冲长度的记录 2 必须安全返回 null
    expect_eq(readEdfDigitalSample(data, hdr, 2, 0, 0) == null, true);

    // 标准经典 EDF（非 EDF+）
    let classic: [792]u8 = [0; 792];
    buildTwoChannelEdf(*classic, "", "2");
    const chdr = parseEdfHeader(classic[0..792]) ?? unreachable;
    expect_eq(chdr.isEdfPlus(), false);
    expect_eq(approxEq(chdr.totalDurationSec(), 1.0, 1e-9), true);
});

test("5. Malformed version, header_bytes mismatch, truncated buffers, and invalid fields", () => {
    let buf: [792]u8 = [0; 792];
    buildTwoChannelEdf(*buf, "EDF+C", "2");

    // 截断主头 (< 256B)
    expect_eq(isEdf(buf[0..128]), false);
    expect_eq(parseEdfHeader(buf[0..255]) == null, true);

    // 错误版本号（不是 "0       "）
    let bad_ver: [792]u8 = buf;
    bad_ver[0] = '1';
    expect_eq(isEdf(bad_ver[0..792]), false);
    expect_eq(parseEdfHeader(bad_ver[0..792]) == null, true);

    let bad_ver_pad: [792]u8 = buf;
    bad_ver_pad[1] = '0';
    expect_eq(isEdf(bad_ver_pad[0..792]), false);

    // header_bytes 与 256 * (1 + num_signals) 不匹配（应为 768，改为 512）
    let bad_hdr_bytes: [792]u8 = buf;
    let slice_bhb: []u8 = bad_hdr_bytes[0..792];
    writePadded(*slice_bhb, 184, 8, "512");
    expect_eq(isEdf(bad_hdr_bytes[0..792]), false);
    expect_eq(parseEdfHeader(bad_hdr_bytes[0..792]) == null, true);

    // 非法 num_records (< -1)
    let bad_nrec: [792]u8 = buf;
    let slice_bnr: []u8 = bad_nrec[0..792];
    writePadded(*slice_bnr, 236, 8, "-2");
    expect_eq(parseEdfHeader(bad_nrec[0..792]) == null, true);

    // 负 record_duration_sec
    let bad_dur: [792]u8 = buf;
    let slice_bd: []u8 = bad_dur[0..792];
    writePadded(*slice_bd, 244, 8, "-0.5");
    expect_eq(parseEdfHeader(bad_dur[0..792]) == null, true);

    // 主头完整但通道头截断 (< 768B)
    const hdr = parseEdfHeader(buf[0..256]) ?? unreachable;
    expect_eq(parseEdfSignal(buf[0..500], hdr, 0) == null, true);
    expect_eq(edfRecordByteSize(buf[0..500], hdr) == null, true);
});

test("6. Out-of-bounds signal, record, sample indices and truncated half-sample protection", () => {
    let buf: [792]u8 = [0; 792];
    buildTwoChannelEdf(*buf, "EDF+C", "2");
    const data: []const u8 = buf[0..792];
    const hdr = parseEdfHeader(data) ?? unreachable;

    // 通道索引越界
    expect_eq(parseEdfSignal(data, hdr, 2) == null, true);
    expect_eq(readEdfDigitalSample(data, hdr, 0, 2, 0) == null, true);

    // 记录索引越界（共 2 条记录：0 和 1）
    expect_eq(readEdfDigitalSample(data, hdr, 2, 0, 0) == null, true);
    expect_eq(readEdfDigitalSample(data, hdr, 999, 1, 0) == null, true);

    // 样本索引越界（sig 0 有 4 样本，sig 1 有 2 样本）
    expect_eq(readEdfDigitalSample(data, hdr, 0, 0, 4) == null, true);
    expect_eq(readEdfDigitalSample(data, hdr, 1, 1, 2) == null, true);

    // 半样本截断防护（最后一个 16-bit 样本少 1 字节，长度 791）
    const truncated = buf[0..791];
    expect_eq(readEdfDigitalSample(truncated, hdr, 1, 1, 0) ?? 0, -16384);
    expect_eq(readEdfDigitalSample(truncated, hdr, 1, 1, 1) == null, true);
});
