// Source: cp/lib/geo/spatial.cp (In-Memory Radius & Nearest-Neighbor Spatial Index)
// Pure CP in-memory spatial index supporting O(1) upsert/remove, latitude-pruned radius queries,
// and exact Haversine nearest-K proximity ranking.

const std = @import("std");
import { GeoError, LatLng, Geo } from "./coord.cp";

// Minimum meters per degree of latitude along a meridian (conservative lower bound for pruning).
const MIN_METERS_PER_LAT_DEG: f64 = 111190.0;

/// Spatial query hit containing borrowed entry `id`, coordinates, and Haversine distance in meters.
/// `id` borrows from the owning `SpatialIndex` until updated, removed, or `deinit()`.
pub struct GeoPointEntry {
    pub id: string = "";
    pub coord: LatLng = LatLng{};
    pub distance_m: f64 = 0.0;
}

struct StoredGeoPoint {
    id: []u8 = [],
    coord: LatLng = LatLng{},
}

fn entryCloserAsc(a: GeoPointEntry, b: GeoPointEntry): bool {
    if (a.distance_m != b.distance_m) return a.distance_m < b.distance_m;
    return std.mem.order(u8, a.id, b.id) == .lt;
}

fn siftDownWorseRoot(items: []GeoPointEntry, start: usize, len: usize) {
    let idx = start;
    while (true) {
        const left = idx * 2 + 1;
        if (left >= len) break;
        const right = left + 1;
        let worst = left;
        // Pick the strictly larger/worse distance (NOT entryCloserAsc) so max-heap root is largest.
        if (right < len && entryCloserAsc(items[worst], items[right])) {
            worst = right;
        }
        if (!entryCloserAsc(items[idx], items[worst])) break;
        const tmp = items[idx];
        items[idx] = items[worst];
        items[worst] = tmp;
        idx = worst;
    }
}

fn sortEntriesAsc(items: []GeoPointEntry) {
    if (items.len <= 1) return;
    let n = items.len;
    let i: usize = n / 2;
    while (i > 0) {
        i -= 1;
        siftDownWorseRoot(items, i, n);
    }
    let end: usize = n;
    while (end > 1) {
        end -= 1;
        const tmp = items[0];
        items[0] = items[end];
        items[end] = tmp;
        siftDownWorseRoot(items, 0, end);
    }
}

/// In-memory geospatial point index supporting O(1) upsert/remove, Haversine radius search,
/// and exact nearest-K proximity ranking.
pub struct SpatialIndex {
    alloc: std.mem.Allocator = undefined;
    entries: std.ArrayList(StoredGeoPoint) = .empty;
    id_to_index: std.StringHashMapUnmanaged(usize) = .empty;

    /// Initializes an empty `SpatialIndex` backed by `alloc`.
    pub static fn init(alloc: std.mem.Allocator): SpatialIndex {
        return SpatialIndex {
            alloc: alloc,
            entries: .empty,
            id_to_index: .empty,
        };
    }

    /// Releases all owned entry IDs and backing storage.
    pub fn deinit(*_) {
        for (e of _.entries.items) {
            if (e.id.len > 0) _.alloc.free(e.id);
        }
        _.entries.deinit(_.alloc);
        _.id_to_index.deinit(_.alloc);
    }

    /// Returns the number of points currently stored in the index.
    pub fn len(): usize {
        return _.entries.items.len;
    }

    /// Inserts a new point or updates the coordinates of an existing `id`.
    /// Duplicates `id` internally so caller buffers may be mutated or freed immediately.
    pub fn upsert(*_, id: string, pt: LatLng): void ! GeoError {
        if (!pt.isValid()) return GeoError.InvalidCoordinate;

        if (existing_idx := _.id_to_index.get(id)) {
            _.entries.items[existing_idx].coord = pt;
        } else {
            const owned_id: []u8 = if (id.len > 0)
                (_.alloc.dupe(u8, id) catch return GeoError.OutOfMemory)
            else
                [];
            errdefer if (owned_id.len > 0) _.alloc.free(owned_id);

            const new_idx = _.entries.items.len;
            _.entries.append(_.alloc, StoredGeoPoint {
                id: owned_id,
                coord: pt,
            }) catch return GeoError.OutOfMemory;
            errdefer * = _.entries.pop();

            _.id_to_index.put(_.alloc, owned_id, new_idx) catch return GeoError.OutOfMemory;
        }
    }

