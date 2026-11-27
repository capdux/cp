// Type definitions for CP

export enum BadgeStyle {
    Flat,
    FlatSquare,
    Plastic
}

export class BadgeOptions {
    label: string,
    message: string,
    labelColor: string,
    color: string,
    style: BadgeStyle,
}

export fn defaultBadge(label: string, message: string, color: string): BadgeOptions;
export fn resolveBadgeColor(name: string): string;
export fn estimateTextWidth(s: string): usize;
export class BufWriter {
    out: []u8,
    pos: usize,
    fn writeStr(_: auto, s: string);
    fn writeXmlEscaped(_: auto, s: string);
    fn writeUsize(_: auto, val: usize);
}

export fn renderBadge(opts: BadgeOptions, out: []u8): string;
export fn makeBadge(label: string, message: string, color: string, out: []u8): string;
