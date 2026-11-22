// Origin: Pure-CP Stdlib Test Suite (lib/prompt/multimodal.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Multimodal Attachment Validator & Tile Token Estimator:
//   1. Exact Vision 512x512 tile scaling (Low, High, Auto, 1024x1024=765, 2048x4096=1105, 0x0=0)
//   2. Audio ceiling token estimation & document page/byte estimation
//   3. Case-insensitive MIME whitelist with `;params` stripping & unsafe/cross-modality rejection
//   4. Full mixed 4-modality audit pass with exact per-modality token breakdown
//   5. All policy rejection reasons (TooManyParts, TotalBytes, ImageDim, AudioDur, DocPages, TokenBudget)
//   6. Empty parts list, zero image dimensions & invalid MultimodalLimits defensive guards

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MediaPartKind,
    ImageDetailMode,
    MultimodalRejectReason,
    MultimodalPart,
    MultimodalLimits,
    estimateImageTokens,
    estimateAudioTokens,
    isAllowedMultimodalMime,
    auditMultimodalPayload,
} from "cap:prompt";

// ============================================================================
// Test 1: Exact Vision Tile Token Calculation (Low / High / Auto)
// ============================================================================

fn testVisionTileTokenEstimation()! {
    // Low detail: always fixed 85 tokens regardless of dimensions
    expect_eq(cap.prompt.estimateImageTokens(4096, 8192, ImageDetailMode.Low), 85);
    expect_eq(estimateImageTokens(64, 64, ImageDetailMode.Low), 85);

    // High detail canonical Vision cases:
    //   512x512   -> 1x1 = 1 tile  -> 85 + 170*1 = 255
    //   1024x1024 -> 768x768 -> 2x2 = 4 tiles -> 85 + 170*4 = 765
    //   2048x4096 -> 1024x2048 -> 768x1536 -> 2x3 = 6 tiles -> 85 + 170*6 = 1105
    expect_eq(estimateImageTokens(512, 512, ImageDetailMode.High), 255);
    expect_eq(estimateImageTokens(1024, 1024, ImageDetailMode.High), 765);
    expect_eq(estimateImageTokens(2048, 4096, ImageDetailMode.High), 1105);

    // Auto detail: <=512x512 uses Low (85); larger uses High
    expect_eq(estimateImageTokens(512, 512, ImageDetailMode.Auto), 85);
    expect_eq(estimateImageTokens(1024, 1024, ImageDetailMode.Auto), 765);

    // Zero width or height safely returns 0
    expect_eq(estimateImageTokens(0, 1024, ImageDetailMode.High), 0);
    expect_eq(estimateImageTokens(1024, 0, ImageDetailMode.Low), 0);
}

// ============================================================================
// Test 2: Audio Ceiling Token Estimation
// ============================================================================

fn testAudioTokenEstimation()! {
    expect_eq(estimateAudioTokens(0, 32), 0);
    expect_eq(estimateAudioTokens(1, 32), 1);         // ceil(0.032) = 1
    expect_eq(estimateAudioTokens(1000, 32), 32);     // exact 1s
    expect_eq(estimateAudioTokens(1500, 32), 48);     // 1.5s * 32 = 48
    expect_eq(estimateAudioTokens(1501, 32), 49);     // ceil(48.032) = 49
    expect_eq(estimateAudioTokens(2000, 0), 64);      // default 32 tok/s when rate == 0
    expect_eq(estimateAudioTokens(10000, 50), 500);   // custom rate 50 tok/s
}

// ============================================================================
// Test 3: MIME Whitelist, Case-Insensitivity & Parameter Stripping
// ============================================================================

