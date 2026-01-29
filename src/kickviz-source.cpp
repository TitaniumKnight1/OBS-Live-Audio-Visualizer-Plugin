#include "kickviz-source.hpp"
#include "dsp_fft.hpp"

#include <obs-module.h>
#include <util/platform.h>
#include <util/dstr.h>
#include <algorithm>

static const char *T_(const char *k) { return obs_module_text(k); }

// ---------- helpers ----------
static inline float clampf(float v, float lo, float hi) { return std::max(lo, std::min(hi, v)); }

static inline vec4 rgba_u32_to_vec4(uint32_t rgba)
{
  float r = float((rgba >> 24) & 0xFF) / 255.0f;
  float g = float((rgba >> 16) & 0xFF) / 255.0f;
  float b = float((rgba >> 8)  & 0xFF) / 255.0f;
  float a = float((rgba)       & 0xFF) / 255.0f;
  return {r,g,b,a};
}

static inline uint32_t get_color_u32(obs_data_t *s, const char *key, uint32_t defv)
{
  if (!s) return defv;
  return (uint32_t)obs_data_get_int(s, key);
}

static void kickviz_detach_audio(KickVizState *st);

static void audio_capture_cb(void *param, obs_source_t *source, const struct audio_data *audio, bool muted)
{
  auto *st = (KickVizState*)param;
  if (!st || !audio || muted) return;

  const uint32_t frames = audio->frames;
  if (frames == 0) return;

  const float *ch0 = (const float*)audio->data[0];
  const float *ch1 = (const float*)audio->data[1];

  if (!ch0) return;

  std::lock_guard<std::mutex> lk(st->audio_mtx);
  if (st->ring.empty()) return;

  for (uint32_t i = 0; i < frames; i++) {
    float v = ch0[i];
    if (ch1) v = 0.5f * (v + ch1[i]);
    st->ring[st->ring_write] = v;
    st->ring_write = (st->ring_write + 1) % st->ring.size();
  }
}

static void kickviz_attach_audio(KickVizState *st, const std::string &name)
{
  kickviz_detach_audio(st);

  if (name.empty() || name == "(none)")
    return;

  obs_source_t *src = obs_get_source_by_name(name.c_str());
  if (!src) return;

  st->audio_source = src;
  obs_source_add_audio_capture_callback(st->audio_source, audio_capture_cb, st);
}

static void kickviz_detach_audio(KickVizState *st)
{
  if (st->audio_source) {
    obs_source_remove_audio_capture_callback(st->audio_source, audio_capture_cb, st);
    obs_source_release(st->audio_source);
    st->audio_source = nullptr;
  }
}

static const char *kickviz_get_name(void *)
{
  return T_("KickViz.SourceName");
}

static void kickviz_get_defaults(obs_data_t *settings)
{
  struct obs_video_info ovi {};
  obs_get_video_info(&ovi);

  obs_data_set_default_bool(settings, "use_base_size", true);
  obs_data_set_default_int(settings, "width",  (int)ovi.base_width);
  obs_data_set_default_int(settings, "height", (int)ovi.base_height);

  obs_data_set_default_string(settings, "audio_source_name", "(none)");
  obs_data_set_default_int(settings, "mode", 0);
  obs_data_set_default_int(settings, "shape", 1);

  obs_data_set_default_int(settings, "color", 0xFFFFFFFF);
  obs_data_set_default_int(settings, "bg_color", 0x00000000);
  obs_data_set_default_bool(settings, "use_gradient", false);
  obs_data_set_default_int(settings, "color2", 0xFF00FFFF);

  obs_data_set_default_double(settings, "sensitivity", 1.25);
  obs_data_set_default_double(settings, "smoothing", 0.55);
  obs_data_set_default_double(settings, "decay", 0.12);

  obs_data_set_default_int(settings, "bar_width", 10);
  obs_data_set_default_int(settings, "gap", 3);
}

static void *kickviz_create(obs_data_t *settings, obs_source_t *source)
{
  auto *st = new KickVizState();
  st->context = source;

  st->ring.assign(48000 * 2, 0.0f);
  st->window.assign(2048, 0.0f);

  st->white_tex = nullptr;

  obs_source_update(source, settings);

  return st;
}

