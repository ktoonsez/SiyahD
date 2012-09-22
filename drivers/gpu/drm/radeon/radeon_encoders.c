/*
 * Copyright 2007-8 Advanced Micro Devices, Inc.
 * Copyright 2008 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Dave Airlie
 *          Alex Deucher
 */
#include "drmP.h"
#include "drm_crtc_helper.h"
#include "radeon_drm.h"
#include "radeon.h"
#include "atom.h"

static uint32_t radeon_encoder_clones(struct drm_encoder *encoder)
{
	struct drm_device *dev = encoder->dev;
	struct radeon_device *rdev = dev->dev_private;
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	struct drm_encoder *clone_encoder;
	uint32_t index_mask = 0;
	int count;

	/* DIG routing gets problematic */
	if (rdev->family >= CHIP_R600)
		return index_mask;
	/* LVDS/TV are too wacky */
	if (radeon_encoder->devices & ATOM_DEVICE_LCD_SUPPORT)
		return index_mask;
	/* DVO requires 2x ppll clocks depending on tmds chip */
	if (radeon_encoder->devices & ATOM_DEVICE_DFP2_SUPPORT)
		return index_mask;

	count = -1;
	list_for_each_entry(clone_encoder, &dev->mode_config.encoder_list, head) {
		struct radeon_encoder *radeon_clone = to_radeon_encoder(clone_encoder);
		count++;

		if (clone_encoder == encoder)
			continue;
		if (radeon_clone->devices & (ATOM_DEVICE_LCD_SUPPORT))
			continue;
		if (radeon_clone->devices & ATOM_DEVICE_DFP2_SUPPORT)
			continue;
		else
			index_mask |= (1 << count);
	}
	return index_mask;
}

void radeon_setup_encoder_clones(struct drm_device *dev)
{
	struct drm_encoder *encoder;

	list_for_each_entry(encoder, &dev->mode_config.encoder_list, head) {
		encoder->possible_clones = radeon_encoder_clones(encoder);
	}
}

uint32_t
radeon_get_encoder_enum(struct drm_device *dev, uint32_t supported_device, uint8_t dac)
{
	struct radeon_device *rdev = dev->dev_private;
	uint32_t ret = 0;

	switch (supported_device) {
	case ATOM_DEVICE_CRT1_SUPPORT:
	case ATOM_DEVICE_TV1_SUPPORT:
	case ATOM_DEVICE_TV2_SUPPORT:
	case ATOM_DEVICE_CRT2_SUPPORT:
	case ATOM_DEVICE_CV_SUPPORT:
		switch (dac) {
		case 1: /* dac a */
			if ((rdev->family == CHIP_RS300) ||
			    (rdev->family == CHIP_RS400) ||
			    (rdev->family == CHIP_RS480))
				ret = ENCODER_INTERNAL_DAC2_ENUM_ID1;
			else if (ASIC_IS_AVIVO(rdev))
				ret = ENCODER_INTERNAL_KLDSCP_DAC1_ENUM_ID1;
			else
				ret = ENCODER_INTERNAL_DAC1_ENUM_ID1;
			break;
		case 2: /* dac b */
			if (ASIC_IS_AVIVO(rdev))
				ret = ENCODER_INTERNAL_KLDSCP_DAC2_ENUM_ID1;
			else {
				/*if (rdev->family == CHIP_R200)
				  ret = ENCODER_INTERNAL_DVO1_ENUM_ID1;
				  else*/
				ret = ENCODER_INTERNAL_DAC2_ENUM_ID1;
			}
			break;
		case 3: /* external dac */
			if (ASIC_IS_AVIVO(rdev))
				ret = ENCODER_INTERNAL_KLDSCP_DVO1_ENUM_ID1;
			else
				ret = ENCODER_INTERNAL_DVO1_ENUM_ID1;
			break;
		}
		break;
	case ATOM_DEVICE_LCD1_SUPPORT:
		if (ASIC_IS_AVIVO(rdev))
			ret = ENCODER_INTERNAL_LVTM1_ENUM_ID1;
		else
			ret = ENCODER_INTERNAL_LVDS_ENUM_ID1;
		break;
	case ATOM_DEVICE_DFP1_SUPPORT:
		if ((rdev->family == CHIP_RS300) ||
		    (rdev->family == CHIP_RS400) ||
		    (rdev->family == CHIP_RS480))
			ret = ENCODER_INTERNAL_DVO1_ENUM_ID1;
		else if (ASIC_IS_AVIVO(rdev))
			ret = ENCODER_INTERNAL_KLDSCP_TMDS1_ENUM_ID1;
		else
			ret = ENCODER_INTERNAL_TMDS1_ENUM_ID1;
		break;
	case ATOM_DEVICE_LCD2_SUPPORT:
	case ATOM_DEVICE_DFP2_SUPPORT:
		if ((rdev->family == CHIP_RS600) ||
		    (rdev->family == CHIP_RS690) ||
		    (rdev->family == CHIP_RS740))
			ret = ENCODER_INTERNAL_DDI_ENUM_ID1;
		else if (ASIC_IS_AVIVO(rdev))
			ret = ENCODER_INTERNAL_KLDSCP_DVO1_ENUM_ID1;
		else
			ret = ENCODER_INTERNAL_DVO1_ENUM_ID1;
		break;
	case ATOM_DEVICE_DFP3_SUPPORT:
		ret = ENCODER_INTERNAL_LVTM1_ENUM_ID1;
		break;
	}

	return ret;
}

void
radeon_link_encoder_connector(struct drm_device *dev)
{
	struct drm_connector *connector;
	struct radeon_connector *radeon_connector;
	struct drm_encoder *encoder;
	struct radeon_encoder *radeon_encoder;

	/* walk the list and link encoders to connectors */
	list_for_each_entry(connector, &dev->mode_config.connector_list, head) {
		radeon_connector = to_radeon_connector(connector);
		list_for_each_entry(encoder, &dev->mode_config.encoder_list, head) {
			radeon_encoder = to_radeon_encoder(encoder);
			if (radeon_encoder->devices & radeon_connector->devices)
				drm_mode_connector_attach_encoder(connector, encoder);
		}
	}
}

void radeon_encoder_set_active_device(struct drm_encoder *encoder)
{
	struct drm_device *dev = encoder->dev;
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	struct drm_connector *connector;

	list_for_each_entry(connector, &dev->mode_config.connector_list, head) {
		if (connector->encoder == encoder) {
			struct radeon_connector *radeon_connector = to_radeon_connector(connector);
			radeon_encoder->active_device = radeon_encoder->devices & radeon_connector->devices;
			DRM_DEBUG_KMS("setting active device to %08x from %08x %08x for encoder %d\n",
				  radeon_encoder->active_device, radeon_encoder->devices,
				  radeon_connector->devices, encoder->encoder_type);
		}
	}
}

struct drm_connector *
radeon_get_connector_for_encoder(struct drm_encoder *encoder)
{
	struct drm_device *dev = encoder->dev;
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	struct drm_connector *connector;
	struct radeon_connector *radeon_connector;

	list_for_each_entry(connector, &dev->mode_config.connector_list, head) {
		radeon_connector = to_radeon_connector(connector);
		if (radeon_encoder->active_device & radeon_connector->devices)
			return connector;
	}
	return NULL;
}

struct drm_connector *
radeon_get_connector_for_encoder_init(struct drm_encoder *encoder)
{
	struct drm_device *dev = encoder->dev;
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	struct drm_connector *connector;
	struct radeon_connector *radeon_connector;

	list_for_each_entry(connector, &dev->mode_config.connector_list, head) {
		radeon_connector = to_radeon_connector(connector);
		if (radeon_encoder->devices & radeon_connector->devices)
			return connector;
	}
	return NULL;
}

struct drm_encoder *radeon_get_external_encoder(struct drm_encoder *encoder)
{
	struct drm_device *dev = encoder->dev;
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	struct drm_encoder *other_encoder;
	struct radeon_encoder *other_radeon_encoder;

	if (radeon_encoder->is_ext_encoder)
		return NULL;

	list_for_each_entry(other_encoder, &dev->mode_config.encoder_list, head) {
		if (other_encoder == encoder)
			continue;
		other_radeon_encoder = to_radeon_encoder(other_encoder);
		if (other_radeon_encoder->is_ext_encoder &&
		    (radeon_encoder->devices & other_radeon_encoder->devices))
			return other_encoder;
	}
	return NULL;
}

u16 radeon_encoder_get_dp_bridge_encoder_id(struct drm_encoder *encoder)
{
	struct drm_encoder *other_encoder = radeon_get_external_encoder(encoder);

	if (other_encoder) {
		struct radeon_encoder *radeon_encoder = to_radeon_encoder(other_encoder);

		switch (radeon_encoder->encoder_id) {
		case ENCODER_OBJECT_ID_TRAVIS:
		case ENCODER_OBJECT_ID_NUTMEG:
			return radeon_encoder->encoder_id;
		default:
			return ENCODER_OBJECT_ID_NONE;
		}
	}
	return ENCODER_OBJECT_ID_NONE;
}

void radeon_panel_mode_fixup(struct drm_encoder *encoder,
			     struct drm_display_mode *adjusted_mode)
{
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	struct drm_device *dev = encoder->dev;
	struct radeon_device *rdev = dev->dev_private;
	struct drm_display_mode *native_mode = &radeon_encoder->native_mode;
	unsigned hblank = native_mode->htotal - native_mode->hdisplay;
	unsigned vblank = native_mode->vtotal - native_mode->vdisplay;
	unsigned hover = native_mode->hsync_start - native_mode->hdisplay;
	unsigned vover = native_mode->vsync_start - native_mode->vdisplay;
	unsigned hsync_width = native_mode->hsync_end - native_mode->hsync_start;
	unsigned vsync_width = native_mode->vsync_end - native_mode->vsync_start;

