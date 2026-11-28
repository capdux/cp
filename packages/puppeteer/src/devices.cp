// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/common/Device.ts | Synced: 2026-11-28
// ============================================================================
// packages/puppeteer/src/devices.cp — KnownDevices Dictionary & Device Profiles
// Zero-heap-allocation static device descriptor table for Page.emulate() (155 profiles)
// ============================================================================

const std = @import("std");
import { Viewport } from "./types";

pub struct Device {
    name: []const u8 = "",
    user_agent: []const u8 = "",
    viewport: Viewport = .{},
}

const UA_0: []const u8 = "Mozilla/5.0 (PlayBook; U; RIM Tablet OS 2.1.0; en-US) AppleWebKit/536.2+ (KHTML like Gecko) Version/7.2.1.0 Safari/536.2+";
const UA_1: []const u8 = "Mozilla/5.0 (BB10; Touch) AppleWebKit/537.10+ (KHTML, like Gecko) Version/10.0.9.2372 Mobile Safari/537.10+";
const UA_2: []const u8 = "Mozilla/5.0 (Linux; U; Android 4.3; en-us; SM-N900T Build/JSS15J) AppleWebKit/534.30 (KHTML, like Gecko) Version/4.0 Mobile Safari/534.30";
const UA_3: []const u8 = "Mozilla/5.0 (Linux; U; Android 4.1; en-us; GT-N7100 Build/JRO03C) AppleWebKit/534.30 (KHTML, like Gecko) Version/4.0 Mobile Safari/534.30";
const UA_4: []const u8 = "Mozilla/5.0 (Linux; U; Android 4.0; en-us; GT-I9300 Build/IMM76D) AppleWebKit/534.30 (KHTML, like Gecko) Version/4.0 Mobile Safari/534.30";
const UA_5: []const u8 = "Mozilla/5.0 (Linux; Android 5.0; SM-G900P Build/LRX21T) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3765.0 Mobile Safari/537.36";
const UA_6: []const u8 = "Mozilla/5.0 (Linux; Android 7.0; SM-G950U Build/NRD90M) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/62.0.3202.84 Mobile Safari/537.36";
const UA_7: []const u8 = "Mozilla/5.0 (Linux; Android 8.0.0; SM-G965U Build/R16NW) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/63.0.3239.111 Mobile Safari/537.36";
const UA_8: []const u8 = "Mozilla/5.0 (Linux; Android 8.1.0; SM-T837A) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/70.0.3538.80 Safari/537.36";
const UA_9: []const u8 = "Mozilla/5.0 (iPad; CPU OS 11_0 like Mac OS X) AppleWebKit/604.1.34 (KHTML, like Gecko) Version/11.0 Mobile/15A5341f Safari/604.1";
const UA_10: []const u8 = "Mozilla/5.0 (iPad; CPU OS 12_2 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/15.4 Mobile/15E148 Safari/604.1";
const UA_11: []const u8 = "Mozilla/5.0 (iPhone; CPU iPhone OS 7_1_2 like Mac OS X) AppleWebKit/537.51.2 (KHTML, like Gecko) Version/7.0 Mobile/11D257 Safari/9537.53";
const UA_12: []const u8 = "Mozilla/5.0 (iPhone; CPU iPhone OS 10_3_1 like Mac OS X) AppleWebKit/603.1.30 (KHTML, like Gecko) Version/10.0 Mobile/14E304 Safari/602.1";
const UA_13: []const u8 = "Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1";
const UA_14: []const u8 = "Mozilla/5.0 (iPhone; CPU iPhone OS 18_5 like Mac OS X) AppleWebKit/603.1.30 (KHTML, like Gecko) Version/26.5 Mobile/19E241 Safari/602.1";
const UA_15: []const u8 = "Mozilla/5.0 (iPhone; CPU iPhone OS 12_0 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/12.0 Mobile/15E148 Safari/604.1";
const UA_16: []const u8 = "Mozilla/5.0 (iPhone; CPU iPhone OS 13_7 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/13.1 Mobile/15E148 Safari/604.1";
const UA_17: []const u8 = "Mozilla/5.0 (iPhone; CPU iPhone OS 14_4 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/15.4 Mobile/15E148 Safari/604.1";
const UA_18: []const u8 = "Mozilla/5.0 (iPhone; CPU iPhone OS 15_0 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/15.4 Mobile/15E148 Safari/604.1";
const UA_19: []const u8 = "Mozilla/5.0 (iPhone; CPU iPhone OS 16_6 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/16.6 Mobile/15E148 Safari/604.1";
const UA_20: []const u8 = "Mozilla/5.0 (iPhone; CPU iPhone OS 17_5 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.5 Mobile/15E148 Safari/604.1";
const UA_21: []const u8 = "Mozilla/5.0 (iPhone; CPU iPhone OS 18_7 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/26.5 Mobile/15E148 Safari/604.1";
const UA_22: []const u8 = "Mozilla/5.0 (Mobile; LYF/F300B/LYF-F300B-001-01-15-130718-i;Android; rv:48.0) Gecko/48.0 Firefox/48.0 KAIOS/2.5";
const UA_23: []const u8 = "Mozilla/5.0 (Linux; U; en-us; KFAPWI Build/JDQ39) AppleWebKit/535.19 (KHTML, like Gecko) Silk/3.13 Safari/535.19 Silk-Accelerated=true";
const UA_24: []const u8 = "Mozilla/5.0 (Linux; U; Android 4.4.2; en-us; LGMS323 Build/KOT49I.MS32310c) AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/75.0.3765.0 Mobile Safari/537.36";
const UA_25: []const u8 = "Mozilla/5.0 (Windows Phone 10.0; Android 4.2.1; Microsoft; Lumia 550) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2486.0 Mobile Safari/537.36 Edge/14.14263";
const UA_26: []const u8 = "Mozilla/5.0 (Windows Phone 10.0; Android 4.2.1; Microsoft; Lumia 950) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2486.0 Mobile Safari/537.36 Edge/14.14263";
const UA_27: []const u8 = "Mozilla/5.0 (Linux; Android 6.0.1; Nexus 10 Build/MOB31T) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3765.0 Safari/537.36";
const UA_28: []const u8 = "Mozilla/5.0 (Linux; Android 4.4.2; Nexus 4 Build/KOT49H) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3765.0 Mobile Safari/537.36";
const UA_29: []const u8 = "Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3765.0 Mobile Safari/537.36";
const UA_30: []const u8 = "Mozilla/5.0 (Linux; Android 8.0.0; Nexus 5X Build/OPR4.170623.006) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3765.0 Mobile Safari/537.36";
const UA_31: []const u8 = "Mozilla/5.0 (Linux; Android 7.1.1; Nexus 6 Build/N6F26U) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3765.0 Mobile Safari/537.36";
const UA_32: []const u8 = "Mozilla/5.0 (Linux; Android 8.0.0; Nexus 6P Build/OPP3.170518.006) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3765.0 Mobile Safari/537.36";
const UA_33: []const u8 = "Mozilla/5.0 (Linux; Android 6.0.1; Nexus 7 Build/MOB30X) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3765.0 Safari/537.36";
const UA_34: []const u8 = "Mozilla/5.0 (compatible; MSIE 10.0; Windows Phone 8.0; Trident/6.0; IEMobile/10.0; ARM; Touch; NOKIA; Lumia 520)";
const UA_35: []const u8 = "Mozilla/5.0 (MeeGo; NokiaN9) AppleWebKit/534.13 (KHTML, like Gecko) NokiaBrowser/8.5.0 Mobile Safari/534.13";
const UA_36: []const u8 = "Mozilla/5.0 (Linux; Android 8.0; Pixel 2 Build/OPD3.170816.012) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3765.0 Mobile Safari/537.36";
const UA_37: []const u8 = "Mozilla/5.0 (Linux; Android 8.0.0; Pixel 2 XL Build/OPD1.170816.004) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3765.0 Mobile Safari/537.36";
const UA_38: []const u8 = "Mozilla/5.0 (Linux; Android 9; Pixel 3 Build/PQ1A.181105.017.A1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/66.0.3359.158 Mobile Safari/537.36";
const UA_39: []const u8 = "Mozilla/5.0 (Linux; Android 10; Pixel 4) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/81.0.4044.138 Mobile Safari/537.36";
const UA_40: []const u8 = "Mozilla/5.0 (Linux; Android 11; Pixel 4a (5G)) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/99.0.4812.0 Mobile Safari/537.36";
const UA_41: []const u8 = "Mozilla/5.0 (Linux; Android 11; Pixel 5) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/99.0.4812.0 Mobile Safari/537.36";
const UA_42: []const u8 = "Mozilla/5.0 (Linux; Android 7.0; Moto G (4)) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/99.0.4812.0 Mobile Safari/537.36";
const UA_43: []const u8 = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36";

