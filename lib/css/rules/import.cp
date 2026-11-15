import css from "../css_parser.zig";
const Printer = css.Printer;
const PrintErr = css.PrintErr;
const Location = css.css_rules.Location;
const LayerName = css.css_rules.layer.LayerName;
const SupportsCondition = css.css_rules.supports.SupportsCondition;
const MediaList = css.MediaList;
const dependencies = css.dependencies;
const Dependency = dependencies.Dependency;

pub struct ImportLayer {
    v: LayerName? = null,

    static fn eql(lhs: *const ImportLayer, rhs: *const ImportLayer): bool {
        if (lhs.v == null && rhs.v == null) return true;
        if (lhs.v == null || rhs.v == null) return false;
        return lhs.v.?.eql(&rhs.v.?);
    }

    fn deepClone(allocator: std.mem.Allocator): @This() {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

pub struct ImportConditions {
    layer: ImportLayer? = null,
    supports: SupportsCondition? = null,
    media: css.MediaList = .{},

    fn hash(hasher: auto) {
        return css.implementHash(@This(), _, hasher);
    }

    fn hasAnonymousLayer(): bool {
        return _.layer != null && _.layer.?.v == null;
    }

    fn deepClone(allocator: std.mem.Allocator): ImportConditions {
        let new_layer: ImportLayer? = null;
        if (l := _.layer) {
            if (layer := l.v) {
                new_layer = ImportLayer{ .v = layer.deepClone(allocator) };
            } else {
                new_layer = ImportLayer{ .v = null };
            }
        }
        let new_supports: SupportsCondition? = null;
        if (*s := _.supports) {
            new_supports = s.deepClone(allocator);
        }
        return ImportConditions{
            .layer = new_layer,
            .supports = new_supports,
            .media = _.media.deepClone(allocator),
        };
    }

    fn toCss(dest: *Printer): PrintErr!void {
        if (*lyr := _.layer) {
            dest.writeStr(" layer")?;
            if (l := lyr.v) {
                dest.writeChar('(')?;
                l.toCss(dest)?;
                dest.writeChar(')')?;
            }
        }

        if (*sup := _.supports) {
            dest.writeStr(" supports")?;
            if (sup.* == .declaration) {
                sup.toCss(dest)?;
            } else {
                dest.writeChar('(')?;
                sup.toCss(dest)?;
                dest.writeChar(')')?;
            }
        }

        if (_.media.media_queries.items.len > 0) {
            dest.writeChar(' ')?;
            _.media.toCss(dest)?;
        }
    }

    fn cloneWithImportRecords(allocator: std.mem.Allocator, import_records: *bun.BabyList(bun.ImportRecord)): ImportConditions {
        let new_layer: ImportLayer? = null;
        if (l := _.layer) {
            if (layer := l.v) {
                new_layer = ImportLayer{ .v = layer.cloneWithImportRecords(allocator, import_records) };
            } else {
                new_layer = ImportLayer{ .v = null };
            }
        }
        let new_supp: SupportsCondition? = null;
        if (*supp := _.supports) {
            new_supp = supp.cloneWithImportRecords(allocator, import_records);
        }
        return ImportConditions{
            .layer = new_layer,
            .supports = new_supp,
            .media = _.media.cloneWithImportRecords(allocator, import_records),
        };
    }

    static fn layersEql(lhs: *const @This(), rhs: *const @This()): bool {
        if (lhs.layer == null && rhs.layer == null) return true;
        if (lhs.layer == null || rhs.layer == null) return false;
        return lhs.layer.?.eql(&rhs.layer.?);
    }

    static fn supportsEql(lhs: *const @This(), rhs: *const @This()): bool {
        if (lhs.supports == null && rhs.supports == null) return true;
        if (lhs.supports == null || rhs.supports == null) return false;
        return lhs.supports.?.eql(&rhs.supports.?);
    }
}

pub struct ImportRule {
    url: []const u8,
    layer: ImportLayer? = null,
    supports: SupportsCondition? = null,
    media: css.MediaList = .{},
    import_record_idx: u32 = std.math.maxInt(u32),
    loc: Location,

    const This = @This();

    static fn fromUrl(url: []const u8): This {
        return .{
            .url = url,
            .layer = null,
            .supports = null,
            .media = MediaList{ .media_queries = .{} },
            .import_record_idx = std.math.maxInt(u32),
            .loc = Location.dummy(),
        };
    }

    static fn fromUrlAndImportRecordIdx(url: []const u8, import_record_idx: u32): This {
        return .{
            .url = url,
            .layer = null,
            .supports = null,
            .media = MediaList{ .media_queries = .{} },
            .import_record_idx = import_record_idx,
            .loc = Location.dummy(),
        };
    }

    static fn fromConditionsAndUrl(url: []const u8, conds: ImportConditions): This {
        let new_layer: ImportLayer? = null;
        if (l := conds.layer) {
            new_layer = ImportLayer{ .v = l.v };
        }
        return ImportRule{
            .url = url,
            .layer = new_layer,
            .supports = conds.supports,
            .media = conds.media,
            .import_record_idx = std.math.maxInt(u32),
            .loc = Location.dummy(),
        };
    }

    fn conditions(): *const ImportConditions {
        return @ptrCast(&_.layer);
    }

    fn conditionsMut(*_): *ImportConditions {
        return @ptrCast(&_.layer);
    }

    fn conditionsWithImportRecords(allocator: std.mem.Allocator, import_records: *bun.BabyList(bun.ImportRecord)): ImportConditions {
        let new_layer: ImportLayer? = null;
        if (l := _.layer) {
            if (layer := l.v) {
                new_layer = ImportLayer{ .v = layer.cloneWithImportRecords(allocator, import_records) };
            } else {
                new_layer = ImportLayer{ .v = null };
            }
        }
        let new_supp: SupportsCondition? = null;
        if (*supp := _.supports) {
            new_supp = supp.cloneWithImportRecords(allocator, import_records);
        }
        return ImportConditions{
            .layer = new_layer,
            .supports = new_supp,
            .media = _.media.cloneWithImportRecords(allocator, import_records),
        };
    }

    fn hasConditions(): bool {
        return _.layer != null || _.supports != null || _.media.media_queries.items.len > 0;
    }

    fn toCss(dest: *Printer): PrintErr!void {
        const dep = if (dest.dependencies != null) dependencies.ImportDependency.new(
            dest.allocator,
            _,
            dest.filename(),
            dest.local_names,
            dest.symbols,
        ) else null;

        dest.writeStr("@import ")?;
        if (d := dep) {
            css.serializer.serializeString(d.placeholder, dest) catch return dest.addFmtError();

            if (*deps := dest.dependencies) {
                deps.append(
                    dest.allocator,
                    Dependency{ .@"import" = d },
                ) catch unreachable;
            }
        } else {
            css.serializer.serializeString(_.url, dest) catch return dest.addFmtError();
        }

        if (*lyr := _.layer) {
            dest.writeStr(" layer")?;
            if (l := lyr.v) {
                dest.writeChar('(')?;
                l.toCss(dest)?;
                dest.writeChar(')')?;
            }
        }

        if (*sup := _.supports) {
            dest.writeStr(" supports")?;
            if (sup.* == .declaration) {
                sup.toCss(dest)?;
            } else {
                dest.writeChar('(')?;
                sup.toCss(dest)?;
                dest.writeChar(')')?;
            }
        }

        if (_.media.media_queries.items.len > 0) {
            dest.writeChar(' ')?;
            _.media.toCss(dest)?;
        }
        try dest.writeStr(";");
    }

    fn deepClone(allocator: std.mem.Allocator): This {
        return css.implementDeepClone(@This(), _, allocator);
    }
}

import bun from "bun";
import std from "std";
