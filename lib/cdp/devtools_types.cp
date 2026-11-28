// Source: Absorbed from _ref/puppeteer/node_modules/devtools-protocol & Chrome DevTools Protocol 1.3 | Synced: 2026-11-28
// ============================================================================
// lib/cdp/devtools_types.cp — Strongly-Typed Extended CDP Protocol Domains
// Fetch, Emulation, Storage, Target, Browser, Log, Tracing (Zero-Heap)
// ============================================================================

const std = @import("std");
import { CdpError, CdpRequest } from "./protocol";

// ----------------------------------------------------------------------------
// Target Domain
// ----------------------------------------------------------------------------

pub struct TargetCreateTargetParams {
    url: []const u8 = "about:blank",
    width: u32 = 0,
    height: u32 = 0,
    browser_context_id: []const u8 = "",
}

pub struct TargetDomain {
    pub static fn createTarget(id: i64, p: TargetCreateTargetParams, params_buf: []u8): CdpRequest!CdpError {
        let s = if (p.browser_context_id.len > 0)
            std.fmt.bufPrint(params_buf, "{{\"url\":\"{s}\",\"browserContextId\":\"{s}\"}}", .{ p.url, p.browser_context_id }) catch return CdpError.BufferTooSmall
        else
            std.fmt.bufPrint(params_buf, "{{\"url\":\"{s}\"}}", .{p.url}) catch return CdpError.BufferTooSmall;
        return CdpRequest{ id: id, method: "Target.createTarget", params_json: s };
    }

    pub static fn attachToTarget(id: i64, target_id: []const u8, flatten: bool, params_buf: []u8): CdpRequest!CdpError {
        let s = std.fmt.bufPrint(
            params_buf,
            "{{\"targetId\":\"{s}\",\"flatten\":{}}}",
            .{ target_id, flatten },
        ) catch return CdpError.BufferTooSmall;
        return CdpRequest{ id: id, method: "Target.attachToTarget", params_json: s };
    }

    pub static fn closeTarget(id: i64, target_id: []const u8, params_buf: []u8): CdpRequest!CdpError {
        let s = std.fmt.bufPrint(params_buf, "{{\"targetId\":\"{s}\"}}", .{target_id}) catch return CdpError.BufferTooSmall;
        return CdpRequest{ id: id, method: "Target.closeTarget", params_json: s };
    }

    pub static fn setDiscoverTargets(id: i64, discover: bool, params_buf: []u8): CdpRequest!CdpError {
        let s = std.fmt.bufPrint(params_buf, "{{\"discover\":{}}}", .{discover}) catch return CdpError.BufferTooSmall;
        return CdpRequest{ id: id, method: "Target.setDiscoverTargets", params_json: s };
    }
}

// ----------------------------------------------------------------------------
// Emulation Domain (Viewport / Device / Timezone / Geolocation)
// ----------------------------------------------------------------------------

pub struct EmulationDeviceMetricsParams {
    width: u32 = 1280,
    height: u32 = 720,
    device_scale_factor: f64 = 1.0,
    mobile: bool = false,
}

pub struct EmulationDomain {
    pub static fn setDeviceMetricsOverride(id: i64, session_id: []const u8, p: EmulationDeviceMetricsParams, params_buf: []u8): CdpRequest!CdpError {
        let s = std.fmt.bufPrint(
            params_buf,
            "{{\"width\":{d},\"height\":{d},\"deviceScaleFactor\":{d:.2},\"mobile\":{}}}",
            .{ p.width, p.height, p.device_scale_factor, p.mobile },
        ) catch return CdpError.BufferTooSmall;
        return CdpRequest{ id: id, method: "Emulation.setDeviceMetricsOverride", params_json: s, session_id: session_id };
    }

    pub static fn setUserAgentOverride(id: i64, session_id: []const u8, user_agent: []const u8, params_buf: []u8): CdpRequest!CdpError {
        let s = std.fmt.bufPrint(params_buf, "{{\"userAgent\":\"{s}\"}}", .{user_agent}) catch return CdpError.BufferTooSmall;
        return CdpRequest{ id: id, method: "Emulation.setUserAgentOverride", params_json: s, session_id: session_id };
    }

    pub static fn setTimezoneOverride(id: i64, session_id: []const u8, timezone_id: []const u8, params_buf: []u8): CdpRequest!CdpError {
        let s = std.fmt.bufPrint(params_buf, "{{\"timezoneId\":\"{s}\"}}", .{timezone_id}) catch return CdpError.BufferTooSmall;
        return CdpRequest{ id: id, method: "Emulation.setTimezoneOverride", params_json: s, session_id: session_id };
    }
}