const KNOWN_DEVICES_TABLE: [155]Device = .{
    Device{
        name: "Blackberry PlayBook",
        user_agent: UA_0,
        viewport: Viewport{ width: 600, height: 1024, device_scale_factor: 1.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Blackberry PlayBook landscape",
        user_agent: UA_0,
        viewport: Viewport{ width: 1024, height: 600, device_scale_factor: 1.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "BlackBerry Z30",
        user_agent: UA_1,
        viewport: Viewport{ width: 360, height: 640, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "BlackBerry Z30 landscape",
        user_agent: UA_1,
        viewport: Viewport{ width: 640, height: 360, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Galaxy Note 3",
        user_agent: UA_2,
        viewport: Viewport{ width: 360, height: 640, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Galaxy Note 3 landscape",
        user_agent: UA_2,
        viewport: Viewport{ width: 640, height: 360, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Galaxy Note II",
        user_agent: UA_3,
        viewport: Viewport{ width: 360, height: 640, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Galaxy Note II landscape",
        user_agent: UA_3,
        viewport: Viewport{ width: 640, height: 360, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Galaxy S III",
        user_agent: UA_4,
        viewport: Viewport{ width: 360, height: 640, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Galaxy S III landscape",
        user_agent: UA_4,
        viewport: Viewport{ width: 640, height: 360, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Galaxy S5",
        user_agent: UA_5,
        viewport: Viewport{ width: 360, height: 640, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Galaxy S5 landscape",
        user_agent: UA_5,
        viewport: Viewport{ width: 640, height: 360, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Galaxy S8",
        user_agent: UA_6,
        viewport: Viewport{ width: 360, height: 740, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Galaxy S8 landscape",
        user_agent: UA_6,
        viewport: Viewport{ width: 740, height: 360, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Galaxy S9+",
        user_agent: UA_7,
        viewport: Viewport{ width: 320, height: 658, device_scale_factor: 4.5, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Galaxy S9+ landscape",
        user_agent: UA_7,
        viewport: Viewport{ width: 658, height: 320, device_scale_factor: 4.5, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Galaxy Tab S4",
        user_agent: UA_8,
        viewport: Viewport{ width: 712, height: 1138, device_scale_factor: 2.25, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Galaxy Tab S4 landscape",
        user_agent: UA_8,
        viewport: Viewport{ width: 1138, height: 712, device_scale_factor: 2.25, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPad",
        user_agent: UA_9,
        viewport: Viewport{ width: 768, height: 1024, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPad landscape",
        user_agent: UA_9,
        viewport: Viewport{ width: 1024, height: 768, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPad (gen 6)",
        user_agent: UA_10,
        viewport: Viewport{ width: 768, height: 1024, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPad (gen 6) landscape",
        user_agent: UA_10,
        viewport: Viewport{ width: 1024, height: 768, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPad (gen 7)",
        user_agent: UA_10,
        viewport: Viewport{ width: 810, height: 1080, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPad (gen 7) landscape",
        user_agent: UA_10,
        viewport: Viewport{ width: 1080, height: 810, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPad Mini",
        user_agent: UA_9,
        viewport: Viewport{ width: 768, height: 1024, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPad Mini landscape",
        user_agent: UA_9,
        viewport: Viewport{ width: 1024, height: 768, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPad Pro",
        user_agent: UA_9,
        viewport: Viewport{ width: 1024, height: 1366, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPad Pro landscape",
        user_agent: UA_9,
        viewport: Viewport{ width: 1366, height: 1024, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPad Pro 11",
        user_agent: UA_10,
        viewport: Viewport{ width: 834, height: 1194, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPad Pro 11 landscape",
        user_agent: UA_10,
        viewport: Viewport{ width: 1194, height: 834, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 4",
        user_agent: UA_11,
        viewport: Viewport{ width: 320, height: 480, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 4 landscape",
        user_agent: UA_11,
        viewport: Viewport{ width: 480, height: 320, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 5",
        user_agent: UA_12,
        viewport: Viewport{ width: 320, height: 568, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 5 landscape",
        user_agent: UA_12,
        viewport: Viewport{ width: 568, height: 320, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 6",
        user_agent: UA_13,
        viewport: Viewport{ width: 375, height: 667, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 6 landscape",
        user_agent: UA_13,
        viewport: Viewport{ width: 667, height: 375, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 6 Plus",
        user_agent: UA_13,
        viewport: Viewport{ width: 414, height: 736, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 6 Plus landscape",
        user_agent: UA_13,
        viewport: Viewport{ width: 736, height: 414, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 7",
        user_agent: UA_13,
        viewport: Viewport{ width: 375, height: 667, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 7 landscape",
        user_agent: UA_13,
        viewport: Viewport{ width: 667, height: 375, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 7 Plus",
        user_agent: UA_13,
        viewport: Viewport{ width: 414, height: 736, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 7 Plus landscape",
        user_agent: UA_13,
        viewport: Viewport{ width: 736, height: 414, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 8",
        user_agent: UA_13,
        viewport: Viewport{ width: 375, height: 667, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 8 landscape",
        user_agent: UA_13,
        viewport: Viewport{ width: 667, height: 375, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 8 Plus",
        user_agent: UA_13,
        viewport: Viewport{ width: 414, height: 736, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 8 Plus landscape",
        user_agent: UA_13,
        viewport: Viewport{ width: 736, height: 414, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone SE",
        user_agent: UA_12,
        viewport: Viewport{ width: 320, height: 568, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone SE landscape",
        user_agent: UA_12,
        viewport: Viewport{ width: 568, height: 320, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone SE (3rd gen)",
        user_agent: UA_14,
        viewport: Viewport{ width: 375, height: 667, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone SE (3rd gen) landscape",
        user_agent: UA_14,
        viewport: Viewport{ width: 667, height: 375, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone X",
        user_agent: UA_13,
        viewport: Viewport{ width: 375, height: 812, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone X landscape",
        user_agent: UA_13,
        viewport: Viewport{ width: 812, height: 375, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone XR",
        user_agent: UA_15,
        viewport: Viewport{ width: 414, height: 896, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone XR landscape",
        user_agent: UA_15,
        viewport: Viewport{ width: 896, height: 414, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 11",
        user_agent: UA_16,
        viewport: Viewport{ width: 414, height: 828, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 11 landscape",
        user_agent: UA_16,
        viewport: Viewport{ width: 828, height: 414, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 11 Pro",
        user_agent: UA_16,
        viewport: Viewport{ width: 375, height: 812, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 11 Pro landscape",
        user_agent: UA_16,
        viewport: Viewport{ width: 812, height: 375, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 11 Pro Max",
        user_agent: UA_16,
        viewport: Viewport{ width: 414, height: 896, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 11 Pro Max landscape",
        user_agent: UA_16,
        viewport: Viewport{ width: 896, height: 414, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 12",
        user_agent: UA_17,
        viewport: Viewport{ width: 390, height: 844, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 12 landscape",
        user_agent: UA_17,
        viewport: Viewport{ width: 844, height: 390, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 12 Pro",
        user_agent: UA_17,
        viewport: Viewport{ width: 390, height: 844, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 12 Pro landscape",
        user_agent: UA_17,
        viewport: Viewport{ width: 844, height: 390, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 12 Pro Max",
        user_agent: UA_17,
        viewport: Viewport{ width: 428, height: 926, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 12 Pro Max landscape",
        user_agent: UA_17,
        viewport: Viewport{ width: 926, height: 428, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 12 Mini",
        user_agent: UA_17,
        viewport: Viewport{ width: 375, height: 812, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 12 Mini landscape",
        user_agent: UA_17,
        viewport: Viewport{ width: 812, height: 375, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 13",
        user_agent: UA_18,
        viewport: Viewport{ width: 390, height: 844, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 13 landscape",
        user_agent: UA_18,
        viewport: Viewport{ width: 844, height: 390, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 13 Pro",
        user_agent: UA_18,
        viewport: Viewport{ width: 390, height: 844, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 13 Pro landscape",
        user_agent: UA_18,
        viewport: Viewport{ width: 844, height: 390, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 13 Pro Max",
        user_agent: UA_18,
        viewport: Viewport{ width: 428, height: 926, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 13 Pro Max landscape",
        user_agent: UA_18,
        viewport: Viewport{ width: 926, height: 428, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 13 Mini",
        user_agent: UA_18,
        viewport: Viewport{ width: 375, height: 812, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 13 Mini landscape",
        user_agent: UA_18,
        viewport: Viewport{ width: 812, height: 375, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 14",
        user_agent: UA_19,
        viewport: Viewport{ width: 390, height: 663, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 14 landscape",
        user_agent: UA_19,
        viewport: Viewport{ width: 750, height: 340, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 14 Plus",
        user_agent: UA_19,
        viewport: Viewport{ width: 428, height: 745, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 14 Plus landscape",
        user_agent: UA_19,
        viewport: Viewport{ width: 832, height: 378, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 14 Pro",
        user_agent: UA_19,
        viewport: Viewport{ width: 393, height: 659, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 14 Pro landscape",
        user_agent: UA_19,
        viewport: Viewport{ width: 734, height: 343, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 14 Pro Max",
        user_agent: UA_19,
        viewport: Viewport{ width: 430, height: 739, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 14 Pro Max landscape",
        user_agent: UA_19,
        viewport: Viewport{ width: 814, height: 380, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 15",
        user_agent: UA_20,
        viewport: Viewport{ width: 393, height: 659, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 15 landscape",
        user_agent: UA_20,
        viewport: Viewport{ width: 734, height: 343, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 15 Plus",
        user_agent: UA_20,
        viewport: Viewport{ width: 430, height: 739, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 15 Plus landscape",
        user_agent: UA_20,
        viewport: Viewport{ width: 814, height: 380, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 15 Pro",
        user_agent: UA_20,
        viewport: Viewport{ width: 393, height: 659, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 15 Pro landscape",
        user_agent: UA_20,
        viewport: Viewport{ width: 734, height: 343, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 15 Pro Max",
        user_agent: UA_20,
        viewport: Viewport{ width: 430, height: 739, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 15 Pro Max landscape",
        user_agent: UA_20,
        viewport: Viewport{ width: 814, height: 380, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 16",
        user_agent: UA_21,
        viewport: Viewport{ width: 393, height: 659, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 16 landscape",
        user_agent: UA_21,
        viewport: Viewport{ width: 734, height: 343, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 16 Plus",
        user_agent: UA_21,
        viewport: Viewport{ width: 430, height: 739, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 16 Plus landscape",
        user_agent: UA_21,
        viewport: Viewport{ width: 814, height: 380, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 16 Pro",
        user_agent: UA_21,
        viewport: Viewport{ width: 402, height: 681, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 16 Pro landscape",
        user_agent: UA_21,
        viewport: Viewport{ width: 756, height: 352, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 16 Pro Max",
        user_agent: UA_21,
        viewport: Viewport{ width: 440, height: 763, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 16 Pro Max landscape",
        user_agent: UA_21,
        viewport: Viewport{ width: 838, height: 390, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 16e",
        user_agent: UA_21,
        viewport: Viewport{ width: 390, height: 651, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 16e landscape",
        user_agent: UA_21,
        viewport: Viewport{ width: 726, height: 340, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 17",
        user_agent: UA_21,
        viewport: Viewport{ width: 402, height: 681, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 17 landscape",
        user_agent: UA_21,
        viewport: Viewport{ width: 756, height: 352, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone Air",
        user_agent: UA_21,
        viewport: Viewport{ width: 420, height: 719, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone Air landscape",
        user_agent: UA_21,
        viewport: Viewport{ width: 794, height: 370, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 17 Pro",
        user_agent: UA_21,
        viewport: Viewport{ width: 402, height: 681, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 17 Pro landscape",
        user_agent: UA_21,
        viewport: Viewport{ width: 756, height: 352, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 17 Pro Max",
        user_agent: UA_21,
        viewport: Viewport{ width: 440, height: 763, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 17 Pro Max landscape",
        user_agent: UA_21,
        viewport: Viewport{ width: 838, height: 390, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "iPhone 17e",
        user_agent: UA_21,
        viewport: Viewport{ width: 390, height: 651, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "iPhone 17e landscape",
        user_agent: UA_21,
        viewport: Viewport{ width: 726, height: 340, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "JioPhone 2",
        user_agent: UA_22,
        viewport: Viewport{ width: 240, height: 320, device_scale_factor: 1.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "JioPhone 2 landscape",
        user_agent: UA_22,
        viewport: Viewport{ width: 320, height: 240, device_scale_factor: 1.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Kindle Fire HDX",
        user_agent: UA_23,
        viewport: Viewport{ width: 800, height: 1280, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Kindle Fire HDX landscape",
        user_agent: UA_23,
        viewport: Viewport{ width: 1280, height: 800, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "LG Optimus L70",
        user_agent: UA_24,
        viewport: Viewport{ width: 384, height: 640, device_scale_factor: 1.25, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "LG Optimus L70 landscape",
        user_agent: UA_24,
        viewport: Viewport{ width: 640, height: 384, device_scale_factor: 1.25, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Microsoft Lumia 550",
        user_agent: UA_25,
        viewport: Viewport{ width: 640, height: 360, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Microsoft Lumia 950",
        user_agent: UA_26,
        viewport: Viewport{ width: 360, height: 640, device_scale_factor: 4.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Microsoft Lumia 950 landscape",
        user_agent: UA_26,
        viewport: Viewport{ width: 640, height: 360, device_scale_factor: 4.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Nexus 10",
        user_agent: UA_27,
        viewport: Viewport{ width: 800, height: 1280, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Nexus 10 landscape",
        user_agent: UA_27,
        viewport: Viewport{ width: 1280, height: 800, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Nexus 4",
        user_agent: UA_28,
        viewport: Viewport{ width: 384, height: 640, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Nexus 4 landscape",
        user_agent: UA_28,
        viewport: Viewport{ width: 640, height: 384, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Nexus 5",
        user_agent: UA_29,
        viewport: Viewport{ width: 360, height: 640, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Nexus 5 landscape",
        user_agent: UA_29,
        viewport: Viewport{ width: 640, height: 360, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Nexus 5X",
        user_agent: UA_30,
        viewport: Viewport{ width: 412, height: 732, device_scale_factor: 2.625, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Nexus 5X landscape",
        user_agent: UA_30,
        viewport: Viewport{ width: 732, height: 412, device_scale_factor: 2.625, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Nexus 6",
        user_agent: UA_31,
        viewport: Viewport{ width: 412, height: 732, device_scale_factor: 3.5, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Nexus 6 landscape",
        user_agent: UA_31,
        viewport: Viewport{ width: 732, height: 412, device_scale_factor: 3.5, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Nexus 6P",
        user_agent: UA_32,
        viewport: Viewport{ width: 412, height: 732, device_scale_factor: 3.5, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Nexus 6P landscape",
        user_agent: UA_32,
        viewport: Viewport{ width: 732, height: 412, device_scale_factor: 3.5, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Nexus 7",
        user_agent: UA_33,
        viewport: Viewport{ width: 600, height: 960, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Nexus 7 landscape",
        user_agent: UA_33,
        viewport: Viewport{ width: 960, height: 600, device_scale_factor: 2.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Nokia Lumia 520",
        user_agent: UA_34,
        viewport: Viewport{ width: 320, height: 533, device_scale_factor: 1.5, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Nokia Lumia 520 landscape",
        user_agent: UA_34,
        viewport: Viewport{ width: 533, height: 320, device_scale_factor: 1.5, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Nokia N9",
        user_agent: UA_35,
        viewport: Viewport{ width: 480, height: 854, device_scale_factor: 1.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Nokia N9 landscape",
        user_agent: UA_35,
        viewport: Viewport{ width: 854, height: 480, device_scale_factor: 1.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Pixel 2",
        user_agent: UA_36,
        viewport: Viewport{ width: 411, height: 731, device_scale_factor: 2.625, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Pixel 2 landscape",
        user_agent: UA_36,
        viewport: Viewport{ width: 731, height: 411, device_scale_factor: 2.625, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Pixel 2 XL",
        user_agent: UA_37,
        viewport: Viewport{ width: 411, height: 823, device_scale_factor: 3.5, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Pixel 2 XL landscape",
        user_agent: UA_37,
        viewport: Viewport{ width: 823, height: 411, device_scale_factor: 3.5, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Pixel 3",
        user_agent: UA_38,
        viewport: Viewport{ width: 393, height: 786, device_scale_factor: 2.75, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Pixel 3 landscape",
        user_agent: UA_38,
        viewport: Viewport{ width: 786, height: 393, device_scale_factor: 2.75, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Pixel 4",
        user_agent: UA_39,
        viewport: Viewport{ width: 353, height: 745, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Pixel 4 landscape",
        user_agent: UA_39,
        viewport: Viewport{ width: 745, height: 353, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Pixel 4a (5G)",
        user_agent: UA_40,
        viewport: Viewport{ width: 353, height: 745, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Pixel 4a (5G) landscape",
        user_agent: UA_40,
        viewport: Viewport{ width: 745, height: 353, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Pixel 5",
        user_agent: UA_41,
        viewport: Viewport{ width: 393, height: 851, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Pixel 5 landscape",
        user_agent: UA_41,
        viewport: Viewport{ width: 851, height: 393, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Moto G4",
        user_agent: UA_42,
        viewport: Viewport{ width: 360, height: 640, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: false },
    },
    Device{
        name: "Moto G4 landscape",
        user_agent: UA_42,
        viewport: Viewport{ width: 640, height: 360, device_scale_factor: 3.0, is_mobile: true, has_touch: true, is_landscape: true },
    },
    Device{
        name: "Desktop 1280x720",
        user_agent: UA_43,
        viewport: Viewport{ width: 1280, height: 720, device_scale_factor: 1.0, is_mobile: false, has_touch: false, is_landscape: true },
    },
    Device{
        name: "Desktop 1920x1080",
        user_agent: UA_43,
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

    pub static fn iPhone12(): Device { return KNOWN_DEVICES_TABLE[60]; }
    pub static fn iPhone13(): Device { return KNOWN_DEVICES_TABLE[68]; }
    pub static fn iPhone14(): Device { return KNOWN_DEVICES_TABLE[76]; }
    pub static fn iPhone15Pro(): Device { return KNOWN_DEVICES_TABLE[88]; }
    pub static fn iPhone16Pro(): Device { return KNOWN_DEVICES_TABLE[96]; }
    pub static fn iPhone17Pro(): Device { return KNOWN_DEVICES_TABLE[106]; }
    pub static fn iPad(): Device { return KNOWN_DEVICES_TABLE[18]; }
    pub static fn iPadPro(): Device { return KNOWN_DEVICES_TABLE[26]; }
    pub static fn pixel5(): Device { return KNOWN_DEVICES_TABLE[149]; }
    pub static fn desktop1080p(): Device { return KNOWN_DEVICES_TABLE[154]; }
}
