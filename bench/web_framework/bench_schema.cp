// ReleaseFast Performance Benchmark for `lib/web/schema.cp` & `lib/web/fileType.cp`
import {
    fastDecodeUriComponent,
    ObjectSchema,
    validateJsonObject,
    exactMirrorJson,
} from "../../lib/web/schema.cp";
import {
    detectFileType,
} from "../../lib/web/fileType.cp";
import { WallClock } from "../../lib/time/clock.cp";

fn printThroughput(label: string, ops: usize, bytes_per_op: usize, dt_ns: i64, acc: usize) {
    const dt_safe: f64 = if (dt_ns > 0) @as(f64, @floatFromInt(dt_ns)) else 1.0;
    const dt_ms: f64 = dt_safe / 1000000.0;
    const secs: f64 = dt_safe / 1000000000.0;
    const ops_per_sec: f64 = @as(f64, @floatFromInt(ops)) / secs;
    const mops: f64 = ops_per_sec / 1000000.0;
    const ns_per_op: f64 = dt_safe / @as(f64, @floatFromInt(ops));
    if (bytes_per_op > 0) {
        const total_bytes: f64 = @as(f64, @floatFromInt(ops)) * @as(f64, @floatFromInt(bytes_per_op));
        const mb_per_sec: f64 = (total_bytes / (1024.0 * 1024.0)) / secs;
        @print("{s}: {d:.2}ms ({d:.1} ns/op) | {d:.2}M ops/s ({d:.0} ops/s) | {d:.1} MB/s | acc={}", label, dt_ms, ns_per_op, mops, ops_per_sec, mb_per_sec, acc);
    } else {
        @print("{s}: {d:.2}ms ({d:.1} ns/op) | {d:.2}M ops/s ({d:.0} ops/s) | acc={}", label, dt_ms, ns_per_op, mops, ops_per_sec, acc);
    }
}

