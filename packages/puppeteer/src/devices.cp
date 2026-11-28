// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/common/Device.ts | Synced: 2026-11-28
// ============================================================================
// packages/puppeteer/src/devices.cp — KnownDevices Dictionary & Device Profiles
// Zero-heap-allocation static device descriptor table for Page.emulate()
// ============================================================================

const std = @import("std");
import { Viewport } from "./types";

pub struct Device {
    name: []const u8 = "",
    user_agent: []const u8 = "",
    viewport: Viewport = .{},
}

const UA_IPHONE_14_4: []const u8 = "Mozilla/5.0 (iPhone; CPU iPhone OS 14_4 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/15.4 Mobile/15E148 Safari/604.1";
const UA_IPHONE_15_0: []const u8 = "Mozilla/5.0 (iPhone; CPU iPhone OS 15_0 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/15.4 Mobile/15E148 Safari/604.1";
const UA_IPHONE_16_6: []const u8 = "Mozilla/5.0 (iPhone; CPU iPhone OS 16_6 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/16.6 Mobile/15E148 Safari/604.1";
const UA_IPHONE_17_5: []const u8 = "Mozilla/5.0 (iPhone; CPU iPhone OS 17_5 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.5 Mobile/15E148 Safari/604.1";
const UA_IPHONE_18_7: []const u8 = "Mozilla/5.0 (iPhone; CPU iPhone OS 18_7 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/26.5 Mobile/15E148 Safari/604.1";
const UA_IPHONE_X: []const u8 = "Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1";
const UA_IPAD_11_0: []const u8 = "Mozilla/5.0 (iPad; CPU OS 11_0 like Mac OS X) AppleWebKit/604.1.34 (KHTML, like Gecko) Version/11.0 Mobile/15A5341f Safari/604.1";
const UA_IPAD_12_2: []const u8 = "Mozilla/5.0 (iPad; CPU OS 12_2 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/15.4 Mobile/15E148 Safari/604.1";
const UA_PIXEL_3: []const u8 = "Mozilla/5.0 (Linux; Android 9; Pixel 3 Build/PQ1A.181105.017.A1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/66.0.3359.158 Mobile Safari/537.36";
const UA_PIXEL_4: []const u8 = "Mozilla/5.0 (Linux; Android 10; Pixel 4) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/81.0.4044.138 Mobile Safari/537.36";
const UA_PIXEL_5: []const u8 = "Mozilla/5.0 (Linux; Android 11; Pixel 5) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/99.0.4812.0 Mobile Safari/537.36";
const UA_GALAXY_S9_PLUS: []const u8 = "Mozilla/5.0 (Linux; Android 8.0.0; SM-G965U Build/R16NW) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/63.0.3239.111 Mobile Safari/537.36";
const UA_DESKTOP: []const u8 = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36";

