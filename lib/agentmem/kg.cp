// Origin: Pure-CP Stdlib (AI Agent Temporal Knowledge Graph Memory: SPO Triples & 2-Hop Recall) | Created: 2026-11-22
// Pure CP zero-allocation temporal knowledge graph memory store with deduplicated entities,
// versioned SPO triples (fact superseding without losing history), point-in-time queries,
// and 2-hop neighborhood recall for GraphRAG context expansion.

const std = @import("std");

/// Fixed-layout entity node stored in `KgStore`.
pub struct KgEntity {
    id: u32 = 0,
    name: string = "",
    kind: string = "",
    name_hash: u64 = 0,
}

/// Fixed-layout temporal Subject-Predicate-Object (SPO) fact triple.
/// `valid_until_ms == 0` indicates an open-ended (currently valid) fact.
pub struct KgTriple {
    subject: u32 = 0,
    predicate_hash: u64 = 0,
    predicate: string = "",
    object: u32 = 0,
    confidence: f64 = 1.0,
    valid_from_ms: i64 = 0,
    valid_until_ms: i64 = 0,
    active: bool = true,
}

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn hashStr64(s: string): u64 {
    return std.hash.Fnv1a_64.hash(s);
}

fn isValidConfidence(c: f64): bool {
    return !std.math.isNan(c) && !std.math.isInf(c) && c >= 0.0 && c <= 1.0;
}

fn isTripleValidAt(t: KgTriple, at_ms: i64): bool {
    if (at_ms == 0) {
        return t.active && t.valid_until_ms == 0;
    }
    if (at_ms < t.valid_from_ms) {
        return false;
    }
    if (t.valid_until_ms != 0 && at_ms >= t.valid_until_ms) {
        return false;
    }
    return true;
}

/// Zero-allocation fixed-pool temporal knowledge graph store for AI Agents.
pub struct KgStore {
    entities: []KgEntity,
    entity_count: usize = 0,
    triples: []KgTriple,
    triple_count: usize = 0,

    pub static fn init(entities: *[]KgEntity, triples: *[]KgTriple): KgStore {
        return KgStore{
            entities: entities,
            entity_count: 0,
            triples: triples,
            triple_count: 0,
        };
    }

    fn isValidEntityId(id: u32): bool {
        return (id as usize) < _.entity_count;
    }

    /// Finds an entity ID by exact `name`, or returns `null` if not found.
    pub fn findEntity(name: string): u32? {
        if (name.len == 0) {
            return null;
        }
        const h: u64 = hashStr64(name);
        let i: usize = 0;
        while (i < _.entity_count) : (i += 1) {
            const e = _.entities[i];
            if (e.name_hash == h && strEq(e.name, name)) {
                return e.id;
            }
        }
        return null;
    }

    /// Inserts a new entity or returns the existing entity ID if `name` is already registered.
    /// Rejects empty `name` or a full entity pool by returning `null`.
    pub fn upsertEntity(*_, name: string, kind: string): u32? {
        if (name.len == 0) {
            return null;
        }
        if (existing := _.findEntity(name)) {
            return existing;
        }
        if (_.entity_count >= _.entities.len) {
            return null;
        }
        const idx: usize = _.entity_count;
        const new_id: u32 = idx as u32;
        _.entities[idx] = KgEntity{
            id: new_id,
            name: name,
            kind: kind,
            name_hash: hashStr64(name),
        };
        _.entity_count += 1;
        return new_id;
    }

    /// Appends a new temporal SPO fact triple.
    /// Returns `null` on invalid entity IDs, empty predicate, out-of-range/NaN confidence, or full triple pool.
    pub fn addFact(
        *_,
        subject: u32,
        predicate: string,
        object: u32,
        confidence: f64,
        now_ms: i64,
    ): usize? {
        if (predicate.len == 0) {
            return null;
        }
        if (!_.isValidEntityId(subject) || !_.isValidEntityId(object)) {
            return null;
        }
        if (!isValidConfidence(confidence)) {
            return null;
        }
        if (_.triple_count >= _.triples.len) {
            return null;
        }

        const idx: usize = _.triple_count;
        _.triples[idx] = KgTriple{
            subject: subject,
            predicate_hash: hashStr64(predicate),
            predicate: predicate,
            object: object,
            confidence: confidence,
            valid_from_ms: now_ms,
            valid_until_ms: 0,
            active: true,
        };
        _.triple_count += 1;
        return idx;
    }

