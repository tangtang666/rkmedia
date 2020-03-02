// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _RK_OSD_PRODUCE_H_
#define _RK_OSD_PRODUCE_H_

#include <memory>
#include <vector>

#include "bmp_reader.h"
#include "osd_cache.h"
#include "osd_common.h"

class OSDProducer {
public:
  OSDProducer();
  ~OSDProducer();

  std::shared_ptr<OSDCache>
  CreateFontCaches(std::string font_path, uint32_t font_size, uint32_t color);
  std::shared_ptr<OSDCache> FindOSDCaches(std::string font_path,
                                          uint32_t font_size, uint32_t color);
  int DestoryFontCaches();

  int FillDate(osd_data_s *data);
  int FillImage(osd_data_s *data);
  int FillText(osd_data_s *data);
  int FillYuvMap(osd_data_s *data);

private:
  std::vector<std::shared_ptr<OSDCache>> osd_caches_;
};

#endif // _RK_OSD_MIDDLEWARE_H_
