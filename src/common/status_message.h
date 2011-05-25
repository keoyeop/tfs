/*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id: status_packet.h 186 2011-04-28 16:07:20Z duanfei@taobao.com $
 *
 * Authors:
 *   duanfei <duanfei@taobao.com>
 *      - initial release
 *
 */
#ifndef TFS_COMMON_STATUS_PACKET_H_
#define TFS_COMMON_STATUS_PACKET_H_
#include "base_packet.h"

namespace tfs
{
  namespace common 
  {
    class StatusMessage: public BasePacket 
    {
    public:
      StatusMessage();
      StatusMessage(const int32_t status, const char* const str = NULL);
      void set_message(const int32_t status, const char* const str = NULL);
      virtual ~StatusMessage();
      int serialize(Stream& output);
      int deserialize(Stream& input);
      int64_t length() const;
      const char* get_error() const;
      int32_t get_status() const;

      void dump() const;

      static BasePacket* create(const int32_t type);
    private:
      char msg_[MAX_ERROR_MSG_LENGTH];
      int32_t length_;
      int32_t status_;
    };
  }
}
#endif