// ----------------------------------------------------------------------------
// Fetch Domain (Request Interception)
// ----------------------------------------------------------------------------

pub struct FetchDomain {
    pub static fn enable(id: i64, session_id: []const u8, url_pattern: []const u8, params_buf: []u8): CdpRequest!CdpError {
        let pat = if (url_pattern.len > 0) url_pattern else "*";
        let s = std.fmt.bufPrint(params_buf, "{{\"patterns\":[{{\"urlPattern\":\"{s}\"}}]}}", .{pat}) catch return CdpError.BufferTooSmall;
        return CdpRequest{ id: id, method: "Fetch.enable", params_json: s, session_id: session_id };
    }

    pub static fn continueRequest(id: i64, session_id: []const u8, request_id: []const u8, params_buf: []u8): CdpRequest!CdpError {
        let s = std.fmt.bufPrint(params_buf, "{{\"requestId\":\"{s}\"}}", .{request_id}) catch return CdpError.BufferTooSmall;
        return CdpRequest{ id: id, method: "Fetch.continueRequest", params_json: s, session_id: session_id };
    }

    pub static fn failRequest(id: i64, session_id: []const u8, request_id: []const u8, reason: []const u8, params_buf: []u8): CdpRequest!CdpError {
        let r = if (reason.len > 0) reason else "Failed";
        let s = std.fmt.bufPrint(params_buf, "{{\"requestId\":\"{s}\",\"errorReason\":\"{s}\"}}", .{ request_id, r }) catch return CdpError.BufferTooSmall;
        return CdpRequest{ id: id, method: "Fetch.failRequest", params_json: s, session_id: session_id };
    }
}

// ----------------------------------------------------------------------------
// Storage, Browser, Log & Tracing Domains
// ----------------------------------------------------------------------------

pub struct StorageDomain {
    pub static fn clearDataForOrigin(id: i64, session_id: []const u8, origin: []const u8, storage_types: []const u8, params_buf: []u8): CdpRequest!CdpError {
        let st = if (storage_types.len > 0) storage_types else "all";
        let s = std.fmt.bufPrint(params_buf, "{{\"origin\":\"{s}\",\"storageTypes\":\"{s}\"}}", .{ origin, st }) catch return CdpError.BufferTooSmall;
        return CdpRequest{ id: id, method: "Storage.clearDataForOrigin", params_json: s, session_id: session_id };
    }
}

pub struct BrowserDomain {
    pub static fn getVersion(id: i64): CdpRequest {
        return CdpRequest{ id: id, method: "Browser.getVersion", params_json: "{}" };
    }

    pub static fn close(id: i64): CdpRequest {
        return CdpRequest{ id: id, method: "Browser.close", params_json: "{}" };
    }
}

pub struct LogDomain {
    pub static fn enable(id: i64, session_id: []const u8): CdpRequest {
        return CdpRequest{ id: id, method: "Log.enable", params_json: "{}", session_id: session_id };
    }

    pub static fn clear(id: i64, session_id: []const u8): CdpRequest {
        return CdpRequest{ id: id, method: "Log.clear", params_json: "{}", session_id: session_id };
    }
}

pub struct TracingDomain {
    pub static fn start(id: i64, categories: []const u8, params_buf: []u8): CdpRequest!CdpError {
        let cats = if (categories.len > 0) categories else "devtools.timeline";
        let s = std.fmt.bufPrint(params_buf, "{{\"categories\":\"{s}\"}}", .{cats}) catch return CdpError.BufferTooSmall;
        return CdpRequest{ id: id, method: "Tracing.start", params_json: s };
    }

    pub static fn end(id: i64): CdpRequest {
        return CdpRequest{ id: id, method: "Tracing.end", params_json: "{}" };
    }
}

// ----------------------------------------------------------------------------
// Extended DevTools Domains: Overlay, Audits, Profiler, HeapProfiler,
// IndexedDB, CacheStorage, Security, Performance, ServiceWorker, CSS, Media
// ----------------------------------------------------------------------------

pub struct OverlayDomain {
    pub static fn enable(id: i64, session_id: []const u8): CdpRequest {
        return CdpRequest{ id: id, method: "Overlay.enable", params_json: "{}", session_id: session_id };
    }

    pub static fn hideHighlight(id: i64, session_id: []const u8): CdpRequest {
        return CdpRequest{ id: id, method: "Overlay.hideHighlight", params_json: "{}", session_id: session_id };
    }
}

pub struct AuditsDomain {
    pub static fn enable(id: i64, session_id: []const u8): CdpRequest {
        return CdpRequest{ id: id, method: "Audits.enable", params_json: "{}", session_id: session_id };
    }
}