const KNOWN_DEVICES_TABLE: [26]Device = .{
    Device{
        name: "iPhone X",
        user_agent: UA_IPHONE_X,
        viewport: Viewport{ width: 375, height: 812, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone SE",
        user_agent: UA_IPHONE_X,
        viewport: Viewport{ width: 320, height: 568, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 12",
        user_agent: UA_IPHONE_14_4,
        viewport: Viewport{ width: 390, height: 844, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 12 landscape",
        user_agent: UA_IPHONE_14_4,
        viewport: Viewport{ width: 844, height: 390, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 13",
        user_agent: UA_IPHONE_15_0,
        viewport: Viewport{ width: 390, height: 844, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 13 Pro",
        user_agent: UA_IPHONE_15_0,
        viewport: Viewport{ width: 390, height: 844, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 14",
        user_agent: UA_IPHONE_16_6,
        viewport: Viewport{ width: 390, height: 663, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 14 Pro",
        user_agent: UA_IPHONE_16_6,
        viewport: Viewport{ width: 393, height: 659, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 15",
        user_agent: UA_IPHONE_17_5,
        viewport: Viewport{ width: 393, height: 659, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 15 Pro",
        user_agent: UA_IPHONE_17_5,
        viewport: Viewport{ width: 393, height: 659, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 15 Pro Max",
        user_agent: UA_IPHONE_17_5,
        viewport: Viewport{ width: 430, height: 739, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 16",
        user_agent: UA_IPHONE_18_7,
        viewport: Viewport{ width: 393, height: 659, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 16 Pro",
        user_agent: UA_IPHONE_18_7,
        viewport: Viewport{ width: 402, height: 681, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 17 Pro",
        user_agent: UA_IPHONE_18_7,
        viewport: Viewport{ width: 402, height: 681, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPad",
        user_agent: UA_IPAD_11_0,
        viewport: Viewport{ width: 768, height: 1024, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPad landscape",
        user_agent: UA_IPAD_11_0,
        viewport: Viewport{ width: 1024, height: 768, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPad Mini",
        user_agent: UA_IPAD_11_0,
        viewport: Viewport{ width: 768, height: 1024, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPad Pro",
        user_agent: UA_IPAD_11_0,
        viewport: Viewport{ width: 1024, height: 1366, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPad Pro 11",
        user_agent: UA_IPAD_12_2,
        viewport: Viewport{ width: 834, height: 1194, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Pixel 3",
        user_agent: UA_PIXEL_3,
        viewport: Viewport{ width: 393, height: 786, device_scale_factor: 2.75, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Pixel 4",
        user_agent: UA_PIXEL_4,
        viewport: Viewport{ width: 353, height: 745, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Pixel 5",
        user_agent: UA_PIXEL_5,
        viewport: Viewport{ width: 393, height: 851, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Pixel 5 landscape",
        user_agent: UA_PIXEL_5,
        viewport: Viewport{ width: 851, height: 393, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Galaxy S9+",
        user_agent: UA_GALAXY_S9_PLUS,
        viewport: Viewport{ width: 320, height: 658, device_scale_factor: 4.5, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Desktop 1280x720",
        user_agent: UA_DESKTOP,
        viewport: Viewport{ width: 1280, height: 720, device_scale_factor: 1.0, is_mobile: false, has_touch: false, is_landscape: true },
    },
    Device{
        name: "Desktop 1920x1080",
        user_agent: UA_DESKTOP,
        viewport: Viewport{ width: 1920, height: 1080, device_scale_factor: 1.0, is_mobile: false, has_touch: false, is_landscape: true },
    },
};

pub struct KnownDevices {
    pub static fn count(): usize {
        return KNOWN_DEVICES_TABLE.len;
    }

    pub static fn all(): []const Device {
        return &KNOWN_DEVICES_TABLE;
    }

    pub static fn get(name: []const u8): Device? {
        if (name.len == 0) return null;
        for (dev of KNOWN_DEVICES_TABLE) {
            if (dev.name.len == name.len && dev.name[0] == name[0]) {
                if (std.mem.eql(u8, dev.name, name)) {
                    return dev;
                }
            }
        }
        return null;
    }

    pub static fn iPhone12(): Device { return KNOWN_DEVICES_TABLE[2]; }
    pub static fn iPhone13(): Device { return KNOWN_DEVICES_TABLE[4]; }
    pub static fn iPhone14(): Device { return KNOWN_DEVICES_TABLE[6]; }
    pub static fn iPhone15Pro(): Device { return KNOWN_DEVICES_TABLE[9]; }
    pub static fn iPhone16Pro(): Device { return KNOWN_DEVICES_TABLE[12]; }
    pub static fn iPhone17Pro(): Device { return KNOWN_DEVICES_TABLE[13]; }
    pub static fn iPad(): Device { return KNOWN_DEVICES_TABLE[14]; }
    pub static fn iPadPro(): Device { return KNOWN_DEVICES_TABLE[17]; }
    pub static fn pixel5(): Device { return KNOWN_DEVICES_TABLE[21]; }
    pub static fn desktop1080p(): Device { return KNOWN_DEVICES_TABLE[25]; }
}
