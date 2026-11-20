// Source: /data/dev/bun/src/css/targets.cp
// lib/css/engine/targets.cp
// LightningCSS Browser Targets, Feature Compatibility Matrix, and Vendor Prefix Bitmasks

const std = @import("std");

/// Packs major.minor.patch into LightningCSS's 24-bit integer representation.
pub fn packBrowserVersion(major: u32, minor: u32, patch: u32): u32 {
    return (major << 16) | (minor << 8) | patch;
}

pub fn unpackMajor(packed_ver: u32): u32 {
    return (packed_ver >> 16) & 0xFF;
}

pub fn unpackMinor(packed_ver: u32): u32 {
    return (packed_ver >> 8) & 0xFF;
}

pub fn unpackPatch(packed_ver: u32): u32 {
    return packed_ver & 0xFF;
}

/// Target browser versions (24-bit `major << 16 | minor << 8 | patch`).
pub struct Browsers {
    android: u32? = null;
    chrome: u32? = null;
    edge: u32? = null;
    firefox: u32? = null;
    ie: u32? = null;
    ios_saf: u32? = null;
    opera: u32? = null;
    safari: u32? = null;
    samsung: u32? = null;

    /// Parses a version string such as `"87"`, `"14.1"`, or `"16.4.1"` into a packed u32.
    pub static fn parseVersion(raw: []const u8): u32? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (s.len == 0) return null;