	adjusted_mode->clock = native_mode->clock;
	adjusted_mode->flags = native_mode->flags;

	if (ASIC_IS_AVIVO(rdev)) {
		adjusted_mode->hdisplay = native_mode->hdisplay;
		adjusted_mode->vdisplay = native_mode->vdisplay;
	}

	adjusted_mode->htotal = native_mode->hdisplay + hblank;
	adjusted_mode->hsync_start = native_mode->hdisplay + hover;
	adjusted_mode->hsync_end = adjusted_mode->hsync_start + hsync_width;

	adjusted_mode->vtotal = native_mode->vdisplay + vblank;
	adjusted_mode->vsync_start = native_mode->vdisplay + vover;
	adjusted_mode->vsync_end = adjusted_mode->vsync_start + vsync_width;

	drm_mode_set_crtcinfo(adjusted_mode, CRTC_INTERLACE_HALVE_V);

	if (ASIC_IS_AVIVO(rdev)) {
		adjusted_mode->crtc_hdisplay = native_mode->hdisplay;
		adjusted_mode->crtc_vdisplay = native_mode->vdisplay;
	}

	adjusted_mode->crtc_htotal = adjusted_mode->crtc_hdisplay + hblank;
	adjusted_mode->crtc_hsync_start = adjusted_mode->crtc_hdisplay + hover;
	adjusted_mode->crtc_hsync_end = adjusted_mode->crtc_hsync_start + hsync_width;

	adjusted_mode->crtc_vtotal = adjusted_mode->crtc_vdisplay + vblank;
	adjusted_mode->crtc_vsync_start = adjusted_mode->crtc_vdisplay + vover;
	adjusted_mode->crtc_vsync_end = adjusted_mode->crtc_vsync_start + vsync_width;

}

bool radeon_dig_monitor_is_duallink(struct drm_encoder *encoder,
				    u32 pixel_clock)
{
	struct drm_connector *connector;
	struct radeon_connector *radeon_connector;
	struct radeon_connector_atom_dig *dig_connector;

	connector = radeon_get_connector_for_encoder(encoder);
	/* if we don't have an active device yet, just use one of
	 * the connectors tied to the encoder.
	 */
	if (!connector)
		connector = radeon_get_connector_for_encoder_init(encoder);
	radeon_connector = to_radeon_connector(connector);

	switch (connector->connector_type) {
	case DRM_MODE_CONNECTOR_DVII:
	case DRM_MODE_CONNECTOR_HDMIB:
		if (radeon_connector->use_digital) {
			/* HDMI 1.3 supports up to 340 Mhz over single link */
			if (0 && drm_detect_hdmi_monitor(radeon_connector->edid)) {
				if (pixel_clock > 340000)
					return true;
				else
					return false;
			} else {
				if (pixel_clock > 165000)
					return true;
				else
					return false;
			}
		} else
			return false;
	case DRM_MODE_CONNECTOR_DVID:
	case DRM_MODE_CONNECTOR_HDMIA:
	case DRM_MODE_CONNECTOR_DisplayPort:
		dig_connector = radeon_connector->con_priv;
		if ((dig_connector->dp_sink_type == CONNECTOR_OBJECT_ID_DISPLAYPORT) ||
		    (dig_connector->dp_sink_type == CONNECTOR_OBJECT_ID_eDP))
			return false;
		else {
			/* HDMI 1.3 supports up to 340 Mhz over single link */
			if (0 && drm_detect_hdmi_monitor(radeon_connector->edid)) {
				if (pixel_clock > 340000)
					return true;
				else
					return false;
			} else {
				if (pixel_clock > 165000)
					return true;
				else
					return false;
			}
		}
	default:
		return false;
	}
}

<<<<<<< HEAD
=======
union dig_transmitter_control {
	DIG_TRANSMITTER_CONTROL_PS_ALLOCATION v1;
	DIG_TRANSMITTER_CONTROL_PARAMETERS_V2 v2;
	DIG_TRANSMITTER_CONTROL_PARAMETERS_V3 v3;
	DIG_TRANSMITTER_CONTROL_PARAMETERS_V4 v4;
};

void
atombios_dig_transmitter_setup(struct drm_encoder *encoder, int action, uint8_t lane_num, uint8_t lane_set)
{
	struct drm_device *dev = encoder->dev;
	struct radeon_device *rdev = dev->dev_private;
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	struct radeon_encoder_atom_dig *dig = radeon_encoder->enc_priv;
	struct drm_connector *connector;
	union dig_transmitter_control args;
	int index = 0;
	uint8_t frev, crev;
	bool is_dp = false;
	int pll_id = 0;
	int dp_clock = 0;
	int dp_lane_count = 0;
	int connector_object_id = 0;
	int igp_lane_info = 0;
	int dig_encoder = dig->dig_encoder;

	if (action == ATOM_TRANSMITTER_ACTION_INIT) {
		connector = radeon_get_connector_for_encoder_init(encoder);
		/* just needed to avoid bailing in the encoder check.  the encoder
		 * isn't used for init
		 */
		dig_encoder = 0;
	} else
		connector = radeon_get_connector_for_encoder(encoder);

	if (connector) {
		struct radeon_connector *radeon_connector = to_radeon_connector(connector);
		struct radeon_connector_atom_dig *dig_connector =
			radeon_connector->con_priv;

		dp_clock = dig_connector->dp_clock;
		dp_lane_count = dig_connector->dp_lane_count;
		connector_object_id =
			(radeon_connector->connector_object_id & OBJECT_ID_MASK) >> OBJECT_ID_SHIFT;
		igp_lane_info = dig_connector->igp_lane_info;
	}

	/* no dig encoder assigned */
	if (dig_encoder == -1)
		return;

	if (atombios_get_encoder_mode(encoder) == ATOM_ENCODER_MODE_DP)
		is_dp = true;

	memset(&args, 0, sizeof(args));

	switch (radeon_encoder->encoder_id) {
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DVO1:
		index = GetIndexIntoMasterTable(COMMAND, DVOOutputControl);
		break;
	case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
	case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
	case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
		index = GetIndexIntoMasterTable(COMMAND, UNIPHYTransmitterControl);
		break;
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_LVTMA:
		index = GetIndexIntoMasterTable(COMMAND, LVTMATransmitterControl);
		break;
	}

	if (!atom_parse_cmd_header(rdev->mode_info.atom_context, index, &frev, &crev))
		return;

	args.v1.ucAction = action;
	if (action == ATOM_TRANSMITTER_ACTION_INIT) {
		args.v1.usInitInfo = cpu_to_le16(connector_object_id);
	} else if (action == ATOM_TRANSMITTER_ACTION_SETUP_VSEMPH) {
		args.v1.asMode.ucLaneSel = lane_num;
		args.v1.asMode.ucLaneSet = lane_set;
	} else {
		if (is_dp)
			args.v1.usPixelClock =
				cpu_to_le16(dp_clock / 10);
		else if (radeon_encoder->pixel_clock > 165000)
			args.v1.usPixelClock = cpu_to_le16((radeon_encoder->pixel_clock / 2) / 10);
		else
			args.v1.usPixelClock = cpu_to_le16(radeon_encoder->pixel_clock / 10);
	}
	if (ASIC_IS_DCE4(rdev)) {
		if (is_dp)
			args.v3.ucLaneNum = dp_lane_count;
		else if (radeon_encoder->pixel_clock > 165000)
			args.v3.ucLaneNum = 8;
		else
			args.v3.ucLaneNum = 4;

		if (dig->linkb)
			args.v3.acConfig.ucLinkSel = 1;
		if (dig_encoder & 1)
			args.v3.acConfig.ucEncoderSel = 1;

		/* Select the PLL for the PHY
		 * DP PHY should be clocked from external src if there is
		 * one.
		 */
		if (encoder->crtc) {
			struct radeon_crtc *radeon_crtc = to_radeon_crtc(encoder->crtc);
			pll_id = radeon_crtc->pll_id;
		}

		if (ASIC_IS_DCE5(rdev)) {
			/* On DCE5 DCPLL usually generates the DP ref clock */
			if (is_dp) {
				if (rdev->clock.dp_extclk)
					args.v4.acConfig.ucRefClkSource = ENCODER_REFCLK_SRC_EXTCLK;
				else
					args.v4.acConfig.ucRefClkSource = ENCODER_REFCLK_SRC_DCPLL;
			} else
				args.v4.acConfig.ucRefClkSource = pll_id;
		} else {
			/* On DCE4, if there is an external clock, it generates the DP ref clock */
			if (is_dp && rdev->clock.dp_extclk)
				args.v3.acConfig.ucRefClkSource = 2; /* external src */
			else
				args.v3.acConfig.ucRefClkSource = pll_id;
		}

		switch (radeon_encoder->encoder_id) {
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
			args.v3.acConfig.ucTransmitterSel = 0;
			break;
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
			args.v3.acConfig.ucTransmitterSel = 1;
			break;
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
			args.v3.acConfig.ucTransmitterSel = 2;
			break;
		}

		if (is_dp)
			args.v3.acConfig.fCoherentMode = 1; /* DP requires coherent */
		else if (radeon_encoder->devices & (ATOM_DEVICE_DFP_SUPPORT)) {
			if (dig->coherent_mode)
				args.v3.acConfig.fCoherentMode = 1;
			if (radeon_encoder->pixel_clock > 165000)
				args.v3.acConfig.fDualLinkConnector = 1;
		}
	} else if (ASIC_IS_DCE32(rdev)) {
		args.v2.acConfig.ucEncoderSel = dig_encoder;
		if (dig->linkb)
			args.v2.acConfig.ucLinkSel = 1;

		switch (radeon_encoder->encoder_id) {
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
			args.v2.acConfig.ucTransmitterSel = 0;
			break;
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
			args.v2.acConfig.ucTransmitterSel = 1;
			break;
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
			args.v2.acConfig.ucTransmitterSel = 2;
			break;
		}

		if (is_dp) {
			args.v2.acConfig.fCoherentMode = 1;
			args.v2.acConfig.fDPConnector = 1;
		} else if (radeon_encoder->devices & (ATOM_DEVICE_DFP_SUPPORT)) {
			if (dig->coherent_mode)
				args.v2.acConfig.fCoherentMode = 1;
			if (radeon_encoder->pixel_clock > 165000)
				args.v2.acConfig.fDualLinkConnector = 1;
		}
	} else {
		args.v1.ucConfig = ATOM_TRANSMITTER_CONFIG_CLKSRC_PPLL;

		if (dig_encoder)
			args.v1.ucConfig |= ATOM_TRANSMITTER_CONFIG_DIG2_ENCODER;
		else
			args.v1.ucConfig |= ATOM_TRANSMITTER_CONFIG_DIG1_ENCODER;

		if ((rdev->flags & RADEON_IS_IGP) &&
		    (radeon_encoder->encoder_id == ENCODER_OBJECT_ID_INTERNAL_UNIPHY)) {
			if (is_dp || (radeon_encoder->pixel_clock <= 165000)) {
				if (igp_lane_info & 0x1)
					args.v1.ucConfig |= ATOM_TRANSMITTER_CONFIG_LANE_0_3;
				else if (igp_lane_info & 0x2)
					args.v1.ucConfig |= ATOM_TRANSMITTER_CONFIG_LANE_4_7;
				else if (igp_lane_info & 0x4)
					args.v1.ucConfig |= ATOM_TRANSMITTER_CONFIG_LANE_8_11;
				else if (igp_lane_info & 0x8)
					args.v1.ucConfig |= ATOM_TRANSMITTER_CONFIG_LANE_12_15;
			} else {
				if (igp_lane_info & 0x3)
					args.v1.ucConfig |= ATOM_TRANSMITTER_CONFIG_LANE_0_7;
				else if (igp_lane_info & 0xc)
					args.v1.ucConfig |= ATOM_TRANSMITTER_CONFIG_LANE_8_15;
			}
		}

		if (dig->linkb)
			args.v1.ucConfig |= ATOM_TRANSMITTER_CONFIG_LINKB;
		else
			args.v1.ucConfig |= ATOM_TRANSMITTER_CONFIG_LINKA;

		if (is_dp)
			args.v1.ucConfig |= ATOM_TRANSMITTER_CONFIG_COHERENT;
		else if (radeon_encoder->devices & (ATOM_DEVICE_DFP_SUPPORT)) {
			if (dig->coherent_mode)
				args.v1.ucConfig |= ATOM_TRANSMITTER_CONFIG_COHERENT;
			if (radeon_encoder->pixel_clock > 165000)
				args.v1.ucConfig |= ATOM_TRANSMITTER_CONFIG_8LANE_LINK;
		}
	}

	atom_execute_table(rdev->mode_info.atom_context, index, (uint32_t *)&args);
}

