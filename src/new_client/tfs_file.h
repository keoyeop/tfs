#ifndef TFS_CLIENT_TFSFILE_H_
#define TFS_CLIENT_TFSFILE_H_

#include <sys/types.h>
#include <fcntl.h>

#include "common/error_msg.h"
#include "common/func.h"
#include "message/client_manager.h"

#include "tfs_session.h"
#include "fsname.h"
#include "local_key.h"

namespace tfs
{
  namespace client
  {
    enum InnerFilePhase
    {
      FILE_PHASE_CREATE_FILE = 1,
      FILE_PHASE_WRITE_DATA,
      FILE_PHASE_CLOSE_FILE,
      FILE_PHASE_READ_FILE,
      FILE_PHASE_STAT_FILE,
    };

    enum
    {
      TFS_FILE_OPEN_YES = 0,
      TFS_FILE_OPEN_NO
    };

    enum
    {
      TFS_FILE_EOF_YES = 0,
      TFS_FILE_EOF_NO
    };

    class TfsFile
    {
    public:
      TfsFile();
      virtual ~TfsFile();

      // virtual interface
      virtual int open(const char* file_name, const char *suffix, int flags, ... ) = 0;
      virtual int64_t read(void* buf, int64_t count) = 0;
      virtual int64_t write(const void* buf, int64_t count) = 0;
      virtual int64_t lseek(int64_t offset, int whence) = 0;
      virtual int64_t pread(void* buf, int64_t count, int64_t offset) = 0;
      virtual int64_t pwrite(const void* buf, int64_t count, int64_t offset) = 0;
      virtual int fstat(common::FileInfo* file_info, int32_t mode = common::NORMAL_STAT) = 0;
      virtual int close() = 0;

      const char* get_file_name();
      void set_session(TfsSession* tfs_session);

    protected:
      // virtual level operation
      virtual int get_segment_for_read(int64_t offset, char* buf, int64_t count) = 0;
      virtual int get_segment_for_write(int64_t offset, const char* buf, int64_t count) = 0;
      virtual int read_process() = 0;
      virtual int write_process() = 0;
      virtual int finish_write_process() = 0;
      virtual int close_process() = 0;
      //virtual int stat_process() = 0;

    protected:
      // common operation
      void destroy_seg();
      int get_meta_segment(const int64_t offset, char* buf, const int64_t count);
      int process(const InnerFilePhase file_phase);

      int open_ex(const char* file_name, const char *suffix, const int32_t flags);
      int64_t read_ex(void* buf, const int64_t count, const int64_t offset, const bool modify = true);
      int64_t write_ex(const void* buf, int64_t count, int64_t offset, bool modify = true);
      int64_t lseek_ex(int64_t offset, int whence);
      int64_t pread_ex(void* buf, int64_t count, int64_t offset);
      int64_t pwrite_ex(const void* buf, int64_t count, int64_t offset);
      int fstat_ex(common::FileInfo* file_info, int32_t mode);
      int close_ex();

    private:
      int do_async_request(const InnerFilePhase file_phase, const int64_t wait_id, const int32_t index);
      int do_async_response(const InnerFilePhase file_phase, tbnet::Packet* packet, const int32_t index);

      int async_req_create_file(const int64_t wait_id, const int32_t index);
      int async_rsp_create_file(tbnet::Packet* packet, const int32_t index);

      int async_req_read_file(const int64_t wait_id, const int32_t index);
      int async_rsp_read_file(tbnet::Packet* packet, const int32_t index);

      int async_req_write_data(const int64_t wait_id, const int32_t index);
      int async_rsp_write_data(tbnet::Packet* packet, const int32_t index);

      int async_req_close_file(const int64_t wait_id, const int32_t index);
      int async_rsp_close_file(tbnet::Packet* packet, const int32_t index);

      int async_req_stat_file(const int64_t wait_id, const int32_t index);
      int async_rsp_stat_file(tbnet::Packet* packet, const int32_t index);

    protected:
      static const int64_t WAIT_TIME_OUT = 3000000;
      static const int64_t CLIENT_TRY_COUNT = 3;

      static const int64_t BATCH_COUNT = 8;
      static const int64_t BATCH_SIZE = common::SEGMENT_SIZE * BATCH_COUNT;

    protected:
      FSName fsname_;
      int32_t flags_;
      int32_t is_open_;
      int32_t eof_;
      int64_t offset_;
      SegmentData* meta_seg_;
      //sync flag
      int32_t option_flag_;
      TfsSession* tfs_session_;
      SEG_DATA_LIST processing_seg_list_;
    };
  }
}
#endif  // TFS_CLIENT_TFSFILE_H_
