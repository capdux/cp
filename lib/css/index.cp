// Source: Absorbed from /data/dev/bun/src/css/css_parser.cp | Synced: 2026-11-22T12:01:38Z
// lib/css/index.cp
// Unified Public Exports for cap:css
// Zero-Bun, Pure CP Industrial CSS Engine

export * from "./tokenizer.cp";
export * from "./color.cp";
export * from "./length.cp";
export * from "./selector.cp";
export * from "./variables.cp";
export * from "./prefixes.cp";
export * from "./stylesheet.cp";
export * from "./engine/index.cp";

// --- Merged CSS Submodules (Batch 1) ---

export * from "./animation.cp";
export * from "./atrules.cp";
export * from "./background.cp";
export * from "./borderimage.cp";
export * from "./cascade.cp";
export * from "./display.cp";

// Modules with symbols that overlap with ./engine/index.cp:
// Keep ./engine/index.cp exports intact (zero-breakage) and expose overlapping
// standalone submodule types via Css* aliases.

import _border from "./border.cp";
pub const LineStyle = _border.LineStyle;
pub const BorderLengthUnit = _border.BorderLengthUnit;
pub const BorderSideWidthKind = _border.BorderSideWidthKind;
pub const BorderSideWidth = _border.BorderSideWidth;
pub const BorderColorKind = _border.BorderColorKind;
pub const BorderColorValue = _border.BorderColorValue;
pub const Border = _border.Border;
pub const BorderWidth = _border.BorderWidth;
pub const CssBorderStyle = _border.BorderStyle;
pub const BorderColor = _border.BorderColor;
pub const BorderLogicalWidth = _border.BorderLogicalWidth;

import _borderradius from "./borderradius.cp";
pub const RadiusValue = _borderradius.RadiusValue;
pub const CornerRadius = _borderradius.CornerRadius;
pub const CssBorderRadius = _borderradius.BorderRadius;

import _boxshadow from "./boxshadow.cp";
pub const OutlineStyle = _boxshadow.OutlineStyle;
pub const ShadowLength = _boxshadow.ShadowLength;
pub const CssBoxShadow = _boxshadow.BoxShadow;

import _container from "./container.cp";
pub const CssContainerError = _container.CssContainerError;
pub const ContainerType = _container.ContainerType;
pub const ContainerName = _container.ContainerName;
pub const ContainerSizeFeatureId = _container.ContainerSizeFeatureId;
pub const Orientation = _container.Orientation;
pub const CompareOp = _container.CompareOp;
pub const ContainerSizeFeature = _container.ContainerSizeFeature;
pub const LogicOp = _container.LogicOp;
pub const CssContainerQuery = _container.ContainerQuery;

import _easing from "./easing.cp";
pub const CssEasingError = _easing.CssEasingError;
pub const CssStepPosition = _easing.StepPosition;
pub const CssEasingKind = _easing.EasingKind;
pub const CubicBezier = _easing.CubicBezier;
pub const EasingSteps = _easing.EasingSteps;
pub const CssEasingFunction = _easing.EasingFunction;

import _effects from "./effects.cp";
pub const CssFilterKind = _effects.FilterKind;
pub const FilterLengthUnit = _effects.FilterLengthUnit;
pub const FilterAngleUnit = _effects.FilterAngleUnit;
pub const DropShadow = _effects.DropShadow;
pub const Filter = _effects.Filter;
pub const CssFilterList = _effects.FilterList;


// --- Merged CSS Submodules (Batch 2) ---

export * from "./font.cp";
export * from "./liststyle.cp";
export * from "./masking.cp";
export * from "./position.cp";