        let parts: [3]u32 = [0, 0, 0];
        let idx: usize = 0;
        let it = std.mem.splitScalar(u8, s, '.');
        while (seg := it.next()) {
            if (idx >= 3) break;
            const trimmed = std.mem.trim(u8, seg, " \t");
            if (trimmed.len == 0) return null;
            parts[idx] = std.fmt.parseInt(u32, trimmed, 10) catch return null;
            idx += 1;
        }
        if (idx == 0) return null;
        return packBrowserVersion(parts[0], parts[1], parts[2]);
    }

    pub static fn browserDefault(): Browsers {
        return Browsers {
            chrome: packBrowserVersion(87, 0, 0),
            edge: packBrowserVersion(88, 0, 0),
            firefox: packBrowserVersion(78, 0, 0),
            safari: packBrowserVersion(14, 0, 0),
        };
    }

    pub static fn modern(): Browsers {
        return Browsers {
            chrome: packBrowserVersion(120, 0, 0),
            edge: packBrowserVersion(120, 0, 0),
            firefox: packBrowserVersion(121, 0, 0),
            safari: packBrowserVersion(17, 2, 0),
        };
    }

    pub static fn legacy(): Browsers {
        return Browsers {
            chrome: packBrowserVersion(49, 0, 0),
            edge: packBrowserVersion(14, 0, 0),
            firefox: packBrowserVersion(45, 0, 0),
            ie: packBrowserVersion(11, 0, 0),
            safari: packBrowserVersion(10, 0, 0),
            ios_saf: packBrowserVersion(10, 0, 0),
            android: packBrowserVersion(5, 0, 0),
        };
    }

    fn mergeMin(*_, field_name: []const u8, ver: u32) {
        if (std.mem.eql(u8, field_name, "chrome")) {
            if (_.chrome == null || ver < _.chrome.?) _.chrome = ver;
        } else if (std.mem.eql(u8, field_name, "edge")) {
            if (_.edge == null || ver < _.edge.?) _.edge = ver;
        } else if (std.mem.eql(u8, field_name, "firefox")) {
            if (_.firefox == null || ver < _.firefox.?) _.firefox = ver;
        } else if (std.mem.eql(u8, field_name, "safari")) {
            if (_.safari == null || ver < _.safari.?) _.safari = ver;
        } else if (std.mem.eql(u8, field_name, "ios")) {
            if (_.ios_saf == null || ver < _.ios_saf.?) _.ios_saf = ver;
        } else if (std.mem.eql(u8, field_name, "opera")) {
            if (_.opera == null || ver < _.opera.?) _.opera = ver;
        } else if (std.mem.eql(u8, field_name, "ie")) {
            if (_.ie == null || ver < _.ie.?) _.ie = ver;
        } else if (std.mem.eql(u8, field_name, "android")) {
            if (_.android == null || ver < _.android.?) _.android = ver;
        } else if (std.mem.eql(u8, field_name, "samsung")) {
            if (_.samsung == null || ver < _.samsung.?) _.samsung = ver;
        }
    }

    fn applySingleTarget(*_, entry_raw: []const u8) {
        const entry = std.mem.trim(u8, entry_raw, " \t\r\n");
        if (entry.len == 0 || std.ascii.eqlIgnoreCase(entry, "esnext")) return;

        // Expand es2015..es2024 presets (matches Vite/esbuild/Bun target mapping)
        if (entry.len >= 6 && (entry[0] == 'e' || entry[0] == 'E') && (entry[1] == 's' || entry[1] == 'S')) {
            const year = std.fmt.parseInt(u16, entry[2..], 10) catch 0;
            if (year == 2015) {
                _.mergeMin("chrome", packBrowserVersion(49, 0, 0));
                _.mergeMin("edge", packBrowserVersion(13, 0, 0));
                _.mergeMin("safari", packBrowserVersion(10, 0, 0));
                _.mergeMin("firefox", packBrowserVersion(44, 0, 0));
                _.mergeMin("opera", packBrowserVersion(36, 0, 0));
                return;
            } else if (year == 2016) {
                _.mergeMin("chrome", packBrowserVersion(50, 0, 0));
                _.mergeMin("edge", packBrowserVersion(13, 0, 0));
                _.mergeMin("safari", packBrowserVersion(10, 0, 0));
                _.mergeMin("firefox", packBrowserVersion(43, 0, 0));
                _.mergeMin("opera", packBrowserVersion(37, 0, 0));
                return;
            } else if (year == 2017) {
                _.mergeMin("chrome", packBrowserVersion(58, 0, 0));
                _.mergeMin("edge", packBrowserVersion(15, 0, 0));
                _.mergeMin("safari", packBrowserVersion(11, 0, 0));
                _.mergeMin("firefox", packBrowserVersion(52, 0, 0));
                _.mergeMin("opera", packBrowserVersion(45, 0, 0));
                return;
            } else if (year == 2018) {
                _.mergeMin("chrome", packBrowserVersion(63, 0, 0));
                _.mergeMin("edge", packBrowserVersion(79, 0, 0));
                _.mergeMin("safari", packBrowserVersion(12, 0, 0));
                _.mergeMin("firefox", packBrowserVersion(58, 0, 0));
                _.mergeMin("opera", packBrowserVersion(50, 0, 0));
                return;
            } else if (year == 2019) {
                _.mergeMin("chrome", packBrowserVersion(73, 0, 0));
                _.mergeMin("edge", packBrowserVersion(79, 0, 0));
                _.mergeMin("safari", packBrowserVersion(12, 1, 0));
                _.mergeMin("firefox", packBrowserVersion(64, 0, 0));
                _.mergeMin("opera", packBrowserVersion(60, 0, 0));
                return;
            } else if (year == 2020) {
                _.mergeMin("chrome", packBrowserVersion(80, 0, 0));
                _.mergeMin("edge", packBrowserVersion(80, 0, 0));
                _.mergeMin("safari", packBrowserVersion(14, 1, 0));
                _.mergeMin("firefox", packBrowserVersion(80, 0, 0));
                _.mergeMin("opera", packBrowserVersion(67, 0, 0));
                return;
            } else if (year == 2021) {
                _.mergeMin("chrome", packBrowserVersion(85, 0, 0));
                _.mergeMin("edge", packBrowserVersion(85, 0, 0));
                _.mergeMin("safari", packBrowserVersion(14, 1, 0));
                _.mergeMin("firefox", packBrowserVersion(80, 0, 0));
                _.mergeMin("opera", packBrowserVersion(71, 0, 0));
                return;
            } else if (year == 2022) {
                _.mergeMin("chrome", packBrowserVersion(94, 0, 0));
                _.mergeMin("edge", packBrowserVersion(94, 0, 0));
                _.mergeMin("safari", packBrowserVersion(16, 4, 0));
                _.mergeMin("firefox", packBrowserVersion(93, 0, 0));
                _.mergeMin("opera", packBrowserVersion(80, 0, 0));
                return;
            } else if (year >= 2023) {
                _.mergeMin("chrome", packBrowserVersion(110, 0, 0));
                _.mergeMin("edge", packBrowserVersion(110, 0, 0));
                _.mergeMin("safari", packBrowserVersion(16, 4, 0));
                _.mergeMin("opera", packBrowserVersion(96, 0, 0));
                return;
            }
        }

        // Find first digit separating browser name and version
        let split_idx: usize = 0;
        while (split_idx < entry.len && !std.ascii.isDigit(entry[split_idx])) {
            split_idx += 1;
        }
        if (split_idx == 0 || split_idx >= entry.len) return;

        const name = std.mem.trim(u8, entry[0..split_idx], " \t_-");
        const ver = Browsers.parseVersion(entry[split_idx..]) ?? return;

        if (std.ascii.eqlIgnoreCase(name, "chrome")) _.mergeMin("chrome", ver)
        else if (std.ascii.eqlIgnoreCase(name, "edge")) _.mergeMin("edge", ver)
        else if (std.ascii.eqlIgnoreCase(name, "firefox")) _.mergeMin("firefox", ver)
        else if (std.ascii.eqlIgnoreCase(name, "safari")) _.mergeMin("safari", ver)
        else if (std.ascii.eqlIgnoreCase(name, "ios") || std.ascii.eqlIgnoreCase(name, "iossaf") || std.ascii.eqlIgnoreCase(name, "ios_saf")) _.mergeMin("ios", ver)
        else if (std.ascii.eqlIgnoreCase(name, "opera")) _.mergeMin("opera", ver)
        else if (std.ascii.eqlIgnoreCase(name, "ie")) _.mergeMin("ie", ver)
        else if (std.ascii.eqlIgnoreCase(name, "android")) _.mergeMin("android", ver)
        else if (std.ascii.eqlIgnoreCase(name, "samsung")) _.mergeMin("samsung", ver);
    }

    /// Parses target tokens such as `["chrome87", "safari14.1", "es2020"]`.
    pub static fn fromTargets(targets: []const []const u8): Browsers {
        let b = Browsers {};
        for (t of targets) {
            b.applySingleTarget(t);
        }
        return b;
    }

    pub fn hasAny(): bool {
        return _.android != null || _.chrome != null || _.edge != null ||
            _.firefox != null || _.ie != null || _.ios_saf != null ||
            _.opera != null || _.safari != null || _.samsung != null;
    }
}

