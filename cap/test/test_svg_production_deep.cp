// Source: cp/cap/test/test_svg_production_deep.cp (Production Deep Tests for cap:svg & cap.svg)
const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    SvgError,
    SvgPoint,
    SvgStyle,
    SvgPathBuilder,
    SvgCanvas,
    ChartDatum,
    LineSeries,
    SvgCharts,
} from "cap:image";
import { XmlParser } from "cap:xml";
import cap from "cap";

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    return std.mem.indexOf(u8, haystack, needle) != null;
}

fn expectInvalidDims(width: f64, height: f64): ! {
    let failed = false;
    let c = SvgCanvas.init(std.heap.page_allocator, width, height) catch e => {
        if (e == SvgError.InvalidDimensions) {
            failed = true;
        }
        expect_eq(failed, true);
        return;
    };
    c.deinit();
    expect_eq(false, true);
}

fn caseCanvasAndPath(): ! {
    const alloc = std.heap.page_allocator;

    expectInvalidDims(0.0, 100.0)?;
    expectInvalidDims(100.0, -5.0)?;
    expectInvalidDims(std.math.nan(f64), 100.0)?;

    let canvas = SvgCanvas.init(alloc, 400.0, 300.0)?;
    defer canvas.deinit();
    canvas.setBackground("#f8fafc");

    canvas.beginGroup("layer-1\" onclick=\"alert(1)", "translate(10, 20)")?;
    canvas.rect(10.0, 15.0, 120.0, 60.0, 6.0, SvgStyle{ fill: "#3b82f6", stroke: "#1e40af", stroke_width: 2.0 })?;
    canvas.circle(200.0, 100.0, 35.0, SvgStyle{ fill: "#10b981", opacity: 0.85 })?;
    canvas.line(0.0, 0.0, 400.0, 300.0, SvgStyle{ stroke: "#94a3b8", stroke_width: 1.0 })?;

    const poly_pts: [3]SvgPoint = [
        SvgPoint{ x: 10.0, y: 200.0 },
        SvgPoint{ x: 60.0, y: 160.0 },
        SvgPoint{ x: 110.0, y: 200.0 },
    ];
    canvas.polyline(poly_pts[0..], SvgStyle{ fill: "none", stroke: "#f59e0b", stroke_width: 2.5 })?;

    let pb = SvgPathBuilder.init(alloc);
    defer pb.deinit();
    pb.moveTo(20.0, 240.0)?;
    pb.lineTo(120.0, 240.0)?;
    pb.curveTo(150.0, 240.0, 150.0, 280.0, 180.0, 280.0)?;
    pb.arcTo(20.0, 20.0, 0.0, false, true, 220.0, 280.0)?;
    pb.close()?;
    canvas.path(pb.slice(), SvgStyle{ fill: "#8b5cf6", stroke: "#4c1d95", stroke_width: 1.5 })?;

    canvas.text(
        200.0,
        40.0,
        "Q1 & Q2 <script>alert(\"xss\")</script> 'test'",
        SvgStyle{ fill: "#0f172a", font_size: 14.0, anchor: "middle" }
    )?;
    canvas.endGroup()?;

    const svg_bytes = canvas.render(alloc)?;
    defer alloc.free(svg_bytes);

    expect_eq(containsSubstr(svg_bytes, "<script>"), false);
    expect_eq(containsSubstr(svg_bytes, "&lt;script&gt;alert(&quot;xss&quot;)&lt;/script&gt;"), true);
    expect_eq(containsSubstr(svg_bytes, "Q1 &amp; Q2"), true);
    expect_eq(containsSubstr(svg_bytes, "&apos;test&apos;"), true);
    expect_eq(containsSubstr(svg_bytes, "onclick=&quot;alert(1)"), true);

    let doc = XmlParser.parse(alloc, svg_bytes)?;
    defer doc.deinit();
    expect_eq(std.mem.eql(u8, doc.root.tag, "svg"), true);
    expect_eq(std.mem.eql(u8, doc.root.getAttr("viewBox").?, "0 0 400 300"), true);
    const g_node = doc.root.findChild("g").?;
    expect_eq(g_node.findChild("rect") != null, true);
    expect_eq(g_node.findChild("circle") != null, true);
    expect_eq(g_node.findChild("polyline") != null, true);
    expect_eq(g_node.findChild("path") != null, true);
    expect_eq(g_node.findChild("text") != null, true);
}

fn caseBarChart(): ! {
    const alloc = std.heap.page_allocator;

    const empty_data: []const ChartDatum = [];
    let got_empty_err = false;
    const empty_res = SvgCharts.barChart(alloc, "Empty", empty_data, 600.0, 400.0) catch e => {
        if (e == SvgError.EmptySeries) {
            got_empty_err = true;
        }
        empty_res_blk: {
            break :empty_res_blk "";
        }
    };
    if (empty_res.len > 0) alloc.free(empty_res);
    expect_eq(got_empty_err, true);

    const bars: [4]ChartDatum = [
        ChartDatum{ label: "Jan", value: 125.5, color: "" },
        ChartDatum{ label: "Feb", value: -42.0, color: "#ef4444" },
        ChartDatum{ label: "Mar", value: 0.0, color: "" },
        ChartDatum{ label: "Apr", value: 210.0, color: "#10b981" },
    ];
    const svg_bytes = SvgCharts.barChart(alloc, "Monthly Net <USD>", bars[0..], 640.0, 360.0)?;
    defer alloc.free(svg_bytes);

    expect_eq(containsSubstr(svg_bytes, "Monthly Net &lt;USD&gt;"), true);
    expect_eq(containsSubstr(svg_bytes, "<rect"), true);
    expect_eq(containsSubstr(svg_bytes, "Jan"), true);
    expect_eq(containsSubstr(svg_bytes, "Feb"), true);

    let doc = XmlParser.parse(alloc, svg_bytes)?;
    defer doc.deinit();
    expect_eq(std.mem.eql(u8, doc.root.tag, "svg"), true);
}

