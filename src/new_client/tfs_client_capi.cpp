#include "tfs_client_api.h"
#include "tfs_client_capi.h"

using namespace tfs::client;

int t_initialize(const char* ns_addr, const int32_t cache_time, const int32_t cache_items)
{
  return TfsClient::Instance()->initialize(ns_addr, cache_time, cache_items);
}

int t_open(const char* file_name, const char* suffix, const int flags, const char* key)
{
  if (NULL == key)
  {
    return TfsClient::Instance()->open(file_name, suffix, (const char*)NULL, flags);
  }

  return TfsClient::Instance()->open(file_name, suffix, (const char*)NULL, flags, key);
}

int64_t t_read(const int fd, void* buf, const int64_t count)
{
  return TfsClient::Instance()->read(fd, buf, count);
}

int64_t t_write(const int fd, const void* buf, const int64_t count)
{
  return TfsClient::Instance()->write(fd, buf, count);
}

int64_t t_lseek(const int fd, const int64_t offset, const int whence)
{
  return TfsClient::Instance()->lseek(fd, offset, whence);
}

int64_t t_pread(const int fd, void* buf, const int64_t count, const int64_t offset)
{
  return TfsClient::Instance()->pread(fd, buf, count, offset);
}

int64_t t_pwrite(const int fd, const void* buf, const int64_t count, const int64_t offset)
{
  return TfsClient::Instance()->pwrite(fd, buf, count, offset);
}

int t_fstat(const int fd, tfs::common::FileInfo* buf, const int mode)
{
  return TfsClient::Instance()->fstat(fd, buf, mode);
}

int t_close(const int fd, char* tfs_name, const int32_t len)
{
  return TfsClient::Instance()->close(fd, tfs_name, len);
}