pub struct ProfilerDomain {
    pub static fn enable(id: i64, session_id: []const u8): CdpRequest {
        return CdpRequest{ id: id, method: "Profiler.enable", params_json: "{}", session_id: session_id };
    }

    pub static fn startPreciseCoverage(id: i64, session_id: []const u8, call_count: bool, detailed: bool, params_buf: []u8): CdpRequest!CdpError {
        let s = std.fmt.bufPrint(params_buf, "{{\"callCount\":{},\"detailed\":{}}}", .{ call_count, detailed }) catch return CdpError.BufferTooSmall;
        return CdpRequest{ id: id, method: "Profiler.startPreciseCoverage", params_json: s, session_id: session_id };
    }

    pub static fn takePreciseCoverage(id: i64, session_id: []const u8): CdpRequest {
        return CdpRequest{ id: id, method: "Profiler.takePreciseCoverage", params_json: "{}", session_id: session_id };
    }

    pub static fn stopPreciseCoverage(id: i64, session_id: []const u8): CdpRequest {
        return CdpRequest{ id: id, method: "Profiler.stopPreciseCoverage", params_json: "{}", session_id: session_id };
    }
}

pub struct HeapProfilerDomain {
    pub static fn enable(id: i64, session_id: []const u8): CdpRequest {
        return CdpRequest{ id: id, method: "HeapProfiler.enable", params_json: "{}", session_id: session_id };
    }

    pub static fn collectGarbage(id: i64, session_id: []const u8): CdpRequest {
        return CdpRequest{ id: id, method: "HeapProfiler.collectGarbage", params_json: "{}", session_id: session_id };
    }
}

pub struct IndexedDBDomain {
    pub static fn enable(id: i64, session_id: []const u8): CdpRequest {
        return CdpRequest{ id: id, method: "IndexedDB.enable", params_json: "{}", session_id: session_id };
    }
}

pub struct CacheStorageDomain {
    pub static fn requestCacheNames(id: i64, session_id: []const u8, security_origin: []const u8, params_buf: []u8): CdpRequest!CdpError {
        let s = std.fmt.bufPrint(params_buf, "{{\"securityOrigin\":\"{s}\"}}", .{security_origin}) catch return CdpError.BufferTooSmall;
        return CdpRequest{ id: id, method: "CacheStorage.requestCacheNames", params_json: s, session_id: session_id };
    }
}

pub struct SecurityDomain {
    pub static fn enable(id: i64, session_id: []const u8): CdpRequest {
        return CdpRequest{ id: id, method: "Security.enable", params_json: "{}", session_id: session_id };
    }

    pub static fn setIgnoreCertificateErrors(id: i64, ignore: bool, params_buf: []u8): CdpRequest!CdpError {
        let s = std.fmt.bufPrint(params_buf, "{{\"ignore\":{}}}", .{ignore}) catch return CdpError.BufferTooSmall;
        return CdpRequest{ id: id, method: "Security.setIgnoreCertificateErrors", params_json: s };
    }
}

pub struct PerformanceDomain {
    pub static fn enable(id: i64, session_id: []const u8): CdpRequest {
        return CdpRequest{ id: id, method: "Performance.enable", params_json: "{}", session_id: session_id };
    }

    pub static fn getMetrics(id: i64, session_id: []const u8): CdpRequest {
        return CdpRequest{ id: id, method: "Performance.getMetrics", params_json: "{}", session_id: session_id };
    }
}

pub struct ServiceWorkerDomain {
    pub static fn enable(id: i64, session_id: []const u8): CdpRequest {
        return CdpRequest{ id: id, method: "ServiceWorker.enable", params_json: "{}", session_id: session_id };
    }
}

pub struct CSSDomain {
    pub static fn enable(id: i64, session_id: []const u8): CdpRequest {
        return CdpRequest{ id: id, method: "CSS.enable", params_json: "{}", session_id: session_id };
    }

    pub static fn startRuleUsageTracking(id: i64, session_id: []const u8): CdpRequest {
        return CdpRequest{ id: id, method: "CSS.startRuleUsageTracking", params_json: "{}", session_id: session_id };
    }

    pub static fn stopRuleUsageTracking(id: i64, session_id: []const u8): CdpRequest {
        return CdpRequest{ id: id, method: "CSS.stopRuleUsageTracking", params_json: "{}", session_id: session_id };
    }
}

pub struct AnimationDomain {
    pub static fn enable(id: i64, session_id: []const u8): CdpRequest {
        return CdpRequest{ id: id, method: "Animation.enable", params_json: "{}", session_id: session_id };
    }
}

pub struct MediaDomain {
    pub static fn enable(id: i64, session_id: []const u8): CdpRequest {
        return CdpRequest{ id: id, method: "Media.enable", params_json: "{}", session_id: session_id };
    }
}
