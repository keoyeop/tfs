#include "local_key.h"
#include "common/directory_op.h"
#include "common/error_msg.h"
#include <unistd.h>

using namespace tfs::client;
using namespace tfs::common;

LocalKey::LocalKey() : file_op_(NULL)
{
}

LocalKey::~LocalKey()
{
  tbsys::gDelete(file_op_);
  destroy_info();
}

int LocalKey::initialize(const char* local_key, const uint64_t addr)
{
  const static char* g_tmp_path = "/tmp/TFSlocalkeyDIR/";
  int ret = TFS_SUCCESS;

  if (!DirectoryOp::create_full_path(g_tmp_path))
  {
    TBSYS_LOG(ERROR, "initialize local key fail, create directory %s failed, error: %d",
              g_tmp_path, strerror(errno));
    ret = EXIT_GENERAL_ERROR;
  }
  else
  {
    char name[MAX_PATH_LENGTH];
    strncpy(name, g_tmp_path, MAX_PATH_LENGTH - 1);
    char* tmp_file = name + strlen(g_tmp_path);

    if (NULL == realpath(local_key, tmp_file))
    {
      TBSYS_LOG(ERROR, "initialize local key %s fail: %s", local_key, strerror(errno));
      ret = TFS_ERROR;
    }
    else
    {
      // convert tmp file name
      char* pos = NULL;
      while ((pos = strchr(tmp_file, '/')))
      {
        tmp_file = pos;
        *pos = '!';
      }
      snprintf(name + strlen(name), MAX_PATH_LENGTH - strlen(name), "%" PRI64_PREFIX "u", addr);

      memset(&seg_head_, 0, sizeof(SegmentHead));
      seg_info_.clear();
      int is_exist = access(name, F_OK);
      if (0 != is_exist) //not exist
      {
        file_op_ = new FileOperation(name, O_RDWR|O_CREAT);
      }
      else
      {
        file_op_ = new FileOperation(name, O_RDWR);
        ret = load();
      }
    }
  }
  return ret;
}

int LocalKey::load()
{
  int ret = TFS_SUCCESS;
  if (NULL == file_op_)
  {
    TBSYS_LOG(ERROR, "local key file path not initialize");
    ret = TFS_ERROR;
  }

  if (TFS_SUCCESS == ret)
  {
    if ((ret = file_op_->pread_file(reinterpret_cast<char*>(&seg_head_), sizeof(SegmentHead), 0)) != TFS_SUCCESS)
    {
      TBSYS_LOG(ERROR, "load segment head fail, ret: %d", ret);
    }
    else
    {
      TBSYS_LOG(INFO, "load segment count %d, size: %"PRI64_PREFIX"d", seg_head_.count_, seg_head_.size_);

      char* buf = new char[sizeof(SegmentInfo)*seg_head_.count_];
      if ((ret = file_op_->pread_file(buf, sizeof(SegmentInfo) * seg_head_.count_, sizeof(SegmentHead))) != TFS_SUCCESS)
      {
        TBSYS_LOG(ERROR, "load segment info fail, ret: %d", ret);
      }
      else
      {
        ret = load_segment(buf);
      }
      tbsys::gDelete(buf);
    }
  }
  return ret;
}

int LocalKey::load(const char* buf)
{
  load_head(buf);
  return load_segment(buf + sizeof(SegmentHead));
}

int LocalKey::add_segment(SegmentInfo& seg_info)
{
  int ret = seg_info_.insert(seg_info).second ? TFS_SUCCESS : TFS_ERROR;
  if (TFS_SUCCESS == ret)
  {
    seg_head_.count_++;
    seg_head_.size_ += seg_info.size_;
    TBSYS_LOG(DEBUG, "add segment successful. blockid: %u, fileid: %"PRI64_PREFIX"u, offset: %"PRI64_PREFIX"d, size: %d, crc: %u",
              seg_info.block_id_, seg_info.file_id_, seg_info.offset_, seg_info.size_, seg_info.crc_);
  }
  else
  {
    TBSYS_LOG(ERROR, "add segment fail. blockid: %u, fileid: %"PRI64_PREFIX"u, offset: %"PRI64_PREFIX"d, size: %d, crc: %u",
              seg_info.block_id_, seg_info.file_id_, seg_info.offset_, seg_info.size_, seg_info.crc_);
  }
  return ret;
}