import _flexgrid from "./flexgrid.cp";
pub const CssLayoutError = _flexgrid.CssLayoutError;
pub const FlexDirection = _flexgrid.FlexDirection;
pub const FlexWrap = _flexgrid.FlexWrap;
pub const FlexFlow = _flexgrid.FlexFlow;
pub const FlexBasisKind = _flexgrid.FlexBasisKind;
pub const FlexBasis = _flexgrid.FlexBasis;
pub const CssFlexShorthand = _flexgrid.FlexShorthand;
pub const JustifyContent = _flexgrid.JustifyContent;
pub const AlignItems = _flexgrid.AlignItems;
pub const AlignSelf = _flexgrid.AlignSelf;
pub const AlignContent = _flexgrid.AlignContent;
pub const GridAutoFlow = _flexgrid.GridAutoFlow;
pub const TrackBreadthKind = _flexgrid.TrackBreadthKind;
pub const TrackBreadth = _flexgrid.TrackBreadth;

import _geometry from "./geometry.cp";
pub const CssRatio = _geometry.Ratio;
pub const Rect = _geometry.Rect;
pub const OverflowKeyword = _geometry.OverflowKeyword;
pub const Overflow = _geometry.Overflow;
pub const TextOverflow = _geometry.TextOverflow;

import _gradient from "./gradient.cp";
pub const MAX_STOPS = _gradient.MAX_STOPS;
pub const DirectionSide = _gradient.DirectionSide;
pub const LineDirection = _gradient.LineDirection;
pub const RadialShape = _gradient.RadialShape;
pub const RadialExtent = _gradient.RadialExtent;
pub const CssColorStop = _gradient.ColorStop;
pub const LinearGradient = _gradient.LinearGradient;
pub const RadialGradient = _gradient.RadialGradient;
pub const ConicGradient = _gradient.ConicGradient;
pub const CssGradientKind = _gradient.GradientKind;
pub const CssGradient = _gradient.Gradient;

import _imageset from "./imageset.cp";
pub const CssResolutionUnit = _imageset.ResolutionUnit;
pub const CssResolution = _imageset.Resolution;
pub const ImageSetOption = _imageset.ImageSetOption;
pub const MimeSupportFn = _imageset.MimeSupportFn;
pub const ImageSet = _imageset.ImageSet;
pub const CssImageKind = _imageset.CssImageKind;
pub const CssImage = _imageset.CssImage;

import _layoutctrl from "./layoutctrl.cp";
pub const LayoutOverflowKeyword = _layoutctrl.OverflowKeyword;
pub const LayoutOverflow = _layoutctrl.Overflow;
pub const LayoutTextOverflow = _layoutctrl.TextOverflow;
pub const LayoutPosition = _layoutctrl.Position;
pub const LayoutContainerType = _layoutctrl.ContainerType;
pub const ContainFlags = _layoutctrl.ContainFlags;
pub const ZIndex = _layoutctrl.ZIndex;

import _media from "./media.cp";
pub const CssMediaError = _media.CssMediaError;
pub const MediaType = _media.MediaType;
pub const MediaQualifier = _media.MediaQualifier;
pub const ColorScheme = _media.ColorScheme;
pub const ReducedMotion = _media.ReducedMotion;
pub const HoverCapability = _media.HoverCapability;
pub const PointerCapability = _media.PointerCapability;
pub const MediaOrientation = _media.MediaOrientation;
pub const MediaFeatureId = _media.MediaFeatureId;
pub const MediaCompareOp = _media.CompareOp;
pub const MediaEnv = _media.MediaEnv;
pub const MediaFeature = _media.MediaFeature;
pub const MediaQuery = _media.MediaQuery;
pub const MediaQueryList = _media.MediaQueryList;

import _pagerule from "./pagerule.cp";
pub const CssPageError = _pagerule.CssPageError;
pub const PagePseudoClass = _pagerule.PagePseudoClass;
pub const PageMarginEdge = _pagerule.PageMarginEdge;
pub const PageMarginBox = _pagerule.PageMarginBox;
pub const PageSelector = _pagerule.PageSelector;
pub const PagePosition = _pagerule.Position;