static void kickviz_destroy(void *data)
{
  auto *st = (KickVizState*)data;
  if (!st) return;

  kickviz_detach_audio(st);

  if (st->white_tex) {
    gs_texture_destroy(st->white_tex);
    st->white_tex = nullptr;
  }

  delete st;
}

static void kickviz_update(void *data, obs_data_t *settings)
{
  auto *st = (KickVizState*)data;
  if (!st) return;

  st->S.use_base_size = obs_data_get_bool(settings, "use_base_size");
  st->S.width  = (int)obs_data_get_int(settings, "width");
  st->S.height = (int)obs_data_get_int(settings, "height");

  st->S.audio_source_name = obs_data_get_string(settings, "audio_source_name");
  st->S.mode  = (int)obs_data_get_int(settings, "mode");
  st->S.shape = (int)obs_data_get_int(settings, "shape");
  st->S.freq_range = (int)obs_data_get_int(settings, "freq_range");

  st->S.color = get_color_u32(settings, "color", 0xFFFFFFFF);
  st->S.bg_color = get_color_u32(settings, "bg_color", 0x00000000);
  st->S.use_gradient = obs_data_get_bool(settings, "use_gradient");
  st->S.color2 = get_color_u32(settings, "color2", 0xFF00FFFF);

  st->S.magnitude   = (float)obs_data_get_double(settings, "magnitude");
  st->S.sensitivity = (float)obs_data_get_double(settings, "sensitivity");
  st->S.smoothing   = (float)obs_data_get_double(settings, "smoothing");
  st->S.decay       = (float)obs_data_get_double(settings, "decay");
  st->S.bar_width   = (int)obs_data_get_int(settings, "bar_width");
  st->S.gap         = (int)obs_data_get_int(settings, "gap");

  if (st->S.magnitude < 0.1f) st->S.magnitude = 1.0f;
  st->S.sensitivity = clampf(st->S.sensitivity, 0.1f, 10.0f);
  st->S.smoothing   = clampf(st->S.smoothing,   0.0f, 0.95f);
  st->S.decay       = clampf(st->S.decay,       0.0f, 1.0f);
  st->S.bar_width   = std::max(1, st->S.bar_width);
  st->S.gap         = std::max(0, st->S.gap);

  kickviz_attach_audio(st, st->S.audio_source_name);
}

static bool enum_sources_cb(void *param, obs_source_t *source)
{
  auto *list = (obs_property_t*)param;
  if (!source) return true;

  const uint32_t flags = obs_source_get_output_flags(source);
  if ((flags & OBS_SOURCE_AUDIO) == 0) return true;

  const char *name = obs_source_get_name(source);
  if (!name || !*name) return true;

  obs_property_list_add_string(list, name, name);
  return true;
}