fn testMimeWhitelist()! {
    // Allowed types across all 4 modalities (including uppercase & ;charset params)
    expect_eq(isAllowedMultimodalMime(MediaPartKind.Image, "image/png"), true);
    expect_eq(isAllowedMultimodalMime(MediaPartKind.Image, "IMAGE/JPEG"), true);
    expect_eq(isAllowedMultimodalMime(MediaPartKind.Image, "image/webp"), true);
    expect_eq(isAllowedMultimodalMime(MediaPartKind.Image, "image/gif"), true);

    expect_eq(isAllowedMultimodalMime(MediaPartKind.Audio, "audio/wav"), true);
    expect_eq(isAllowedMultimodalMime(MediaPartKind.Audio, "audio/mpeg"), true);
    expect_eq(isAllowedMultimodalMime(MediaPartKind.Audio, "audio/ogg; codecs=opus"), true);
    expect_eq(isAllowedMultimodalMime(MediaPartKind.Audio, "audio/flac"), true);

    expect_eq(isAllowedMultimodalMime(MediaPartKind.Document, "application/pdf"), true);
    expect_eq(isAllowedMultimodalMime(MediaPartKind.Document, "text/markdown; charset=utf-8"), true);
    expect_eq(isAllowedMultimodalMime(MediaPartKind.Text, "text/csv"), true);

    // Rejected: unsafe types, cross-modality mismatch, empty string
    expect_eq(isAllowedMultimodalMime(MediaPartKind.Image, "image/svg+xml"), false);
    expect_eq(isAllowedMultimodalMime(MediaPartKind.Document, "text/html"), false);
    expect_eq(isAllowedMultimodalMime(MediaPartKind.Audio, "image/png"), false);
    expect_eq(isAllowedMultimodalMime(MediaPartKind.Text, "application/pdf"), false);
    expect_eq(isAllowedMultimodalMime(MediaPartKind.Text, ""), false);
}

// ============================================================================
// Test 4: Full Mixed 4-Modality Payload Audit Pass
// ============================================================================

fn testMixedPayloadAuditPass()! {
    const parts: [4]MultimodalPart = [
        MultimodalPart{ kind: MediaPartKind.Text, mime_type: "text/plain", byte_size: 400 },
        MultimodalPart{ kind: MediaPartKind.Image, mime_type: "image/png", byte_size: 120000, width: 1024, height: 1024, image_detail: ImageDetailMode.High },
        MultimodalPart{ kind: MediaPartKind.Audio, mime_type: "audio/wav", byte_size: 64000, duration_ms: 5000 },
        MultimodalPart{ kind: MediaPartKind.Document, mime_type: "application/pdf", byte_size: 50000, page_count: 3 },
    ];
    const res = auditMultimodalPayload(parts[0..], MultimodalLimits.default());
    expect_eq(res.allowed, true);
    expect_eq(res.reject_reason == MultimodalRejectReason.None, true);
    expect_eq(res.total_parts, 4);
    expect_eq(res.total_bytes, 234400);
    expect_eq(res.text_tokens, 100);      // 400 / 4
    expect_eq(res.image_tokens, 765);     // 1024x1024 High
    expect_eq(res.audio_tokens, 160);     // 5s * 32
    expect_eq(res.document_tokens, 768);  // 3 pages * 256
    expect_eq(res.total_tokens, 1793);
}

// ============================================================================
// Test 5: Policy Limit Rejections (Count, MIME, Bytes, Dim, Duration, Pages, Budget)
// ============================================================================