import _primitives from "./primitives.cp";
pub const Percentage = _primitives.Percentage;
pub const NumOrPctKind = _primitives.NumOrPctKind;
pub const NumberOrPercentage = _primitives.NumberOrPercentage;
pub const CssAlphaValue = _primitives.AlphaValue;
pub const isAbsoluteCssUrl = _primitives.isAbsoluteCssUrl;
pub const isDataCssUrl = _primitives.isDataCssUrl;


// --- Merged CSS Submodules (Batch 3 — Final) ---

export * from "./selectorbuilder.cp";
export * from "./sizing.cp";
export * from "./svg.cp";
export * from "./syntax.cp";
export * from "./text.cp";

import _supports from "./supports.cp";
pub const SupportsKind = _supports.SupportsKind;
pub const CssSupportsNode = _supports.SupportsNode;
pub const SupportsTree = _supports.SupportsTree;

import _textlist from "./textlist.cp";
pub const TextListTransformCase = _textlist.TextTransformCase;
pub const TextTransformOther = _textlist.TextTransformOther;
pub const TextListWhiteSpace = _textlist.WhiteSpace;
pub const TextListWordBreak = _textlist.WordBreak;
pub const TextListOverflowWrap = _textlist.OverflowWrap;
pub const TextListTextAlign = _textlist.TextAlign;
pub const Direction = _textlist.Direction;
pub const TextListDecorationLine = _textlist.TextDecorationLine;
pub const TextListStyleType = _textlist.ListStyleType;
pub const TextListStylePosition = _textlist.ListStylePosition;
pub const TextListFilterKind = _textlist.FilterKind;
pub const BlendMode = _textlist.BlendMode;
pub const BoxShadowFlags = _textlist.BoxShadowFlags;

import _time from "./time.cp";
pub const CssTimeUnit = _time.TimeUnit;
pub const CssTime = _time.CssTime;

import _transform from "./transform.cp";
pub const Matrix2d = _transform.Matrix2d;
pub const Matrix3d = _transform.Matrix3d;
pub const TransformStyle = _transform.TransformStyle;
pub const BackfaceVisibility = _transform.BackfaceVisibility;
pub const TransformOrigin = _transform.TransformOrigin;
pub const CssTransformKind = _transform.TransformKind;
pub const Transform = _transform.Transform;
pub const CssTransformList = _transform.TransformList;

import _transition from "./transition.cp";
pub const CssTransitionError = _transition.CssTransitionError;
pub const TransitionBehavior = _transition.TransitionBehavior;
pub const TransitionPropertyKind = _transition.TransitionPropertyKind;
pub const TransitionProperty = _transition.TransitionProperty;
pub const TransitionTime = _transition.TransitionTime;
pub const TransitionEasingKind = _transition.TransitionEasingKind;
pub const TransitionEasing = _transition.TransitionEasing;
pub const CssTransition = _transition.Transition;
pub const TransitionList = _transition.TransitionList;

import _ui from "./ui.cp";
pub const UiColorScheme = _ui.ColorScheme;
pub const Resize = _ui.Resize;
pub const UserSelect = _ui.UserSelect;
pub const CaretShape = _ui.CaretShape;
pub const Appearance = _ui.Appearance;
pub const CursorKeyword = _ui.CursorKeyword;

import _units from "./units.cp";
pub const UnitAngleUnit = _units.AngleUnit;
pub const UnitAngle = _units.Angle;
pub const UnitResolutionUnit = _units.ResolutionUnit;
pub const UnitResolution = _units.Resolution;

export {
    CssDependencyOptions,
    CssDependencyLocation,
    CssSourceRange,
    CssDependencyKind,
    CssDependency,
    formatDependencyPlaceholder,
    CssDependencyIter,
} from "./dependencies.cp";

export {
    MAX_CSS_DIAGNOSTICS,
    CssDiagnosticSeverity,
    CssErrorLocation,
    CssParserErrorKind,
    CssPrinterErrorKind,
    CssDiagnosticDomain,
    CssDiagnostic,
    CssDiagnosticBag,
} from "./diagnostics.cp";