bool
atombios_set_edp_panel_power(struct drm_connector *connector, int action)
{
	struct radeon_connector *radeon_connector = to_radeon_connector(connector);
	struct drm_device *dev = radeon_connector->base.dev;
	struct radeon_device *rdev = dev->dev_private;
	union dig_transmitter_control args;
	int index = GetIndexIntoMasterTable(COMMAND, UNIPHYTransmitterControl);
	uint8_t frev, crev;

	if (connector->connector_type != DRM_MODE_CONNECTOR_eDP)
		goto done;

	if (!ASIC_IS_DCE4(rdev))
		goto done;

	if ((action != ATOM_TRANSMITTER_ACTION_POWER_ON) &&
	    (action != ATOM_TRANSMITTER_ACTION_POWER_OFF))
		goto done;

	if (!atom_parse_cmd_header(rdev->mode_info.atom_context, index, &frev, &crev))
		goto done;

	memset(&args, 0, sizeof(args));

	args.v1.ucAction = action;

	atom_execute_table(rdev->mode_info.atom_context, index, (uint32_t *)&args);

	/* wait for the panel to power up */
	if (action == ATOM_TRANSMITTER_ACTION_POWER_ON) {
		int i;

		for (i = 0; i < 300; i++) {
			if (radeon_hpd_sense(rdev, radeon_connector->hpd.hpd))
				return true;
			mdelay(1);
		}
		return false;
	}
done:
	return true;
}

union external_encoder_control {
	EXTERNAL_ENCODER_CONTROL_PS_ALLOCATION v1;
	EXTERNAL_ENCODER_CONTROL_PS_ALLOCATION_V3 v3;
};

static void
atombios_external_encoder_setup(struct drm_encoder *encoder,
				struct drm_encoder *ext_encoder,
				int action)
{
	struct drm_device *dev = encoder->dev;
	struct radeon_device *rdev = dev->dev_private;
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	struct radeon_encoder *ext_radeon_encoder = to_radeon_encoder(ext_encoder);
	union external_encoder_control args;
	struct drm_connector *connector;
	int index = GetIndexIntoMasterTable(COMMAND, ExternalEncoderControl);
	u8 frev, crev;
	int dp_clock = 0;
	int dp_lane_count = 0;
	int connector_object_id = 0;
	u32 ext_enum = (ext_radeon_encoder->encoder_enum & ENUM_ID_MASK) >> ENUM_ID_SHIFT;
	int bpc = 8;

	if (action == EXTERNAL_ENCODER_ACTION_V3_ENCODER_INIT)
		connector = radeon_get_connector_for_encoder_init(encoder);
	else
		connector = radeon_get_connector_for_encoder(encoder);

	if (connector) {
		struct radeon_connector *radeon_connector = to_radeon_connector(connector);
		struct radeon_connector_atom_dig *dig_connector =
			radeon_connector->con_priv;

		dp_clock = dig_connector->dp_clock;
		dp_lane_count = dig_connector->dp_lane_count;
		connector_object_id =
			(radeon_connector->connector_object_id & OBJECT_ID_MASK) >> OBJECT_ID_SHIFT;
		bpc = connector->display_info.bpc;
	}

	memset(&args, 0, sizeof(args));

	if (!atom_parse_cmd_header(rdev->mode_info.atom_context, index, &frev, &crev))
		return;

	switch (frev) {
	case 1:
		/* no params on frev 1 */
		break;
	case 2:
		switch (crev) {
		case 1:
		case 2:
			args.v1.sDigEncoder.ucAction = action;
			args.v1.sDigEncoder.usPixelClock = cpu_to_le16(radeon_encoder->pixel_clock / 10);
			args.v1.sDigEncoder.ucEncoderMode = atombios_get_encoder_mode(encoder);

			if (args.v1.sDigEncoder.ucEncoderMode == ATOM_ENCODER_MODE_DP) {
				if (dp_clock == 270000)
					args.v1.sDigEncoder.ucConfig |= ATOM_ENCODER_CONFIG_DPLINKRATE_2_70GHZ;
				args.v1.sDigEncoder.ucLaneNum = dp_lane_count;
			} else if (radeon_encoder->pixel_clock > 165000)
				args.v1.sDigEncoder.ucLaneNum = 8;
			else
				args.v1.sDigEncoder.ucLaneNum = 4;
			break;
		case 3:
			args.v3.sExtEncoder.ucAction = action;
			if (action == EXTERNAL_ENCODER_ACTION_V3_ENCODER_INIT)
				args.v3.sExtEncoder.usConnectorId = cpu_to_le16(connector_object_id);
			else
				args.v3.sExtEncoder.usPixelClock = cpu_to_le16(radeon_encoder->pixel_clock / 10);
			args.v3.sExtEncoder.ucEncoderMode = atombios_get_encoder_mode(encoder);

			if (args.v3.sExtEncoder.ucEncoderMode == ATOM_ENCODER_MODE_DP) {
				if (dp_clock == 270000)
					args.v3.sExtEncoder.ucConfig |= EXTERNAL_ENCODER_CONFIG_V3_DPLINKRATE_2_70GHZ;
				else if (dp_clock == 540000)
					args.v3.sExtEncoder.ucConfig |= EXTERNAL_ENCODER_CONFIG_V3_DPLINKRATE_5_40GHZ;
				args.v3.sExtEncoder.ucLaneNum = dp_lane_count;
			} else if (radeon_encoder->pixel_clock > 165000)
				args.v3.sExtEncoder.ucLaneNum = 8;
			else
				args.v3.sExtEncoder.ucLaneNum = 4;
			switch (ext_enum) {
			case GRAPH_OBJECT_ENUM_ID1:
				args.v3.sExtEncoder.ucConfig |= EXTERNAL_ENCODER_CONFIG_V3_ENCODER1;
				break;
			case GRAPH_OBJECT_ENUM_ID2:
				args.v3.sExtEncoder.ucConfig |= EXTERNAL_ENCODER_CONFIG_V3_ENCODER2;
				break;
			case GRAPH_OBJECT_ENUM_ID3:
				args.v3.sExtEncoder.ucConfig |= EXTERNAL_ENCODER_CONFIG_V3_ENCODER3;
				break;
			}
			switch (bpc) {
			case 0:
				args.v3.sExtEncoder.ucBitPerColor = PANEL_BPC_UNDEFINE;
				break;
			case 6:
				args.v3.sExtEncoder.ucBitPerColor = PANEL_6BIT_PER_COLOR;
				break;
			case 8:
			default:
				args.v3.sExtEncoder.ucBitPerColor = PANEL_8BIT_PER_COLOR;
				break;
			case 10:
				args.v3.sExtEncoder.ucBitPerColor = PANEL_10BIT_PER_COLOR;
				break;
			case 12:
				args.v3.sExtEncoder.ucBitPerColor = PANEL_12BIT_PER_COLOR;
				break;
			case 16:
				args.v3.sExtEncoder.ucBitPerColor = PANEL_16BIT_PER_COLOR;
				break;
			}
			break;
		default:
			DRM_ERROR("Unknown table version: %d, %d\n", frev, crev);
			return;
		}
		break;
	default:
		DRM_ERROR("Unknown table version: %d, %d\n", frev, crev);
		return;
	}
	atom_execute_table(rdev->mode_info.atom_context, index, (uint32_t *)&args);
}