fn checkVer(actual: u32?, min_req: u32): bool {
    if (v := actual) {
        return v >= min_req;
    }
    return true;
}

fn checkVerNoIe(browsers: Browsers, c: u32, e: u32, f: u32, s: u32, ios: u32, op: u32, andr: u32): bool {
    if (browsers.ie != null) return false;
    return checkVer(browsers.chrome, c) &&
        checkVer(browsers.edge, e) &&
        checkVer(browsers.firefox, f) &&
        checkVer(browsers.safari, s) &&
        checkVer(browsers.ios_saf, ios) &&
        checkVer(browsers.opera, op) &&
        checkVer(browsers.android, andr);
}

/// CSS features tracked for target browser compatibility & lowering.
pub enum Feature {
    Nesting,
    MediaRangeSyntax,
    MediaIntervalSyntax,
    LogicalProperties,
    ColorFunction,
    OklabColors,
    LabColors,
    LightDark,
    ClampFunction,
    CustomMedia,
    CascadeLayers,
    DirSelector,
    IsSelector,
    NotSelector,
    AnyLink,
    FocusVisible,
    HasSelector,
    DoublePositionGradients,
    P3Colors,
    HexAlphaColors,
    SpaceSeparatedColorNotation,
    Subgrid,
    XResolutionUnit,
    FontFamilySystemUi,
    VendorPrefixes,
    ContainerQueries,
    ScopeRule,

    fn mask(): u32 {
        return @as(u32, 1) << @intFromEnum(_);
    }

