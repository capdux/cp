// Type definitions for CP

export fn d3pPushStr(buf: []u8, pos: usize, s: string): usize;
export fn d3pPushU64(buf: []u8, pos: usize, val: u64): usize;
export fn d3pPushNum(buf: []u8, pos: usize, val: f64): usize;
export class SvgPath {
    buf: [2048]u8,
    len: usize,
    has_subpath: bool,
    x0: f64,
    y0: f64,
    x1: f64,
    y1: f64,
    fn new(): SvgPath;
    fn byteLen(): usize;
    fn reset(_: auto);
    fn moveTo(_: auto, x: f64, y: f64);
    fn lineTo(_: auto, x: f64, y: f64);
    fn quadraticCurveTo(_: auto, cpx: f64, cpy: f64, x: f64, y: f64);
    fn bezierCurveTo(_: auto, cp1x: f64, cp1y: f64, cp2x: f64, cp2y: f64, x: f64, y: f64);
    fn rect(_: auto, x: f64, y: f64, w: f64, h: f64);
    fn closePath(_: auto);
    fn writeTo(out: []u8): usize;
}

