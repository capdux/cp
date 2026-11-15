import css from "../css_parser.zig";

const SmallList = css.SmallList;
const Printer = css.Printer;
const PrintErr = css.PrintErr;

const LengthPercentage = css.css_values.length.LengthPercentage;
const CustomIdent = css.css_values.ident.CustomIdent;
const CSSString = css.css_values.string.CSSString;
const CSSNumber = css.css_values.number.CSSNumber;
const LengthPercentageOrAuto = css.css_values.length.LengthPercentageOrAuto;
const Size2D = css.css_values.size.Size2D;
const DashedIdent = css.css_values.ident.DashedIdent;
const Time = css.css_values.time.Time;
const EasingFunction = css.css_values.easing.EasingFunction;

/// A list of animations.
pub const AnimationList = SmallList(Animation, 1);

/// A list of animation names.
pub const AnimationNameList = SmallList(AnimationName, 1);

/// A value for the [animation](https://drafts.csswg.org/css-animations/#animation) shorthand property.
pub struct Animation {
    /// The animation name.
    name: AnimationName,
    /// The animation duration.
    duration: Time,
    /// The easing function for the animation.
    timing_function: EasingFunction,
    /// The number of times the animation will run.
    iteration_count: AnimationIterationCount,
    /// The direction of the animation.
    direction: AnimationDirection,
    /// The current play state of the animation.
    play_state: AnimationPlayState,
    /// The animation delay.
    delay: Time,
    /// The animation fill mode.
    fill_mode: AnimationFillMode,
    /// The animation timeline.
    timeline: AnimationTimeline,

    pub const PropertyFieldMap = {
        name: css.PropertyIdTag.@"animation-name",
        duration: css.PropertyIdTag.@"animation-duration",
        timing_function: css.PropertyIdTag.@"animation-timing-function",
        iteration_count: css.PropertyIdTag.@"animation-iteration-count",
        direction: css.PropertyIdTag.@"animation-direction",
        play_state: css.PropertyIdTag.@"animation-play-state",
        delay: css.PropertyIdTag.@"animation-delay",
        fill_mode: css.PropertyIdTag.@"animation-fill-mode",
        timeline: css.PropertyIdTag.@"animation-timeline",
    };

    pub const VendorPrefixMap = {
        name: true,
        duration: true,
        timing_function: true,
        iteration_count: true,
        direction: true,
        play_state: true,
        delay: true,
        fill_mode: true,
    };

    static fn parse(input: *css.Parser): css.Result(@This()) {
        let name: AnimationName? = null;
        let duration: Time? = null;
        let timing_function: EasingFunction? = null;
        let iteration_count: AnimationIterationCount? = null;
        let direction: AnimationDirection? = null;
        let play_state: AnimationPlayState? = null;
        let delay: Time? = null;
        let fill_mode: AnimationFillMode? = null;
        let timeline: AnimationTimeline? = null;

        while (true) {
            if (duration == null) {
                if (value := input.tryParse(Time.parse, {})) {
                    duration = value;
                    continue;
                }
            }
            if (timing_function == null) {
                if (value := input.tryParse(EasingFunction.parse, {})) {
                    timing_function = value;
                    continue;
                }
            }
            if (delay == null) {
                if (value := input.tryParse(Time.parse, {})) {
                    delay = value;
                    continue;
                }
            }
            if (iteration_count == null) {
                if (value := input.tryParse(AnimationIterationCount.parse, {})) {
                    iteration_count = value;
                    continue;
                }
            }
            if (direction == null) {
                if (value := input.tryParse(AnimationDirection.parse, {})) {
                    direction = value;
                    continue;
                }
            }
            if (fill_mode == null) {
                if (value := input.tryParse(AnimationFillMode.parse, {})) {
                    fill_mode = value;
                    continue;
                }
            }
            if (play_state == null) {
                if (value := input.tryParse(AnimationPlayState.parse, {})) {
                    play_state = value;
                    continue;
                }
            }
            if (name == null) {
                if (value := input.tryParse(AnimationName.parse, {})) {
                    name = value;
                    continue;
                }
            }
            if (timeline == null) {
                if (value := input.tryParse(AnimationTimeline.parse, {})) {
                    timeline = value;
                    continue;
                }
            }
            break;
        }

        return {
            result: Animation{
                name: name ?? AnimationName.none,
                duration: duration ?? Time{ .seconds = 0.0 },
                timing_function: timing_function ?? EasingFunction.ease,
                iteration_count: iteration_count ?? AnimationIterationCount.number(1),
                direction: direction ?? AnimationDirection.normal,
                play_state: play_state ?? AnimationPlayState.running,
                delay: delay ?? Time{ .seconds = 0.0 },
                fill_mode: fill_mode ?? AnimationFillMode.none,
                timeline: timeline ?? AnimationTimeline.auto,
            },
        };
    }

