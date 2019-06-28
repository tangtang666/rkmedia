/*
 * Copyright (C) 2019 Hertz Wang 1989wanghang@163.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see http://www.gnu.org/licenses
 *
 * Any non-GPL usage of this software or parts of this software is strictly
 * forbidden.
 *
 */

#include "v4l2_utils.h"

#include <assert.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#ifdef HAVE_LIBV4L2
#include <libv4l2.h>
#endif

namespace easymedia {

static const struct V4L2FmtStringEntry {
  __u32 fmt;
  const char *type_str;
} v4l2_fmt_string_map[] = {{V4L2_PIX_FMT_YUV420, IMAGE_YUV420P},
                           {V4L2_PIX_FMT_NV12, IMAGE_NV12},
                           {V4L2_PIX_FMT_NV21, IMAGE_NV21},
                           {V4L2_PIX_FMT_YUV422P, IMAGE_YUV422P},
                           {V4L2_PIX_FMT_NV16, IMAGE_NV16},
                           {V4L2_PIX_FMT_NV61, IMAGE_NV61},
                           {V4L2_PIX_FMT_YUYV, IMAGE_YUYV422},
                           {V4L2_PIX_FMT_UYVY, IMAGE_UYVY422},
                           {V4L2_PIX_FMT_SRGGB8, IMAGE_RGB332},
                           {V4L2_PIX_FMT_RGB565, IMAGE_RGB565},
                           {V4L2_PIX_FMT_RGB24, IMAGE_RGB888},
                           {V4L2_PIX_FMT_BGR24, IMAGE_BGR888},
                           {V4L2_PIX_FMT_ARGB32, IMAGE_ARGB8888},
                           {V4L2_PIX_FMT_ABGR32, IMAGE_ABGR8888},
                           {V4L2_PIX_FMT_MJPEG, IMAGE_JPEG},
                           {V4L2_PIX_FMT_H264, VIDEO_H264}};

__u32 GetV4L2FmtByString(const char *type) {
  if (!type)
    return 0;
  for (size_t i = 0; i < ARRAY_ELEMS(v4l2_fmt_string_map) - 1; i++) {
    if (!strcmp(type, v4l2_fmt_string_map[i].type_str))
      return v4l2_fmt_string_map[i].fmt;
  }
  return 0;
}

class _V4L2_SUPPORT_TYPES : public SupportMediaTypes {
public:
  _V4L2_SUPPORT_TYPES() {
    for (size_t i = 0; i < ARRAY_ELEMS(v4l2_fmt_string_map) - 1; i++) {
      types.append(v4l2_fmt_string_map[i].type_str);
    }
  }
};
static _V4L2_SUPPORT_TYPES priv_types;
const std::string &GetStringOfV4L2Fmts() { return priv_types.types; }

__u32 GetV4L2Type(const char *v4l2type) {
  if (!v4l2type)
    return 0;
  if (!strcmp(v4l2type, KEY_V4L2_C_TYPE(VIDEO_CAPTURE)))
    return V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (!strcmp(v4l2type, KEY_V4L2_C_TYPE(VIDEO_OUTPUT)))
    return V4L2_BUF_TYPE_VIDEO_OUTPUT;

  if (!strcmp(v4l2type, KEY_V4L2_M_TYPE(MEMORY_MMAP)))
    return V4L2_MEMORY_MMAP;
  if (!strcmp(v4l2type, KEY_V4L2_M_TYPE(MEMORY_USERPTR)))
    return V4L2_MEMORY_USERPTR;
  if (!strcmp(v4l2type, KEY_V4L2_M_TYPE(MEMORY_OVERLAY)))
    return V4L2_MEMORY_OVERLAY;
  if (!strcmp(v4l2type, KEY_V4L2_M_TYPE(MEMORY_DMABUF)))
    return V4L2_MEMORY_DMABUF;

  return 0;
}

bool SetV4L2IoFunction(v4l2_io *vio, bool use_libv4l2) {
#define SET_WRAPPERS(prefix)                                                   \
  do {                                                                         \
    vio->open_f = prefix##open;                                                \
    vio->close_f = prefix##close;                                              \
    vio->dup_f = prefix##dup;                                                  \
    vio->ioctl_f = prefix##ioctl;                                              \
    vio->read_f = prefix##read;                                                \
    vio->mmap_f = prefix##mmap;                                                \
    vio->munmap_f = prefix##munmap;                                            \
  } while (0)

  if (use_libv4l2) {
#ifdef HAVE_LIBV4L2
    SET_WRAPPERS(v4l2_);
#else
    LOG("libv4l2 is not configured.\n");
    return false;
#endif
  } else {
    SET_WRAPPERS();
  }
  return true;
}

int V4L2IoCtl(v4l2_io *vio, int fd, unsigned long int request, void *arg) {
  assert(vio->ioctl_f);
  return xioctl(vio->ioctl_f, fd, request, arg);
}

} // namespace easymedia