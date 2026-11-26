// @cap/sitemap — Zero-heap Sitemaps.org 0.9 XML Sitemap & SitemapIndex Generator
// Clean-room MIT implementation in pure CP (modeled after npm `sitemap`).

pub const FREQ_NONE: u8 = 0;
pub const FREQ_ALWAYS: u8 = 1;
pub const FREQ_HOURLY: u8 = 2;
pub const FREQ_DAILY: u8 = 3;
pub const FREQ_WEEKLY: u8 = 4;
pub const FREQ_MONTHLY: u8 = 5;
pub const FREQ_YEARLY: u8 = 6;
pub const FREQ_NEVER: u8 = 7;

pub struct SitemapUrl {
    loc: []const u8,
    lastmod: []const u8,
    changefreq: u8,
    priority_tenths: i8, // -1 = omit, 0..10 -> "0.0".."1.0"
}

fn freqName(f: u8): []const u8 {
    if (f == FREQ_ALWAYS) { return "always"; }
    if (f == FREQ_HOURLY) { return "hourly"; }
    if (f == FREQ_DAILY) { return "daily"; }
    if (f == FREQ_WEEKLY) { return "weekly"; }
    if (f == FREQ_MONTHLY) { return "monthly"; }
    if (f == FREQ_YEARLY) { return "yearly"; }
    if (f == FREQ_NEVER) { return "never"; }
    return "";
}

pub fn isValidPriorityTenths(p: i8): bool {
    return p >= 0 && p <= 10;
}

pub struct SitemapWriter {
    buf: []u8,
    pos: usize,
    url_count: usize,
    closed: bool,
    overflow: bool,

    pub static fn init(buf: []u8): SitemapWriter {
        let w = SitemapWriter {
            buf: buf,
            pos: 0,
            url_count: 0,
            closed: false,
            overflow: false,
        };
        * = w.writeRaw("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<urlset xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\">\n");
        return w;
    }

    fn writeRaw(*_, s: []const u8): bool {
        if (_.overflow || _.pos + s.len > _.buf.len) {
            _.overflow = true;
            return false;
        }
        let i: usize = 0;
        while (i < s.len) {
            _.buf[_.pos + i] = s[i];
            i += 1;
        }
        _.pos = _.pos + s.len;
        return true;
    }

    fn writeXmlEscaped(*_, s: []const u8): bool {
        let i: usize = 0;
        while (i < s.len) {
            let c: u8 = s[i];
            if (c == '&') {
                if (!_.writeRaw("&amp;")) { return false; }
            } else if (c == '<') {
                if (!_.writeRaw("&lt;")) { return false; }
            } else if (c == '>') {
                if (!_.writeRaw("&gt;")) { return false; }
            } else if (c == '"') {
                if (!_.writeRaw("&quot;")) { return false; }
            } else if (c == '\'') {
                if (!_.writeRaw("&apos;")) { return false; }
            } else {
                if (_.overflow || _.pos >= _.buf.len) {
                    _.overflow = true;
                    return false;
                }
                _.buf[_.pos] = c;
                _.pos += 1;
            }
            i += 1;
        }
        return true;
    }

    pub fn addUrl(*_, entry: SitemapUrl): bool {
        if (_.closed || _.overflow || entry.loc.len == 0) {
            return false;
        }
        if (!_.writeRaw("  <url>\n    <loc>")) { return false; }
        if (!_.writeXmlEscaped(entry.loc)) { return false; }
        if (!_.writeRaw("</loc>\n")) { return false; }

        if (entry.lastmod.len > 0) {
            if (!_.writeRaw("    <lastmod>")) { return false; }
            if (!_.writeXmlEscaped(entry.lastmod)) { return false; }
            if (!_.writeRaw("</lastmod>\n")) { return false; }
        }

        let freq: []const u8 = freqName(entry.changefreq);
        if (freq.len > 0) {
            if (!_.writeRaw("    <changefreq>")) { return false; }
            if (!_.writeRaw(freq)) { return false; }
            if (!_.writeRaw("</changefreq>\n")) { return false; }
        }

        if (entry.priority_tenths >= 0 && entry.priority_tenths <= 10) {
            if (!_.writeRaw("    <priority>")) { return false; }
            if (entry.priority_tenths == 10) {
                if (!_.writeRaw("1.0")) { return false; }
            } else {
                let pbuf: [3]u8 = undefined;
                pbuf[0] = '0';
                pbuf[1] = '.';
                pbuf[2] = ('0' as u8) + (entry.priority_tenths as u8);
                if (!_.writeRaw(pbuf[0..3])) { return false; }
            }
            if (!_.writeRaw("</priority>\n")) { return false; }
        }

        if (!_.writeRaw("  </url>\n")) { return false; }
        _.url_count += 1;
        return true;
    }

