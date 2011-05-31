/*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id$
 *
 * Authors:
 *   duolong <duolong@taobao.com>
 *      - initial release
 *   qushan<qushan@taobao.com> 
 *      - modify 2009-03-27
 *   duanfei <duanfei@taobao.com> 
 *      - modify 2010-04-23
 *
 */
#include <tbsys.h>
#include <Memory.hpp>
#include "oplog.h"
#include "common/error_msg.h"
#include "common/directory_op.h"
#include "common/parameter.h"

using namespace tfs::common;

namespace tfs
{
  namespace nameserver
  {
    int OpLogHeader::serialize(char* data, const int64_t data_len, int64_t& pos) const
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, seqno_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, time_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, crc_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int16(data, data_len, pos, length_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int8(data, data_len, pos, type_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int8(data, data_len, pos, reserve_);
      }
      return iret;
    }

    int OpLogHeader::deserialize(const char* data, const int64_t data_len, int64_t& pos)
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, reinterpret_cast<int32_t*>(&seqno_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, reinterpret_cast<int32_t*>(&time_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, reinterpret_cast<int32_t*>(&crc_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int16(data, data_len, pos, reinterpret_cast<int16_t*>(&length_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int8(data, data_len, pos, &type_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int8(data, data_len, pos, &reserve_);
      }
      return iret;
    }
    int64_t OpLogHeader::length() const
    {
      return common::INT_SIZE * 4;
    }
    int OpLogRotateHeader::serialize(char* data, const int64_t data_len, int64_t& pos) const
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, seqno_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, rotate_seqno_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, rotate_offset_);
      }
      return iret;
    }
    int OpLogRotateHeader::deserialize(const char* data, const int64_t data_len, int64_t& pos)
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, reinterpret_cast<int32_t*>(&seqno_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, reinterpret_cast<int32_t*>(&rotate_seqno_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &rotate_offset_);
      }
      return iret;
    }
    int64_t OpLogRotateHeader::length() const
    {
      return common::INT_SIZE * 3;
    }
    void BlockOpLog::dump(void) const
    {
      /*std::string dsstr = OpLogSyncManager::printDsList(ds_list);
      TBSYS_LOG(DEBUG, "cmd(%s), id(%u) version(%u) file_count(%u) size(%u) delfile_count(%u) del_size(%u) seqno(%u), ds_size(%u), dataserver(%s)",
          cmd == OPLOG_INSERT ? "insert" : cmd == OPLOG_REMOVE ? "remove" : cmd == OPLOG_RELEASE_RELATION ? "release" : "update",
          block_info.id, block_info.version, block_info.file_count, block_info.size, block_info.delfile_count, block_info.del_size,
          block_info.seqno, ds_list.size(), dsstr.c_str());*/
    }

    int BlockOpLog::serialize(char* data, const int64_t data_len, int64_t& pos) const
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int8(data, data_len, pos, cmd_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = info_.serialize(data, data_len, pos);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int8(data, data_len, pos, blocks_.size());
      }
      if (TFS_SUCCESS == iret)
      {
        std::vector<uint32_t>::const_iterator iter = blocks_.begin();
        for (; iter != blocks_.end(); ++iter)
        {
          iret =  Serialization::set_int32(data, data_len, pos, (*iter));
          if (TFS_SUCCESS != iret)
            break;
        }
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int8(data, data_len, pos, servers_.size());
      }
      if (TFS_SUCCESS == iret)
      {
        std::vector<uint64_t>::const_iterator iter = servers_.begin();
        for (; iter != servers_.end(); ++iter)
        {
          iret =  Serialization::set_int64(data, data_len, pos, (*iter));
          if (TFS_SUCCESS != iret)
            break;
        }
      }
      return iret;
    }

    int BlockOpLog::deserialize(const char* data, const int64_t data_len, int64_t& pos)
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int8(data, data_len, pos, &cmd_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = info_.deserialize(data, data_len, pos);
      }
      if (TFS_SUCCESS == iret)
      {
        int8_t size = 0;
        iret = Serialization::get_int8(data, data_len, pos, &size);
        if (TFS_SUCCESS == iret)
        {
          uint32_t block_id = 0;
          for (int8_t i = 0; i < size; ++i)
          {
            iret = Serialization::get_int32(data, data_len, pos, reinterpret_cast<int32_t*>(&block_id));
            if (TFS_SUCCESS == iret)
              blocks_.push_back(block_id);
            else
              break;
          }
        }
      }