fn caseLineChart(): ! {
    const alloc = std.heap.page_allocator;

    const bad_pts: [2]SvgPoint = [
        SvgPoint{ x: 0.0, y: 10.0 },
        SvgPoint{ x: 1.0, y: std.math.nan(f64) },
    ];
    const bad_series: [1]LineSeries = [
        LineSeries{ name: "Bad", points: bad_pts[0..], color: "" },
    ];
    let caught_nan = false;
    const bad_res = SvgCharts.lineChart(alloc, "Bad", bad_series[0..], 640.0, 360.0) catch e => {
        if (e == SvgError.InvalidValue) caught_nan = true;
        bad_res_blk: {
            break :bad_res_blk "";
        }
    };
    if (bad_res.len > 0) alloc.free(bad_res);
    expect_eq(caught_nan, true);

    const pts_a: [4]SvgPoint = [
        SvgPoint{ x: 0.0, y: 12.0 },
        SvgPoint{ x: 1.0, y: 45.0 },
        SvgPoint{ x: 2.0, y: 38.0 },
        SvgPoint{ x: 3.0, y: 92.0 },
    ];
    const pts_b: [4]SvgPoint = [
        SvgPoint{ x: 0.0, y: 20.0 },
        SvgPoint{ x: 1.0, y: 15.0 },
        SvgPoint{ x: 2.0, y: 64.0 },
        SvgPoint{ x: 3.0, y: 80.0 },
    ];
    const series: [2]LineSeries = [
        LineSeries{ name: "CPU %", points: pts_a[0..], color: "#3b82f6" },
        LineSeries{ name: "Mem %", points: pts_b[0..], color: "#10b981" },
    ];

    const svg_bytes = SvgCharts.lineChart(alloc, "Telemetry", series[0..], 720.0, 400.0)?;
    defer alloc.free(svg_bytes);

    expect_eq(containsSubstr(svg_bytes, "<polyline"), true);
    expect_eq(containsSubstr(svg_bytes, "<circle"), true);
    expect_eq(containsSubstr(svg_bytes, "CPU %"), true);
    expect_eq(containsSubstr(svg_bytes, "Mem %"), true);

    let doc = XmlParser.parse(alloc, svg_bytes)?;
    defer doc.deinit();
    expect_eq(std.mem.eql(u8, doc.root.tag, "svg"), true);
}

fn casePieChart(): ! {
    const alloc = std.heap.page_allocator;
    const slices: [3]ChartDatum = [
        ChartDatum{ label: "Compute", value: 50.0, color: "#3b82f6" },
        ChartDatum{ label: "Storage", value: 30.0, color: "#10b981" },
        ChartDatum{ label: "Network", value: 20.0, color: "#f59e0b" },
    ];

    const pie_svg = SvgCharts.pieChart(alloc, "Cost Breakdown", slices[0..], 560.0, 360.0, 0.0)?;
    defer alloc.free(pie_svg);
    expect_eq(containsSubstr(pie_svg, "<path d="), true);
    expect_eq(containsSubstr(pie_svg, "Compute (50%)"), true);

    const donut_svg = SvgCharts.pieChart(alloc, "Donut", slices[0..], 560.0, 360.0, 0.55)?;
    defer alloc.free(donut_svg);
    expect_eq(containsSubstr(donut_svg, "<path d="), true);

    const full_slice: [1]ChartDatum = [
        ChartDatum{ label: "All", value: 100.0, color: "#8b5cf6" },
    ];
    const full_svg = SvgCharts.pieChart(alloc, "100% Full", full_slice[0..], 480.0, 320.0, 0.4)?;
    defer alloc.free(full_svg);
    expect_eq(containsSubstr(full_svg, "A "), true);

    let doc = XmlParser.parse(alloc, full_svg)?;
    defer doc.deinit();
    expect_eq(std.mem.eql(u8, doc.root.tag, "svg"), true);
}

fn caseDynamicCapSvg(): ! {
    const alloc = std.heap.page_allocator;
    let canvas = cap.image.SvgCanvas.init(alloc, 200.0, 100.0)?;
    defer canvas.deinit();
    canvas.rect(0.0, 0.0, 200.0, 100.0, 4.0, cap.image.SvgStyle{ fill: "#0ea5e9" })?;
    const out = canvas.render(alloc)?;
    defer alloc.free(out);
    expect_eq(containsSubstr(out, "viewBox=\"0 0 200 100\""), true);
    expect_eq(containsSubstr(out, "fill=\"#0ea5e9\""), true);
}

fn main() {
    test("SvgCanvas & SvgPathBuilder primitives, grouping, XSS escaping, and cap:xml DOM validation", () => {
        caseCanvasAndPath() catch unreachable;
    });

    test("SvgCharts.barChart positive, negative, zero values and EmptySeries guard", () => {
        caseBarChart() catch unreachable;
    });

    test("SvgCharts.lineChart multi-series normalization, NaN/Inf rejection, and polyline output", () => {
        caseLineChart() catch unreachable;
    });

    test("SvgCharts.pieChart solid pie, donut ring, and 100% single-slice boundary", () => {
        casePieChart() catch unreachable;
    });

    test("import cap from \"cap\" dynamic cap.svg namespace mounting", () => {
        caseDynamicCapSvg() catch unreachable;
    });
}
