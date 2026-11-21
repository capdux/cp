// Source: Absorbed from /data/dev/bun/src/css/rules/page.cp, /data/dev/bun/src/css/properties/position.cp

/// CSS Paged Media 与定位属性解析错误集
pub err CssPageError {
    InvalidPageSelector,
    InvalidPseudoClass,
    InvalidMarginBox,
    InvalidPosition,
    TooManyPseudoClasses,
    BufferTooSmall,
}

/// `@page` 选择器页伪类（`:left` / `:right` / `:first` / `:last` / `:blank`）
pub enum PagePseudoClass: u8 {
    left = 0,
    right = 1,
    first = 2,
    last = 3,
    blank = 4,

    /// 解析页伪类（兼容带或不带前导 `:`，ASCII 大小写无关）
    static fn parse(input: string): PagePseudoClass? {
        let s = input;
        if (s.len > 0 && s[0] == ':') {
            s = s[1..];
        }
        if (std.ascii.eqlIgnoreCase(s, "left")) { return .left; }
        if (std.ascii.eqlIgnoreCase(s, "right")) { return .right; }
        if (std.ascii.eqlIgnoreCase(s, "first")) { return .first; }
        if (std.ascii.eqlIgnoreCase(s, "last")) { return .last; }
        if (std.ascii.eqlIgnoreCase(s, "blank")) { return .blank; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .left => "left",
            .right => "right",
            .first => "first",
            .last => "last",
            .blank => "blank",
        };
    }

    fn toSelectorCss(): string {
        return switch (_) {
            .left => ":left",
            .right => ":right",
            .first => ":first",
            .last => ":last",
            .blank => ":blank",
        };
    }
}

/// 页边盒方位大类
pub enum PageMarginEdge: u8 {
    top = 0,
    bottom = 1,
    left = 2,
    right = 3,
    corner = 4,
}

/// CSS Paged Media Level 3 全部 16 种页边盒（`@top-left-corner` .. `@bottom-right-corner`）
pub enum PageMarginBox: u8 {
    top_left_corner = 0,
    top_left = 1,
    top_center = 2,
    top_right = 3,
    top_right_corner = 4,
    left_top = 5,
    left_middle = 6,
    left_bottom = 7,
    right_top = 8,
    right_middle = 9,
    right_bottom = 10,
    bottom_left_corner = 11,
    bottom_left = 12,
    bottom_center = 13,
    bottom_right = 14,
    bottom_right_corner = 15,

    /// 解析页边盒名称（兼容带或不带前导 `@`，ASCII 大小写无关）
    static fn parse(input: string): PageMarginBox? {
        let s = input;
        if (s.len > 0 && s[0] == '@') {
            s = s[1..];
        }
        if (std.ascii.eqlIgnoreCase(s, "top-left-corner")) { return .top_left_corner; }
        if (std.ascii.eqlIgnoreCase(s, "top-left")) { return .top_left; }
        if (std.ascii.eqlIgnoreCase(s, "top-center")) { return .top_center; }
        if (std.ascii.eqlIgnoreCase(s, "top-right")) { return .top_right; }
        if (std.ascii.eqlIgnoreCase(s, "top-right-corner")) { return .top_right_corner; }
        if (std.ascii.eqlIgnoreCase(s, "left-top")) { return .left_top; }
        if (std.ascii.eqlIgnoreCase(s, "left-middle")) { return .left_middle; }
        if (std.ascii.eqlIgnoreCase(s, "left-bottom")) { return .left_bottom; }
        if (std.ascii.eqlIgnoreCase(s, "right-top")) { return .right_top; }
        if (std.ascii.eqlIgnoreCase(s, "right-middle")) { return .right_middle; }
        if (std.ascii.eqlIgnoreCase(s, "right-bottom")) { return .right_bottom; }
        if (std.ascii.eqlIgnoreCase(s, "bottom-left-corner")) { return .bottom_left_corner; }
        if (std.ascii.eqlIgnoreCase(s, "bottom-left")) { return .bottom_left; }
        if (std.ascii.eqlIgnoreCase(s, "bottom-center")) { return .bottom_center; }
        if (std.ascii.eqlIgnoreCase(s, "bottom-right")) { return .bottom_right; }
        if (std.ascii.eqlIgnoreCase(s, "bottom-right-corner")) { return .bottom_right_corner; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .top_left_corner => "top-left-corner",
            .top_left => "top-left",
            .top_center => "top-center",
            .top_right => "top-right",
            .top_right_corner => "top-right-corner",
            .left_top => "left-top",
            .left_middle => "left-middle",
            .left_bottom => "left-bottom",
            .right_top => "right-top",
            .right_middle => "right-middle",
            .right_bottom => "right-bottom",
            .bottom_left_corner => "bottom-left-corner",
            .bottom_left => "bottom-left",
            .bottom_center => "bottom-center",
            .bottom_right => "bottom-right",
            .bottom_right_corner => "bottom-right-corner",
        };
    }