    /// Invalidates all currently active facts matching `(subject, predicate)` by setting
    /// `valid_until_ms = now_ms` and `active = false`, then appends the new fact version.
    /// Preserves full historical timeline; atomically rejects without mutating old facts if validation fails.
    pub fn supersedeFact(
        *_,
        subject: u32,
        predicate: string,
        new_object: u32,
        confidence: f64,
        now_ms: i64,
    ): usize? {
        if (predicate.len == 0) {
            return null;
        }
        if (!_.isValidEntityId(subject) || !_.isValidEntityId(new_object)) {
            return null;
        }
        if (!isValidConfidence(confidence)) {
            return null;
        }
        if (_.triple_count >= _.triples.len) {
            return null;
        }

        const pred_hash: u64 = hashStr64(predicate);
        let i: usize = 0;
        while (i < _.triple_count) : (i += 1) {
            const t = _.triples[i];
            if (t.active && t.subject == subject && t.predicate_hash == pred_hash && strEq(t.predicate, predicate)) {
                _.triples[i].valid_until_ms = now_ms;
                _.triples[i].active = false;
            }
        }

        const idx: usize = _.triple_count;
        _.triples[idx] = KgTriple{
            subject: subject,
            predicate_hash: pred_hash,
            predicate: predicate,
            object: new_object,
            confidence: confidence,
            valid_from_ms: now_ms,
            valid_until_ms: 0,
            active: true,
        };
        _.triple_count += 1;
        return idx;
    }

    /// Queries triple indices for `subject` valid at timestamp `at_ms`.
    /// - `predicate == ""` matches all predicates for `subject`.
    /// - `at_ms == 0` queries currently active facts (`active && valid_until_ms == 0`).
    /// - `at_ms != 0` performs point-in-time recall on `[valid_from_ms, valid_until_ms)`.
    pub fn queryFactsAt(
        subject: u32,
        predicate: string,
        at_ms: i64,
        out_indices: *[]usize,
    ): usize {
        if (!_.isValidEntityId(subject) || out_indices.len == 0) {
            return 0;
        }
        const filter_pred: bool = predicate.len > 0;
        const pred_hash: u64 = if (filter_pred) hashStr64(predicate) else 0;

        let written: usize = 0;
        let i: usize = 0;
        while (i < _.triple_count && written < out_indices.len) : (i += 1) {
            const t = _.triples[i];
            if (t.subject != subject) {
                continue;
            }
            if (filter_pred && (t.predicate_hash != pred_hash || !strEq(t.predicate, predicate))) {
                continue;
            }
            if (!isTripleValidAt(t, at_ms)) {
                continue;
            }
            out_indices[written] = i;
            written += 1;
        }
        return written;
    }

    /// Recalls deduplicated 1-hop and 2-hop neighboring entities reachable from `start_entity`
    /// via facts valid at `at_ms` (undirected traversal, excludes `start_entity`).
    /// Caller provides `visited_s` scratch slice with length >= `entity_count`.
    pub fn queryTwoHopNeighbors(
        start_entity: u32,
        at_ms: i64,
        out_entities: *[]u32,
        visited_s: *[]u8,
    ): usize {
        if (!_.isValidEntityId(start_entity) || out_entities.len == 0) {
            return 0;
        }
        if (visited_s.len < _.entity_count) {
            return 0;
        }

        // State codes: 0 = unvisited, 1 = start, 2 = 1-hop neighbor, 3 = 2-hop neighbor
        let v: usize = 0;
        while (v < _.entity_count) : (v += 1) {
            visited_s[v] = 0;
        }
        visited_s[start_entity as usize] = 1;

        let written: usize = 0;

        // Pass 1: 1-hop neighbors
        let i: usize = 0;
        while (i < _.triple_count && written < out_entities.len) : (i += 1) {
            const t = _.triples[i];
            if (!isTripleValidAt(t, at_ms)) {
                continue;
            }
            if (t.subject == start_entity && visited_s[t.object as usize] == 0) {
                visited_s[t.object as usize] = 2;
                out_entities[written] = t.object;
                written += 1;
            } else if (t.object == start_entity && visited_s[t.subject as usize] == 0) {
                visited_s[t.subject as usize] = 2;
                out_entities[written] = t.subject;
                written += 1;
            }
        }

        // Pass 2: 2-hop neighbors expanded strictly from state-2 (1-hop) entities
        i = 0;
        while (i < _.triple_count && written < out_entities.len) : (i += 1) {
            const t = _.triples[i];
            if (!isTripleValidAt(t, at_ms)) {
                continue;
            }
            if (visited_s[t.subject as usize] == 2 && visited_s[t.object as usize] == 0) {
                visited_s[t.object as usize] = 3;
                out_entities[written] = t.object;
                written += 1;
            } else if (visited_s[t.object as usize] == 2 && visited_s[t.subject as usize] == 0) {
                visited_s[t.subject as usize] = 3;
                out_entities[written] = t.subject;
                written += 1;
            }
        }

        return written;
    }
}
