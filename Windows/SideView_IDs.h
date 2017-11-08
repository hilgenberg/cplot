#pragma once

static const int SLIDER_MAX = 64000;
#define HISTO_MAX 1.0e4

enum
{
	ID_parameters = 1050,
	ID_definitions, ID_graphs,
	ID_settings,

	ID_qualityLabel, ID_quality,
	ID_discoLabel, ID_disco,
	ID_displayModeLabel, ID_displayMode, ID_vfMode,
	ID_histoModeLabel, ID_histoMode,
	ID_histoScaleLabel, ID_histoScale, ID_histoScaleSlider,

	ID_aaModeLabel, ID_aaMode,
	ID_transparencyModeLabel, ID_transparencyMode,
	ID_fogLabel, ID_fog,
	ID_lineWidthLabel, ID_lineWidth,
	ID_shinynessLabel, ID_shinyness,

	ID_bgLabel, ID_fillLabel, ID_axisLabel, ID_gridLabel,
	ID_bgColor, ID_fillColor, ID_axisColor, ID_gridColor,
	ID_bgAlpha, ID_fillAlpha, ID_axisAlpha, ID_gridAlpha,

	ID_textureLabel, ID_reflectionLabel,
	ID_texture, ID_reflection,
	ID_textureStrength, ID_reflectionStrength,
	ID_textureMode,

	ID_gridModeLabel, ID_meshModeLabel,
	ID_gridMode, ID_meshMode,
	ID_gridDensity, ID_meshDensity,

	ID_drawAxis, ID_axisModeLabel,
	ID_clip, ID_axisMode,
	ID_clipCustom, ID_clipLock, ID_clipReset,
	ID_clipDistance,

	//-----------------------------------------------------

	ID_axis,
	ID_centerLabel, ID_rangeLabel,
	ID_xLabel, ID_xCenter, ID_xRange, ID_xDelta,
	ID_yLabel, ID_yCenter, ID_yRange, ID_yDelta,
	ID_zLabel, ID_zCenter, ID_zRange, ID_zDelta,
	ID_xyzDelta,
	ID_uLabel, ID_uCenter, ID_uRange, ID_uDelta,
	ID_vLabel, ID_vCenter, ID_vRange, ID_vDelta,
	ID_uvDelta,
	ID_phiLabel, ID_psiLabel, ID_thetaLabel,
	ID_phi, ID_psi, ID_theta,
	ID_phiDelta, ID_psiDelta, ID_thetaDelta,
	ID_distLabel, ID_dist, ID_distDelta,
	ID_center, ID_top, ID_front
};