    /// Returns true if ALL configured target browsers support this feature natively.
    fn isCompatible(b: Browsers): bool {
        return switch (_) {
            .Nesting => checkVerNoIe(
                b,
                packBrowserVersion(120, 0, 0),
                packBrowserVersion(120, 0, 0),
                packBrowserVersion(117, 0, 0),
                packBrowserVersion(17, 2, 0),
                packBrowserVersion(17, 2, 0),
                packBrowserVersion(106, 0, 0),
                packBrowserVersion(120, 0, 0),
            ),
            .MediaRangeSyntax, .MediaIntervalSyntax => checkVerNoIe(
                b,
                packBrowserVersion(104, 0, 0),
                packBrowserVersion(104, 0, 0),
                packBrowserVersion(63, 0, 0),
                packBrowserVersion(16, 4, 0),
                packBrowserVersion(16, 4, 0),
                packBrowserVersion(90, 0, 0),
                packBrowserVersion(104, 0, 0),
            ),
            .LogicalProperties => checkVerNoIe(
                b,
                packBrowserVersion(89, 0, 0),
                packBrowserVersion(89, 0, 0),
                packBrowserVersion(66, 0, 0),
                packBrowserVersion(15, 0, 0),
                packBrowserVersion(15, 0, 0),
                packBrowserVersion(75, 0, 0),
                packBrowserVersion(89, 0, 0),
            ),
            .ColorFunction, .OklabColors, .LabColors => checkVerNoIe(
                b,
                packBrowserVersion(111, 0, 0),
                packBrowserVersion(111, 0, 0),
                packBrowserVersion(113, 0, 0),
                packBrowserVersion(15, 4, 0),
                packBrowserVersion(15, 4, 0),
                packBrowserVersion(97, 0, 0),
                packBrowserVersion(111, 0, 0),
            ),
            .LightDark => checkVerNoIe(
                b,
                packBrowserVersion(123, 0, 0),
                packBrowserVersion(123, 0, 0),
                packBrowserVersion(120, 0, 0),
                packBrowserVersion(17, 5, 0),
                packBrowserVersion(17, 5, 0),
                packBrowserVersion(109, 0, 0),
                packBrowserVersion(123, 0, 0),
            ),
            .ClampFunction => checkVerNoIe(
                b,
                packBrowserVersion(79, 0, 0),
                packBrowserVersion(79, 0, 0),
                packBrowserVersion(75, 0, 0),
                packBrowserVersion(13, 1, 0),
                packBrowserVersion(13, 4, 0),
                packBrowserVersion(66, 0, 0),
                packBrowserVersion(79, 0, 0),
            ),
            .CustomMedia => false, // Draft spec: always compile
            .CascadeLayers => checkVerNoIe(
                b,
                packBrowserVersion(99, 0, 0),
                packBrowserVersion(99, 0, 0),
                packBrowserVersion(97, 0, 0),
                packBrowserVersion(15, 4, 0),
                packBrowserVersion(15, 4, 0),
                packBrowserVersion(85, 0, 0),
                packBrowserVersion(99, 0, 0),
            ),
            .DirSelector => checkVerNoIe(
                b,
                packBrowserVersion(120, 0, 0),
                packBrowserVersion(120, 0, 0),
                packBrowserVersion(49, 0, 0),
                packBrowserVersion(16, 4, 0),
                packBrowserVersion(16, 4, 0),
                packBrowserVersion(106, 0, 0),
                packBrowserVersion(120, 0, 0),
            ),
            .IsSelector => checkVerNoIe(
                b,
                packBrowserVersion(88, 0, 0),
                packBrowserVersion(88, 0, 0),
                packBrowserVersion(78, 0, 0),
                packBrowserVersion(14, 0, 0),
                packBrowserVersion(14, 0, 0),
                packBrowserVersion(74, 0, 0),
                packBrowserVersion(88, 0, 0),
            ),
            .NotSelector => checkVerNoIe(
                b,
                packBrowserVersion(88, 0, 0),
                packBrowserVersion(88, 0, 0),
                packBrowserVersion(84, 0, 0),
                packBrowserVersion(9, 0, 0),
                packBrowserVersion(9, 0, 0),
                packBrowserVersion(74, 0, 0),
                packBrowserVersion(88, 0, 0),
            ),
            .AnyLink => checkVerNoIe(
                b,
                packBrowserVersion(65, 0, 0),
                packBrowserVersion(79, 0, 0),
                packBrowserVersion(50, 0, 0),
                packBrowserVersion(9, 0, 0),
                packBrowserVersion(9, 0, 0),
                packBrowserVersion(52, 0, 0),
                packBrowserVersion(65, 0, 0),
            ),
            .FocusVisible => checkVerNoIe(
                b,
                packBrowserVersion(86, 0, 0),
                packBrowserVersion(86, 0, 0),
                packBrowserVersion(85, 0, 0),
                packBrowserVersion(15, 4, 0),
                packBrowserVersion(15, 4, 0),
                packBrowserVersion(72, 0, 0),
                packBrowserVersion(86, 0, 0),
            ),
            .HasSelector => checkVerNoIe(
                b,
                packBrowserVersion(105, 0, 0),
                packBrowserVersion(105, 0, 0),
                packBrowserVersion(121, 0, 0),
                packBrowserVersion(15, 4, 0),
                packBrowserVersion(15, 4, 0),
                packBrowserVersion(91, 0, 0),
                packBrowserVersion(105, 0, 0),
            ),
            .DoublePositionGradients => checkVerNoIe(
                b,
                packBrowserVersion(72, 0, 0),
                packBrowserVersion(79, 0, 0),
                packBrowserVersion(83, 0, 0),
                packBrowserVersion(12, 1, 0),
                packBrowserVersion(12, 2, 0),
                packBrowserVersion(59, 0, 0),
                packBrowserVersion(72, 0, 0),
            ),
            .P3Colors => {
                if (b.ie != null || b.firefox != null || b.chrome != null || b.edge != null || b.opera != null || b.android != null || b.samsung != null) {
                    return checkVerNoIe(
                        b,
                        packBrowserVersion(111, 0, 0),
                        packBrowserVersion(111, 0, 0),
                        packBrowserVersion(113, 0, 0),
                        packBrowserVersion(10, 1, 0),
                        packBrowserVersion(10, 3, 0),
                        packBrowserVersion(97, 0, 0),
                        packBrowserVersion(111, 0, 0),
                    );
                }
                return checkVer(b.safari, packBrowserVersion(10, 1, 0)) &&
                    checkVer(b.ios_saf, packBrowserVersion(10, 3, 0));
            },
            .HexAlphaColors => checkVerNoIe(
                b,
                packBrowserVersion(62, 0, 0),
                packBrowserVersion(79, 0, 0),
                packBrowserVersion(49, 0, 0),
                packBrowserVersion(10, 0, 0),
                packBrowserVersion(10, 0, 0),
                packBrowserVersion(49, 0, 0),
                packBrowserVersion(62, 0, 0),
            ),
            .SpaceSeparatedColorNotation => checkVerNoIe(
                b,
                packBrowserVersion(65, 0, 0),
                packBrowserVersion(79, 0, 0),
                packBrowserVersion(52, 0, 0),
                packBrowserVersion(12, 1, 0),
                packBrowserVersion(12, 2, 0),
                packBrowserVersion(52, 0, 0),
                packBrowserVersion(65, 0, 0),
            ),
            .Subgrid => checkVerNoIe(
                b,
                packBrowserVersion(117, 0, 0),
                packBrowserVersion(117, 0, 0),
                packBrowserVersion(71, 0, 0),
                packBrowserVersion(16, 0, 0),
                packBrowserVersion(16, 0, 0),
                packBrowserVersion(103, 0, 0),
                packBrowserVersion(117, 0, 0),
            ),
            .XResolutionUnit => checkVerNoIe(
                b,
                packBrowserVersion(1, 0, 0),
                packBrowserVersion(79, 0, 0),
                packBrowserVersion(60, 0, 0),
                packBrowserVersion(3, 1, 0),
                packBrowserVersion(3, 2, 0),
                packBrowserVersion(15, 0, 0),
                packBrowserVersion(1, 0, 0),
            ),
            .FontFamilySystemUi => checkVerNoIe(
                b,
                packBrowserVersion(56, 0, 0),
                packBrowserVersion(79, 0, 0),
                packBrowserVersion(92, 0, 0),
                packBrowserVersion(11, 0, 0),
                packBrowserVersion(11, 0, 0),
                packBrowserVersion(43, 0, 0),
                packBrowserVersion(56, 0, 0),
            ),
            .VendorPrefixes => !b.hasAny(),
            .ContainerQueries => checkVerNoIe(
                b,
                packBrowserVersion(105, 0, 0),
                packBrowserVersion(105, 0, 0),
                packBrowserVersion(110, 0, 0),
                packBrowserVersion(16, 0, 0),
                packBrowserVersion(16, 0, 0),
                packBrowserVersion(91, 0, 0),
                packBrowserVersion(105, 0, 0),
            ),
            .ScopeRule => checkVerNoIe(
                b,
                packBrowserVersion(118, 0, 0),
                packBrowserVersion(118, 0, 0),
                packBrowserVersion(128, 0, 0),
                packBrowserVersion(17, 4, 0),
                packBrowserVersion(17, 4, 0),
                packBrowserVersion(104, 0, 0),
                packBrowserVersion(118, 0, 0),
            ),
        };
    }