// check segment info sequencial and completed
int LocalKey::validate()
{
  int ret = TFS_SUCCESS;
  if (static_cast<size_t>(seg_head_.count_) != seg_info_.size())
  {
    TBSYS_LOG(ERROR, "segment count conflict with head meta info count: %d <> %d",
              seg_head_.count_, seg_info_.size());
    ret = TFS_ERROR;
  }
  else
  {
    int64_t size = 0;
    if (seg_info_.size() > 0)   // not empty
    {
      SEG_SET_ITER it = seg_info_.begin();
      if (it->offset_ != 0)
      {
        TBSYS_LOG(ERROR, "segment info offset not start with 0: %"PRI64_PREFIX"d", it->offset_);
        ret = TFS_ERROR;
      }
      else
      {
        SEG_SET_ITER nit = it;
        nit++;
        size += it->size_;

        for (; nit != seg_info_.end(); ++it, ++nit)
        {
          if (it->offset_ + it->size_ != nit->offset_)
          {
            TBSYS_LOG(ERROR, "segment info conflict: (offset + size != next_offset) %"PRI64_PREFIX"d + %d != %"PRI64_PREFIX"d", it->offset_, it->size_, nit->offset_);
            ret = TFS_ERROR;
            break;
          }
          size += nit->size_;
        }
      }
    }

    if (TFS_SUCCESS == ret && size != seg_head_.size_)
    {
      TBSYS_LOG(ERROR, "segment size conflict with head meta info size: %"PRI64_PREFIX"d <> %"PRI64_PREFIX"d",
                size, seg_head_.size_);
      ret = TFS_ERROR;
    }
  }
  return ret;
}

int LocalKey::save()
{
  int ret = TFS_SUCCESS;

  if (NULL == file_op_)
  {
    TBSYS_LOG(ERROR, "local save file path not initialize");
    ret = TFS_ERROR;
  }
  else
  {
    int32_t size = get_data_size();
    char* buf = new char[size];
    dump_data(buf);

    if ((ret = file_op_->pwrite_file(buf, size, 0)) != TFS_SUCCESS)
    {
      TBSYS_LOG(ERROR, "save segment info fail, count: %d, raw size: %d, file size: %"PRI64_PREFIX"d, ret: %d",
                seg_info_.size(), size, seg_head_.size_, ret);
    }
    else
    {
      TBSYS_LOG(INFO, "save segment info successful, count: %d, raw size: %d, file size: %"PRI64_PREFIX"d",
                seg_info_.size(), size, seg_head_.size_, ret);
      file_op_->flush_file();
    }

    tbsys::gDelete(buf);
  }
  return ret;
}

int LocalKey::over()
{
  seg_head_.over_ = LOCAL_KEY_OVER;
  int ret = TFS_SUCCESS;
  if ((ret = file_op_->pwrite_file(reinterpret_cast<const char*>(&seg_head_), sizeof(SegmentHead), 0)) != TFS_SUCCESS)
  {
    TBSYS_LOG(ERROR, "done over fail, ret: %d", ret);
  }
  else
  {
    TBSYS_LOG(INFO, "done over success, ret: %d", ret);
    file_op_->flush_file();
  }
  return ret;
}

int LocalKey::remove()
{
  return file_op_->unlink_file();
}

int32_t LocalKey::get_segment_size()
{
  return seg_head_.count_;
}

int64_t LocalKey::get_file_size()
{
  return seg_head_.size_;
}

int32_t LocalKey::get_data_size()
{
  return sizeof(SegmentHead) + seg_info_.size() * sizeof(SegmentInfo);
}

int LocalKey::dump_data(char* buf)
{
  memcpy(buf, &seg_head_, sizeof(SegmentHead));
  char* pos = buf + sizeof(SegmentHead);
  SEG_SET_ITER it;
  for (it = seg_info_.begin(); it != seg_info_.end(); ++it)
  {
    memcpy(pos, &(*it), sizeof(SegmentInfo));
    pos += sizeof(SegmentInfo);
  }
  return TFS_SUCCESS;
}

int LocalKey::get_segment_for_write(const int64_t offset, const char* buf,
                                    int64_t size, SEG_DATA_LIST& seg_list)
{
  int64_t cur_offset = offset, next_offset = offset, remain_size = size, last_remain_size = size;
  const char* cur_buf = buf;
  SegmentInfo seg_info;
  SEG_SET_ITER it;

  while (remain_size > 0)
  {
    last_remain_size = remain_size;
    seg_info.offset_ = next_offset = cur_offset;

    if ((it = seg_info_.lower_bound(seg_info)) == seg_info_.end()) // cur_offset ~ end hasn't been written
    {
      next_offset = cur_offset + remain_size;
    }
    else
    {
      if (it->offset_ == cur_offset) // current segment's offset+size has written
      {
        remain_size -= it->size_;
      }
      else
      {
        if (seg_info_.begin() == it) // offset ~ least_offset hasn't written. actually only first loop may occur
        {
          next_offset = it->offset_;
        }
        else
        {
          SEG_SET_ITER pre_it = it;
          pre_it--;
          if (pre_it->offset_ + pre_it->size_ > cur_offset) // overlap
          {
            remain_size -= pre_it->size_ - (cur_offset - pre_it->offset_);
          }
          else                  // cutover. hole in pre and cur
          {
            next_offset = it->offset_;
          }
        }
      }
    }
    if (remain_size > 0)
    {
      get_segment(cur_offset, next_offset, cur_buf, remain_size, seg_list);
      cur_buf += last_remain_size - remain_size;
      cur_offset += last_remain_size - remain_size;
    }
  }
  return TFS_SUCCESS;
}