fn testPolicyLimitRejections()! {
    // 1. TooManyParts
    const two_parts: [2]MultimodalPart = [
        MultimodalPart{ kind: MediaPartKind.Text, mime_type: "text/plain", byte_size: 10 },
        MultimodalPart{ kind: MediaPartKind.Text, mime_type: "text/plain", byte_size: 10 },
    ];
    const r_cnt = auditMultimodalPayload(two_parts[0..], MultimodalLimits{ max_parts: 1 });
    expect_eq(r_cnt.allowed, false);
    expect_eq(r_cnt.reject_reason == MultimodalRejectReason.TooManyParts, true);

    // 2. DisallowedMime
    const bad_mime: [1]MultimodalPart = [
        MultimodalPart{ kind: MediaPartKind.Image, mime_type: "image/svg+xml", byte_size: 100, width: 100, height: 100 },
    ];
    const r_mime = auditMultimodalPayload(bad_mime[0..], MultimodalLimits.default());
    expect_eq(r_mime.allowed, false);
    expect_eq(r_mime.reject_reason == MultimodalRejectReason.DisallowedMime, true);
    expect_eq(r_mime.offending_index ?? 99, 0);

    // 3. TotalBytesExceeded
    const big_bytes: [1]MultimodalPart = [
        MultimodalPart{ kind: MediaPartKind.Text, mime_type: "text/plain", byte_size: 2048 },
    ];
    const r_bytes = auditMultimodalPayload(big_bytes[0..], MultimodalLimits{ max_total_bytes: 1024 });
    expect_eq(r_bytes.allowed, false);
    expect_eq(r_bytes.reject_reason == MultimodalRejectReason.TotalBytesExceeded, true);

    // 4. ImageDimensionExceeded
    const huge_img: [1]MultimodalPart = [
        MultimodalPart{ kind: MediaPartKind.Image, mime_type: "image/png", byte_size: 1000, width: 9000, height: 100 },
    ];
    const r_dim = auditMultimodalPayload(huge_img[0..], MultimodalLimits.default());
    expect_eq(r_dim.allowed, false);
    expect_eq(r_dim.reject_reason == MultimodalRejectReason.ImageDimensionExceeded, true);

    // 5. AudioDurationExceeded
    const long_aud: [1]MultimodalPart = [
        MultimodalPart{ kind: MediaPartKind.Audio, mime_type: "audio/mp3", byte_size: 1000, duration_ms: 400000 },
    ];
    const r_aud = auditMultimodalPayload(long_aud[0..], MultimodalLimits.default());
    expect_eq(r_aud.allowed, false);
    expect_eq(r_aud.reject_reason == MultimodalRejectReason.AudioDurationExceeded, true);

    // 6. DocumentPagesExceeded
    const big_doc: [1]MultimodalPart = [
        MultimodalPart{ kind: MediaPartKind.Document, mime_type: "application/pdf", byte_size: 1000, page_count: 150 },
    ];
    const r_doc = auditMultimodalPayload(big_doc[0..], MultimodalLimits.default());
    expect_eq(r_doc.allowed, false);
    expect_eq(r_doc.reject_reason == MultimodalRejectReason.DocumentPagesExceeded, true);

    // 7. TokenBudgetExceeded
    const pricey_img: [1]MultimodalPart = [
        MultimodalPart{ kind: MediaPartKind.Image, mime_type: "image/png", byte_size: 1000, width: 2048, height: 4096, image_detail: ImageDetailMode.High },
    ];
    const r_tok = auditMultimodalPayload(pricey_img[0..], MultimodalLimits{ max_total_tokens: 500 });
    expect_eq(r_tok.allowed, false);
    expect_eq(r_tok.reject_reason == MultimodalRejectReason.TokenBudgetExceeded, true);
}

// ============================================================================
// Test 6: Empty Parts, Zero Image Dimensions & Invalid Limits Guards
// ============================================================================

fn testBoundaryAndInvalidGuards()! {
    const dummy: [1]MultimodalPart = [MultimodalPart{}];
    const empty_parts: []const MultimodalPart = dummy[0..0];

    // Empty parts list is valid with 0 tokens
    const r_empty = auditMultimodalPayload(empty_parts, MultimodalLimits.default());
    expect_eq(r_empty.allowed, true);
    expect_eq(r_empty.total_tokens, 0);

    // Zero width/height image rejected with InvalidDimensions
    const zero_img: [1]MultimodalPart = [
        MultimodalPart{ kind: MediaPartKind.Image, mime_type: "image/png", byte_size: 100, width: 0, height: 512 },
    ];
    const r_z = auditMultimodalPayload(zero_img[0..], MultimodalLimits.default());
    expect_eq(r_z.allowed, false);
    expect_eq(r_z.reject_reason == MultimodalRejectReason.InvalidDimensions, true);

    // Invalid limits rejected with InvalidLimits
    expect_eq(auditMultimodalPayload(empty_parts, MultimodalLimits{ max_parts: 0 }).reject_reason == MultimodalRejectReason.InvalidLimits, true);
    expect_eq(auditMultimodalPayload(empty_parts, MultimodalLimits{ max_total_bytes: 0 }).reject_reason == MultimodalRejectReason.InvalidLimits, true);
    expect_eq(auditMultimodalPayload(empty_parts, MultimodalLimits{ text_bytes_per_token: 0 }).reject_reason == MultimodalRejectReason.InvalidLimits, true);
}

// ============================================================================
// Test Runner Registration
// ============================================================================

test("prompt/multimodal: exact Vision 512x512 tile scaling (Low/High/Auto)", testVisionTileTokenEstimation);
test("prompt/multimodal: audio ceiling token estimation", testAudioTokenEstimation);
test("prompt/multimodal: case-insensitive MIME whitelist & parameter stripping", testMimeWhitelist);
test("prompt/multimodal: mixed 4-modality payload audit pass & token breakdown", testMixedPayloadAuditPass);
test("prompt/multimodal: policy limit rejections & offending_index tracking", testPolicyLimitRejections);
test("prompt/multimodal: empty parts, zero dimensions & invalid limits guards", testBoundaryAndInvalidGuards);