    /// Returns true if AT LEAST ONE configured browser supports the feature.
    fn isPartiallyCompatible(targets: Browsers): bool {
        if (targets.android != null && _.isCompatible(Browsers { android: targets.android })) return true;
        if (targets.chrome != null && _.isCompatible(Browsers { chrome: targets.chrome })) return true;
        if (targets.edge != null && _.isCompatible(Browsers { edge: targets.edge })) return true;
        if (targets.firefox != null && _.isCompatible(Browsers { firefox: targets.firefox })) return true;
        if (targets.ie != null && _.isCompatible(Browsers { ie: targets.ie })) return true;
        if (targets.ios_saf != null && _.isCompatible(Browsers { ios_saf: targets.ios_saf })) return true;
        if (targets.opera != null && _.isCompatible(Browsers { opera: targets.opera })) return true;
        if (targets.safari != null && _.isCompatible(Browsers { safari: targets.safari })) return true;
        if (targets.samsung != null && _.isCompatible(Browsers { samsung: targets.samsung })) return true;
        return false;
    }
}

/// Bitmask set of `Feature` flags for explicit include/exclude compilation overrides.
pub struct Features {
    bits: u32 = 0;

    pub static fn empty(): Features {
        return Features { bits: 0 };
    }

    pub static fn all(): Features {
        return Features { bits: 0x07FFFFFF };
    }

    pub static fn selectors(): Features {
        return Features {
            bits: Feature.Nesting.mask() |
                Feature.NotSelector.mask() |
                Feature.DirSelector.mask() |
                Feature.IsSelector.mask() |
                Feature.AnyLink.mask() |
                Feature.FocusVisible.mask() |
                Feature.HasSelector.mask(),
        };
    }

    pub static fn mediaQueries(): Features {
        return Features {
            bits: Feature.MediaRangeSyntax.mask() |
                Feature.MediaIntervalSyntax.mask() |
                Feature.CustomMedia.mask(),
        };
    }

    pub static fn colors(): Features {
        return Features {
            bits: Feature.ColorFunction.mask() |
                Feature.OklabColors.mask() |
                Feature.LabColors.mask() |
                Feature.LightDark.mask() |
                Feature.P3Colors.mask() |
                Feature.HexAlphaColors.mask() |
                Feature.SpaceSeparatedColorNotation.mask(),
        };
    }

    pub fn has(f: Feature): bool {
        return (_.bits & f.mask()) != 0;
    }

    pub fn insert(*_, f: Feature) {
        _.bits |= f.mask();
    }

    pub fn remove(*_, f: Feature) {
        _.bits &= ~f.mask();
    }

    pub fn unionWith(other: Features): Features {
        return Features { bits: _.bits | other.bits };
    }

    pub fn intersects(other: Features): bool {
        return (_.bits & other.bits) != 0;
    }
}

/// Browser vendor prefix bitmask (`None | Webkit | Moz | Ms | O`).
pub struct PrefixFlags {
    bits: u8 = 1;

