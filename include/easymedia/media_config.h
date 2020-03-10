// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EASYMEDIA_MEDIA_CONFIG_H_
#define EASYMEDIA_MEDIA_CONFIG_H_

#include "image.h"
#include "media_type.h"
#include "sound.h"
#include "flow.h"

typedef struct {
  ImageInfo image_info;
  CodecType codec_type;
  int qp_init; // h264 : 0 - 48, higher value means higher compress
               //        but lower quality
               // jpeg (quantization coefficient): 1 - 10,
               // higher value means lower compress but higher quality,
               // contrary to h264
} ImageConfig;

typedef struct {
  ImageConfig image_cfg;
  int qp_step;
  int qp_min;
  int qp_max;
  int max_i_qp; // h265 encoder.
  int min_i_qp; // h265 encoder.
  int bit_rate;
  int frame_rate;
  int trans_8x8; // h264 encoder.
  int level;
  int gop_size;
  int profile;
  // quality - quality parameter
  //    (extra CQP level means special constant-qp (CQP) mode)
  //    (extra AQ_ONLY means special aq only mode)
  // "worst", "worse", "medium", "better", "best", "cqp", "aq_only"
  const char *rc_quality;
  // rc_mode - rate control mode
  // "vbr", "cbr"
  const char *rc_mode;
} VideoConfig;

typedef struct {
  SampleInfo sample_info;
  CodecType codec_type;
  // uint64_t channel_layout;
  int bit_rate;
  float quality; // vorbis: 0.0 ~ 1.0;
} AudioConfig;

typedef struct {
  union {
    VideoConfig vid_cfg;
    ImageConfig img_cfg;
    AudioConfig aud_cfg;
  };
  Type type;
} MediaConfig;

#define OSD_REGIONS_CNT 8

typedef struct  {
  char path[128]; //bmp file path.
  char str[128]; //The string to be superimposed.
  uint32_t width;
  uint32_t height;
  uint32_t offset_x;
  uint32_t offset_y;
  uint32_t str_corlor;
  uint32_t inverse;
  uint32_t is_ts; // is timestamp?
  uint32_t region_id; // max = 8.
  uint32_t enable;
} OsdRegionData;

#include <map>

namespace easymedia {
extern const char *rc_quality_strings[7];
extern const char *rc_mode_strings[2];
bool ParseMediaConfigFromMap(std::map<std::string, std::string> &params,
                             MediaConfig &mc);
_API std::string to_param_string(const ImageConfig &img_cfg);
_API std::string to_param_string(const VideoConfig &vid_cfg);
_API std::string to_param_string(const AudioConfig &aud_cfg);
_API std::string to_param_string(const MediaConfig &mc,
                                 const std::string &out_type);

_API int video_encoder_set_maxbps(
  std::shared_ptr<Flow> &enc_flow, unsigned int bpsmax);

_API int video_encoder_set_fps(
  std::shared_ptr<Flow> &enc_flow, uint8_t num, uint8_t den);

_API int video_encoder_set_osd_plt(
  std::shared_ptr<Flow> &enc_flow, uint32_t *yuv_plt);

_API int video_encoder_set_osd_region(
  std::shared_ptr<Flow> &enc_flow, OsdRegionData *region_data);

_API int video_encoder_set_move_detection(std::shared_ptr<Flow> &enc_flow,
  std::shared_ptr<Flow> &md_flow);

_API int video_encoder_enable_statistics(
  std::shared_ptr<Flow> &enc_flow, int enable);
} // namespace easymedia

#endif // #ifndef EASYMEDIA_MEDIA_CONFIG_H_
