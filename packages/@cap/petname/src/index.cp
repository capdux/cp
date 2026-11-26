// @cap/petname — Zero-allocation human-readable name generator for containers, sandboxes & subdomains
// Clean-room MIT implementation inspired by npm `unique-names-generator`, `human-id`, and Rust `petname`.

fn splitmix64Next(state: u64) -> u64 {
    let z = state +% (((0x9e3779b9 as u64) << 32) | (0x7f4a7c15 as u64));
    z = (z ^ (z >> 30)) *% (((0xbf58476d as u64) << 32) | (0x1ce4e5b9 as u64));
    z = (z ^ (z >> 27)) *% (((0x94d049bb as u64) << 32) | (0x133111eb as u64));
    return z ^ (z >> 31);
}

fn adjectiveAt(idx: usize) -> string {
    const tbl: [64]string = [
        "amber", "ancient", "arctic", "astral", "azure", "bold", "brave", "bright",
        "brisk", "calm", "cedar", "civic", "clean", "clear", "clever", "coastal",
        "cobalt", "coral", "cosmic", "crisp", "crimson", "crystal", "cyber", "daring",
        "dawn", "delta", "eager", "echo", "ember", "emerald", "epic", "fast",
        "fierce", "fleet", "frost", "gentle", "gilded", "golden", "grand", "green",
        "happy", "harbor", "hyper", "indigo", "ivory", "jade", "keen", "kind",
        "lively", "lunar", "magic", "maple", "mighty", "misty", "modern", "noble",
        "nordic", "nova", "ocean", "olive", "onyx", "opal", "orbital", "swift",
    ];
    return tbl[idx % 64];
}

fn adverbAt(idx: usize) -> string {
    const tbl: [32]string = [
        "ably", "boldly", "bravely", "brightly", "briskly", "calmly", "cleanly", "clearly",
        "deftly", "eagerly", "fairly", "firmly", "freely", "gently", "gladly", "grandly",
        "keenly", "kindly", "lightly", "neatly", "nicely", "openly", "purely", "quickly",
        "quietly", "rapidly", "safely", "sharply", "simply", "smoothly", "softly", "swiftly",
    ];
    return tbl[idx % 32];
}

fn nounAt(idx: usize) -> string {
    const tbl: [64]string = [
        "albatross", "alpaca", "antelope", "atlas", "aurora", "badger", "beacon", "bison",
        "bobcat", "breeze", "canyon", "caribou", "cedar", "cheetah", "citadel", "cloud",
        "comet", "condor", "cougar", "crane", "dolphin", "dragon", "eagle", "falcon",
        "fern", "finch", "fjord", "fox", "gazelle", "gecko", "glacier", "griffin",
        "harbor", "hawk", "heron", "horizon", "ibex", "iguana", "jaguar", "kestrel",
        "koala", "lagoon", "lemur", "leopard", "lynx", "mantis", "marten", "meadow",
        "nebula", "orca", "osprey", "otter", "owl", "panda", "panther", "pelican",
        "phoenix", "puma", "quasar", "raven", "river", "robin", "salmon", "wolf",
    ];
    return tbl[idx % 64];
}

fn appendStr(out: []u8, pos: usize, s: string) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn appendHex4(out: []u8, pos: usize, val: u64) -> usize {
    const hex = "0123456789abcdef";
    let p = pos;
    let shift: u6 = 12;
    let i: usize = 0;
    while (i < 4 && p < out.len) {
        const nib = ((val >> shift) & 0x0f) as usize;
        out[p] = hex[nib];
        p += 1;
        if (shift >= 4) {
            shift -= 4;
        }
        i += 1;
    }
    return p;
}

pub struct Petname {
    pub state: u64,

    pub static fn init(seed: u64) -> Petname {
        return Petname{ state: seed };
    }

    pub fn nextU64(*_) -> u64 {
        _.state = splitmix64Next(_.state);
        return _.state;
    }

    pub fn generate(*_, words: u8, sep: string, out: []u8) -> string {
        let pos: usize = 0;
        const w = if (words == 0) { 2 as u8 } else { words };
        if (w == 1) {
            pos = appendStr(out, pos, nounAt((_.nextU64() % 64) as usize));
            return out[0..pos];
        }
        if (w == 2) {
            pos = appendStr(out, pos, adjectiveAt((_.nextU64() % 64) as usize));
            pos = appendStr(out, pos, sep);
            pos = appendStr(out, pos, nounAt((_.nextU64() % 64) as usize));
            return out[0..pos];
        }
        // 3 or more words: adverb + adj... + noun
        pos = appendStr(out, pos, adverbAt((_.nextU64() % 32) as usize));
        let mid: u8 = 2;
        while (mid < w) {
            pos = appendStr(out, pos, sep);
            pos = appendStr(out, pos, adjectiveAt((_.nextU64() % 64) as usize));
            mid += 1;
        }
        pos = appendStr(out, pos, sep);
        pos = appendStr(out, pos, nounAt((_.nextU64() % 64) as usize));
        return out[0..pos];
    }

    pub fn generateSlug(*_, out: []u8) -> string {
        return _.generate(2 as u8, "-", out);
    }

    pub fn generateSubdomain(*_, out: []u8) -> string {
        let pos: usize = 0;
        pos = appendStr(out, pos, adjectiveAt((_.nextU64() % 64) as usize));
        pos = appendStr(out, pos, "-");
        pos = appendStr(out, pos, nounAt((_.nextU64() % 64) as usize));
        pos = appendStr(out, pos, "-");
        pos = appendHex4(out, pos, _.nextU64());
        return out[0..pos];
    }
}

pub fn petnameFromSeed(seed: u64, words: u8, sep: string, out: []u8) -> string {
    let gen = Petname.init(seed);
    return gen.generate(words, sep, out);
}

pub fn subdomainFromSeed(seed: u64, out: []u8) -> string {
    let gen = Petname.init(seed);
    return gen.generateSubdomain(out);
}

export {
    Petname,
    petnameFromSeed,
    subdomainFromSeed,
};