    pub const NONE: u8 = 1;
    pub const WEBKIT: u8 = 2;
    pub const MOZ: u8 = 4;
    pub const MS: u8 = 8;
    pub const O: u8 = 16;
    pub const ALL: u8 = 31;

    pub static fn none(): PrefixFlags { return PrefixFlags { bits: PrefixFlags.NONE }; }
    pub static fn webkit(): PrefixFlags { return PrefixFlags { bits: PrefixFlags.WEBKIT }; }
    pub static fn moz(): PrefixFlags { return PrefixFlags { bits: PrefixFlags.MOZ }; }
    pub static fn ms(): PrefixFlags { return PrefixFlags { bits: PrefixFlags.MS }; }
    pub static fn o(): PrefixFlags { return PrefixFlags { bits: PrefixFlags.O }; }
    pub static fn all(): PrefixFlags { return PrefixFlags { bits: PrefixFlags.ALL }; }
    pub static fn empty(): PrefixFlags { return PrefixFlags { bits: 0 }; }
    pub static fn fromBits(b: u8): PrefixFlags { return PrefixFlags { bits: b }; }

    pub fn hasNone(): bool { return (_.bits & PrefixFlags.NONE) != 0; }
    pub fn hasWebkit(): bool { return (_.bits & PrefixFlags.WEBKIT) != 0; }
    pub fn hasMoz(): bool { return (_.bits & PrefixFlags.MOZ) != 0; }
    pub fn hasMs(): bool { return (_.bits & PrefixFlags.MS) != 0; }
    pub fn hasO(): bool { return (_.bits & PrefixFlags.O) != 0; }
    pub fn isEmpty(): bool { return _.bits == 0; }

    pub fn unionWith(other: PrefixFlags): PrefixFlags {
        return PrefixFlags { bits: _.bits | other.bits };
    }

    pub fn andWith(other: PrefixFlags): PrefixFlags {
        return PrefixFlags { bits: _.bits & other.bits };
    }

    pub fn countPrefixes(): usize {
        let c: usize = 0;
        if (_.hasWebkit()) c += 1;
        if (_.hasMoz()) c += 1;
        if (_.hasMs()) c += 1;
        if (_.hasO()) c += 1;
        if (_.hasNone()) c += 1;
        return c;
    }
}

/// CSS properties/features that require vendor prefixes on older targets.
pub enum PrefixFeature {
    Transform,
    Transition,
    Animation,
    AtKeyframes,
    BoxShadow,
    BorderRadius,
    UserSelect,
    Appearance,
    BackdropFilter,
    BoxSizing,
    ClipPath,
    Hyphens,
    MaskImage,
    LinearGradient,
    RadialGradient,
    DisplayFlex,
    Flex,
    Sticky,
    TabSize,
    TextSizeAdjust,
    Filter,