    fn toAtRuleCss(): string {
        return switch (_) {
            .top_left_corner => "@top-left-corner",
            .top_left => "@top-left",
            .top_center => "@top-center",
            .top_right => "@top-right",
            .top_right_corner => "@top-right-corner",
            .left_top => "@left-top",
            .left_middle => "@left-middle",
            .left_bottom => "@left-bottom",
            .right_top => "@right-top",
            .right_middle => "@right-middle",
            .right_bottom => "@right-bottom",
            .bottom_left_corner => "@bottom-left-corner",
            .bottom_left => "@bottom-left",
            .bottom_center => "@bottom-center",
            .bottom_right => "@bottom-right",
            .bottom_right_corner => "@bottom-right-corner",
        };
    }

    fn isCorner(): bool {
        return switch (_) {
            .top_left_corner, .top_right_corner, .bottom_left_corner, .bottom_right_corner => true,
            else => false,
        };
    }

    fn isTop(): bool {
        return switch (_) {
            .top_left_corner, .top_left, .top_center, .top_right, .top_right_corner => true,
            else => false,
        };
    }

    fn isBottom(): bool {
        return switch (_) {
            .bottom_left_corner, .bottom_left, .bottom_center, .bottom_right, .bottom_right_corner => true,
            else => false,
        };
    }

    fn isLeft(): bool {
        return switch (_) {
            .top_left_corner, .left_top, .left_middle, .left_bottom, .bottom_left_corner => true,
            else => false,
        };
    }

    fn isRight(): bool {
        return switch (_) {
            .top_right_corner, .right_top, .right_middle, .right_bottom, .bottom_right_corner => true,
            else => false,
        };
    }

    fn edge(): PageMarginEdge {
        if (_.isCorner()) {
            return PageMarginEdge.corner;
        }
        if (_.isTop()) {
            return PageMarginEdge.top;
        }
        if (_.isBottom()) {
            return PageMarginEdge.bottom;
        }
        if (_.isLeft()) {
            return PageMarginEdge.left;
        }
        return PageMarginEdge.right;
    }
}

fn isIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '-';
}

fn isIdentChar(c: u8): bool {
    return isIdentStart(c) || (c >= '0' && c <= '9');
}

/// `@page` 页选择器（可选命名页 + 零个或多个页伪类，零堆分配内联存储）
pub struct PageSelector {
    name: string? = null,
    pseudo_classes: [8]PagePseudoClass = [
        PagePseudoClass.left,
        PagePseudoClass.left,
        PagePseudoClass.left,
        PagePseudoClass.left,
        PagePseudoClass.left,
        PagePseudoClass.left,
        PagePseudoClass.left,
        PagePseudoClass.left,
    ],
    pseudo_count: usize = 0,

    /// 解析单个 `@page` 选择器（如 `"toc:first:left"`、`":blank"`、`"landscape"`）；禁止内部空白与非法伪类。
    static fn parse(input: string): PageSelector ! CssPageError {
        if (input.len == 0) {
            return err.InvalidPageSelector;
        }
        let sel = PageSelector{};
        let pos: usize = 0;

        // 可选命名页标识符（首个 ':' 之前）
        if (input[0] != ':') {
            if (!isIdentStart(input[0])) {
                return err.InvalidPageSelector;
            }
            while (pos < input.len && input[pos] != ':') {
                if (!isIdentChar(input[pos])) {
                    return err.InvalidPageSelector;
                }
                pos += 1;
            }
            sel.name = input[0..pos];
        }

        // 解析连续页伪类（不允许空白）
        while (pos < input.len) {
            if (input[pos] != ':') {
                return err.InvalidPageSelector;
            }
            pos += 1;
            const start = pos;
            while (pos < input.len && input[pos] != ':') {
                pos += 1;
            }
            if (pos == start) {
                return err.InvalidPseudoClass;
            }
            const pseudo = PagePseudoClass.parse(input[start..pos]) ?? return err.InvalidPseudoClass;
            if (sel.pseudo_count >= 8) {
                return err.TooManyPseudoClasses;
            }
            sel.pseudo_classes[sel.pseudo_count] = pseudo;
            sel.pseudo_count += 1;
        }

        if (sel.name == null && sel.pseudo_count == 0) {
            return err.InvalidPageSelector;
        }
        return sel;
    }

