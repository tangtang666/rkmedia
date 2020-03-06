// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#include <string>
#include <iostream>

#include "buffer.h"
#include "encoder.h"
#include "flow.h"
#include "key_string.h"
#include "media_config.h"
#include "media_type.h"
#include "message.h"
#include "stream.h"
#include "utils.h"
#include "image.h"

static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

static char optstr[] = "?:i:o:w:h:f:t:";

static void print_usage(char *name) {
  printf("usage example: \n");
  printf("%s -i /dev/video0 -o output.h265 -w 1920 -h 1080 "
         "-f nv12 -t h265\n", name);
  printf("#[-t] support list:\n\th264\n\th265\n\tjpeg\n");
  printf("#[-f] support list:\n\tyuyv422\n\tnv12\n");
}

int main(int argc, char **argv) {
  int c, local_file_flag;
  int video_width = 1920;
  int video_height = 1080;
  int vir_width, vir_height;
  std::string pixel_format;
  int video_fps = 30;
  unsigned int bpsmax = video_width * video_height;
  std::string video_enc_type = VIDEO_H264;

  std::string output_path;
  std::string input_path;

  std::string flow_name;
  std::string flow_param;
  std::string stream_param;
  std::string enc_param;

  std::shared_ptr<easymedia::Flow> video_read_flow;
  std::shared_ptr<easymedia::Flow> video_encoder_flow;
  std::shared_ptr<easymedia::Flow> video_save_flow;

  opterr = 1;
  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'i':
      input_path = optarg;
      printf("#IN ARGS: input path: %s\n", input_path.c_str());
      break;
    case 'o':
      output_path = optarg;
      printf("#IN ARGS: output file path: %s\n", output_path.c_str());
      break;
    case 'w':
      video_width = atoi(optarg);
      printf("#IN ARGS: video_width: %d\n", video_width);
      break;
    case 'h':
      video_height = atoi(optarg);
      printf("#IN ARGS: video_height: %d\n", video_height);
      break;
    case 'f':
      pixel_format = optarg;
      printf("#IN ARGS: pixel_format: %s\n", pixel_format.c_str());
      break;
    case 't':
      video_enc_type = optarg;
      printf("#IN ARGS: video_enc_type: %s\n", video_enc_type.c_str());
      break;
    case '?':
    default:
      print_usage(argv[0]);
      exit(0);
    }
  }

  if (input_path.empty() || output_path.empty()) {
    printf("ERROR: path is not valid!\n");
    exit(EXIT_FAILURE);
  }

  //add prefix for pixformat
  if((pixel_format == "yuyv422") || (pixel_format == "nv12"))
    pixel_format = "image:" + pixel_format;
  else {
    printf("ERROR: image type:%s not support!\n", pixel_format.c_str());
    print_usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  //add prefix for encoder type
  if ((video_enc_type =="h264") || (video_enc_type == "h265"))
    video_enc_type = "video:" + video_enc_type;
  else if (video_enc_type == "jpeg")
    video_enc_type = "image:" + video_enc_type;
  else {
    printf("ERROR: encoder type:%s not support!\n", video_enc_type.c_str());
    print_usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  signal(SIGINT, sigterm_handler);
  printf("#Dump factroys:");
  easymedia::REFLECTOR(Flow)::DumpFactories();
  printf("#Dump streams:");
  easymedia::REFLECTOR(Stream)::DumpFactories();

  if (strstr(input_path.c_str(), "/dev/video")) {
    printf("INFO: reading yuv frome camera!\n");
    local_file_flag = 0;
  } else {
    printf("INFO: reading yuv frome file!\n");
    local_file_flag = 1;
  }

  if (local_file_flag) {
    //Reading yuv from file.
    flow_name = "file_read_flow";
    flow_param = "";
    ImageInfo info;
    info.pix_fmt = StringToPixFmt(pixel_format.c_str());
    info.width = video_width;
    info.height = video_height;
    info.vir_width = UPALIGNTO16(video_width);
    info.vir_height = UPALIGNTO16(video_height);
    vir_width = info.vir_width;
    vir_height = info.vir_height;

    flow_param.append(easymedia::to_param_string(info, 1).c_str());
    PARAM_STRING_APPEND(flow_param, KEY_PATH, input_path);
    PARAM_STRING_APPEND(flow_param, KEY_OPEN_MODE, "re"); // read and close-on-exec
    PARAM_STRING_APPEND_TO(flow_param, KEY_FPS, video_fps);
    PARAM_STRING_APPEND_TO(flow_param, KEY_LOOP_TIME, 0);
    LOG("\n#FileRead flow param:\n%s\n", flow_param.c_str());

    video_read_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
        flow_name.c_str(), flow_param.c_str());
    if (!video_read_flow) {
      LOG("Create flow %s failed\n", flow_name.c_str());
      exit(EXIT_FAILURE);
    }
  } else {
    //Reading yuv from camera
    flow_name = "source_stream";
    flow_param = "";
    vir_width = UPALIGNTO(video_width, 8);;
    vir_height = UPALIGNTO(video_height, 8);
    PARAM_STRING_APPEND(flow_param, KEY_NAME, "v4l2_capture_stream");
    PARAM_STRING_APPEND(flow_param, KEK_THREAD_SYNC_MODEL, KEY_SYNC);
    PARAM_STRING_APPEND(flow_param, KEK_INPUT_MODEL, KEY_DROPFRONT);
    PARAM_STRING_APPEND_TO(flow_param, KEY_INPUT_CACHE_NUM, 5);
    stream_param = "";
    PARAM_STRING_APPEND_TO(stream_param, KEY_USE_LIBV4L2, 1);
    PARAM_STRING_APPEND(stream_param, KEY_DEVICE, input_path);
    PARAM_STRING_APPEND(stream_param, KEY_V4L2_CAP_TYPE, KEY_V4L2_C_TYPE(VIDEO_CAPTURE));
    PARAM_STRING_APPEND(stream_param, KEY_V4L2_MEM_TYPE, KEY_V4L2_M_TYPE(MEMORY_DMABUF));
    PARAM_STRING_APPEND_TO(stream_param, KEY_FRAMES, 4); // if not set, default is 2
    PARAM_STRING_APPEND(stream_param, KEY_OUTPUTDATATYPE, pixel_format);
    PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_WIDTH, video_width);
    PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_HEIGHT, video_height);

    flow_param = easymedia::JoinFlowParam(flow_param, 1, stream_param);
    printf("\n#VideoCapture flow param:\n%s\n", flow_param.c_str());
    video_read_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
        flow_name.c_str(), flow_param.c_str());
    if (!video_read_flow) {
      fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
      exit(EXIT_FAILURE);
    }
  }

  flow_name = "video_enc";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_NAME, "rkmpp");
  PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE, pixel_format);
  PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE, video_enc_type);

  MediaConfig enc_config;
  memset(&enc_config, 0, sizeof(enc_config));
  VideoConfig &vid_cfg = enc_config.vid_cfg;
  ImageConfig &img_cfg = vid_cfg.image_cfg;
  img_cfg.image_info.pix_fmt = StringToPixFmt(pixel_format.c_str());
  img_cfg.image_info.width = video_width;
  img_cfg.image_info.height = video_height;
  img_cfg.image_info.vir_width = vir_width;
  img_cfg.image_info.vir_height = vir_height;
  if ((video_enc_type == VIDEO_H264)) {
    img_cfg.qp_init = 24;
    vid_cfg.qp_step = 4;
    vid_cfg.qp_min = 12;
    vid_cfg.qp_max = 48;
    vid_cfg.bit_rate = bpsmax;
    vid_cfg.frame_rate = video_fps;
    vid_cfg.level = 40;
    vid_cfg.gop_size = video_fps;
    vid_cfg.profile = 100;
    // vid_cfg.rc_quality = "aq_only"; vid_cfg.rc_mode = "vbr";
    vid_cfg.rc_quality = KEY_MEDIUM;
    vid_cfg.rc_mode = KEY_CBR;
  } else if (video_enc_type == VIDEO_H265) {
    img_cfg.qp_init = -1;
    vid_cfg.max_i_qp = 46;
    vid_cfg.min_i_qp = 24;
    vid_cfg.qp_min = 10;
    vid_cfg.qp_max = 51;
    vid_cfg.bit_rate = bpsmax;
    vid_cfg.frame_rate = video_fps;
    vid_cfg.gop_size = video_fps * 2;
    // vid_cfg.rc_quality = "aq_only"; vid_cfg.rc_mode = "vbr";
    vid_cfg.rc_quality = KEY_MEDIUM;
    vid_cfg.rc_mode = KEY_CBR;
  } else if (video_enc_type == IMAGE_JPEG)
    img_cfg.qp_init = 10;

  enc_param = "";
  enc_param.append(easymedia::to_param_string(enc_config, video_enc_type));
  flow_param = easymedia::JoinFlowParam(flow_param, 1, enc_param);
  printf("\n#VideoEncoder flow param:\n%s\n", flow_param.c_str());
  video_encoder_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!video_encoder_flow) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }

  flow_name = "file_write_flow";
  flow_param = "";
  PARAM_STRING_APPEND(flow_param, KEY_PATH, output_path.c_str());
  PARAM_STRING_APPEND(flow_param, KEY_OPEN_MODE, "w+"); // read and close-on-exec
  printf("\n#FileWrite:\n%s\n", flow_param.c_str());
  video_save_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>(
      flow_name.c_str(), flow_param.c_str());
  if (!video_save_flow) {
    fprintf(stderr, "Create flow %s failed\n", flow_name.c_str());
    exit(EXIT_FAILURE);
  }