static void
atombios_yuv_setup(struct drm_encoder *encoder, bool enable)
{
	struct drm_device *dev = encoder->dev;
	struct radeon_device *rdev = dev->dev_private;
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	struct radeon_crtc *radeon_crtc = to_radeon_crtc(encoder->crtc);
	ENABLE_YUV_PS_ALLOCATION args;
	int index = GetIndexIntoMasterTable(COMMAND, EnableYUV);
	uint32_t temp, reg;

	memset(&args, 0, sizeof(args));

	if (rdev->family >= CHIP_R600)
		reg = R600_BIOS_3_SCRATCH;
	else
		reg = RADEON_BIOS_3_SCRATCH;

	/* XXX: fix up scratch reg handling */
	temp = RREG32(reg);
	if (radeon_encoder->active_device & (ATOM_DEVICE_TV_SUPPORT))
		WREG32(reg, (ATOM_S3_TV1_ACTIVE |
			     (radeon_crtc->crtc_id << 18)));
	else if (radeon_encoder->active_device & (ATOM_DEVICE_CV_SUPPORT))
		WREG32(reg, (ATOM_S3_CV_ACTIVE | (radeon_crtc->crtc_id << 24)));
	else
		WREG32(reg, 0);

	if (enable)
		args.ucEnable = ATOM_ENABLE;
	args.ucCRTC = radeon_crtc->crtc_id;

	atom_execute_table(rdev->mode_info.atom_context, index, (uint32_t *)&args);

	WREG32(reg, temp);
}

static void
radeon_atom_encoder_dpms(struct drm_encoder *encoder, int mode)
{
	struct drm_device *dev = encoder->dev;
	struct radeon_device *rdev = dev->dev_private;
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	struct drm_encoder *ext_encoder = radeon_atom_get_external_encoder(encoder);
	DISPLAY_DEVICE_OUTPUT_CONTROL_PS_ALLOCATION args;
	int index = 0;
	bool is_dig = false;
	bool is_dce5_dac = false;
	bool is_dce5_dvo = false;

	memset(&args, 0, sizeof(args));

	DRM_DEBUG_KMS("encoder dpms %d to mode %d, devices %08x, active_devices %08x\n",
		  radeon_encoder->encoder_id, mode, radeon_encoder->devices,
		  radeon_encoder->active_device);
	switch (radeon_encoder->encoder_id) {
	case ENCODER_OBJECT_ID_INTERNAL_TMDS1:
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_TMDS1:
		index = GetIndexIntoMasterTable(COMMAND, TMDSAOutputControl);
		break;
	case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
	case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
	case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_LVTMA:
		is_dig = true;
		break;
	case ENCODER_OBJECT_ID_INTERNAL_DVO1:
	case ENCODER_OBJECT_ID_INTERNAL_DDI:
		index = GetIndexIntoMasterTable(COMMAND, DVOOutputControl);
		break;
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DVO1:
		if (ASIC_IS_DCE5(rdev))
			is_dce5_dvo = true;
		else if (ASIC_IS_DCE3(rdev))
			is_dig = true;
		else
			index = GetIndexIntoMasterTable(COMMAND, DVOOutputControl);
		break;
	case ENCODER_OBJECT_ID_INTERNAL_LVDS:
		index = GetIndexIntoMasterTable(COMMAND, LCD1OutputControl);
		break;
	case ENCODER_OBJECT_ID_INTERNAL_LVTM1:
		if (radeon_encoder->devices & (ATOM_DEVICE_LCD_SUPPORT))
			index = GetIndexIntoMasterTable(COMMAND, LCD1OutputControl);
		else
			index = GetIndexIntoMasterTable(COMMAND, LVTMAOutputControl);
		break;
	case ENCODER_OBJECT_ID_INTERNAL_DAC1:
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC1:
		if (ASIC_IS_DCE5(rdev))
			is_dce5_dac = true;
		else {
			if (radeon_encoder->active_device & (ATOM_DEVICE_TV_SUPPORT))
				index = GetIndexIntoMasterTable(COMMAND, TV1OutputControl);
			else if (radeon_encoder->active_device & (ATOM_DEVICE_CV_SUPPORT))
				index = GetIndexIntoMasterTable(COMMAND, CV1OutputControl);
			else
				index = GetIndexIntoMasterTable(COMMAND, DAC1OutputControl);
		}
		break;
	case ENCODER_OBJECT_ID_INTERNAL_DAC2:
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC2:
		if (radeon_encoder->active_device & (ATOM_DEVICE_TV_SUPPORT))
			index = GetIndexIntoMasterTable(COMMAND, TV1OutputControl);
		else if (radeon_encoder->active_device & (ATOM_DEVICE_CV_SUPPORT))
			index = GetIndexIntoMasterTable(COMMAND, CV1OutputControl);
		else
			index = GetIndexIntoMasterTable(COMMAND, DAC2OutputControl);
		break;
	}

	if (is_dig) {
		switch (mode) {
		case DRM_MODE_DPMS_ON:
			/* some early dce3.2 boards have a bug in their transmitter control table */
			if ((rdev->family == CHIP_RV710) || (rdev->family == CHIP_RV730))
				atombios_dig_transmitter_setup(encoder, ATOM_TRANSMITTER_ACTION_ENABLE, 0, 0);
			else
				atombios_dig_transmitter_setup(encoder, ATOM_TRANSMITTER_ACTION_ENABLE_OUTPUT, 0, 0);
			if (atombios_get_encoder_mode(encoder) == ATOM_ENCODER_MODE_DP) {
				struct drm_connector *connector = radeon_get_connector_for_encoder(encoder);

				if (connector &&
				    (connector->connector_type == DRM_MODE_CONNECTOR_eDP)) {
					struct radeon_connector *radeon_connector = to_radeon_connector(connector);
					struct radeon_connector_atom_dig *radeon_dig_connector =
						radeon_connector->con_priv;
					atombios_set_edp_panel_power(connector,
								     ATOM_TRANSMITTER_ACTION_POWER_ON);
					radeon_dig_connector->edp_on = true;
				}
				if (ASIC_IS_DCE4(rdev))
					atombios_dig_encoder_setup(encoder, ATOM_ENCODER_CMD_DP_VIDEO_OFF, 0);
				radeon_dp_link_train(encoder, connector);
				if (ASIC_IS_DCE4(rdev))
					atombios_dig_encoder_setup(encoder, ATOM_ENCODER_CMD_DP_VIDEO_ON, 0);
			}
			if (radeon_encoder->devices & (ATOM_DEVICE_LCD_SUPPORT))
				atombios_dig_transmitter_setup(encoder, ATOM_TRANSMITTER_ACTION_LCD_BLON, 0, 0);
			break;
		case DRM_MODE_DPMS_STANDBY:
		case DRM_MODE_DPMS_SUSPEND:
		case DRM_MODE_DPMS_OFF:
			atombios_dig_transmitter_setup(encoder, ATOM_TRANSMITTER_ACTION_DISABLE_OUTPUT, 0, 0);
			if (atombios_get_encoder_mode(encoder) == ATOM_ENCODER_MODE_DP) {
				struct drm_connector *connector = radeon_get_connector_for_encoder(encoder);

				if (ASIC_IS_DCE4(rdev))
					atombios_dig_encoder_setup(encoder, ATOM_ENCODER_CMD_DP_VIDEO_OFF, 0);
				if (connector &&
				    (connector->connector_type == DRM_MODE_CONNECTOR_eDP)) {
					struct radeon_connector *radeon_connector = to_radeon_connector(connector);
					struct radeon_connector_atom_dig *radeon_dig_connector =
						radeon_connector->con_priv;
					atombios_set_edp_panel_power(connector,
								     ATOM_TRANSMITTER_ACTION_POWER_OFF);
					radeon_dig_connector->edp_on = false;
				}
			}
			if (radeon_encoder->devices & (ATOM_DEVICE_LCD_SUPPORT))
				atombios_dig_transmitter_setup(encoder, ATOM_TRANSMITTER_ACTION_LCD_BLOFF, 0, 0);
			break;
		}
	} else if (is_dce5_dac) {
		switch (mode) {
		case DRM_MODE_DPMS_ON:
			atombios_dac_setup(encoder, ATOM_ENABLE);
			break;
		case DRM_MODE_DPMS_STANDBY:
		case DRM_MODE_DPMS_SUSPEND:
		case DRM_MODE_DPMS_OFF:
			atombios_dac_setup(encoder, ATOM_DISABLE);
			break;
		}
	} else if (is_dce5_dvo) {
		switch (mode) {
		case DRM_MODE_DPMS_ON:
			atombios_dvo_setup(encoder, ATOM_ENABLE);
			break;
		case DRM_MODE_DPMS_STANDBY:
		case DRM_MODE_DPMS_SUSPEND:
		case DRM_MODE_DPMS_OFF:
			atombios_dvo_setup(encoder, ATOM_DISABLE);
			break;
		}
	} else {
		switch (mode) {
		case DRM_MODE_DPMS_ON:
			args.ucAction = ATOM_ENABLE;
			atom_execute_table(rdev->mode_info.atom_context, index, (uint32_t *)&args);
			if (radeon_encoder->devices & (ATOM_DEVICE_LCD_SUPPORT)) {
				args.ucAction = ATOM_LCD_BLON;
				atom_execute_table(rdev->mode_info.atom_context, index, (uint32_t *)&args);
			}
			break;
		case DRM_MODE_DPMS_STANDBY:
		case DRM_MODE_DPMS_SUSPEND:
		case DRM_MODE_DPMS_OFF:
			args.ucAction = ATOM_DISABLE;
			atom_execute_table(rdev->mode_info.atom_context, index, (uint32_t *)&args);
			if (radeon_encoder->devices & (ATOM_DEVICE_LCD_SUPPORT)) {
				args.ucAction = ATOM_LCD_BLOFF;
				atom_execute_table(rdev->mode_info.atom_context, index, (uint32_t *)&args);
			}
			break;
		}
	}

	if (ext_encoder) {
		switch (mode) {
		case DRM_MODE_DPMS_ON:
		default:
			if (ASIC_IS_DCE41(rdev)) {
				atombios_external_encoder_setup(encoder, ext_encoder,
								EXTERNAL_ENCODER_ACTION_V3_ENABLE_OUTPUT);
				atombios_external_encoder_setup(encoder, ext_encoder,
								EXTERNAL_ENCODER_ACTION_V3_ENCODER_BLANKING_OFF);
			} else
				atombios_external_encoder_setup(encoder, ext_encoder, ATOM_ENABLE);
			break;
		case DRM_MODE_DPMS_STANDBY:
		case DRM_MODE_DPMS_SUSPEND:
		case DRM_MODE_DPMS_OFF:
			if (ASIC_IS_DCE41(rdev)) {
				atombios_external_encoder_setup(encoder, ext_encoder,
								EXTERNAL_ENCODER_ACTION_V3_ENCODER_BLANKING);
				atombios_external_encoder_setup(encoder, ext_encoder,
								EXTERNAL_ENCODER_ACTION_V3_DISABLE_OUTPUT);
			} else
				atombios_external_encoder_setup(encoder, ext_encoder, ATOM_DISABLE);
			break;
		}
	}

	radeon_atombios_encoder_dpms_scratch_regs(encoder, (mode == DRM_MODE_DPMS_ON) ? true : false);

}