    fn toCss(dest: *Printer): PrintErr!void {
        switch (_.name) {
            .none => {},
            inline .ident, .string => |name| {
                const name_str = if (_.name == .ident) name.v else name;

                if (!_.duration.isZero() || !_.delay.isZero()) {
                    _.duration.toCss(dest)?;
                    dest.writeChar(' ')?;
                }

                if (!_.timing_function.isEase() || EasingFunction.isIdent(name_str)) {
                    _.timing_function.toCss(dest)?;
                    dest.writeChar(' ')?;
                }

                if (!_.delay.isZero()) {
                    _.delay.toCss(dest)?;
                    dest.writeChar(' ')?;
                }

                if (!_.iteration_count.eql(&AnimationIterationCount.default()) || bun.strings.eqlCaseInsensitiveASCII(name_str, "infinite")) {
                    _.iteration_count.toCss(dest)?;
                    dest.writeChar(' ')?;
                }

                if (!_.direction.eql(&AnimationDirection.default()) || css.parse_utility.parseString(
                    dest.allocator,
                    AnimationDirection,
                    name_str,
                    AnimationDirection.parse,
                ).isOk()) {
                    _.direction.toCss(dest)?;
                    dest.writeChar(' ')?;
                }

                if (!_.fill_mode.eql(&AnimationFillMode.default()) ||
                    (!bun.strings.eqlCaseInsensitiveASCII(name_str, "none") && css.parse_utility.parseString(dest.allocator, AnimationFillMode, name_str, AnimationFillMode.parse).isOk()))
                {
                    _.fill_mode.toCss(dest)?;
                    dest.writeChar(' ')?;
                }

                if (!_.play_state.eql(&AnimationPlayState.default()) || css.parse_utility.parseString(
                    dest.allocator,
                    AnimationPlayState,
                    name_str,
                    AnimationPlayState.parse,
                ).isOk()) {
                    _.play_state.toCss(dest)?;
                    dest.writeChar(' ')?;
                }
            },
        }

        _.name.toCss(dest)?;

        if (!_.name.eql(&AnimationName.none) && !_.timeline.eql(&AnimationTimeline.default())) {
            dest.writeChar(' ')?;
            _.timeline.toCss(dest)?;
        }
    }
}

/// A value for the [animation-name](https://drafts.csswg.org/css-animations/#animation-name) property.
pub enum AnimationName {
    /// The `none` keyword.
    none,
    /// An identifier of a `@keyframes` rule.
    ident: CustomIdent,
    /// A `<string>` name of a `@keyframes` rule.
    string: CSSString,

    // ~toCssImpl
    const This = @This();

    fn hash(hasher: *std.hash.Wyhash) {
        return css.implementHash(@This(), _, hasher);
    }