fn runBenchmarks() {
    let out_buf: [512]u8 = [0; 512];

    // ========================================================================
    // Bench 1a: 16-lane SIMD fastDecodeUriComponent pure ASCII fast-path (5M, 64B)
    // ========================================================================
    const ascii_inputs: [4]string = [
        "api_v1_users_profile_settings_notifications_preferences_01234567",
        "cdn_assets_static_bundles_main_chunk_9f8e7d6c5b4a3210_abcdef1234",
        "search_query_parameters_sort_by_created_at_order_desc_page_00042",
        "workspace_projects_deployments_production_us_east_1_cluster_0099",
    ];
    const n_uri_ascii: usize = 5000000;
    let acc1a: usize = 0;
    const t0 = WallClock.monotonicNs();
    let i: usize = 0;
    while (i < n_uri_ascii) : (i += 1) {
        if (n := fastDecodeUriComponent(ascii_inputs[i & 3], out_buf[0..])) {
            acc1a += n + @as(usize, out_buf[i & 31]);
        }
    }
    const dt1a = WallClock.monotonicNs() - t0;

    // ========================================================================
    // Bench 1b: 16-lane SIMD fastDecodeUriComponent UTF-8 percent-decoding (2M)
    // ========================================================================
    const utf8_inputs: [4]string = [
        "q=%E4%B8%AD%E6%96%87%20search%2Fitem%3Fid%3D1001%26tag%3Dfast_cp",
        "p=%E4%B8%AD%E6%96%87%2D%E6%B5%8B%E8%AF%95%2Fpath%2Fto%2Ffile%21",
        "name=%E4%B8%AD%E6%96%87%20user%40example.com%26role%3Dadmin_001",
        "msg=hello%20%E4%B8%AD%E6%96%87%20world%21%20status%3Dok%26v%3D2",
    ];
    const n_uri_utf8: usize = 2000000;
    let acc1b: usize = 0;
    const t1 = WallClock.monotonicNs();
    i = 0;
    while (i < n_uri_utf8) : (i += 1) {
        if (n := fastDecodeUriComponent(utf8_inputs[i & 3], out_buf[0..])) {
            acc1b += n + @as(usize, out_buf[0]);
        }
    }
    const dt1b = WallClock.monotonicNs() - t1;

    // ========================================================================
    // Bench 2: Zero-reflection validateJsonObject (1M, 5-field user signup JSON)
    // ========================================================================
    let signup_schema = ObjectSchema.init("UserSignup");
    * = signup_schema
        .strField("username", true, 3, 32)
        .emailField("email", true)
        .intField("age", true, 18, 120)
        .enumField("role", true, "user,admin,moderator")
        .boolField("accepted_tos", true);

    const signup_jsons: [4]string = [
        "{\"username\":\"alice_dev\",\"email\":\"alice@example.com\",\"age\":28,\"role\":\"user\",\"accepted_tos\":true}",
        "{\"username\":\"bob_ops\",\"email\":\"bob@cp-lang.org\",\"age\":35,\"role\":\"admin\",\"accepted_tos\":true}",
        "{\"username\":\"carol_qa\",\"email\":\"carol@web.io\",\"age\":24,\"role\":\"moderator\",\"accepted_tos\":false}",
        "{\"username\":\"dave_sec\",\"email\":\"dave@cloud.dev\",\"age\":41,\"role\":\"user\",\"accepted_tos\":true}",
    ];
    const n_validate: usize = 1000000;
    let acc2: usize = 0;
    const t2 = WallClock.monotonicNs();
    i = 0;
    while (i < n_validate) : (i += 1) {
        const rep = validateJsonObject(signup_jsons[i & 3], &signup_schema);
        if (rep.ok) {
            acc2 += 1;
        }
    }
    const dt2 = WallClock.monotonicNs() - t2;

    // ========================================================================
    // Bench 3: exactMirrorJson (1M, strip sensitive fields & emit clean JSON)
    // ========================================================================
    const dirty_jsons: [4]string = [
        "{\"username\":\"alice_dev\",\"password_hash\":\"$2b$12$secret1\",\"email\":\"alice@example.com\",\"internal_role\":\"superadmin\",\"age\":28,\"role\":\"user\",\"accepted_tos\":true,\"api_secret\":\"sk_live_001\"}",
        "{\"username\":\"bob_ops\",\"password_hash\":\"$2b$12$secret2\",\"email\":\"bob@cp-lang.org\",\"internal_role\":\"root\",\"age\":35,\"role\":\"admin\",\"accepted_tos\":true,\"api_secret\":\"sk_live_002\"}",
        "{\"username\":\"carol_qa\",\"password_hash\":\"$2b$12$secret3\",\"email\":\"carol@web.io\",\"internal_role\":\"staff\",\"age\":24,\"role\":\"moderator\",\"accepted_tos\":false,\"api_secret\":\"sk_live_003\"}",
        "{\"username\":\"dave_sec\",\"password_hash\":\"$2b$12$secret4\",\"email\":\"dave@cloud.dev\",\"internal_role\":\"ops\",\"age\":41,\"role\":\"user\",\"accepted_tos\":true,\"api_secret\":\"sk_live_004\"}",
    ];
    const n_mirror: usize = 1000000;
    let acc3: usize = 0;
    const t3 = WallClock.monotonicNs();
    i = 0;
    while (i < n_mirror) : (i += 1) {
        if (written := exactMirrorJson(dirty_jsons[i & 3], &signup_schema, out_buf[0..])) {
            acc3 += written + @as(usize, out_buf[written - 1]);
        }
    }
    const dt3 = WallClock.monotonicNs() - t3;

    // ========================================================================
    // Bench 4: detectFileType magic-number probe (5M, PNG/PDF/WASM/JSON mixed)
    // ========================================================================
    const png_hdr: [16]u8 = [0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 'I', 'H', 'D', 'R'];
    const pdf_hdr: [16]u8 = ['%', 'P', 'D', 'F', '-', '1', '.', '7', '\n', '%', 0xE2, 0xE3, 0xCF, 0xD3, '\n', '1'];
    const wasm_hdr: [16]u8 = [0x00, 'a', 's', 'm', 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60, 0x00, 0x01, 0x7F, 0x00];
    const json_hdr: [16]u8 = ['{', '"', 'o', 'k', '"', ':', 't', 'r', 'u', 'e', ',', '"', 'v', '"', ':', '1'];
    const json_full: string = "{\"ok\":true,\"v\":1}";
    const samples: [4]string = [
        png_hdr[0..],
        pdf_hdr[0..],
        wasm_hdr[0..],
        json_full,
    ];
    * = json_hdr;
    const n_ftype: usize = 5000000;
    let acc4: usize = 0;
    const t4 = WallClock.monotonicNs();
    i = 0;
    while (i < n_ftype) : (i += 1) {
        if (info := detectFileType(samples[i & 3])) {
            acc4 += info.ext.len + info.mime.len;
        }
    }
    const dt4 = WallClock.monotonicNs() - t4;

    printThroughput("1a_uri_ascii_5M_64B", n_uri_ascii, 64, dt1a, acc1a);
    printThroughput("1b_uri_utf8_2M_63B", n_uri_utf8, 63, dt1b, acc1b);
    printThroughput("2_validate_json_1M_5f", n_validate, 102, dt2, acc2);
    printThroughput("3_exact_mirror_1M", n_mirror, 196, dt3, acc3);
    printThroughput("4_detect_filetype_5M_mix", n_ftype, 16, dt4, acc4);
}

fn main() {
    runBenchmarks();
}