union crtc_source_param {
	SELECT_CRTC_SOURCE_PS_ALLOCATION v1;
	SELECT_CRTC_SOURCE_PARAMETERS_V2 v2;
};

static void
atombios_set_encoder_crtc_source(struct drm_encoder *encoder)
{
	struct drm_device *dev = encoder->dev;
	struct radeon_device *rdev = dev->dev_private;
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	struct radeon_crtc *radeon_crtc = to_radeon_crtc(encoder->crtc);
	union crtc_source_param args;
	int index = GetIndexIntoMasterTable(COMMAND, SelectCRTC_Source);
	uint8_t frev, crev;
	struct radeon_encoder_atom_dig *dig;

	memset(&args, 0, sizeof(args));

	if (!atom_parse_cmd_header(rdev->mode_info.atom_context, index, &frev, &crev))
		return;

	switch (frev) {
	case 1:
		switch (crev) {
		case 1:
		default:
			if (ASIC_IS_AVIVO(rdev))
				args.v1.ucCRTC = radeon_crtc->crtc_id;
			else {
				if (radeon_encoder->encoder_id == ENCODER_OBJECT_ID_INTERNAL_DAC1) {
					args.v1.ucCRTC = radeon_crtc->crtc_id;
				} else {
					args.v1.ucCRTC = radeon_crtc->crtc_id << 2;
				}
			}
			switch (radeon_encoder->encoder_id) {
			case ENCODER_OBJECT_ID_INTERNAL_TMDS1:
			case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_TMDS1:
				args.v1.ucDevice = ATOM_DEVICE_DFP1_INDEX;
				break;
			case ENCODER_OBJECT_ID_INTERNAL_LVDS:
			case ENCODER_OBJECT_ID_INTERNAL_LVTM1:
				if (radeon_encoder->devices & ATOM_DEVICE_LCD1_SUPPORT)
					args.v1.ucDevice = ATOM_DEVICE_LCD1_INDEX;
				else
					args.v1.ucDevice = ATOM_DEVICE_DFP3_INDEX;
				break;
			case ENCODER_OBJECT_ID_INTERNAL_DVO1:
			case ENCODER_OBJECT_ID_INTERNAL_DDI:
			case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DVO1:
				args.v1.ucDevice = ATOM_DEVICE_DFP2_INDEX;
				break;
			case ENCODER_OBJECT_ID_INTERNAL_DAC1:
			case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC1:
				if (radeon_encoder->active_device & (ATOM_DEVICE_TV_SUPPORT))
					args.v1.ucDevice = ATOM_DEVICE_TV1_INDEX;
				else if (radeon_encoder->active_device & (ATOM_DEVICE_CV_SUPPORT))
					args.v1.ucDevice = ATOM_DEVICE_CV_INDEX;
				else
					args.v1.ucDevice = ATOM_DEVICE_CRT1_INDEX;
				break;
			case ENCODER_OBJECT_ID_INTERNAL_DAC2:
			case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC2:
				if (radeon_encoder->active_device & (ATOM_DEVICE_TV_SUPPORT))
					args.v1.ucDevice = ATOM_DEVICE_TV1_INDEX;
				else if (radeon_encoder->active_device & (ATOM_DEVICE_CV_SUPPORT))
					args.v1.ucDevice = ATOM_DEVICE_CV_INDEX;
				else
					args.v1.ucDevice = ATOM_DEVICE_CRT2_INDEX;
				break;
			}
			break;
		case 2:
			args.v2.ucCRTC = radeon_crtc->crtc_id;
			args.v2.ucEncodeMode = atombios_get_encoder_mode(encoder);
			switch (radeon_encoder->encoder_id) {
			case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
			case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
			case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
			case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_LVTMA:
				dig = radeon_encoder->enc_priv;
				switch (dig->dig_encoder) {
				case 0:
					args.v2.ucEncoderID = ASIC_INT_DIG1_ENCODER_ID;
					break;
				case 1:
					args.v2.ucEncoderID = ASIC_INT_DIG2_ENCODER_ID;
					break;
				case 2:
					args.v2.ucEncoderID = ASIC_INT_DIG3_ENCODER_ID;
					break;
				case 3:
					args.v2.ucEncoderID = ASIC_INT_DIG4_ENCODER_ID;
					break;
				case 4:
					args.v2.ucEncoderID = ASIC_INT_DIG5_ENCODER_ID;
					break;
				case 5:
					args.v2.ucEncoderID = ASIC_INT_DIG6_ENCODER_ID;
					break;
				}
				break;
			case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DVO1:
				args.v2.ucEncoderID = ASIC_INT_DVO_ENCODER_ID;
				break;
			case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC1:
				if (radeon_encoder->active_device & (ATOM_DEVICE_TV_SUPPORT))
					args.v2.ucEncoderID = ASIC_INT_TV_ENCODER_ID;
				else if (radeon_encoder->active_device & (ATOM_DEVICE_CV_SUPPORT))
					args.v2.ucEncoderID = ASIC_INT_TV_ENCODER_ID;
				else
					args.v2.ucEncoderID = ASIC_INT_DAC1_ENCODER_ID;
				break;
			case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC2:
				if (radeon_encoder->active_device & (ATOM_DEVICE_TV_SUPPORT))
					args.v2.ucEncoderID = ASIC_INT_TV_ENCODER_ID;
				else if (radeon_encoder->active_device & (ATOM_DEVICE_CV_SUPPORT))
					args.v2.ucEncoderID = ASIC_INT_TV_ENCODER_ID;
				else
					args.v2.ucEncoderID = ASIC_INT_DAC2_ENCODER_ID;
				break;
			}
			break;
		}
		break;
	default:
		DRM_ERROR("Unknown table version: %d, %d\n", frev, crev);
		return;
	}

	atom_execute_table(rdev->mode_info.atom_context, index, (uint32_t *)&args);

	/* update scratch regs with new routing */
	radeon_atombios_encoder_crtc_scratch_regs(encoder, radeon_crtc->crtc_id);
}

static void
atombios_apply_encoder_quirks(struct drm_encoder *encoder,
			      struct drm_display_mode *mode)
{
	struct drm_device *dev = encoder->dev;
	struct radeon_device *rdev = dev->dev_private;
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	struct radeon_crtc *radeon_crtc = to_radeon_crtc(encoder->crtc);

	/* Funky macbooks */
	if ((dev->pdev->device == 0x71C5) &&
	    (dev->pdev->subsystem_vendor == 0x106b) &&
	    (dev->pdev->subsystem_device == 0x0080)) {
		if (radeon_encoder->devices & ATOM_DEVICE_LCD1_SUPPORT) {
			uint32_t lvtma_bit_depth_control = RREG32(AVIVO_LVTMA_BIT_DEPTH_CONTROL);

			lvtma_bit_depth_control &= ~AVIVO_LVTMA_BIT_DEPTH_CONTROL_TRUNCATE_EN;
			lvtma_bit_depth_control &= ~AVIVO_LVTMA_BIT_DEPTH_CONTROL_SPATIAL_DITHER_EN;

			WREG32(AVIVO_LVTMA_BIT_DEPTH_CONTROL, lvtma_bit_depth_control);
		}
	}

