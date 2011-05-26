/*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id: internal.h 311 2011-05-18 05:38:41Z duanfei@taobao.com $
 *
 * Authors:
 *   duolong <duolong@taobao.com>
 *      - initial release
 *
 */

#include "define.h"
#include "internal.h"
#include "serialization.h"

namespace tfs
{
  namespace common
  {
    int FileInfo::serialize(char* data, const int64_t data_len, int64_t& pos) const
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int64(data, data_len, pos, id_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, offset_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, size_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, usize_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, modify_time_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, create_time_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, flag_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, crc_);
      }
      return iret;
    }

    int FileInfo::deserialize(const char*data, const int64_t data_len, int64_t& pos)
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int64(data, data_len, pos, reinterpret_cast<int64_t*>(&id_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &offset_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &size_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &usize_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &modify_time_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &create_time_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &flag_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, reinterpret_cast<int32_t*>(&crc_));
      }
      return iret;
    }

    int64_t FileInfo::length() const
    {
      return INT64_SIZE  + INT_SIZE  * 7;
    }

    int SSMScanParameter::serialize(char* data, const int64_t data_len, int64_t& pos ) const
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, addition_param1_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, addition_param2_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, start_next_position_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, should_actual_count_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int16(data, data_len, pos, child_type_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int8(data, data_len, pos, type_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int8(data, data_len, pos, end_flag_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, data_.getDataLen());
      }
      if (TFS_SUCCESS == iret)
      {
        if (data_.getDataLen())
        {
          iret = Serialization::set_bytes(data, data_len, pos, data_.getData(), data_.getDataLen());
        }
      }
      return iret;
    }
    int SSMScanParameter::deserialize(const char* data, const int64_t data_len, int64_t& pos)
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, reinterpret_cast<int32_t*>(&addition_param1_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, reinterpret_cast<int32_t*>(&addition_param2_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, reinterpret_cast<int32_t*>(&start_next_position_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, reinterpret_cast<int32_t*>(&should_actual_count_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int16(data, data_len, pos, &child_type_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int8(data, data_len, pos, &type_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int8(data, data_len, pos, &end_flag_);
      }
      int32_t len = 0;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &len);
      }
      if (TFS_SUCCESS == iret)
      {
        if (len > 0)
        {
          data_.ensureFree(data_len);
          data_.pourData(data_len);
          iret = Serialization::get_bytes(data, data_len, pos, data_.getData(), len);
        }
      }
      return iret;
    }

    int64_t SSMScanParameter::length() const
    {
      return INT_SIZE * 6 + data_.getDataLen();
    }

    int BlockInfo::serialize(char* data, const int64_t data_len, int64_t& pos) const
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, block_id_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, version_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, file_count_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, size_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, del_file_count_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, del_size_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, seq_no_);
      }
      return iret;
    }

    int BlockInfo::deserialize(const char* data, const int64_t data_len, int64_t& pos)
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, reinterpret_cast<int32_t*>(&block_id_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &version_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &file_count_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &size_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &del_file_count_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &del_size_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, reinterpret_cast<int32_t*>(&seq_no_));
      }
      return iret;
    }

    int64_t BlockInfo::length() const
    {
      return INT_SIZE * 7;
    }
    int RawMeta::deserialize(const char* data, const int64_t data_len, int64_t& pos)
    {

      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int64(data, data_len, pos, reinterpret_cast<int64_t*>(&fileid_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &location_.inner_offset_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &location_.size_);
      }
      return iret;
    }
    int RawMeta::serialize(char* data, const int64_t data_len, int64_t& pos) const
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int64(data, data_len, pos, fileid_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, location_.inner_offset_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, location_.size_);
      }
      return iret;
    }
    int64_t RawMeta::length() const
    {
      return INT64_SIZE + INT_SIZE * 2;
    }

    int ReplBlock::serialize(char* data, const int64_t data_len, int64_t& pos) const
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, block_id_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int64(data, data_len, pos, source_id_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int64(data, data_len, pos, destination_id_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, start_time_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, is_move_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, server_count_);
      }
      return iret;
    }

    int ReplBlock::deserialize(const char* data, const int64_t data_len, int64_t& pos)
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, reinterpret_cast<int32_t*>(&block_id_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int64(data, data_len, pos, reinterpret_cast<int64_t*>(&source_id_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int64(data, data_len, pos, reinterpret_cast<int64_t*>(&destination_id_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &start_time_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &is_move_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &server_count_);
      }
      return iret;
    }

    int64_t ReplBlock::length() const
    {
      return  INT_SIZE * 4 + INT64_SIZE * 2;
    }

    int Throughput::deserialize(const char* data, const int64_t data_len, int64_t& pos)
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int64(data, data_len, pos, reinterpret_cast<int64_t*>(&write_byte_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int64(data, data_len, pos, reinterpret_cast<int64_t*>(&write_file_count_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int64(data, data_len, pos, reinterpret_cast<int64_t*>(&read_byte_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int64(data, data_len, pos, reinterpret_cast<int64_t*>(&read_file_count_));
      }
      return iret;
    }
    int Throughput::serialize(char* data, const int64_t data_len, int64_t& pos) const
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int64(data, data_len, pos, write_byte_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int64(data, data_len, pos, write_file_count_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int64(data, data_len, pos, read_byte_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int64(data, data_len, pos, read_file_count_);
      }
      return iret;
    }

    int64_t Throughput::length() const
    {
      return INT64_SIZE * 4;
    }
    int DataServerStatInfo::deserialize(const char* data, const int64_t data_len, int64_t& pos)
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int64(data, data_len, pos, reinterpret_cast<int64_t*>(&id_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int64(data, data_len, pos, &use_capacity_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int64(data, data_len, pos, &total_capacity_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &current_load_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &block_count_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &last_update_time_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &startup_time_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = total_tp_.deserialize(data, data_len, pos);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &current_time_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, reinterpret_cast<int32_t*>(&status_));
      }
      return iret;
    }
    int DataServerStatInfo::serialize(char* data, const int64_t data_len, int64_t& pos) const
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int64(data, data_len, pos, id_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int64(data, data_len, pos, use_capacity_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int64(data, data_len, pos, total_capacity_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, current_load_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, block_count_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, last_update_time_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, startup_time_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = total_tp_.serialize(data, data_len, pos);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, current_time_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, status_);
      }
      return iret;
    }
    int64_t DataServerStatInfo::length() const
    {
      return  INT64_SIZE * 3 + total_tp_.length() + INT_SIZE * 6;
    }
    int WriteDataInfo::deserialize(const char* data, const int64_t data_len, int64_t& pos)
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, reinterpret_cast<int32_t*>(&block_id_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int64(data, data_len, pos, reinterpret_cast<int64_t*>(&file_id_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &offset_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &length_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, reinterpret_cast<int32_t*>(&is_server_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int64(data, data_len, pos, reinterpret_cast<int64_t*>(&file_number_));
      }
      return iret;
    }
    int WriteDataInfo::serialize(char* data, const int64_t data_len, int64_t& pos) const
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, block_id_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int64(data, data_len, pos, file_id_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, offset_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, length_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, is_server_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int64(data, data_len, pos, file_number_);
      }
      return iret;
    }
    int64_t WriteDataInfo::length() const
    {
      return INT_SIZE * 4 + INT64_SIZE * 2;
    }

    int CloseFileInfo::deserialize(const char* data, const int64_t data_len, int64_t& pos)
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, reinterpret_cast<int32_t*>(&block_id_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int64(data, data_len, pos, reinterpret_cast<int64_t*>(&file_id_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, reinterpret_cast<int32_t*>(&mode_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, reinterpret_cast<int32_t*>(&crc_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int64(data, data_len, pos, reinterpret_cast<int64_t*>(&file_number_));
      }
      return iret;
    }
    int CloseFileInfo::serialize(char* data, const int64_t data_len, int64_t& pos) const
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, block_id_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int64(data, data_len, pos, file_id_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, mode_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, crc_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int64(data, data_len, pos, file_number_);
      }
      return iret;
    }
    int64_t CloseFileInfo::length() const
    {
      return INT_SIZE * 3 + INT64_SIZE * 2;
    }

    int RenameFileInfo::deserialize(const char* data, const int64_t data_len, int64_t& pos)
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, reinterpret_cast<int32_t*>(&block_id_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int64(data, data_len, pos, reinterpret_cast<int64_t*>(&file_id_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int64(data, data_len, pos, reinterpret_cast<int64_t*>(&new_file_id_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, reinterpret_cast<int32_t*>(&is_server_));
      }
      return iret;
    }

    int RenameFileInfo::serialize(char* data, const int64_t data_len, int64_t& pos) const
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, block_id_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int64(data, data_len, pos, file_id_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int64(data, data_len, pos, new_file_id_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int64(data, data_len, pos, is_server_);
      }
      return iret;
    }
    int64_t RenameFileInfo::length() const
    {
      return INT_SIZE * 2 + INT64_SIZE * 2;
    }
    int ServerMetaInfo::deserialize(const char* data, const int64_t data_len, int64_t& pos)
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &capacity_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &available_);
      }
      return iret;
    }
    int ServerMetaInfo::serialize(char* data, const int64_t data_len, int64_t& pos) const
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, capacity_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, available_);
      }
      return iret;
    }

    int64_t ServerMetaInfo::length() const
    {
      return INT_SIZE * 2;
    }
    int SegmentHead::deserialize(const char* data, const int64_t data_len, int64_t& pos)
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &count_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int64(data, data_len, pos, &size_);
      }
      return iret;
 
    }
    int SegmentHead::serialize(char* data, const int64_t data_len, int64_t& pos) const
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, count_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int64(data, data_len, pos, size_);
      }
      return iret; 
    }
    int64_t SegmentHead::length() const
    {
      return INT_SIZE + INT64_SIZE;
    } 
    int SegmentInfo::deserialize(const char* data, const int64_t data_len, int64_t& pos)
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, reinterpret_cast<int32_t*>(&block_id_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int64(data, data_len, pos, reinterpret_cast<int64_t*>(&file_id_));
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int64(data, data_len, pos, &offset_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &size_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &crc_);
      }
      return iret;
    }
    int SegmentInfo::serialize(char* data, const int64_t data_len, int64_t& pos) const
    {
      int32_t iret = NULL != data && data_len - pos >= length() ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, block_id_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int64(data, data_len, pos, file_id_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int64(data, data_len, pos, offset_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, size_);
      }
      if (TFS_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, crc_);
      }
      return iret;
    }
    int64_t SegmentInfo::length() const
    {
      return INT_SIZE * 3 + INT64_SIZE * 2;
    }
  }
}