export {
    MAX_PROPERTY_SYNTAX_COMPONENTS,
    CssPropertyRuleError,
    isValidDashedIdent,
    PropertySyntax,
    CssPropertyRule,
} from "./property_rule.cp";

export {
    MAX_PALETTE_OVERRIDES,
    CssFontPaletteError,
    BasePaletteKind,
    BasePalette,
    PaletteOverrideColor,
    CssFontPaletteValuesRule,
} from "./font_palette_values.cp";

export {
    CssModuleConfig,
    PatternKind,
    PatternSegment,
    Pattern,
    ReferenceKind,
    CssModuleRef,
    isValidCssIdent,
    extractFileStemSanitized,
    computeModuleHash,
    scopeClassName,
    parseComposesValue,
} from "./css_modules.cp";

import _keyframes from "./keyframes.cp";
pub const VendorPrefix = _keyframes.VendorPrefix;
pub const KeyframesNameKind = _keyframes.KeyframesNameKind;
pub const KeyframesName = _keyframes.KeyframesName;
pub const SelectorKind = _keyframes.SelectorKind;
pub const CssKeyframeRuleSelector = _keyframes.KeyframeSelector;
pub const KeyframeStop = _keyframes.KeyframeStop;
pub const ParsedKeyframes = _keyframes.ParsedKeyframes;
pub const KeyframeSegment = _keyframes.KeyframeSegment;
pub const isReservedCssKeyword = _keyframes.isReservedCssKeyword;
pub const parseKeyframesRule = _keyframes.parseKeyframesRule;
pub const sortStopsByOffset = _keyframes.sortStopsByOffset;
pub const sampleKeyframeSegment = _keyframes.sampleKeyframeSegment;

export {
    CUSTOM_MEDIA_MAX_RULES,
    CustomMediaError,
    CustomMediaValueKind,
    CustomMediaRule,
    CustomMediaRegistry,
} from "./custom_media.cp";

export {
    CSS_NAMESPACE_MAX_RULES,
    CSS_HTML_NS,
    CSS_SVG_NS,
    CSS_MATHML_NS,
    CssNamespaceError,
    NamespaceRule,
    NamespaceRegistry,
} from "./namespace_rule.cp";

export {
    CSS_SCOPE_MAX_SELECTORS,
    CSS_SCOPE_MAX_RULES,
    CssScopeError,
    ScopeSelectorList,
    ScopeRule,
    ScopeRuleList,
} from "./scope_rule.cp";

export {
    CssIdentError,
    isCssWideKeyword,
    serializeCssIdentifier,
    CssIdent,
    CustomIdent,
    DashedIdent,
    CssIdentFromKind,
    DashedIdentReference,
} from "./ident.cp";

export {
    CssUrlError,
    CssUrlKind,
    CssUrlValue,
} from "./url_value.cp";

export {
    CssBoxRectError,
    CssBoxRect,
    CssSize2D,
} from "./box_rect.cp";

export {
    CSS_LAYER_MAX_SEGMENTS,
    CSS_LAYER_MAX_STATEMENT_NAMES,
    CSS_LAYER_MAX_RULES,
    CSS_LAYER_MAX_REGISTRY,
    CssLayerError,
    CssLayerName,
    CssLayerStatementRule,
    CssLayerBlockRule,
    CssLayerRuleKind,
    CssLayerRule,
    CssLayerRuleList,
    CssLayerOrderRegistry,
} from "./layer_rule.cp";

export {
    CSS_GRID_MAX_TRACKS,
    CSS_GRID_MAX_LINE_NAMES,
    CSS_GRID_MAX_AREA_CELLS,
    CssGridError,
    CssGridTrackBreadthKind,
    CssGridTrackBreadth,
    CssGridTrackSizeKind,
    CssGridTrackSize,
    CssGridRepeatCountKind,
    CssGridRepeatCount,
    CssGridLineNameSet,
    CssGridTrackRepeat,
    CssGridTrackItemKind,
    CssGridTrackItem,
    CssGridTrackSizing,
    CssGridTemplateAreas,
} from "./grid.cp";

