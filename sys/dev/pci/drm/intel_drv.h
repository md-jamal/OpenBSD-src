#include "i915_drm.h"

struct intel_framebuffer {
	struct drm_framebuffer base;
#ifdef notyet
	struct drm_i915_gem_object *obj;
#endif
};

struct intel_encoder {
	struct drm_encoder		 base;
	int				 type;
	void				 (*hot_plug)(struct intel_encoder *);
	int				 crtc_mask;
	int				 clone_mask;
};

struct intel_connector {
	struct drm_connector		 base;
	struct intel_encoder		*encoder;
};

struct intel_crtc {
	struct drm_crtc			 base;
	enum pipe			 pipe;
	enum plane			 plane;
	u8				 lut_r[256];
	u8				 lut_g[256];
	u8				 lut_b[256];
	int				 dpms_mode;
	bool active; /* is the crtc on? independent of the dpms mode */
	bool busy; /* is scanout buffer being updated frequently? */
	unsigned int bpp;

	bool no_pll; /* tertiary pipe for IVB */
	bool use_pll_a;
};

struct intel_plane {
	struct drm_plane base;
	enum pipe pipe;
//	struct drm_i915_gem_object *obj;
	bool primary_disabled;
	int max_downscale;
	u32 lut_r[1024], lut_g[1024], lut_b[1024];
#ifdef notyet
	void (*update_plane)(struct drm_plane *plane,
			     struct drm_framebuffer *fb,
			     struct drm_i915_gem_object *obj,
			     int crtc_x, int crtc_y,
			     unsigned int crtc_w, unsigned int crtc_h,
			     uint32_t x, uint32_t y,
			     uint32_t src_w, uint32_t src_h);
#endif
	void (*disable_plane)(struct drm_plane *plane);
	int (*update_colorkey)(struct drm_plane *plane,
			       struct drm_intel_sprite_colorkey *key);
	void (*get_colorkey)(struct drm_plane *plane,
			     struct drm_intel_sprite_colorkey *key);
};

#define to_intel_crtc(x) container_of(x, struct intel_crtc, base)
#define to_intel_connector(x) container_of(x, struct intel_connector, base)
#define to_intel_encoder(x) container_of(x, struct intel_encoder, base)
#define to_intel_framebuffer(x) container_of(x, struct intel_framebuffer, base)
#define to_intel_plane(x) container_of(x, struct intel_plane, base)

extern bool intel_lvds_init(struct drm_device *dev);
extern struct drm_encoder *intel_best_encoder(struct drm_connector *connector);
extern struct drm_display_mode *intel_crtc_mode_get(struct drm_device *dev,
						    struct drm_crtc *crtc);
extern void intel_encoder_destroy(struct drm_encoder *encoder);

extern void intel_connector_attach_encoder(struct intel_connector *connector,
					   struct intel_encoder *encoder);
extern int intel_panel_setup_backlight(struct drm_device *dev);
extern void intel_fb_output_poll_changed(struct drm_device *dev);
extern int intel_plane_init(struct drm_device *dev, enum pipe pipe);
extern void intel_init_clock_gating(struct drm_device *dev);
extern void ironlake_enable_drps(struct drm_device *dev);
extern void ironlake_disable_drps(struct drm_device *dev);
extern void gen6_enable_rps(struct inteldrm_softc *dev_priv);
extern void gen6_update_ring_freq(struct inteldrm_softc *dev_priv);
extern void gen6_disable_rps(struct drm_device *dev);
extern void intel_init_emon(struct drm_device *dev);

extern int intel_fbdev_init(struct drm_device *dev);

extern void intel_crtc_load_lut(struct drm_crtc *crtc);

/* For use by IVB LP watermark workaround in intel_sprite.c */
extern void sandybridge_update_wm(struct drm_device *dev);

/* maximum connectors per crtcs in the mode set */
#define INTELFB_CONN_LIMIT 4

/* these are outputs from the chip - integrated only
   external chips are via DVO or SDVO output */
#define INTEL_OUTPUT_UNUSED 0
#define INTEL_OUTPUT_ANALOG 1
#define INTEL_OUTPUT_DVO 2
#define INTEL_OUTPUT_SDVO 3
#define INTEL_OUTPUT_LVDS 4
#define INTEL_OUTPUT_TVOUT 5
#define INTEL_OUTPUT_HDMI 6
#define INTEL_OUTPUT_DISPLAYPORT 7
#define INTEL_OUTPUT_EDP 8

/* Intel Pipe Clone Bit */
#define INTEL_HDMIB_CLONE_BIT 1
#define INTEL_HDMIC_CLONE_BIT 2
#define INTEL_HDMID_CLONE_BIT 3
#define INTEL_HDMIE_CLONE_BIT 4
#define INTEL_HDMIF_CLONE_BIT 5
#define INTEL_SDVO_NON_TV_CLONE_BIT 6
#define INTEL_SDVO_TV_CLONE_BIT 7
#define INTEL_SDVO_LVDS_CLONE_BIT 8
#define INTEL_ANALOG_CLONE_BIT 9
#define INTEL_TV_CLONE_BIT 10
#define INTEL_DP_B_CLONE_BIT 11
#define INTEL_DP_C_CLONE_BIT 12
#define INTEL_DP_D_CLONE_BIT 13
#define INTEL_LVDS_CLONE_BIT 14
#define INTEL_DVO_TMDS_CLONE_BIT 15
#define INTEL_DVO_LVDS_CLONE_BIT 16
#define INTEL_EDP_CLONE_BIT 17

static inline struct drm_crtc *
intel_get_crtc_for_pipe(struct drm_device *dev, int pipe)
{
	printf("%s stub\n", __func__);
	return (NULL);
}

extern int intel_sprite_set_colorkey(struct drm_device *dev, void *data,
				     struct drm_file *file_priv);
extern int intel_sprite_get_colorkey(struct drm_device *dev, void *data,
				     struct drm_file *file_priv);