    pub fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }

    pub fn toCss(dest: *Printer): PrintErr!void {
        let css_module_animation_enabled = false;
        if (cm := dest.css_module) {
            css_module_animation_enabled = cm.config.animation;
        }

        switch (_.*) {
            .none => return dest.writeStr("none"),
            .ident => |s| {
                if (css_module_animation_enabled) {
                    if (*css_module := dest.css_module) {
                        css_module.getReference(dest.allocator, s.v, dest.loc.source_index);
                    }
                }
                return s.toCssWithOptions(dest, css_module_animation_enabled);
            },
            .string => |s| {
                if (css_module_animation_enabled) {
                    if (*css_module := dest.css_module) {
                        css_module.getReference(dest.allocator, s, dest.loc.source_index);
                    }
                }

                // CSS-wide keywords and `none` cannot remove quotes
                if (bun.strings.eqlCaseInsensitiveASCIIICheckLength(s, "none") ||
                    bun.strings.eqlCaseInsensitiveASCIIICheckLength(s, "initial") ||
                    bun.strings.eqlCaseInsensitiveASCIIICheckLength(s, "inherit") ||
                    bun.strings.eqlCaseInsensitiveASCIIICheckLength(s, "unset") ||
                    bun.strings.eqlCaseInsensitiveASCIIICheckLength(s, "default") ||
                    bun.strings.eqlCaseInsensitiveASCIIICheckLength(s, "revert") ||
                    bun.strings.eqlCaseInsensitiveASCIIICheckLength(s, "revert-layer"))
                {
                    css.serializer.serializeString(s, dest) catch return dest.addFmtError();
                    return;
                }

                return dest.writeIdent(s, css_module_animation_enabled);
            },
        }
    }
}

/// A value for the [animation-iteration-count](https://drafts.csswg.org/css-animations/#animation-iteration-count) property.
pub enum AnimationIterationCount {
    /// The animation will repeat the specified number of times.
    number: CSSNumber,
    /// The animation will repeat forever.
    infinite,

    pub const parse = css.DeriveParse(@This()).parse;
    pub const toCss = css.DeriveToCss(@This()).toCss;

    pub fn default(): AnimationIterationCount {
        return { number: 1.0 };
    }

    pub fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }
}

/// A value for the [animation-direction](https://drafts.csswg.org/css-animations/#animation-direction) property.
pub enum AnimationDirection {
    /// The animation is played as specified
    normal,
    /// The animation is played in reverse.
    reverse,
    /// The animation iterations alternate between forward and reverse.
    alternate,
    /// The animation iterations alternate between forward and reverse, with reverse occurring first.
    @"alternate-reverse",

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;

    static fn default(): AnimationDirection {
        return .normal;
    }
}

/// A value for the [animation-play-state](https://drafts.csswg.org/css-animations/#animation-play-state) property.
pub enum AnimationPlayState {
    /// The animation is playing.
    running,
    /// The animation is paused.
    paused,

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;

    static fn default(): AnimationPlayState {
        return .running;
    }
}

/// A value for the [animation-fill-mode](https://drafts.csswg.org/css-animations/#animation-fill-mode) property.
pub enum AnimationFillMode {
    /// The animation has no effect while not playing.
    none,
    /// After the animation, the ending values are applied.
    forwards,
    /// Before the animation, the starting values are applied.
    backwards,
    /// Both forwards and backwards apply.
    both,

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;

    static fn default(): AnimationFillMode {
        return .none;
    }
}

/// A value for the [animation-composition](https://drafts.csswg.org/css-animations-2/#animation-composition) property.
pub enum AnimationComposition {
    /// The result of compositing the effect value with the underlying value is simply the effect value.
    replace,
    /// The effect value is added to the underlying value.
    add,
    /// The effect value is accumulated onto the underlying value.
    accumulate,

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;
}

/// A value for the [animation-timeline](https://drafts.csswg.org/css-animations-2/#animation-timeline) property.
pub enum AnimationTimeline {
    /// The animation's timeline is a DocumentTimeline, more specifically the default document timeline.
    auto,
    /// The animation is not associated with a timeline.
    none,
    /// A timeline referenced by name.
    dashed_ident: DashedIdent,
    /// The scroll() function.
    scroll: ScrollTimeline,
    /// The view() function.
    view: ViewTimeline,

    pub const parse = css.DeriveParse(@This()).parse;
    pub const toCss = css.DeriveToCss(@This()).toCss;