export {
    CSS_IMPORT_MAX_RULES,
    CSS_IMPORT_NO_RECORD,
    CssImportError,
    CssImportLayerKind,
    CssImportLayer,
    CssImportSupportsKind,
    CssImportSupports,
    CssImportConditions,
    CssImportRule,
    CssImportRuleList,
} from "./import_rule.cp";

export {
    CSS_COUNTER_MAX_SYMBOLS,
    CSS_COUNTER_MAX_RANGES,
    CssCounterStyleError,
    CssCounterSystemKind,
    CssCounterSystem,
    CssAdditiveTuple,
    CssCounterRange,
    CssCounterSpeakAs,
    CssCounterStyleRule,
} from "./counter_style.cp";

export {
    CSS_STARTING_STYLE_MAX_ENTRIES,
    CSS_STARTING_STYLE_MAX_RULES,
    CssStartingStyleError,
    CssStartingStyleEntryKind,
    CssStartingStyleEntry,
    CssStartingStyleRule,
    CssStartingStyleRuleList,
} from "./starting_style.cp";

export {
    CssLogicalError,
    LogicalPropertyCategory,
    LogicalPropertyGroup,
    WritingDirection,
    LogicalPhysicalPair,
    classifyLogicalProperty,
    logicalGroupOf,
    mapLogicalToPhysical,
    formatLogicalFallbackRule,
} from "./logical.cp";

export {
    CSS_UNICODE_MAX_CODEPOINT,
    CSS_FONT_FACE_MAX_SOURCES,
    CSS_FONT_FACE_MAX_RANGES,
    CssFontFaceError,
    CssFontFormat,
    CssFontDisplay,
    CssFontSourceKind,
    CssUnicodeRange,
    CssFontFaceSource,
    CssFontFaceRule,
} from "./font_face.cp";

export {
    CSS_DECL_MAX_NORMAL,
    CSS_DECL_MAX_IMPORTANT,
    CssDeclBlockError,
    CssDeclItem,
    CssDeclParseOptions,
    CssDeclarationBlock,
} from "./declaration_block.cp";

export {
    CSS_CTX_MAX_SUPPORTS,
    CSS_CTX_MAX_UNUSED_SYMBOLS,
    CssHandlerContextError,
    CssDeclarationContext,
    CssSupportsFallbackBucket,
    CssPropertyHandlerContext,
} from "./handler_context.cp";

export {
    CSS_VIEWPORT_MAX_DECLS,
    CSS_VIEWPORT_MAX_RULES,
    CssViewportError,
    CssViewportVendorPrefix,
    CssViewportDescriptorKind,
    CssViewportUserZoom,
    CssViewportOrientation,
    CssViewportFit,
    CssViewportMinMaxPair,
    CssViewportDeclaration,
    CssViewportRule,
    CssViewportRuleList,
} from "./viewport_rule.cp";

export {
    CSS_UNKNOWN_AT_MAX_RULES,
    CSS_UNKNOWN_AT_MAX_PRELUDE_TOKENS,
    CssUnknownAtRuleError,
    isStandardKnownAtRule,
    CssUnknownAtRuleKind,
    CssUnknownPreludeTokens,
    CssUnknownAtRule,
    CssUnknownAtRuleList,
} from "./unknown_rule.cp";

export {
    CSS_DOCUMENT_MAX_CONDITIONS,
    CSS_DOCUMENT_MAX_RULES,
    CssDocumentRuleError,
    CssDocumentMatchKind,
    CssDocumentCondition,
    CssMozDocumentRule,
    CssMozDocumentRuleList,
} from "./document_rule.cp";