    pub fn finish(*_): i64 {
        if (_.overflow) {
            return -1;
        }
        if (!_.closed) {
            if (!_.writeRaw("</urlset>\n")) {
                return -1;
            }
            _.closed = true;
        }
        return _.pos as i64;
    }

    pub fn urlCount(): usize {
        return _.url_count;
    }

    pub fn len(): usize {
        return _.pos;
    }

    pub fn written(): []const u8 {
        return _.buf[0.._.pos];
    }
}

pub struct SitemapIndexWriter {
    buf: []u8,
    pos: usize,
    entry_count: usize,
    closed: bool,
    overflow: bool,

    pub static fn init(buf: []u8): SitemapIndexWriter {
        let w = SitemapIndexWriter {
            buf: buf,
            pos: 0,
            entry_count: 0,
            closed: false,
            overflow: false,
        };
        * = w.writeRaw("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<sitemapindex xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\">\n");
        return w;
    }

    fn writeRaw(*_, s: []const u8): bool {
        if (_.overflow || _.pos + s.len > _.buf.len) {
            _.overflow = true;
            return false;
        }
        let i: usize = 0;
        while (i < s.len) {
            _.buf[_.pos + i] = s[i];
            i += 1;
        }
        _.pos = _.pos + s.len;
        return true;
    }

    fn writeXmlEscaped(*_, s: []const u8): bool {
        let i: usize = 0;
        while (i < s.len) {
            let c: u8 = s[i];
            if (c == '&') {
                if (!_.writeRaw("&amp;")) { return false; }
            } else if (c == '<') {
                if (!_.writeRaw("&lt;")) { return false; }
            } else if (c == '>') {
                if (!_.writeRaw("&gt;")) { return false; }
            } else if (c == '"') {
                if (!_.writeRaw("&quot;")) { return false; }
            } else if (c == '\'') {
                if (!_.writeRaw("&apos;")) { return false; }
            } else {
                if (_.overflow || _.pos >= _.buf.len) {
                    _.overflow = true;
                    return false;
                }
                _.buf[_.pos] = c;
                _.pos += 1;
            }
            i += 1;
        }
        return true;
    }

    pub fn addSitemap(*_, loc: []const u8, lastmod: []const u8): bool {
        if (_.closed || _.overflow || loc.len == 0) {
            return false;
        }
        if (!_.writeRaw("  <sitemap>\n    <loc>")) { return false; }
        if (!_.writeXmlEscaped(loc)) { return false; }
        if (!_.writeRaw("</loc>\n")) { return false; }
        if (lastmod.len > 0) {
            if (!_.writeRaw("    <lastmod>")) { return false; }
            if (!_.writeXmlEscaped(lastmod)) { return false; }
            if (!_.writeRaw("</lastmod>\n")) { return false; }
        }
        if (!_.writeRaw("  </sitemap>\n")) { return false; }
        _.entry_count += 1;
        return true;
    }

    pub fn finish(*_): i64 {
        if (_.overflow) {
            return -1;
        }
        if (!_.closed) {
            if (!_.writeRaw("</sitemapindex>\n")) {
                return -1;
            }
            _.closed = true;
        }
        return _.pos as i64;
    }

    pub fn entryCount(): usize {
        return _.entry_count;
    }

    pub fn written(): []const u8 {
        return _.buf[0.._.pos];
    }
}