static obs_properties_t *kickviz_properties(void *data)
{
  auto *st = (KickVizState*)data;
  obs_properties_t *p = obs_properties_create();

  auto *audio = obs_properties_add_list(
    p,
    "audio_source_name",
    T_("KickViz.AudioSource"),
    OBS_COMBO_TYPE_LIST,
    OBS_COMBO_FORMAT_STRING
  );

  obs_property_list_add_string(audio, "(none)", "(none)");
  obs_enum_sources(enum_sources_cb, audio);

  auto *mode = obs_properties_add_list(
    p, "mode", T_("KickViz.Mode"),
    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT
  );
  obs_property_list_add_int(mode, T_("KickViz.Mode.BarsUp"), 0);
  obs_property_list_add_int(mode, T_("KickViz.Mode.Mirrored"), 1);
  obs_property_list_add_int(mode, T_("KickViz.Mode.Radial"), 2);

  auto *shape = obs_properties_add_list(
    p, "shape", T_("KickViz.Shape"),
    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT
  );
  obs_property_list_add_int(shape, T_("KickViz.Shape.Square"), 0);
  obs_property_list_add_int(shape, T_("KickViz.Shape.Rounded"), 1);
  obs_property_list_add_int(shape, T_("KickViz.Shape.Capsule"), 2);
  obs_property_list_add_int(shape, T_("KickViz.Shape.Dots"), 3);
  obs_property_list_add_int(shape, T_("KickViz.Shape.Line"), 4);

  auto *freq = obs_properties_add_list(
    p, "freq_range", T_("KickViz.FreqRange"),
    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT
  );
  obs_property_list_add_int(freq, T_("KickViz.FreqRange.Bass"), 0);
  obs_property_list_add_int(freq, T_("KickViz.FreqRange.Standard"), 1);
  obs_property_list_add_int(freq, T_("KickViz.FreqRange.Wide"), 2);
  obs_property_list_add_int(freq, T_("KickViz.FreqRange.Full"), 3);

  obs_properties_add_color(p, "color", T_("KickViz.Color"));
  obs_properties_add_color(p, "bg_color", T_("KickViz.BgColor"));
  obs_properties_add_bool(p, "use_gradient", T_("KickViz.UseGradient"));
  obs_properties_add_color(p, "color2", T_("KickViz.Color2"));

  obs_properties_add_float_slider(p, "magnitude",   T_("KickViz.Magnitude"),   1.0, 20.0, 0.5);
  obs_properties_add_float_slider(p, "sensitivity", T_("KickViz.Sensitivity"), 0.1, 5.0, 0.05);
  obs_properties_add_float_slider(p, "smoothing",   T_("KickViz.Smoothing"),   0.0, 0.95, 0.01);
  obs_properties_add_float_slider(p, "decay",       T_("KickViz.Decay"),       0.0, 1.0, 0.01);

  obs_properties_add_int_slider(p, "bar_width", T_("KickViz.BarWidth"), 1, 64, 1);
  obs_properties_add_int_slider(p, "gap",       T_("KickViz.Gap"),      0, 64, 1);

  obs_properties_add_bool(p, "use_base_size", T_("KickViz.UseBaseSize"));
  obs_properties_add_int(p, "width",  T_("KickViz.Width"),  16, 8192, 1);
  obs_properties_add_int(p, "height", T_("KickViz.Height"), 16, 8192, 1);

  return p;
}

static uint32_t kickviz_get_width(void *data)
{
  auto *st = (KickVizState*)data;
  if (!st) return 0;

  if (st->S.use_base_size) {
    struct obs_video_info ovi {};
    obs_get_video_info(&ovi);
    return ovi.base_width;
  }
  return (uint32_t)st->S.width;
}

static uint32_t kickviz_get_height(void *data)
{
  auto *st = (KickVizState*)data;
  if (!st) return 0;

  if (st->S.use_base_size) {
    struct obs_video_info ovi {};
    obs_get_video_info(&ovi);
    return ovi.base_height;
  }
  return (uint32_t)st->S.height;
}

static void compute_bins(KickVizState *st, int bars)
{
  {
    std::lock_guard<std::mutex> lk(st->audio_mtx);
    const size_t N = st->window.size();
    const size_t R = st->ring.size();
    size_t idx = (st->ring_write + R - N) % R;
    for (size_t i = 0; i < N; i++) {
      st->window[i] = st->ring[idx];
      idx = (idx + 1) % R;
    }
  }

  hann_window(st->window);
  real_fft_mag_0_to_nyquist(st->window, st->mags);

  const int nyq = (int)st->mags.size();
  if ((int)st->smooth.size() != bars) {
    st->smooth.assign(bars, 0.0f);
    st->peaks.assign(bars, 0.0f);
  }

  const float sens = st->S.sensitivity;
  const float smoothA = st->S.smoothing;
  const float decay = st->S.decay;

  for (int i = 0; i < bars; i++) {
    float t = (bars <= 1) ? 0.0f : float(i) / float(bars - 1);
    const float logScale = 6.5f;
    float mapped = (std::exp(logScale * t) - 1.0f) / (std::exp(logScale) - 1.0f);

    float maxPct = 0.20f;
    if (st->S.freq_range == 0) maxPct = 0.035f;
    else if (st->S.freq_range == 2) maxPct = 0.50f;
    else if (st->S.freq_range == 3) maxPct = 1.00f;

    int maxIdx = (int)(nyq * maxPct);
    int idx = 1 + (int)(mapped * float(maxIdx - 2));
    idx = std::max(1, std::min(nyq - 1, idx));

    float v = st->mags[idx] * 0.015f * sens;
    v = std::pow(v, 0.85f);
    v = clampf(v, 0.0f, 1.0f);

    st->smooth[i] = st->smooth[i] * smoothA + v * (1.0f - smoothA);
    st->peaks[i] = std::max(st->peaks[i] - decay * 0.02f, st->smooth[i]);
  }
}