    /// Removes the point identified by `id` in O(1) time. Returns `true` if removed.
    pub fn remove(*_, id: string): bool {
        const idx = _.id_to_index.get(id) ?? return false;
        * = _.id_to_index.remove(id);

        const removed = _.entries.items[idx];
        if (removed.id.len > 0) _.alloc.free(removed.id);

        const last_idx = _.entries.items.len - 1;
        if (idx != last_idx) {
            const last_entry = _.entries.items[last_idx];
            _.entries.items[idx] = last_entry;
            if (ptr := _.id_to_index.getPtr(last_entry.id)) {
                ptr.* = idx;
            }
        }
        * = _.entries.pop();
        return true;
    }

    /// Returns all indexed points within `radius_m` meters of `center`,
    /// sorted in ascending order of exact Haversine distance.
    /// Caller owns the returned slice (`alloc.free(hits)`).
    pub fn withinRadius(
        alloc: std.mem.Allocator,
        center: LatLng,
        radius_m: f64,
    ): ([]GeoPointEntry) ! GeoError {
        if (!center.isValid() || std.math.isNan(radius_m)) {
            return GeoError.InvalidCoordinate;
        }
        if (radius_m < 0.0 || _.entries.items.len == 0) {
            return alloc.alloc(GeoPointEntry, 0) catch return GeoError.OutOfMemory;
        }

        const max_dlat = radius_m / MIN_METERS_PER_LAT_DEG;
        let hits: std.ArrayList(GeoPointEntry) = .empty;
        defer hits.deinit(alloc);

        for (e of _.entries.items) {
            if (@abs(e.coord.lat - center.lat) > max_dlat) {
                continue;
            }
            const dist = Geo.haversineMeters(center, e.coord);
            if (dist <= radius_m) {
                hits.append(alloc, GeoPointEntry {
                    id: e.id,
                    coord: e.coord,
                    distance_m: dist,
                }) catch return GeoError.OutOfMemory;
            }
        }

        if (hits.items.len == 0) {
            return alloc.alloc(GeoPointEntry, 0) catch return GeoError.OutOfMemory;
        }

        sortEntriesAsc(hits.items);
        const out = alloc.alloc(GeoPointEntry, hits.items.len) catch return GeoError.OutOfMemory;
        @memcpy(out, hits.items);
        return out;
    }

    /// Returns up to `k` nearest points to `center`, sorted by ascending Haversine distance.
    /// Caller owns the returned slice (`alloc.free(hits)`).
    pub fn nearestK(
        alloc: std.mem.Allocator,
        center: LatLng,
        k: usize,
    ): ([]GeoPointEntry) ! GeoError {
        if (!center.isValid()) return GeoError.InvalidCoordinate;
        if (k == 0 || _.entries.items.len == 0) {
            return alloc.alloc(GeoPointEntry, 0) catch return GeoError.OutOfMemory;
        }

        let candidates = alloc.alloc(GeoPointEntry, _.entries.items.len) catch return GeoError.OutOfMemory;
        defer alloc.free(candidates);

        let i: usize = 0;
        while (i < _.entries.items.len) : (i += 1) {
            const e = _.entries.items[i];
            candidates[i] = GeoPointEntry {
                id: e.id,
                coord: e.coord,
                distance_m: Geo.haversineMeters(center, e.coord),
            };
        }

        sortEntriesAsc(candidates);
        const out_len = if (k < candidates.len) k else candidates.len;
        const out = alloc.alloc(GeoPointEntry, out_len) catch return GeoError.OutOfMemory;
        @memcpy(out, candidates[0..out_len]);
        return out;
    }
}