#if 0
  //easymedia::video_encoder_enable_statistics(video_encoder_flow, 1);
#define MPP_ENC_OSD_PLT_WHITE           (uint32_t)((255<<24)|(128<<16)|(128<<8)|235)
#define MPP_ENC_OSD_PLT_YELLOW          (uint32_t)((255<<24)|(146<<16)|( 16<<8 )|210)
#define MPP_ENC_OSD_PLT_CYAN            (uint32_t)((255<<24)|( 16<<16 )|(166<<8)|170)
#define MPP_ENC_OSD_PLT_GREEN           (uint32_t)((255<<24)|( 34<<16 )|( 54<<8 )|145)
#define MPP_ENC_OSD_PLT_TRANS           (uint32_t)((  0<<24)|(222<<16)|(202<<8)|106)
#define MPP_ENC_OSD_PLT_RED             (uint32_t)((255<<24)|(240<<16)|( 90<<8 )|81)
#define MPP_ENC_OSD_PLT_BLUE            (uint32_t)((255<<24)|(110<<16)|(240<<8)|41)
#define MPP_ENC_OSD_PLT_BLACK           (uint32_t)((255<<24)|(128<<16)|(128<<8)|16)

  uint32_t plt_ori[8] = {
    MPP_ENC_OSD_PLT_WHITE,
    MPP_ENC_OSD_PLT_YELLOW,
    MPP_ENC_OSD_PLT_CYAN,
    MPP_ENC_OSD_PLT_GREEN,
    MPP_ENC_OSD_PLT_TRANS,
    MPP_ENC_OSD_PLT_RED,
    MPP_ENC_OSD_PLT_BLUE,
    MPP_ENC_OSD_PLT_BLACK
  };
  uint32_t corlor_plt[256];
  for (int i = 0; i < 256; i++) {
    corlor_plt[i] = plt_ori[i%8];
  }
  video_encoder_set_osd_plt(video_encoder_flow, corlor_plt);
