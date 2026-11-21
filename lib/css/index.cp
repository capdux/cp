// Source: /data/dev/bun/src/css/css_parser.cp
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