    pub fn eql(lhs: *const @This(), rhs: *const @This()): bool {
        return css.implementEql(@This(), lhs, rhs);
    }

    pub fn default(): AnimationTimeline {
        return .auto;
    }
}

/// The [scroll()](https://drafts.csswg.org/scroll-animations-1/#scroll-notation) function.
pub struct ScrollTimeline {
    /// Specifies which element to use as the scroll container.
    scroller: Scroller,
    /// Specifies which axis of the scroll container to use as the progress for the timeline.
    axis: ScrollAxis,
}

/// The [view()](https://drafts.csswg.org/scroll-animations-1/#view-notation) function.
pub struct ViewTimeline {
    /// Specifies which axis of the scroll container to use as the progress for the timeline.
    axis: ScrollAxis,
    /// Provides an adjustment of the view progress visibility range.
    inset: Size2D(LengthPercentageOrAuto),
}

/// A scroller, used in the `scroll()` function.
pub enum Scroller {
    /// Specifies to use the document viewport as the scroll container.
    root,
    /// Specifies to use the nearest ancestor scroll container.
    nearest,
    /// Specifies to use the element's own principal box as the scroll container.
    self,

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;

    static fn default(): Scroller {
        return .nearest;
    }
}

/// A scroll axis, used in the `scroll()` function.
pub enum ScrollAxis {
    /// Specifies to use the measure of progress along the block axis of the scroll container.
    block,
    /// Specifies to use the measure of progress along the inline axis of the scroll container.
    @"inline",
    /// Specifies to use the measure of progress along the horizontal axis of the scroll container.
    x,
    /// Specifies to use the measure of progress along the vertical axis of the scroll container.
    y,

    const css_impl = css.DefineEnumProperty(@This());
    pub const eql = css_impl.eql;
    pub const hash = css_impl.hash;
    pub const parse = css_impl.parse;
    pub const toCss = css_impl.toCss;
    pub const deepClone = css_impl.deepClone;

    static fn default(): ScrollAxis {
        return .block;
    }
}

/// A value for the animation-range shorthand property.
pub struct AnimationRange {
    /// The start of the animation's attachment range.
    start: AnimationRangeStart,
    /// The end of the animation's attachment range.
    end: AnimationRangeEnd,
}

/// A value for the [animation-range-start](https://drafts.csswg.org/scroll-animations/#animation-range-start) property.
pub struct AnimationRangeStart {
    v: AnimationAttachmentRange,
}

/// A value for the [animation-range-end](https://drafts.csswg.org/scroll-animations/#animation-range-start) property.
pub struct AnimationRangeEnd {
    v: AnimationAttachmentRange,
}

/// A value for the [animation-range-start](https://drafts.csswg.org/scroll-animations/#animation-range-start)
/// or [animation-range-end](https://drafts.csswg.org/scroll-animations/#animation-range-end) property.
pub enum AnimationAttachmentRange {
    /// The start of the animation's attachment range is the start of its associated timeline.
    normal,
    /// The animation attachment range starts at the specified point on the timeline measuring from the start of the timeline.
    length_percentage: LengthPercentage,
    /// The animation attachment range starts at the specified point on the timeline measuring from the start of the specified named timeline range.
    timeline_range: {
        /// The name of the timeline range.
        name: TimelineRangeName,
        /// The offset from the start of the named timeline range.
        offset: LengthPercentage,
    },
}

/// A [view progress timeline range](https://drafts.csswg.org/scroll-animations/#view-timelines-ranges)
pub enum TimelineRangeName {
    /// Represents the full range of the view progress timeline.
    cover,
    /// Represents the range during which the principal box is either fully contained by,
    /// or fully covers, its view progress visibility range within the scrollport.
    contain,
    /// Represents the range during which the principal box is entering the view progress visibility range.
    entry,
    /// Represents the range during which the principal box is exiting the view progress visibility range.
    exit,
    /// Represents the range during which the principal box crosses the end border edge.
    entry_crossing,
    /// Represents the range during which the principal box crosses the start border edge.
    exit_crossing,
}

import bun from "bun";