    fn isAnonymous(): bool {
        return _.name == null;
    }

    fn hasPseudo(target: PagePseudoClass): bool {
        let i: usize = 0;
        while (i < _.pseudo_count) {
            if (_.pseudo_classes[i] == target) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// 序列化选择器到目标缓冲区（如 `"toc:first:left"`），返回写入字节数。
    fn toCss(buf: []u8): usize ! CssPageError {
        let pos: usize = 0;
        if (n := _.name) {
            if (pos + n.len > buf.len) {
                return err.BufferTooSmall;
            }
            @memcpy(buf[pos .. pos + n.len], n);
            pos += n.len;
        }
        let i: usize = 0;
        while (i < _.pseudo_count) {
            const pstr = _.pseudo_classes[i].toSelectorCss();
            if (pos + pstr.len > buf.len) {
                return err.BufferTooSmall;
            }
            @memcpy(buf[pos .. pos + pstr.len], pstr);
            pos += pstr.len;
            i += 1;
        }
        return pos;
    }

    /// 序列化完整 `@page` 规则头（如 `"@page toc:first"` 或 `"@page :left"`）。
    fn formatAtPageRule(buf: []u8): usize ! CssPageError {
        const prefix = "@page ";
        if (prefix.len > buf.len) {
            return err.BufferTooSmall;
        }
        @memcpy(buf[0..prefix.len], prefix);
        const written = _.toCss(buf[prefix.len..])?;
        return prefix.len + written;
    }
}

/// CSS `position` 定位属性枚举（`static` / `relative` / `absolute` / `fixed` / `sticky` / `-webkit-sticky`）
pub enum Position: u8 {
    static_pos = 0,
    relative = 1,
    absolute = 2,
    fixed = 3,
    sticky = 4,
    webkit_sticky = 5,

    static fn default(): Position {
        return .static_pos;
    }

    static fn parse(input: string): Position? {
        if (std.ascii.eqlIgnoreCase(input, "static")) { return .static_pos; }
        if (std.ascii.eqlIgnoreCase(input, "relative")) { return .relative; }
        if (std.ascii.eqlIgnoreCase(input, "absolute")) { return .absolute; }
        if (std.ascii.eqlIgnoreCase(input, "fixed")) { return .fixed; }
        if (std.ascii.eqlIgnoreCase(input, "sticky")) { return .sticky; }
        if (std.ascii.eqlIgnoreCase(input, "-webkit-sticky")) { return .webkit_sticky; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .static_pos => "static",
            .relative => "relative",
            .absolute => "absolute",
            .fixed => "fixed",
            .sticky => "sticky",
            .webkit_sticky => "-webkit-sticky",
        };
    }

    /// 是否脱离正常文档流（`absolute` / `fixed`）
    fn isOutOfFlow(): bool {
        return _ == .absolute || _ == .fixed;
    }

    /// 是否为已定位元素（非 `static`）
    fn isPositioned(): bool {
        return _ != .static_pos;
    }

    /// 是否为粘性定位（`sticky` 或 `-webkit-sticky`）
    fn isSticky(): bool {
        return _ == .sticky || _ == .webkit_sticky;
    }

    /// 是否携带 `-webkit-` 厂商前缀
    fn isWebKitPrefixed(): bool {
        return _ == .webkit_sticky;
    }

    /// 是否独立创建层叠上下文（`fixed` / `sticky` / `-webkit-sticky`）
    fn createsStackingContext(): bool {
        return _ == .fixed || _ == .sticky || _ == .webkit_sticky;
    }
}