      if (TFS_SUCCESS == iret)
      {
        int8_t size = 0;
        iret = Serialization::get_int8(data, data_len, pos, &size);
        if (TFS_SUCCESS == iret)
        {
          uint64_t server = 0;
          for (int8_t i = 0; i < size; ++i)
          {
            iret = Serialization::get_int64(data, data_len, pos, reinterpret_cast<int64_t*>(&server));
            if (TFS_SUCCESS == iret)
              servers_.push_back(server);
            else
              break;
          }
        }
      }
      return iret;
    }

    int64_t BlockOpLog::length(void) const
    {
      return INT8_SIZE + info_.length() + INT8_SIZE + blocks_.size() * INT_SIZE + INT8_SIZE + servers_.size() * INT64_SIZE;
    }

    OpLog::OpLog(const std::string& logname, int maxLogSlotsSize) :
      MAX_LOG_SLOTS_SIZE(maxLogSlotsSize), MAX_LOG_BUFFER_SIZE(maxLogSlotsSize * MAX_LOG_SIZE), path_(logname), seqno_(
          0), last_flush_time_(0), slots_offset_(0), fd_(-1), buffer_(new char[maxLogSlotsSize * MAX_LOG_SIZE + 1])
    {
      memset(buffer_, 0, maxLogSlotsSize * MAX_LOG_SIZE + 1); 
      memset(&oplog_rotate_header_, 0, sizeof(oplog_rotate_header_));
    }

    OpLog::~OpLog()
    {
      tbsys::gDeleteA( buffer_);
      if (fd_ > 0)
        ::close( fd_);
    }

    int OpLog::initialize()
    {
      int32_t iret = path_.empty() ? EXIT_GENERAL_ERROR : TFS_SUCCESS;
      if (TFS_SUCCESS == iret)
      {
        if (!DirectoryOp::create_full_path(path_.c_str()))
        {
          TBSYS_LOG(ERROR, "create directory(%s) fail...", path_.c_str());
          iret = EXIT_GENERAL_ERROR;
        }
        else
        {
          std::string headPath = path_ + "/rotateheader.dat";
          fd_ = open(headPath.c_str(), O_RDWR | O_CREAT, 0600);
          if (fd_ < 0)
          {
            TBSYS_LOG(ERROR, "open file(%s) fail(%s)", headPath.c_str(), strerror(errno));
            iret = EXIT_GENERAL_ERROR;
          }
          else
          {
            char buf[oplog_rotate_header_.length()];
            const int64_t length = read(fd_, buf, oplog_rotate_header_.length());
            if (length != oplog_rotate_header_.length())
            {
              oplog_rotate_header_.rotate_seqno_ = 0x01;
              oplog_rotate_header_.rotate_offset_ = 0x00;
            }
            else
            {
              int64_t pos = 0;
              oplog_rotate_header_.deserialize(buf, oplog_rotate_header_.length(), pos);
              if (TFS_SUCCESS != iret)
              {
                oplog_rotate_header_.rotate_seqno_ = 0x01;
                oplog_rotate_header_.rotate_offset_ = 0x00;
              }
            }
          }
        }
      }
      return iret;
    }

    int OpLog::update_oplog_rotate_header(const OpLogRotateHeader& head)
    {
      tbutil::Mutex::Lock lock(mutex_);
      std::string headPath = path_ + "/rotateheader.dat";
      memcpy(&oplog_rotate_header_, &head, sizeof(head));
      int32_t iret = TFS_SUCCESS;
      if (fd_ < 0)
      {
        fd_ = open(headPath.c_str(), O_RDWR | O_CREAT, 0600);
        if (fd_ < 0)
        {
          TBSYS_LOG(ERROR, "open file(%s) fail(%s)", headPath.c_str(), strerror(errno));
          iret = EXIT_GENERAL_ERROR;
        }
      }
      if (TFS_SUCCESS == iret)
      {
        lseek(fd_, 0, SEEK_SET);
        int64_t pos = 0;
        char buf[oplog_rotate_header_.length()];
        memset(buf, 0, sizeof(buf));
        iret = oplog_rotate_header_.serialize(buf, oplog_rotate_header_.length(), pos);
        if (TFS_SUCCESS == iret)
        {
          int64_t length = ::write(fd_, buf, oplog_rotate_header_.length());
          if (length != oplog_rotate_header_.length())
          {
            TBSYS_LOG(ERROR, "wirte data fail: file(%s), erros(%s)...", headPath.c_str(), strerror(errno));
            ::close( fd_);
            fd_ = -1;
            fd_ = open(headPath.c_str(), O_RDWR | O_CREAT, 0600);
            if (fd_ < 0)
            {
              TBSYS_LOG(ERROR, "open file(%s) fail(%s)", headPath.c_str(), strerror(errno));
              iret = EXIT_GENERAL_ERROR;
            }
            else
            {
              lseek(fd_, 0, SEEK_SET);
              length = ::write(fd_, buf, oplog_rotate_header_.length());
              if (length != oplog_rotate_header_.length())
              {
                TBSYS_LOG(ERROR, "wirte data fail: file(%s), erros(%s)...", headPath.c_str(), strerror(errno));
                iret = EXIT_GENERAL_ERROR;
              }
            }
          }
        }
      }
      return iret;
    }

    bool OpLog::finish(time_t now, bool force/* = false*/) const
    {
      if (!force)
      {
        if ((slots_offset_ < MAX_LOG_BUFFER_SIZE) && (MAX_LOG_SLOTS_SIZE != 0))
        {
          return false;
        }
      }
      if (now - last_flush_time_ < (time_t)(SYSPARAM_NAMESERVER.heart_interval_ * 4))
        return false;
      return true;
    }

    int OpLog::write(uint8_t type, const char* const data, const int32_t length)
    {
      if ((NULL == data)
          || (length <= 0)
          || (length > OpLog::MAX_LOG_SIZE))
      {
        return -1;
      }
      const int32_t size = sizeof(OpLogHeader) + length;
      const int32_t dope_offset = slots_offset_ + size;
      if (dope_offset > MAX_LOG_BUFFER_SIZE)
      {
        TBSYS_LOG(DEBUG, "(slots_offset_ + size)(%d) > MAX_LOG_BUFFER_SIZE(%d)",
            dope_offset, MAX_LOG_BUFFER_SIZE);
        return EXIT_SLOTS_OFFSET_SIZE_ERROR;
      }

      char* const buffer = buffer_ + slots_offset_;
      OpLogHeader* header = (OpLogHeader*)buffer;
      header->crc_  = 0;
      header->crc_  = Func::crc(header->crc_, (char*)data, length);
      header->time_ = time(NULL);
      header->length_ = length;
      header->seqno_  = ++seqno_;
      header->type_ = type;
      memcpy(header->data_, data, length);
      slots_offset_ += size;
      return TFS_SUCCESS;
    }
  }//end namespace nameserver
}//end namespace tfs