	/* set scaler clears this on some chips */
	if (ASIC_IS_AVIVO(rdev) &&
	    (!(radeon_encoder->active_device & (ATOM_DEVICE_TV_SUPPORT)))) {
		if (ASIC_IS_DCE4(rdev)) {
			if (mode->flags & DRM_MODE_FLAG_INTERLACE)
				WREG32(EVERGREEN_DATA_FORMAT + radeon_crtc->crtc_offset,
				       EVERGREEN_INTERLEAVE_EN);
			else
				WREG32(EVERGREEN_DATA_FORMAT + radeon_crtc->crtc_offset, 0);
		} else {
			if (mode->flags & DRM_MODE_FLAG_INTERLACE)
				WREG32(AVIVO_D1MODE_DATA_FORMAT + radeon_crtc->crtc_offset,
				       AVIVO_D1MODE_INTERLEAVE_EN);
			else
				WREG32(AVIVO_D1MODE_DATA_FORMAT + radeon_crtc->crtc_offset, 0);
		}
	}
}

static int radeon_atom_pick_dig_encoder(struct drm_encoder *encoder)
{
	struct drm_device *dev = encoder->dev;
	struct radeon_device *rdev = dev->dev_private;
	struct radeon_crtc *radeon_crtc = to_radeon_crtc(encoder->crtc);
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	struct drm_encoder *test_encoder;
	struct radeon_encoder_atom_dig *dig;
	uint32_t dig_enc_in_use = 0;

	/* DCE4/5 */
	if (ASIC_IS_DCE4(rdev)) {
		dig = radeon_encoder->enc_priv;
		if (ASIC_IS_DCE41(rdev))
			return radeon_crtc->crtc_id;
		else {
			switch (radeon_encoder->encoder_id) {
			case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
				if (dig->linkb)
					return 1;
				else
					return 0;
				break;
			case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
				if (dig->linkb)
					return 3;
				else
					return 2;
				break;
			case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
				if (dig->linkb)
					return 5;
				else
					return 4;
				break;
			}
		}
	}

	/* on DCE32 and encoder can driver any block so just crtc id */
	if (ASIC_IS_DCE32(rdev)) {
		return radeon_crtc->crtc_id;
	}

	/* on DCE3 - LVTMA can only be driven by DIGB */
	list_for_each_entry(test_encoder, &dev->mode_config.encoder_list, head) {
		struct radeon_encoder *radeon_test_encoder;

		if (encoder == test_encoder)
			continue;

		if (!radeon_encoder_is_digital(test_encoder))
			continue;

		radeon_test_encoder = to_radeon_encoder(test_encoder);
		dig = radeon_test_encoder->enc_priv;

		if (dig->dig_encoder >= 0)
			dig_enc_in_use |= (1 << dig->dig_encoder);
	}

	if (radeon_encoder->encoder_id == ENCODER_OBJECT_ID_INTERNAL_KLDSCP_LVTMA) {
		if (dig_enc_in_use & 0x2)
			DRM_ERROR("LVDS required digital encoder 2 but it was in use - stealing\n");
		return 1;
	}
	if (!(dig_enc_in_use & 1))
		return 0;
	return 1;
}

/* This only needs to be called once at startup */
void
radeon_atom_encoder_init(struct radeon_device *rdev)
{
	struct drm_device *dev = rdev->ddev;
	struct drm_encoder *encoder;

	list_for_each_entry(encoder, &dev->mode_config.encoder_list, head) {
		struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
		struct drm_encoder *ext_encoder = radeon_atom_get_external_encoder(encoder);

		switch (radeon_encoder->encoder_id) {
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_LVTMA:
			atombios_dig_transmitter_setup(encoder, ATOM_TRANSMITTER_ACTION_INIT, 0, 0);
			break;
		default:
			break;
		}

		if (ext_encoder && ASIC_IS_DCE41(rdev))
			atombios_external_encoder_setup(encoder, ext_encoder,
							EXTERNAL_ENCODER_ACTION_V3_ENCODER_INIT);
	}
}

static void
radeon_atom_encoder_mode_set(struct drm_encoder *encoder,
			     struct drm_display_mode *mode,
			     struct drm_display_mode *adjusted_mode)
{
	struct drm_device *dev = encoder->dev;
	struct radeon_device *rdev = dev->dev_private;
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	struct drm_encoder *ext_encoder = radeon_atom_get_external_encoder(encoder);

	radeon_encoder->pixel_clock = adjusted_mode->clock;

	if (ASIC_IS_AVIVO(rdev) && !ASIC_IS_DCE4(rdev)) {
		if (radeon_encoder->active_device & (ATOM_DEVICE_CV_SUPPORT | ATOM_DEVICE_TV_SUPPORT))
			atombios_yuv_setup(encoder, true);
		else
			atombios_yuv_setup(encoder, false);
	}

	switch (radeon_encoder->encoder_id) {
	case ENCODER_OBJECT_ID_INTERNAL_TMDS1:
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_TMDS1:
	case ENCODER_OBJECT_ID_INTERNAL_LVDS:
	case ENCODER_OBJECT_ID_INTERNAL_LVTM1:
		atombios_digital_setup(encoder, PANEL_ENCODER_ACTION_ENABLE);
		break;
	case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
	case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
	case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_LVTMA:
		if (ASIC_IS_DCE4(rdev)) {
			/* disable the transmitter */
			atombios_dig_transmitter_setup(encoder, ATOM_TRANSMITTER_ACTION_DISABLE, 0, 0);
			/* setup and enable the encoder */
			atombios_dig_encoder_setup(encoder, ATOM_ENCODER_CMD_SETUP, 0);

			/* enable the transmitter */
			atombios_dig_transmitter_setup(encoder, ATOM_TRANSMITTER_ACTION_ENABLE, 0, 0);
		} else {
			/* disable the encoder and transmitter */
			atombios_dig_transmitter_setup(encoder, ATOM_TRANSMITTER_ACTION_DISABLE, 0, 0);
			atombios_dig_encoder_setup(encoder, ATOM_DISABLE, 0);

			/* setup and enable the encoder and transmitter */
			atombios_dig_encoder_setup(encoder, ATOM_ENABLE, 0);
			atombios_dig_transmitter_setup(encoder, ATOM_TRANSMITTER_ACTION_SETUP, 0, 0);
			atombios_dig_transmitter_setup(encoder, ATOM_TRANSMITTER_ACTION_ENABLE, 0, 0);
		}
		break;
	case ENCODER_OBJECT_ID_INTERNAL_DDI:
	case ENCODER_OBJECT_ID_INTERNAL_DVO1:
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DVO1:
		atombios_dvo_setup(encoder, ATOM_ENABLE);
		break;
	case ENCODER_OBJECT_ID_INTERNAL_DAC1:
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC1:
	case ENCODER_OBJECT_ID_INTERNAL_DAC2:
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC2:
		atombios_dac_setup(encoder, ATOM_ENABLE);
		if (radeon_encoder->devices & (ATOM_DEVICE_TV_SUPPORT | ATOM_DEVICE_CV_SUPPORT)) {
			if (radeon_encoder->active_device & (ATOM_DEVICE_TV_SUPPORT | ATOM_DEVICE_CV_SUPPORT))
				atombios_tv_setup(encoder, ATOM_ENABLE);
			else
				atombios_tv_setup(encoder, ATOM_DISABLE);
		}
		break;
	}

	if (ext_encoder) {
		if (ASIC_IS_DCE41(rdev))
			atombios_external_encoder_setup(encoder, ext_encoder,
							EXTERNAL_ENCODER_ACTION_V3_ENCODER_SETUP);
		else
			atombios_external_encoder_setup(encoder, ext_encoder, ATOM_ENABLE);
	}

	atombios_apply_encoder_quirks(encoder, adjusted_mode);

	if (atombios_get_encoder_mode(encoder) == ATOM_ENCODER_MODE_HDMI) {
		r600_hdmi_enable(encoder);
		r600_hdmi_setmode(encoder, adjusted_mode);
	}
}

static bool
atombios_dac_load_detect(struct drm_encoder *encoder, struct drm_connector *connector)
{
	struct drm_device *dev = encoder->dev;
	struct radeon_device *rdev = dev->dev_private;
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	struct radeon_connector *radeon_connector = to_radeon_connector(connector);

	if (radeon_encoder->devices & (ATOM_DEVICE_TV_SUPPORT |
				       ATOM_DEVICE_CV_SUPPORT |
				       ATOM_DEVICE_CRT_SUPPORT)) {
		DAC_LOAD_DETECTION_PS_ALLOCATION args;
		int index = GetIndexIntoMasterTable(COMMAND, DAC_LoadDetection);
		uint8_t frev, crev;

		memset(&args, 0, sizeof(args));

		if (!atom_parse_cmd_header(rdev->mode_info.atom_context, index, &frev, &crev))
			return false;

		args.sDacload.ucMisc = 0;

		if ((radeon_encoder->encoder_id == ENCODER_OBJECT_ID_INTERNAL_DAC1) ||
		    (radeon_encoder->encoder_id == ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC1))
			args.sDacload.ucDacType = ATOM_DAC_A;
		else
			args.sDacload.ucDacType = ATOM_DAC_B;

		if (radeon_connector->devices & ATOM_DEVICE_CRT1_SUPPORT)
			args.sDacload.usDeviceID = cpu_to_le16(ATOM_DEVICE_CRT1_SUPPORT);
		else if (radeon_connector->devices & ATOM_DEVICE_CRT2_SUPPORT)
			args.sDacload.usDeviceID = cpu_to_le16(ATOM_DEVICE_CRT2_SUPPORT);
		else if (radeon_connector->devices & ATOM_DEVICE_CV_SUPPORT) {
			args.sDacload.usDeviceID = cpu_to_le16(ATOM_DEVICE_CV_SUPPORT);
			if (crev >= 3)
				args.sDacload.ucMisc = DAC_LOAD_MISC_YPrPb;
		} else if (radeon_connector->devices & ATOM_DEVICE_TV1_SUPPORT) {
			args.sDacload.usDeviceID = cpu_to_le16(ATOM_DEVICE_TV1_SUPPORT);
			if (crev >= 3)
				args.sDacload.ucMisc = DAC_LOAD_MISC_YPrPb;
		}

		atom_execute_table(rdev->mode_info.atom_context, index, (uint32_t *)&args);

		return true;
	} else
		return false;
}