#endif

  OsdRegionData region_data;
  memset(&region_data, 0, sizeof(region_data));
  region_data.enable = 1;
  region_data.region_id = 1;
  region_data.inverse = 0;
  region_data.offset_x = 0;
  region_data.offset_y = 0;
  region_data.width = 480;
  region_data.height = 32;
  sprintf(region_data.str, "%s", "YYYY-MM-DD WEEKCN TIME24");
  region_data.str_corlor = 0xFF0000;
  region_data.is_ts = 1;
  video_encoder_set_osd_region(video_encoder_flow, &region_data);

  memset(&region_data, 0, sizeof(region_data));
  region_data.enable = 1;
  region_data.region_id = 2;
  region_data.inverse = 0;
  region_data.offset_x = 0;
  region_data.offset_y = 64;
  region_data.width = 480;
  region_data.height = 32;
  sprintf(region_data.str, "%s", "CHR YYYY/MM/DD WEEK TIME12");
  region_data.str_corlor = 0xFFF000;
  region_data.is_ts = 1;
  video_encoder_set_osd_region(video_encoder_flow, &region_data);

  memset(&region_data, 0, sizeof(region_data));
  region_data.enable = 1;
  region_data.region_id = 3;
  region_data.inverse = 0;
  region_data.offset_x = 8*16;
  region_data.offset_y = 96;
  region_data.width = 12*16;
  region_data.height = 32;
  sprintf(region_data.str, "%s", "Hello RV1109");
  region_data.str_corlor = 0x0FF000;
  video_encoder_set_osd_region(video_encoder_flow, &region_data);

  memset(&region_data, 0, sizeof(region_data));
  region_data.enable = 1;
  region_data.region_id = 4;
  region_data.offset_x = 0;
  region_data.offset_y = 96;
  region_data.width = 16;
  region_data.height = 32;
  sprintf(region_data.path, "%s", "/tmp/osd_test.bmp");
  video_encoder_set_osd_region(video_encoder_flow, &region_data);

  video_encoder_flow->AddDownFlow(video_save_flow, 0, 0);
  video_read_flow->AddDownFlow(video_encoder_flow, 0, 0);

  LOG("%s initial finish\n", argv[0]);

  while(!quit) {
    easymedia::msleep(100);
  }

  video_read_flow->RemoveDownFlow(video_encoder_flow);
  video_read_flow.reset();
  video_encoder_flow->RemoveDownFlow(video_save_flow);
  video_encoder_flow.reset();
  video_save_flow.reset();

  return 0;
}
