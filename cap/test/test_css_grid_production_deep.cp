import { test, expect } from "cap:test";
import {
    CssGridRepeatCountKind,
    CssGridTemplateAreas,
    CssGridTrackBreadth,
    CssGridTrackBreadthKind,
    CssGridTrackItemKind,
    CssGridTrackSize,
    CssGridTrackSizeKind,
    CssGridTrackSizing,
} from "cap:css";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("css grid: track breadth, minmax, and fit-content parsing and serialization", () => {
    const b_auto = CssGridTrackBreadth.parse("auto") catch {
        expect(false).toBe(true);
        return;
    };
    expect(b_auto.kind == CssGridTrackBreadthKind.Auto).toBe(true);

    const b_fr = CssGridTrackBreadth.parse("2.5fr") catch {
        expect(false).toBe(true);
        return;
    };
    expect(b_fr.kind == CssGridTrackBreadthKind.Flex).toBe(true);

    const ts_mm = CssGridTrackSize.parse("minmax(min-content, 1fr)") catch {
        expect(false).toBe(true);
        return;
    };
    expect(ts_mm.kind == CssGridTrackSizeKind.MinMax).toBe(true);
    expect(ts_mm.min_breadth.kind == CssGridTrackBreadthKind.MinContent).toBe(true);
    expect(ts_mm.max_breadth.kind == CssGridTrackBreadthKind.Flex).toBe(true);

    const ts_fc = CssGridTrackSize.parse("fit-content(300px)") catch {
        expect(false).toBe(true);
        return;
    };
    expect(ts_fc.kind == CssGridTrackSizeKind.FitContent).toBe(true);
    expect(sliceEq(ts_fc.fit_arg, "300px")).toBe(true);
});

test("css grid: grid-template-columns with named lines, repeat(), pretty and minified output", () => {
    const sizing = CssGridTrackSizing.parse(
        "[sidebar-start] 240px [sidebar-end main-start] repeat(auto-fit, minmax(200px, 1fr)) [main-end]",
    ) catch {
        expect(false).toBe(true);
        return;
    };
    expect(sizing.is_none).toBe(false);
    expect(sizing.item_count).toBe(2);
    expect(sizing.items[0].kind == CssGridTrackItemKind.TrackSize).toBe(true);
    expect(sizing.items[1].kind == CssGridTrackItemKind.Repeat).toBe(true);
    expect(sizing.items[1].repeat.count.kind == CssGridRepeatCountKind.AutoFit).toBe(true);
    expect(sizing.line_names[0].count).toBe(1);
    expect(sizing.line_names[1].count).toBe(2);
    expect(sizing.line_names[2].count).toBe(1);

    let out: [256]u8 = [0; 256];
    const n_pretty = sizing.toCss(out[0..]) catch 0;
    expect(sliceEq(
        out[0..n_pretty],
        "[sidebar-start] 240px [sidebar-end main-start] repeat(auto-fit, minmax(200px, 1fr)) [main-end]",
    )).toBe(true);

    const n_min = sizing.toCssMinified(out[0..]) catch 0;
    expect(sliceEq(
        out[0..n_min],
        "[sidebar-start]240px[sidebar-end main-start]repeat(auto-fit,minmax(200px,1fr))[main-end]",
    )).toBe(true);
});

test("css grid: none keyword and numeric multi-track repeat()", () => {
    let out: [128]u8 = [0; 128];

    const none_val = CssGridTrackSizing.parse("NONE") catch {
        expect(false).toBe(true);
        return;
    };
    expect(none_val.is_none).toBe(true);
    const n0 = none_val.toCss(out[0..]) catch 0;
    expect(sliceEq(out[0..n0], "none")).toBe(true);

    const rep = CssGridTrackSizing.parse("repeat(3, 100px 1fr) auto") catch {
        expect(false).toBe(true);
        return;
    };
    expect(rep.item_count).toBe(2);
    expect(rep.items[0].repeat.count.kind == CssGridRepeatCountKind.Number).toBe(true);
    expect(rep.items[0].repeat.count.number).toBe(3);
    expect(rep.items[0].repeat.track_count).toBe(2);

    const n1 = rep.toCss(out[0..]) catch 0;
    expect(sliceEq(out[0..n1], "repeat(3, 100px 1fr) auto")).toBe(true);
});

test("css grid: grid-template-areas multi-row parsing, multi-dot null cells, and serialization", () => {
    const areas = CssGridTemplateAreas.parse("\"header header\" \"nav main\" \"footer ...\"") catch {
        expect(false).toBe(true);
        return;
    };
    expect(areas.is_none).toBe(false);
    expect(areas.rows).toBe(3);
    expect(areas.columns).toBe(2);

    if (h0 := areas.cellAt(0, 0)) {
        expect(sliceEq(h0, "header")).toBe(true);
    } else {
        expect(false).toBe(true);
    }
    if (m := areas.cellAt(1, 1)) {
        expect(sliceEq(m, "main")).toBe(true);
    } else {
        expect(false).toBe(true);
    }
    expect(areas.cellAt(2, 1) == null).toBe(true);

    let out: [128]u8 = [0; 128];
    const n = areas.toCss(out[0..]) catch 0;
    expect(sliceEq(out[0..n], "\"header header\" \"nav main\" \"footer .\"")).toBe(true);

    const areas_none = CssGridTemplateAreas.parse("none") catch {
        expect(false).toBe(true);
        return;
    };
    expect(areas_none.is_none).toBe(true);
});

test("css grid: rejects invalid minmax flex min, negative flex, zero repeat, and reserved line names", () => {
    // Flex unit is forbidden as min in minmax()
    let flex_min_err = false;
    * = CssGridTrackSize.parse("minmax(1fr, 200px)") catch {
        flex_min_err = true;
    };
    expect(flex_min_err).toBe(true);

    // Negative flex rejected
    let neg_fr_err = false;
    * = CssGridTrackBreadth.parse("-1fr") catch {
        neg_fr_err = true;
    };
    expect(neg_fr_err).toBe(true);

    // repeat(0, ...) rejected
    let zero_rep_err = false;
    * = CssGridTrackSizing.parse("repeat(0, 1fr)") catch {
        zero_rep_err = true;
    };
    expect(zero_rep_err).toBe(true);

    // Reserved keyword `span` in [line-names] rejected
    let span_line_err = false;
    * = CssGridTrackSizing.parse("[span] 1fr") catch {
        span_line_err = true;
    };
    expect(span_line_err).toBe(true);
});

test("css grid: rejects uneven area columns, L-shaped non-rectangular areas, and tiny buffer", () => {
    // Uneven columns
    let uneven_err = false;
    * = CssGridTemplateAreas.parse("\"a b\" \"c\"") catch {
        uneven_err = true;
    };
    expect(uneven_err).toBe(true);

    // Non-rectangular (L-shaped) named area
    let l_shape_err = false;
    * = CssGridTemplateAreas.parse("\"a a\" \"a b\"") catch {
        l_shape_err = true;
    };
    expect(l_shape_err).toBe(true);

    // Disconnected duplicate named area
    let split_err = false;
    * = CssGridTemplateAreas.parse("\"a b a\"") catch {
        split_err = true;
    };
    expect(split_err).toBe(true);

    // BufferTooSmall guard
    const ok_sizing = CssGridTrackSizing.parse("1fr 2fr") catch {
        expect(false).toBe(true);
        return;
    };
    let tiny: [3]u8 = [0; 3];
    let buf_err = false;
    * = ok_sizing.toCss(tiny[0..]) catch {
        buf_err = true;
    };
    expect(buf_err).toBe(true);
});