static void draw_color_rect(gs_effect_t *effect, float x, float y, float w, float h, uint32_t color)
{
  gs_eparam_t *colorParam = gs_effect_get_param_by_name(effect, "color");
  if (colorParam) {
    vec4 colorVec = rgba_u32_to_vec4(color);
    gs_effect_set_vec4(colorParam, &colorVec);
  }

  gs_matrix_push();
  gs_matrix_translate3f(x, y, 0.0f);

  while (gs_effect_loop(effect, "Solid")) {
    gs_draw_sprite(nullptr, 0, (uint32_t)w, (uint32_t)h);
  }

  gs_matrix_pop();
}

static void kickviz_video_render(void *data, gs_effect_t *effect_unused)
{
  auto *st = (KickVizState*)data;
  if (!st) return;

  const uint32_t W = kickviz_get_width(st);
  const uint32_t H = kickviz_get_height(st);
  if (W == 0 || H == 0) return;

  const int slot = std::max(1, st->S.bar_width + st->S.gap);
  int bars = (int)W / slot;
  bars = std::max(16, std::min(320, bars));

  compute_bins(st, bars);

  gs_effect_t *solid = obs_get_base_effect(OBS_EFFECT_SOLID);
  if (!solid) return;

  gs_blend_state_push();
  gs_blend_function(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA);

  draw_color_rect(solid, 0.0f, 0.0f, (float)W, (float)H, st->S.bg_color);

  const float bw = (float)st->S.bar_width;
  const float gap = (float)st->S.gap;
  const float maxH = (float)H;
  const float magnitude = st->S.magnitude;

  float x = 0.0f;
  float centerX = (float)W / 2.0f;
  float centerY = (float)H / 2.0f;

  if (st->S.mode == 1) {
    x = centerX;
  }

  float radius = std::min(centerX, centerY) * 0.65f;
  float maxRadius = std::min(centerX, centerY) * 0.95f;

  for (int i = 0; i < bars; i++) {
    float v = st->smooth[i];
    float barH = v * maxH * magnitude;
    barH = clampf(barH, 0.0f, maxH);
    int shape = st->S.shape;

    if (st->S.mode == 0) {
      if (shape == 3) {
        float dot = std::max(2.0f, bw * 0.35f);
        float step = dot * 2.0f + 2.0f;
        int numDots = (int)(barH / step);
        for (int d = 0; d < numDots; d++) {
          float dotY = maxH - (d + 1) * step;
          draw_color_rect(solid, x + (bw - dot*2.0f)/2.0f, dotY, dot*2.0f, dot*2.0f, st->S.color);
        }
      } else if (shape == 4) {
          float lineY = maxH - barH;
          draw_color_rect(solid, x, lineY, bw, 2.0f, st->S.color);
      } else {
        float inset = (shape == 0) ? 0.0f : std::max(0.0f, bw * 0.06f);
        if (shape == 2) inset = bw * 0.2f;

        float barTop = maxH - barH;
        draw_color_rect(solid, x + inset, barTop, std::max(1.0f, bw - inset*2.0f), barH, st->S.color);
      }

      if (st->peaks[i] > 0.01f) {
        float peakH = st->peaks[i] * maxH * magnitude;
        float capY = maxH - peakH;
        capY = clampf(capY, 0.0f, maxH - 2.0f);
        draw_color_rect(solid, x, capY, bw, 2.0f, st->S.color);
      }

      x += (bw + gap);
      if (x > (float)W) break;
    }
    else if (st->S.mode == 1) {
        float rX = centerX + (i * (bw + gap));
        float lX = centerX - ((i + 1) * (bw + gap));

        if (rX > (float)W) break;

        if (shape == 3) {
            float dot = std::max(2.0f, bw * 0.35f);
            float step = dot * 2.0f + 2.0f;
            int numDots = (int)(barH / step);
            for (int d = 0; d < numDots; d++) {
                float dotY = maxH - (d + 1) * step;
                draw_color_rect(solid, rX + (bw - dot*2.0f)/2.0f, dotY, dot*2.0f, dot*2.0f, st->S.color);
                draw_color_rect(solid, lX + (bw - dot*2.0f)/2.0f, dotY, dot*2.0f, dot*2.0f, st->S.color);
            }
        } else if (shape == 4) {
             float lineY = maxH - barH;
             draw_color_rect(solid, rX, lineY, bw, 2.0f, st->S.color);
             draw_color_rect(solid, lX, lineY, bw, 2.0f, st->S.color);
        } else {
            float inset = (shape == 0) ? 0.0f : std::max(0.0f, bw * 0.06f);
            float barTop = maxH - barH;
            draw_color_rect(solid, rX + inset, barTop, std::max(1.0f, bw - inset*2.0f), barH, st->S.color);
            draw_color_rect(solid, lX + inset, barTop, std::max(1.0f, bw - inset*2.0f), barH, st->S.color);
        }

        if (st->peaks[i] > 0.01f) {
            float peakH = st->peaks[i] * maxH * magnitude;
            float capY = maxH - peakH;
            capY = clampf(capY, 0.0f, maxH - 2.0f);
            draw_color_rect(solid, rX, capY, bw, 2.0f, st->S.color);
            draw_color_rect(solid, lX, capY, bw, 2.0f, st->S.color);
        }
    }
    else if (st->S.mode == 2) {
        float angleStep = 6.28318f / (float)bars;
        float angle = i * angleStep - 1.5708f;

        float cosA = cosf(angle);
        float sinA = sinf(angle);

        float sX = centerX + radius * cosA;
        float sY = centerY + radius * sinA;

        float val = std::max(0.025f, st->smooth[i]);
        float v_rad = val * magnitude * 0.5f;
        float curRadius = radius + (maxRadius - radius) * v_rad;

        gs_matrix_push();
        gs_matrix_translate3f(sX, sY, 0.0f);
        gs_matrix_rotaa4f(0.0f, 0.0f, 1.0f, angle - 1.570796f);

        float radBarH = curRadius - radius;

        if (shape == 3) {
             float dot = std::max(2.0f, bw * 0.35f);
             float step = dot * 2.0f + 2.0f;
             int numDots = (int)(radBarH / step);
             for(int d=0; d<numDots; d++) {
                 draw_color_rect(solid, -dot, d*step, dot*2.0f, dot*2.0f, st->S.color);
             }
        } else {
             draw_color_rect(solid, -bw/2.0f, -1.0f, bw, radBarH + 1.0f, st->S.color);
        }

        gs_matrix_pop();
    }
  }

  gs_blend_state_pop();
}

obs_source_info kickviz_source_info = {};

void kickviz_register_source(void)
{
  kickviz_source_info.id = "audio_visualizer_source";
  kickviz_source_info.type = OBS_SOURCE_TYPE_INPUT;
  kickviz_source_info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW;
  kickviz_source_info.get_name = kickviz_get_name;
  kickviz_source_info.create = kickviz_create;
  kickviz_source_info.destroy = kickviz_destroy;
  kickviz_source_info.get_defaults = kickviz_get_defaults;
  kickviz_source_info.get_properties = kickviz_properties;
  kickviz_source_info.update = kickviz_update;
  kickviz_source_info.video_render = kickviz_video_render;
  kickviz_source_info.get_width = kickviz_get_width;
  kickviz_source_info.get_height = kickviz_get_height;
}