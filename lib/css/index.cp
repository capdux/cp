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