static enum drm_connector_status
radeon_atom_dac_detect(struct drm_encoder *encoder, struct drm_connector *connector)
{
	struct drm_device *dev = encoder->dev;
	struct radeon_device *rdev = dev->dev_private;
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	struct radeon_connector *radeon_connector = to_radeon_connector(connector);
	uint32_t bios_0_scratch;

	if (!atombios_dac_load_detect(encoder, connector)) {
		DRM_DEBUG_KMS("detect returned false \n");
		return connector_status_unknown;
	}

	if (rdev->family >= CHIP_R600)
		bios_0_scratch = RREG32(R600_BIOS_0_SCRATCH);
	else
		bios_0_scratch = RREG32(RADEON_BIOS_0_SCRATCH);

	DRM_DEBUG_KMS("Bios 0 scratch %x %08x\n", bios_0_scratch, radeon_encoder->devices);
	if (radeon_connector->devices & ATOM_DEVICE_CRT1_SUPPORT) {
		if (bios_0_scratch & ATOM_S0_CRT1_MASK)
			return connector_status_connected;
	}
	if (radeon_connector->devices & ATOM_DEVICE_CRT2_SUPPORT) {
		if (bios_0_scratch & ATOM_S0_CRT2_MASK)
			return connector_status_connected;
	}
	if (radeon_connector->devices & ATOM_DEVICE_CV_SUPPORT) {
		if (bios_0_scratch & (ATOM_S0_CV_MASK|ATOM_S0_CV_MASK_A))
			return connector_status_connected;
	}
	if (radeon_connector->devices & ATOM_DEVICE_TV1_SUPPORT) {
		if (bios_0_scratch & (ATOM_S0_TV1_COMPOSITE | ATOM_S0_TV1_COMPOSITE_A))
			return connector_status_connected; /* CTV */
		else if (bios_0_scratch & (ATOM_S0_TV1_SVIDEO | ATOM_S0_TV1_SVIDEO_A))
			return connector_status_connected; /* STV */
	}
	return connector_status_disconnected;
}

static enum drm_connector_status
radeon_atom_dig_detect(struct drm_encoder *encoder, struct drm_connector *connector)
{
	struct drm_device *dev = encoder->dev;
	struct radeon_device *rdev = dev->dev_private;
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	struct radeon_connector *radeon_connector = to_radeon_connector(connector);
	struct drm_encoder *ext_encoder = radeon_atom_get_external_encoder(encoder);
	u32 bios_0_scratch;

	if (!ASIC_IS_DCE4(rdev))
		return connector_status_unknown;

	if (!ext_encoder)
		return connector_status_unknown;

	if ((radeon_connector->devices & ATOM_DEVICE_CRT_SUPPORT) == 0)
		return connector_status_unknown;

	/* load detect on the dp bridge */
	atombios_external_encoder_setup(encoder, ext_encoder,
					EXTERNAL_ENCODER_ACTION_V3_DACLOAD_DETECTION);

	bios_0_scratch = RREG32(R600_BIOS_0_SCRATCH);

	DRM_DEBUG_KMS("Bios 0 scratch %x %08x\n", bios_0_scratch, radeon_encoder->devices);
	if (radeon_connector->devices & ATOM_DEVICE_CRT1_SUPPORT) {
		if (bios_0_scratch & ATOM_S0_CRT1_MASK)
			return connector_status_connected;
	}
	if (radeon_connector->devices & ATOM_DEVICE_CRT2_SUPPORT) {
		if (bios_0_scratch & ATOM_S0_CRT2_MASK)
			return connector_status_connected;
	}
	if (radeon_connector->devices & ATOM_DEVICE_CV_SUPPORT) {
		if (bios_0_scratch & (ATOM_S0_CV_MASK|ATOM_S0_CV_MASK_A))
			return connector_status_connected;
	}
	if (radeon_connector->devices & ATOM_DEVICE_TV1_SUPPORT) {
		if (bios_0_scratch & (ATOM_S0_TV1_COMPOSITE | ATOM_S0_TV1_COMPOSITE_A))
			return connector_status_connected; /* CTV */
		else if (bios_0_scratch & (ATOM_S0_TV1_SVIDEO | ATOM_S0_TV1_SVIDEO_A))
			return connector_status_connected; /* STV */
	}
	return connector_status_disconnected;
}

void
radeon_atom_ext_encoder_setup_ddc(struct drm_encoder *encoder)
{
	struct drm_encoder *ext_encoder = radeon_atom_get_external_encoder(encoder);

	if (ext_encoder)
		/* ddc_setup on the dp bridge */
		atombios_external_encoder_setup(encoder, ext_encoder,
						EXTERNAL_ENCODER_ACTION_V3_DDC_SETUP);

}

static void radeon_atom_encoder_prepare(struct drm_encoder *encoder)
{
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	struct drm_connector *connector = radeon_get_connector_for_encoder(encoder);

	if ((radeon_encoder->active_device &
	     (ATOM_DEVICE_DFP_SUPPORT | ATOM_DEVICE_LCD_SUPPORT)) ||
	    radeon_encoder_is_dp_bridge(encoder)) {
		struct radeon_encoder_atom_dig *dig = radeon_encoder->enc_priv;
		if (dig)
			dig->dig_encoder = radeon_atom_pick_dig_encoder(encoder);
	}

	radeon_atom_output_lock(encoder, true);
	radeon_atom_encoder_dpms(encoder, DRM_MODE_DPMS_OFF);

	if (connector) {
		struct radeon_connector *radeon_connector = to_radeon_connector(connector);

		/* select the clock/data port if it uses a router */
		if (radeon_connector->router.cd_valid)
			radeon_router_select_cd_port(radeon_connector);

		/* turn eDP panel on for mode set */
		if (connector->connector_type == DRM_MODE_CONNECTOR_eDP)
			atombios_set_edp_panel_power(connector,
						     ATOM_TRANSMITTER_ACTION_POWER_ON);
	}

	/* this is needed for the pll/ss setup to work correctly in some cases */
	atombios_set_encoder_crtc_source(encoder);
}

static void radeon_atom_encoder_commit(struct drm_encoder *encoder)
{
	radeon_atom_encoder_dpms(encoder, DRM_MODE_DPMS_ON);
	radeon_atom_output_lock(encoder, false);
}

static void radeon_atom_encoder_disable(struct drm_encoder *encoder)
{
	struct drm_device *dev = encoder->dev;
	struct radeon_device *rdev = dev->dev_private;
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	struct radeon_encoder_atom_dig *dig;

	/* check for pre-DCE3 cards with shared encoders;
	 * can't really use the links individually, so don't disable
	 * the encoder if it's in use by another connector
	 */
	if (!ASIC_IS_DCE3(rdev)) {
		struct drm_encoder *other_encoder;
		struct radeon_encoder *other_radeon_encoder;

		list_for_each_entry(other_encoder, &dev->mode_config.encoder_list, head) {
			other_radeon_encoder = to_radeon_encoder(other_encoder);
			if ((radeon_encoder->encoder_id == other_radeon_encoder->encoder_id) &&
			    drm_helper_encoder_in_use(other_encoder))
				goto disable_done;
		}
	}

	radeon_atom_encoder_dpms(encoder, DRM_MODE_DPMS_OFF);

	switch (radeon_encoder->encoder_id) {
	case ENCODER_OBJECT_ID_INTERNAL_TMDS1:
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_TMDS1:
	case ENCODER_OBJECT_ID_INTERNAL_LVDS:
	case ENCODER_OBJECT_ID_INTERNAL_LVTM1:
		atombios_digital_setup(encoder, PANEL_ENCODER_ACTION_DISABLE);
		break;
	case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
	case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
	case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_LVTMA:
		if (ASIC_IS_DCE4(rdev))
			/* disable the transmitter */
			atombios_dig_transmitter_setup(encoder, ATOM_TRANSMITTER_ACTION_DISABLE, 0, 0);
		else {
			/* disable the encoder and transmitter */
			atombios_dig_transmitter_setup(encoder, ATOM_TRANSMITTER_ACTION_DISABLE, 0, 0);
			atombios_dig_encoder_setup(encoder, ATOM_DISABLE, 0);
		}
		break;
	case ENCODER_OBJECT_ID_INTERNAL_DDI:
	case ENCODER_OBJECT_ID_INTERNAL_DVO1:
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DVO1:
		atombios_dvo_setup(encoder, ATOM_DISABLE);
		break;
	case ENCODER_OBJECT_ID_INTERNAL_DAC1:
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC1:
	case ENCODER_OBJECT_ID_INTERNAL_DAC2:
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC2:
		atombios_dac_setup(encoder, ATOM_DISABLE);
		if (radeon_encoder->devices & (ATOM_DEVICE_TV_SUPPORT | ATOM_DEVICE_CV_SUPPORT))
			atombios_tv_setup(encoder, ATOM_DISABLE);
		break;
	}

disable_done:
	if (radeon_encoder_is_digital(encoder)) {
		if (atombios_get_encoder_mode(encoder) == ATOM_ENCODER_MODE_HDMI)
			r600_hdmi_disable(encoder);
		dig = radeon_encoder->enc_priv;
		dig->dig_encoder = -1;
	}
	radeon_encoder->active_device = 0;
}

/* these are handled by the primary encoders */
static void radeon_atom_ext_prepare(struct drm_encoder *encoder)
{

}