int LocalKey::get_segment_for_read(const int64_t offset, const char* buf,
                                   const int64_t size, SEG_DATA_LIST& seg_list)
{
  SegmentInfo seg_info;
  seg_info.offset_ = offset;
  SEG_SET_ITER it = seg_info_.lower_bound(seg_info);

  if (seg_info_.end() == it)
  {
    TBSYS_LOG(ERROR, "can not find meta info for offset: %"PRI64_PREFIX"d", offset);
    return TFS_ERROR;
  }

  int64_t check_size = 0, cur_size = 0;
  SegmentData* seg_data = NULL;

  // To read, segment info SHOULD and MUST be adjacent and completed
  // but not check here ...

  if (it->offset_ != offset)    // offset found in previous segment middle
  {
    if (seg_info_.begin() == it) // should never happen: queried offset less than least offset in stored segment info
    {
      TBSYS_LOG(ERROR, "can not find meta info for offset: %"PRI64_PREFIX"d", offset);
      return TFS_ERROR;
    }
    else                        // found previous segment middle, get info
    {
      SEG_SET_ITER pre_it = it;
      --pre_it;
      check_size += pre_it->size_ - (offset - pre_it->offset_);
      seg_data = new SegmentData();
      seg_data->buf_ = const_cast<char*>(buf);
      seg_data->seg_info_ = *pre_it;
      seg_data->seg_info_.size_ = check_size; // real size
      seg_data->seg_info_.offset_ = offset; // real offset

      seg_list.push_back(seg_data);
    }
  }

  // get following adjacent segment info
  for (; it != seg_info_.end() && check_size < size; check_size += cur_size, ++it)
  {
    if (check_size + it->size_ > size)
    {
      cur_size = size - check_size;
    }
    else
    {
      cur_size = it->size_;
    }

    seg_data = new SegmentData();
    seg_data->seg_info_ = *it;
    seg_data->seg_info_.size_ = cur_size;
    seg_data->buf_ = const_cast<char*>(buf) + check_size;

    seg_list.push_back(seg_data);
  }

  return TFS_SUCCESS;
}

void LocalKey::get_segment(const int64_t start, const int64_t end,
                         const char* buf, int64_t& size, SEG_DATA_LIST& seg_list)
{
  if (start < end)
  {
    int64_t offset = start, cur_size = 0, check_size = 0;
    SegmentData* seg_data = NULL;
    bool not_end = true;

    while (not_end)
    {
      if (offset + SEGMENT_SIZE > end || // reach file offset end
          check_size + SEGMENT_SIZE > size) // reach buffer offset end
      {
        cur_size = min(end - offset, size - check_size);
        not_end = false;
        if (0 == cur_size)
        {
          break;
        }
      }
      else
      {
        cur_size = SEGMENT_SIZE;
      }

      seg_data = new SegmentData();
      seg_data->seg_info_.offset_ = offset;
      seg_data->seg_info_.size_ = cur_size;
      seg_data->buf_ = const_cast<char*>(buf) + check_size;
      seg_list.push_back(seg_data);

      TBSYS_LOG(DEBUG, "get segment, seg info size: %d, offset: %"PRI64_PREFIX"d", seg_data->seg_info_.size_, seg_data->seg_info_.offset_);
      check_size += cur_size;
      offset += cur_size;
    }
    size -= check_size;
  }
}

int LocalKey::load_head(const char* buf)
{
  memcpy(&seg_head_, buf, sizeof(SegmentHead));
  TBSYS_LOG(DEBUG, "load segment head, count %d, size: %"PRI64_PREFIX"d", seg_head_.count_, seg_head_.size_);
  return TFS_SUCCESS;
}

int LocalKey::load_segment(const char* buf)
{
  int ret = TFS_SUCCESS;
  // clear last segment info ?
  destroy_info();

  int64_t size = 0;
  int32_t count = seg_head_.count_;
  const SegmentInfo* segment = reinterpret_cast<const SegmentInfo*>(buf);
  for (int32_t i = 0; i < count; ++i)
  {
    TBSYS_LOG(DEBUG, "load segment info, offset: %"PRI64_PREFIX"d, blockid: %u, fileid: %"PRI64_PREFIX"u, size: %d, crc: %u",
              segment[i].offset_, segment[i].block_id_, segment[i].file_id_, segment[i].size_, segment[i].crc_);

    if (!seg_info_.insert(segment[i]).second)
    {
      TBSYS_LOG(ERROR, "load segment info fail, count: %d, failno: %d", count, i + 1);
      ret = TFS_ERROR;
      break;
    }
    size += segment[i].size_;
  }

  if (TFS_SUCCESS == ret && size != seg_head_.size_)
  {
    TBSYS_LOG(ERROR, "segment size conflict with head meta info size: %"PRI64_PREFIX"d <> %"PRI64_PREFIX"d",
              size, seg_head_.size_);
    ret = TFS_ERROR;
  }

  return ret;
}

void LocalKey::destroy_info()
{
  seg_info_.clear();
}