    fn prefixesFor(b: Browsers): PrefixFlags {
        let p: u8 = PrefixFlags.NONE;

        switch (_) {
            .UserSelect => {
                if (v := b.chrome) { if (v < packBrowserVersion(54, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.firefox) { if (v < packBrowserVersion(69, 0, 0)) p |= PrefixFlags.MOZ; }
                if (v := b.safari) { if (v <= packBrowserVersion(17, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.ios_saf) { if (v <= packBrowserVersion(17, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.edge) { if (v < packBrowserVersion(79, 0, 0)) p |= PrefixFlags.MS; }
                if (b.ie != null) p |= PrefixFlags.MS;
            },
            .BackdropFilter => {
                if (v := b.safari) { if (v >= packBrowserVersion(9, 0, 0) && v < packBrowserVersion(18, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.ios_saf) { if (v >= packBrowserVersion(9, 0, 0) && v < packBrowserVersion(18, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.chrome) { if (v >= packBrowserVersion(47, 0, 0) && v < packBrowserVersion(76, 0, 0)) p |= PrefixFlags.WEBKIT; }
            },
            .Appearance => {
                if (v := b.chrome) { if (v < packBrowserVersion(84, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.edge) { if (v < packBrowserVersion(84, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.firefox) { if (v < packBrowserVersion(80, 0, 0)) p |= PrefixFlags.MOZ; }
                if (v := b.safari) { if (v < packBrowserVersion(15, 4, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.ios_saf) { if (v < packBrowserVersion(15, 4, 0)) p |= PrefixFlags.WEBKIT; }
            },
            .Transform => {
                if (v := b.chrome) { if (v < packBrowserVersion(36, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.safari) { if (v < packBrowserVersion(9, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.ios_saf) { if (v < packBrowserVersion(9, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.firefox) { if (v < packBrowserVersion(16, 0, 0)) p |= PrefixFlags.MOZ; }
                if (v := b.ie) { if (v <= packBrowserVersion(9, 0, 0)) p |= PrefixFlags.MS; }
                if (v := b.opera) { if (v < packBrowserVersion(23, 0, 0)) p |= PrefixFlags.O | PrefixFlags.WEBKIT; }
            },
            .Transition => {
                if (v := b.chrome) { if (v < packBrowserVersion(26, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.safari) { if (v < packBrowserVersion(6, 1, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.firefox) { if (v < packBrowserVersion(16, 0, 0)) p |= PrefixFlags.MOZ; }
                if (v := b.opera) { if (v < packBrowserVersion(12, 1, 0)) p |= PrefixFlags.O; }
            },
            .Animation, .AtKeyframes => {
                if (v := b.chrome) { if (v < packBrowserVersion(43, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.safari) { if (v < packBrowserVersion(9, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.ios_saf) { if (v < packBrowserVersion(9, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.firefox) { if (v < packBrowserVersion(16, 0, 0)) p |= PrefixFlags.MOZ; }
                if (v := b.opera) { if (v < packBrowserVersion(30, 0, 0)) p |= PrefixFlags.WEBKIT; }
            },
            .BoxShadow => {
                if (v := b.chrome) { if (v < packBrowserVersion(10, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.safari) { if (v < packBrowserVersion(5, 1, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.firefox) { if (v < packBrowserVersion(4, 0, 0)) p |= PrefixFlags.MOZ; }
            },
            .BorderRadius => {
                if (v := b.chrome) { if (v < packBrowserVersion(5, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.safari) { if (v < packBrowserVersion(5, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.firefox) { if (v < packBrowserVersion(4, 0, 0)) p |= PrefixFlags.MOZ; }
            },
            .BoxSizing => {
                if (v := b.chrome) { if (v < packBrowserVersion(10, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.safari) { if (v < packBrowserVersion(5, 1, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.firefox) { if (v < packBrowserVersion(29, 0, 0)) p |= PrefixFlags.MOZ; }
            },
            .ClipPath => {
                if (v := b.chrome) { if (v < packBrowserVersion(55, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.safari) { if (v < packBrowserVersion(13, 1, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.ios_saf) { if (v < packBrowserVersion(13, 4, 0)) p |= PrefixFlags.WEBKIT; }
            },
            .Hyphens => {
                if (v := b.safari) { if (v < packBrowserVersion(17, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.ios_saf) { if (v < packBrowserVersion(17, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.edge) { if (v < packBrowserVersion(79, 0, 0)) p |= PrefixFlags.MS; }
                if (b.ie != null) p |= PrefixFlags.MS;
                if (v := b.firefox) { if (v < packBrowserVersion(43, 0, 0)) p |= PrefixFlags.MOZ; }
            },
            .MaskImage => {
                if (v := b.chrome) { if (v < packBrowserVersion(120, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.edge) { if (v < packBrowserVersion(120, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.safari) { if (v < packBrowserVersion(15, 4, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.ios_saf) { if (v < packBrowserVersion(15, 4, 0)) p |= PrefixFlags.WEBKIT; }
            },
            .LinearGradient, .RadialGradient => {
                if (v := b.chrome) { if (v < packBrowserVersion(26, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.safari) { if (v < packBrowserVersion(6, 1, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.firefox) { if (v < packBrowserVersion(16, 0, 0)) p |= PrefixFlags.MOZ; }
                if (v := b.opera) { if (v < packBrowserVersion(12, 1, 0)) p |= PrefixFlags.O; }
            },
            .DisplayFlex, .Flex => {
                if (v := b.chrome) { if (v < packBrowserVersion(29, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.safari) { if (v < packBrowserVersion(9, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.ie) { if (v <= packBrowserVersion(10, 0, 0)) p |= PrefixFlags.MS; }
            },
            .Sticky => {
                if (v := b.safari) { if (v < packBrowserVersion(13, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.ios_saf) { if (v < packBrowserVersion(13, 0, 0)) p |= PrefixFlags.WEBKIT; }
            },
            .TabSize => {
                if (v := b.firefox) { if (v < packBrowserVersion(91, 0, 0)) p |= PrefixFlags.MOZ; }
                if (v := b.opera) { if (v < packBrowserVersion(15, 0, 0)) p |= PrefixFlags.O; }
            },
            .TextSizeAdjust => {
                if (v := b.chrome) { if (v < packBrowserVersion(54, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.safari) { if (v >= packBrowserVersion(3, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.ios_saf) { if (v >= packBrowserVersion(3, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.edge) { if (v < packBrowserVersion(79, 0, 0)) p |= PrefixFlags.MS; }
            },
            .Filter => {
                if (v := b.chrome) { if (v < packBrowserVersion(53, 0, 0)) p |= PrefixFlags.WEBKIT; }
                if (v := b.safari) { if (v < packBrowserVersion(9, 1, 0)) p |= PrefixFlags.WEBKIT; }
            },
        }
        return PrefixFlags.fromBits(p);
    }

    static fn fromPropertyName(prop_name: []const u8): PrefixFeature? {
        if (std.ascii.eqlIgnoreCase(prop_name, "transform") || std.ascii.eqlIgnoreCase(prop_name, "transform-origin")) return PrefixFeature.Transform;
        if (std.ascii.eqlIgnoreCase(prop_name, "transition") || std.ascii.startsWithIgnoreCase(prop_name, "transition-")) return PrefixFeature.Transition;
        if (std.ascii.eqlIgnoreCase(prop_name, "animation") || std.ascii.startsWithIgnoreCase(prop_name, "animation-")) return PrefixFeature.Animation;
        if (std.ascii.eqlIgnoreCase(prop_name, "box-shadow")) return PrefixFeature.BoxShadow;
        if (std.ascii.eqlIgnoreCase(prop_name, "border-radius") ||
            std.ascii.eqlIgnoreCase(prop_name, "border-top-left-radius") ||
            std.ascii.eqlIgnoreCase(prop_name, "border-top-right-radius") ||
            std.ascii.eqlIgnoreCase(prop_name, "border-bottom-right-radius") ||
            std.ascii.eqlIgnoreCase(prop_name, "border-bottom-left-radius")) return PrefixFeature.BorderRadius;
        if (std.ascii.eqlIgnoreCase(prop_name, "user-select")) return PrefixFeature.UserSelect;
        if (std.ascii.eqlIgnoreCase(prop_name, "appearance")) return PrefixFeature.Appearance;
        if (std.ascii.eqlIgnoreCase(prop_name, "backdrop-filter")) return PrefixFeature.BackdropFilter;
        if (std.ascii.eqlIgnoreCase(prop_name, "box-sizing")) return PrefixFeature.BoxSizing;
        if (std.ascii.eqlIgnoreCase(prop_name, "clip-path")) return PrefixFeature.ClipPath;
        if (std.ascii.eqlIgnoreCase(prop_name, "hyphens")) return PrefixFeature.Hyphens;
        if (std.ascii.eqlIgnoreCase(prop_name, "mask-image") || std.ascii.eqlIgnoreCase(prop_name, "mask")) return PrefixFeature.MaskImage;
        if (std.ascii.eqlIgnoreCase(prop_name, "flex") || std.ascii.eqlIgnoreCase(prop_name, "flex-grow") || std.ascii.eqlIgnoreCase(prop_name, "flex-shrink") || std.ascii.eqlIgnoreCase(prop_name, "flex-basis") || std.ascii.eqlIgnoreCase(prop_name, "flex-direction") || std.ascii.eqlIgnoreCase(prop_name, "flex-wrap") || std.ascii.eqlIgnoreCase(prop_name, "flex-flow")) return PrefixFeature.Flex;
        if (std.ascii.eqlIgnoreCase(prop_name, "tab-size")) return PrefixFeature.TabSize;
        if (std.ascii.eqlIgnoreCase(prop_name, "text-size-adjust")) return PrefixFeature.TextSizeAdjust;
        if (std.ascii.eqlIgnoreCase(prop_name, "filter")) return PrefixFeature.Filter;
        return null;
    }
}

/// Complete compilation target configuration (`browsers` + `include`/`exclude` overrides).
pub struct Targets {
    browsers: Browsers? = null;
    include: Features = Features {};
    exclude: Features = Features {};

    pub static fn browserDefault(): Targets {
        return Targets { browsers: Browsers.browserDefault() };
    }

    pub static fn runtimeDefault(): Targets {
        return Targets { browsers: null };
    }

    pub static fn fromBrowsers(b: Browsers): Targets {
        return Targets { browsers: b };
    }

    pub static fn fromList(targets: []const []const u8): Targets {
        return Targets { browsers: Browsers.fromTargets(targets) };
    }

    pub fn isCompatible(feature: Feature): bool {
        if (b := _.browsers) {
            return feature.isCompatible(b);
        }
        return true;
    }

    pub fn shouldCompile(feature: Feature): bool {
        if (_.include.has(feature)) return true;
        if (_.exclude.has(feature)) return false;
        return !_.isCompatible(feature);
    }

    pub fn shouldCompileLogical(): bool {
        return _.shouldCompile(Feature.LogicalProperties);
    }

    pub fn shouldCompileSelectors(): bool {
        const sel_mask = Features.selectors();
        if (_.include.intersects(sel_mask)) return true;
        if (_.exclude.intersects(sel_mask)) return false;
        return _.browsers != null;
    }

    pub fn prefixes(feature: PrefixFeature): PrefixFlags {
        if (_.exclude.has(Feature.VendorPrefixes)) {
            return PrefixFlags.none();
        }
        if (_.include.has(Feature.VendorPrefixes)) {
            return PrefixFlags.all();
        }
        if (b := _.browsers) {
            return feature.prefixesFor(b);
        }
        return PrefixFlags.none();
    }

    pub fn prefixesForProperty(prop_name: []const u8): PrefixFlags {
        if (pf := PrefixFeature.fromPropertyName(prop_name)) {
            return _.prefixes(pf);
        }
        return PrefixFlags.none();
    }
}