static void radeon_atom_ext_commit(struct drm_encoder *encoder)
{

}

static void
radeon_atom_ext_mode_set(struct drm_encoder *encoder,
			 struct drm_display_mode *mode,
			 struct drm_display_mode *adjusted_mode)
{

}

static void radeon_atom_ext_disable(struct drm_encoder *encoder)
{

}

static void
radeon_atom_ext_dpms(struct drm_encoder *encoder, int mode)
{

}

static bool radeon_atom_ext_mode_fixup(struct drm_encoder *encoder,
				       struct drm_display_mode *mode,
				       struct drm_display_mode *adjusted_mode)
{
	return true;
}

static const struct drm_encoder_helper_funcs radeon_atom_ext_helper_funcs = {
	.dpms = radeon_atom_ext_dpms,
	.mode_fixup = radeon_atom_ext_mode_fixup,
	.prepare = radeon_atom_ext_prepare,
	.mode_set = radeon_atom_ext_mode_set,
	.commit = radeon_atom_ext_commit,
	.disable = radeon_atom_ext_disable,
	/* no detect for TMDS/LVDS yet */
};

static const struct drm_encoder_helper_funcs radeon_atom_dig_helper_funcs = {
	.dpms = radeon_atom_encoder_dpms,
	.mode_fixup = radeon_atom_mode_fixup,
	.prepare = radeon_atom_encoder_prepare,
	.mode_set = radeon_atom_encoder_mode_set,
	.commit = radeon_atom_encoder_commit,
	.disable = radeon_atom_encoder_disable,
	.detect = radeon_atom_dig_detect,
};

static const struct drm_encoder_helper_funcs radeon_atom_dac_helper_funcs = {
	.dpms = radeon_atom_encoder_dpms,
	.mode_fixup = radeon_atom_mode_fixup,
	.prepare = radeon_atom_encoder_prepare,
	.mode_set = radeon_atom_encoder_mode_set,
	.commit = radeon_atom_encoder_commit,
	.detect = radeon_atom_dac_detect,
};

void radeon_enc_destroy(struct drm_encoder *encoder)
{
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	kfree(radeon_encoder->enc_priv);
	drm_encoder_cleanup(encoder);
	kfree(radeon_encoder);
}

static const struct drm_encoder_funcs radeon_atom_enc_funcs = {
	.destroy = radeon_enc_destroy,
};

struct radeon_encoder_atom_dac *
radeon_atombios_set_dac_info(struct radeon_encoder *radeon_encoder)
{
	struct drm_device *dev = radeon_encoder->base.dev;
	struct radeon_device *rdev = dev->dev_private;
	struct radeon_encoder_atom_dac *dac = kzalloc(sizeof(struct radeon_encoder_atom_dac), GFP_KERNEL);

	if (!dac)
		return NULL;

	dac->tv_std = radeon_atombios_get_tv_info(rdev);
	return dac;
}

struct radeon_encoder_atom_dig *
radeon_atombios_set_dig_info(struct radeon_encoder *radeon_encoder)
{
	int encoder_enum = (radeon_encoder->encoder_enum & ENUM_ID_MASK) >> ENUM_ID_SHIFT;
	struct radeon_encoder_atom_dig *dig = kzalloc(sizeof(struct radeon_encoder_atom_dig), GFP_KERNEL);

	if (!dig)
		return NULL;

	/* coherent mode by default */
	dig->coherent_mode = true;
	dig->dig_encoder = -1;

	if (encoder_enum == 2)
		dig->linkb = true;
	else
		dig->linkb = false;

	return dig;
}

void
radeon_add_atom_encoder(struct drm_device *dev,
			uint32_t encoder_enum,
			uint32_t supported_device,
			u16 caps)
{
	struct radeon_device *rdev = dev->dev_private;
	struct drm_encoder *encoder;
	struct radeon_encoder *radeon_encoder;

	/* see if we already added it */
	list_for_each_entry(encoder, &dev->mode_config.encoder_list, head) {
		radeon_encoder = to_radeon_encoder(encoder);
		if (radeon_encoder->encoder_enum == encoder_enum) {
			radeon_encoder->devices |= supported_device;
			return;
		}

	}

	/* add a new one */
	radeon_encoder = kzalloc(sizeof(struct radeon_encoder), GFP_KERNEL);
	if (!radeon_encoder)
		return;

	encoder = &radeon_encoder->base;
	switch (rdev->num_crtc) {
	case 1:
		encoder->possible_crtcs = 0x1;
		break;
	case 2:
	default:
		encoder->possible_crtcs = 0x3;
		break;
	case 4:
		encoder->possible_crtcs = 0xf;
		break;
	case 6:
		encoder->possible_crtcs = 0x3f;
		break;
	}

	radeon_encoder->enc_priv = NULL;

	radeon_encoder->encoder_enum = encoder_enum;
	radeon_encoder->encoder_id = (encoder_enum & OBJECT_ID_MASK) >> OBJECT_ID_SHIFT;
	radeon_encoder->devices = supported_device;
	radeon_encoder->rmx_type = RMX_OFF;
	radeon_encoder->underscan_type = UNDERSCAN_OFF;
	radeon_encoder->is_ext_encoder = false;
	radeon_encoder->caps = caps;

	switch (radeon_encoder->encoder_id) {
	case ENCODER_OBJECT_ID_INTERNAL_LVDS:
	case ENCODER_OBJECT_ID_INTERNAL_TMDS1:
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_TMDS1:
	case ENCODER_OBJECT_ID_INTERNAL_LVTM1:
		if (radeon_encoder->devices & (ATOM_DEVICE_LCD_SUPPORT)) {
			radeon_encoder->rmx_type = RMX_FULL;
			drm_encoder_init(dev, encoder, &radeon_atom_enc_funcs, DRM_MODE_ENCODER_LVDS);
			radeon_encoder->enc_priv = radeon_atombios_get_lvds_info(radeon_encoder);
		} else {
			drm_encoder_init(dev, encoder, &radeon_atom_enc_funcs, DRM_MODE_ENCODER_TMDS);
			radeon_encoder->enc_priv = radeon_atombios_set_dig_info(radeon_encoder);
		}
		drm_encoder_helper_add(encoder, &radeon_atom_dig_helper_funcs);
		break;
	case ENCODER_OBJECT_ID_INTERNAL_DAC1:
		drm_encoder_init(dev, encoder, &radeon_atom_enc_funcs, DRM_MODE_ENCODER_DAC);
		radeon_encoder->enc_priv = radeon_atombios_set_dac_info(radeon_encoder);
		drm_encoder_helper_add(encoder, &radeon_atom_dac_helper_funcs);
		break;
	case ENCODER_OBJECT_ID_INTERNAL_DAC2:
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC1:
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC2:
		drm_encoder_init(dev, encoder, &radeon_atom_enc_funcs, DRM_MODE_ENCODER_TVDAC);
		radeon_encoder->enc_priv = radeon_atombios_set_dac_info(radeon_encoder);
		drm_encoder_helper_add(encoder, &radeon_atom_dac_helper_funcs);
		break;
	case ENCODER_OBJECT_ID_INTERNAL_DVO1:
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DVO1:
	case ENCODER_OBJECT_ID_INTERNAL_DDI:
	case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_LVTMA:
	case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
	case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
		if (radeon_encoder->devices & (ATOM_DEVICE_LCD_SUPPORT)) {
			radeon_encoder->rmx_type = RMX_FULL;
			drm_encoder_init(dev, encoder, &radeon_atom_enc_funcs, DRM_MODE_ENCODER_LVDS);
			radeon_encoder->enc_priv = radeon_atombios_get_lvds_info(radeon_encoder);
		} else if (radeon_encoder->devices & (ATOM_DEVICE_CRT_SUPPORT)) {
			drm_encoder_init(dev, encoder, &radeon_atom_enc_funcs, DRM_MODE_ENCODER_DAC);
			radeon_encoder->enc_priv = radeon_atombios_set_dig_info(radeon_encoder);
		} else {
			drm_encoder_init(dev, encoder, &radeon_atom_enc_funcs, DRM_MODE_ENCODER_TMDS);
			radeon_encoder->enc_priv = radeon_atombios_set_dig_info(radeon_encoder);
		}
		drm_encoder_helper_add(encoder, &radeon_atom_dig_helper_funcs);
		break;
	case ENCODER_OBJECT_ID_SI170B:
	case ENCODER_OBJECT_ID_CH7303:
	case ENCODER_OBJECT_ID_EXTERNAL_SDVOA:
	case ENCODER_OBJECT_ID_EXTERNAL_SDVOB:
	case ENCODER_OBJECT_ID_TITFP513:
	case ENCODER_OBJECT_ID_VT1623:
	case ENCODER_OBJECT_ID_HDMI_SI1930:
	case ENCODER_OBJECT_ID_TRAVIS:
	case ENCODER_OBJECT_ID_NUTMEG:
		/* these are handled by the primary encoders */
		radeon_encoder->is_ext_encoder = true;
		if (radeon_encoder->devices & (ATOM_DEVICE_LCD_SUPPORT))
			drm_encoder_init(dev, encoder, &radeon_atom_enc_funcs, DRM_MODE_ENCODER_LVDS);
		else if (radeon_encoder->devices & (ATOM_DEVICE_CRT_SUPPORT))
			drm_encoder_init(dev, encoder, &radeon_atom_enc_funcs, DRM_MODE_ENCODER_DAC);
		else
			drm_encoder_init(dev, encoder, &radeon_atom_enc_funcs, DRM_MODE_ENCODER_TMDS);
		drm_encoder_helper_add(encoder, &radeon_atom_ext_helper_funcs);
		break;
	}
}
>>>>>>> bfa322c... Merge branch 'linus' into sched/core
