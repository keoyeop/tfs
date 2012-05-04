/*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id
 *
 * Authors:
 *   duanfei <duanfei@taobao.com>
 *      - initial release
 *
 */

#include <time.h>
#include <iostream>
#include <functional>
#include <numeric>
#include <math.h>
#include <tbsys.h>
#include <Memory.hpp>
#include "ns_define.h"
#include "nameserver.h"
#include "layout_manager.h"
#include "global_factory.h"
#include "common/error_msg.h"
#include "common/base_packet.h"
#include "common/base_service.h"
#include "common/status_message.h"
#include "common/client_manager.h"
#include "common/array_helper.h"
#include "message/block_info_message.h"
#include "message/replicate_block_message.h"
#include "message/compact_block_message.h"

using namespace tfs::common;
using namespace tfs::message;
using namespace tbsys;

namespace tfs
{
  namespace nameserver
  {
    LayoutManager::LayoutManager(NameServer& manager):
      build_plan_thread_(0),
      run_plan_thread_(0),
      check_dataserver_thread_(0),
      add_block_in_all_server_thread_(0),
      check_dataserver_report_block_thread_(0),
      balance_thread_(0),
      timeout_thread_(0),
      last_rotate_log_time_(0),
      plan_run_flag_(PLAN_RUN_FLAG_REPLICATE),
      manager_(manager),
      block_manager_(*this),
      server_manager_(*this),
      task_manager_(*this),
      oplog_sync_mgr_(*this),
      client_request_server_(*this),
      gc_manager_(*this)
    {
      srand(time(NULL));
      tzset();
      zonesec_ = 86400 + timezone;
      last_rotate_log_time_ = 0;
      plan_run_flag_ |= PLAN_RUN_FLAG_MOVE;
      plan_run_flag_ |= PLAN_RUN_FLAG_COMPACT;
      plan_run_flag_ |= PLAN_RUN_FLAG_DELETE;
    }

    LayoutManager::~LayoutManager()
    {
      build_plan_thread_ = 0;
      run_plan_thread_ = 0;
      check_dataserver_thread_ = 0;
      add_block_in_all_server_thread_ = 0;
      check_dataserver_report_block_thread_ = 0;
      balance_thread_ = 0;
      timeout_thread_ = 0;
    }

    int LayoutManager::initialize()
    {
      int32_t ret = oplog_sync_mgr_.initialize();
      if (TFS_SUCCESS != ret)
      {
        TBSYS_LOG(ERROR, "initialize oplog sync manager fail, must be exit, ret: %d", ret);
      }
      else
      {
        //initialize thread
        build_plan_thread_ = new BuildPlanThreadHelper(*this);
        check_dataserver_thread_ = new CheckDataServerThreadHelper(*this);
        run_plan_thread_ = new RunPlanThreadHelper(*this);
        add_block_in_all_server_thread_ = new AddBlockInAllServerThreadHelper(*this);
        check_dataserver_report_block_thread_ = new CheckDataServerReportBlockThreadHelper(*this);
        run_plan_thread_ = new RunPlanThreadHelper(*this);
        balance_thread_  = new BuildBalanceThreadHelper(*this);
        timeout_thread_  = new TimeoutThreadHelper(*this);
      }
      return ret;
    }

    void LayoutManager::wait_for_shut_down()
    {
      if (build_plan_thread_ != 0)
      {
        build_plan_thread_->join();
      }
      if (check_dataserver_thread_ != 0)
      {
        check_dataserver_thread_->join();
      }
      if (run_plan_thread_ != 0)
      {
        run_plan_thread_->join();
      }
      if (add_block_in_all_server_thread_ != 0)
      {
        add_block_in_all_server_thread_->join();
      }
      if (check_dataserver_report_block_thread_ != 0)
      {
        check_dataserver_report_block_thread_->join();
      }
      if (balance_thread_ != 0)
      {
        balance_thread_->join();
      }
      if (timeout_thread_ != 0)
      {
        timeout_thread_->join();
      }
      oplog_sync_mgr_.wait_for_shut_down();
    }

    void LayoutManager::destroy()
    {
      oplog_sync_mgr_.destroy();
    }

    /**
     * dsataserver start, send heartbeat message to nameserver.
     * update all relations of blocks belongs to it
     * @param [in] dsInfo: dataserver system info , like capacity, load, etc..
     * @param [in] blocks: data blocks' info which belongs to dataserver.
     * @param [out] expires: need expire blocks
     * @return success or failure
     */
    int LayoutManager::update_relation(ServerCollect* server,const std::set<BlockInfo>& blocks, const time_t now)
    {
      int32_t ret = ((NULL != server) && (server->is_alive())) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        //release relation//这里每做一次都需要拷贝整个列表，并且要解除与这个server相关block的关系,
        //是否可以考虑其他方式，例如：做一次for将新加入，删除的，不变化的先找出来,然后再建立关系
        //这里可以放到后面来优化
        server->clear(*this, now);
        TBSYS_LOG(DEBUG, "%s update relation", CNetUtil::addrToString(server->id()).c_str());
        ret = get_block_manager().update_relation(server, blocks, now);
        TBSYS_LOG(DEBUG, "%s update relation end", CNetUtil::addrToString(server->id()).c_str());
      }
      return ret;
    }

    bool LayoutManager::build_relation(BlockCollect* block, ServerCollect* server, const time_t now, const bool set)
    {
      bool ret = ((NULL != block) && (NULL != server));
      if (ret)
      {
        bool writable = false;
        bool master   = false;
        int32_t ret = get_block_manager().build_relation(block, writable, master, server, now, set);
        if (TFS_SUCCESS == ret)
        {
          ret = get_server_manager().build_relation(server, block, writable, master);
        }
        ret =  TFS_SUCCESS == ret;
      }
      return ret;
    }

    bool LayoutManager::relieve_relation(BlockCollect* block, ServerCollect* server, time_t now)
    {
      bool ret = ((NULL != block) && (NULL != server));
      if (ret)
      {
        //release relation between block and dataserver
        bool bremove = get_block_manager().relieve_relation(block, server, now);
        if (!bremove)
        {
          TBSYS_LOG(INFO, "failed when relieve between block: %u and dataserver: %s",
              block->id(), CNetUtil::addrToString(server->id()).c_str());
        }
        bool sremove = get_server_manager().relieve_relation(server,block);
        if (!sremove)
        {
          TBSYS_LOG(INFO, "failed when relieve between block: %u and dataserver: %s",
              block->id(), CNetUtil::addrToString(server->id()).c_str());
        }
        ret = bremove && sremove;
      }
      return ret;
    }

    int LayoutManager::update_block_info(const BlockInfo& info, const uint64_t server, const time_t now,const bool addnew)
    {
      bool isnew = false;
      bool master = false;
      bool writable = false;
      BlockCollect* block = NULL;
      ServerCollect* pserver = get_server_manager().get(server);
      int32_t ret = (NULL != pserver) ? TFS_SUCCESS : EXIT_DATASERVER_NOT_FOUND;
      if (TFS_SUCCESS == ret)
      {
        pserver->update_last_time(now);
        block = get_block_manager().get(info.block_id_);
        ret = (NULL != block) ? TFS_SUCCESS : EXIT_BLOCK_NOT_FOUND;
      }
      if (TFS_SUCCESS == ret)
      {
        ret = get_block_manager().update_block_info(block, isnew, writable, master,
            info, pserver, now, addnew);
      }
      if ((TFS_SUCCESS == ret)
          && (isnew))
      {
        get_server_manager().build_relation(pserver, block, writable, master);
      }
      if (TFS_SUCCESS == ret)
      {
        //write oplog
        std::vector<uint32_t> blocks;
        std::vector<uint64_t> servers;
        blocks.push_back(info.block_id_);
        servers.push_back(server);
        #ifndef TFS_GTEST
        ret = block_oplog_write_helper(isnew ? OPLOG_INSERT : OPLOG_UPDATE, info, blocks, servers, now);
        #endif
      }
      return ret;
    }

    int LayoutManager::repair(char* msg, const int32_t length, const uint32_t block_id,
        const uint64_t server, const int32_t flag, const time_t now)
    {
      int32_t ret = ((NULL != msg) && (length > 0)) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        ServerCollect* servers[SYSPARAM_NAMESERVER.max_replication_];
        ArrayHelper<ServerCollect*> helper(SYSPARAM_NAMESERVER.max_replication_, servers);
        BlockCollect* block = get_block_manager().get(block_id);
        ret = (NULL == block) ? EXIT_BLOCK_NOT_FOUND : TFS_SUCCESS;
        if (TFS_SUCCESS != ret)
          snprintf(msg, length, "repair block, block collect not found by block: %u", block_id);
        else
          get_block_manager().get_servers(helper, block_id);

        if (TFS_SUCCESS == ret)
        {
          //need repair this block.
          if ((UPDATE_BLOCK_MISSING == flag)
              && (helper.get_array_index() >= SYSPARAM_NAMESERVER.max_replication_))
          {
            snprintf(msg, length, "already got block: %u, replicate: %"PRI64_PREFIX"d", block_id, helper.get_array_index());
            ret = EXIT_UPDATE_BLOCK_MISSING_ERROR;
          }
        }

        if (TFS_SUCCESS == ret)
        {
          ret = helper.get_array_index() <= 0 ? EXIT_DATASERVER_NOT_FOUND : TFS_SUCCESS;
          if (TFS_SUCCESS != ret)
          {
            snprintf(msg, length, "repair block: %u no any dataserver hold it", block_id);
          }
        }

        ServerCollect* source = NULL;
        if (TFS_SUCCESS == ret)
        {
          for (int32_t i = 0; i < helper.get_array_index() && NULL == source; ++i)
          {
            ServerCollect* pserver = *helper.at(i);
            assert(NULL != pserver);
            if (pserver->id() != server)
              source = pserver;
          }
          ret = (NULL == source) ? EXIT_NO_DATASERVER : TFS_SUCCESS;
          if (TFS_SUCCESS != ret)
          {
            snprintf(msg, length, "repair block: %u no any other dataserver: %"PRI64_PREFIX"d hold a correct replica", block_id, helper.get_array_index());
          }
        }

        ServerCollect* target = NULL;
        if (TFS_SUCCESS == ret)
        {
          target = get_server_manager().get(server);
          ret = (NULL == target) ? EXIT_NO_DATASERVER : TFS_SUCCESS;
          if (TFS_SUCCESS != ret)
          {
            snprintf(msg, length, "repair block: %u not found target: %s", block_id, CNetUtil::addrToString(server).c_str());
          }
          if (TFS_SUCCESS == ret)
          {
            block = get_block_manager().get(block_id);
            ret = (NULL == block) ? EXIT_BLOCK_NOT_FOUND: TFS_SUCCESS;
            if (TFS_SUCCESS == ret)
            {
              ret = relieve_relation(block, target, now) ? TFS_SUCCESS : TFS_ERROR;
              if (TFS_SUCCESS != ret)
              {
                snprintf(msg, length, "relieve relation failed between block: %u and server: %s", block_id, CNetUtil::addrToString(server).c_str());
              }
              else
              {
                std::vector<ServerCollect*> runer;
                runer.push_back(source);
                runer.push_back(target);
                ret = get_task_manager().add(block_id, runer, PLAN_TYPE_REPLICATE, now, PLAN_PRIORITY_EMERGENCY);
              }
            }
          }
        }
        if (TFS_SUCCESS != ret)
        {
          TBSYS_LOG(INFO, "%s", msg);
        }
        else
        {
          //TODO
          get_block_manager().push_to_delete_queue(block_id, server);//需要改下dataserverTODO
        }
      }
      return ret;
    }

    int LayoutManager::scan(SSMScanParameter& param)
    {
      int32_t start = (param.start_next_position_ & 0xFFFF0000) >> 16;
      int32_t next  = start;
      int32_t should= (param.should_actual_count_ & 0xFFFF0000) >> 16;
      int32_t actual= 0;
      bool    all_over = true;
      bool    cutover = ((param.end_flag_) & SSM_SCAN_CUTOVER_FLAG_YES);

      if (param.type_ & SSM_TYPE_SERVER)
      {
        actual = get_server_manager().scan(param, should, start, next, all_over);
      }
      else if (param.type_ & SSM_TYPE_BLOCK)
      {
        actual = get_block_manager().scan(param, next, all_over, cutover, should);
      }
      next &= 0x0000FFFF;
      param.start_next_position_ &= 0xFFFF0000;
      param.start_next_position_ |= next;
      actual &= 0x0000FFFF;
      param.should_actual_count_ &= 0xFFFF0000;
      param.should_actual_count_ |= actual;
      param.end_flag_ = all_over ? SSM_SCAN_END_FLAG_YES : SSM_SCAN_END_FLAG_NO;
      param.end_flag_ <<= 4;
      param.end_flag_ &= 0xF0;
      param.end_flag_ |= cutover ? SSM_SCAN_CUTOVER_FLAG_YES : SSM_SCAN_CUTOVER_FLAG_NO;
      return TFS_SUCCESS;
    }

    int LayoutManager::handle_task_complete(common::BasePacket* msg)
    {
      //handle complete message
      int32_t ret = (NULL != msg) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        ret = msg->getPCode() == BLOCK_COMPACT_COMPLETE_MESSAGE
          || msg->getPCode() == REPLICATE_BLOCK_MESSAGE
          || msg->getPCode() == REMOVE_BLOCK_RESPONSE_MESSAGE ? TFS_SUCCESS : EXIT_UNKNOWN_MSGTYPE;
        if (TFS_SUCCESS != ret)
        {
          TBSYS_LOG(INFO, "handle_task_complete unkonw message PCode = %d", msg->getPCode());
        }
        else
        {
          BaseTaskMessage* base = dynamic_cast<BaseTaskMessage*>(msg);
          ret = get_task_manager().handle(msg, base->get_seqno()) ? TFS_SUCCESS : EXIT_EXECUTE_TASK_ERROR;
        }
      }
      return ret;
    }

    int LayoutManager::open_helper_create_new_block_by_id(uint32_t block_id)
    {
      BlockCollect* block = get_block_manager().get(block_id);
      int32_t ret = (NULL != block) ? TFS_SUCCESS : EXIT_BLOCK_NOT_FOUND;
      if (TFS_SUCCESS != ret)//block not found by block_id
      {
        block = add_new_block_(block_id);
        ret = (NULL == block) ? EXIT_CREATE_BLOCK_BY_ID_ERROR : TFS_SUCCESS;
        if (TFS_SUCCESS != ret)
        {
          TBSYS_LOG(INFO, "add new block: %u failed because create block by blockid error", block_id);
        }
      }

      if (TFS_SUCCESS == ret)
      {
        block = get_block_manager().get(block_id);
        ret = NULL != block ? TFS_SUCCESS : EXIT_BLOCK_NOT_FOUND;
        if (TFS_SUCCESS == ret)
        {
          if (block->get_servers_size() <= 0)
          {
            if (block->is_creating())
            {
              TBSYS_LOG(INFO, "block: %u found meta data, but creating by another thread, must be return", block_id);
              ret = EXIT_NO_BLOCK;
            }
            else
            {
              TBSYS_LOG(INFO, "block: %u found meta data, but no dataserver hold it.", block_id);
              ret = EXIT_NO_DATASERVER;
            }
          }
        }
      }
      return ret;
    }

    int LayoutManager::add_report_block_server(ServerCollect* server, const bool rb_expire)
    {
      int32_t ret = (NULL != server) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        tbutil::Mutex::Lock lock(wait_report_block_server_mutex_);
        if (rb_expire)
        {
          del_report_block_server_(server);
          wait_report_block_servers_.push_front(server);
        }
        else
        {
          wait_report_block_servers_.push_back(server);
        }
      }
      return ret;
    }

    int LayoutManager::del_report_block_server(ServerCollect* server)
    {
      tbutil::Mutex::Lock lock(wait_report_block_server_mutex_);
      return del_report_block_server_(server);
    }

    int LayoutManager::del_report_block_server_(ServerCollect* server)
    {
      int32_t ret = (NULL != server) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        std::vector<ServerCollect*>::iterator iter = std::find(current_reporting_block_servers_.begin(),
            current_reporting_block_servers_.end(), server);
        if ((current_reporting_block_servers_.end() != iter)
            && ((*iter)->id() == server->id()))
        {
          current_reporting_block_servers_.erase(iter);
        }
      }
      return ret;
    }

    int LayoutManager::block_oplog_write_helper(const int32_t cmd, const BlockInfo& info,
        const std::vector<uint32_t>& blocks, const std::vector<uint64_t>& servers, const time_t now)
    {
      BlockOpLog oplog;
      memset(&oplog, 0, sizeof(oplog));
      oplog.info_ = info;
      oplog.cmd_ = cmd;
      oplog.blocks_ = blocks;
      oplog.servers_ = servers;
      int64_t size = oplog.length();
      int64_t pos = 0;
      char buf[size];
      int32_t ret = oplog.serialize(buf, size, pos);
      if (TFS_SUCCESS != ret)
      {
        TBSYS_LOG(INFO, "%s", "oplog serialize error");
      }
      else
      {
        // treat log operation as a trivial thing..
        // don't rollback the insert operation, cause block meta info
        // build from all dataserver, log info not been very serious.
        ret = oplog_sync_mgr_.log(OPLOG_TYPE_BLOCK_OP, buf, size, now);
        if (TFS_SUCCESS != ret)
        {
          TBSYS_LOG(INFO, "write oplog failed, block: %u", info.block_id_);
        }
      }
      return ret;
    }

    int LayoutManager::set_runtime_param(const uint32_t value1, const uint32_t value2, const int64_t length, char *retstr)
    {
      int32_t ret = ((NULL != retstr) && (length > 0)) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        retstr[0] = '\0';
        int32_t index = (value1 & 0x0FFFFFFF);
        int32_t set = (value1& 0xF0000000);
        int32_t* param[] =
        {
          &TBSYS_LOGGER._level,
          &plan_run_flag_,
          &SYSPARAM_NAMESERVER.task_expired_time_,
          &SYSPARAM_NAMESERVER.safe_mode_time_,
          &SYSPARAM_NAMESERVER.max_write_timeout_,
          &SYSPARAM_NAMESERVER.max_write_file_count_,
          &SYSPARAM_NAMESERVER.add_primary_block_count_,
          &SYSPARAM_NAMESERVER.cleanup_write_timeout_threshold_,
          &SYSPARAM_NAMESERVER.max_use_capacity_ratio_,
          &SYSPARAM_NAMESERVER.heart_interval_,
          &SYSPARAM_NAMESERVER.replicate_ratio_,
          &SYSPARAM_NAMESERVER.replicate_wait_time_,
          &SYSPARAM_NAMESERVER.compact_delete_ratio_,
          &SYSPARAM_NAMESERVER.compact_max_load_,
          &SYSPARAM_NAMESERVER.compact_time_lower_,
          &SYSPARAM_NAMESERVER.compact_time_upper_,
          &SYSPARAM_NAMESERVER.max_task_in_machine_nums_,
          &SYSPARAM_NAMESERVER.discard_newblk_safe_mode_time_,
          &SYSPARAM_NAMESERVER.discard_max_count_,
          &SYSPARAM_NAMESERVER.cluster_index_,
          &SYSPARAM_NAMESERVER.object_dead_max_time_,
          &SYSPARAM_NAMESERVER.group_count_,
          &SYSPARAM_NAMESERVER.group_seq_,
          &SYSPARAM_NAMESERVER.object_clear_max_time_,
          &SYSPARAM_NAMESERVER.report_block_queue_size_,
          &SYSPARAM_NAMESERVER.report_block_time_lower_,
          &SYSPARAM_NAMESERVER.report_block_time_upper_,
          &SYSPARAM_NAMESERVER.report_block_time_interval_,
          &SYSPARAM_NAMESERVER.report_block_expired_time_
        };
        int32_t size = sizeof(param) / sizeof(int32_t*);
        ret = (index > 1 && index <= size) ? TFS_SUCCESS : TFS_ERROR;
        if (TFS_SUCCESS != ret)
        {
          snprintf(retstr, length, "index : %d invalid.", index);
        }
        else
        {
          int32_t* current_value = param[index - 1];
          if (set)
            *current_value = (int32_t)(value2 & 0xFFFFFFFF);
          else
            snprintf(retstr, 256, "%d", *current_value);
          TBSYS_LOG(DEBUG, "index: %d %s name: %s value: %d", index, set ? "set" : "get", dynamic_parameter_str[index - 1], *current_value);
        }
      }
      return ret;
    }

    void LayoutManager::switch_role(time_t now)
    {
      get_task_manager().clear();
      GFactory::get_runtime_info().switch_role(now);
      get_server_manager().set_all_server_next_report_time(now);
      oplog_sync_mgr_.switch_role();
    }

    void LayoutManager::rotate_(time_t now)
    {
      if ((now % 86400 >= zonesec_)
          && (now % 86400 < zonesec_ + 300)
          && (last_rotate_log_time_ < now - 600))
      {
        last_rotate_log_time_ = now;
        TBSYS_LOGGER.rotateLog(NULL);
      }
    }

    uint32_t LayoutManager::get_alive_block_id_()
    {
      uint32_t block_id = oplog_sync_mgr_.generation();
      while (true)
      {
        if (!get_block_manager().exist(block_id))
          break;
        block_id = oplog_sync_mgr_.generation();
      }
      return block_id;
    }

    static bool in_hour_range(const time_t now, int32_t& min, int32_t& max)
    {
      struct tm* lt = gmtime(&now);
      if (min > max)
        std::swap(min, max);
      return (lt->tm_hour >= min && lt->tm_hour <= max);
    }

    void LayoutManager::build_()
    {
      bool over = false;
      time_t  now = 0, current = 0;
      int64_t need = 0, index = 0;
      uint32_t start = 0;
      const int32_t MAX_QUERY_BLOCK_NUMS = 2048;
      BlockCollect* pblock = NULL;
      BlockCollect* blocks[MAX_QUERY_BLOCK_NUMS];
      ArrayHelper<BlockCollect*> results(MAX_QUERY_BLOCK_NUMS, blocks);
      std::deque<BlockCollect*> emergency_replicate_queue;
      NsRuntimeGlobalInformation& ngi = GFactory::get_runtime_info();
      while (!ngi.is_destroyed())
      {
        current = time(NULL);
        now = Func::get_monotonic_time();
        if (ngi.in_safe_mode_time(now))
          Func::sleep(SYSPARAM_NAMESERVER.safe_mode_time_, ngi.destroy_flag_);

        //checkpoint
        rotate_(current);

        if (ngi.is_master())
        {
          while ((has_report_block_server_()) && (!ngi.is_destroyed()))
            usleep(1000);

          while (((need = has_space_in_task_queue_()) <= 0) && (!ngi.is_destroyed()))
            usleep(1000);

          results.clear();

          check_emergency_replicate_(emergency_replicate_queue, results, MAX_QUERY_BLOCK_NUMS, now);

          build_emergency_replicate_(need, emergency_replicate_queue, now);

          results.clear();

          if (need > 0)
          {
            bool range = in_hour_range(current, SYSPARAM_NAMESERVER.compact_time_lower_, SYSPARAM_NAMESERVER.compact_time_upper_);
            over = get_block_manager().scan(results, start, MAX_QUERY_BLOCK_NUMS);
            for (index = 0; index < results.get_array_index(); ++index)
            {
              pblock = *results.at(index);
              assert(NULL != pblock);
              bool ret = build_replicate_task_(need, pblock, now);
              if (!ret && range)
                ret = build_compact_task_(pblock, now);
              if (ret)
                --need;
            }
            //TBSYS_LOG(DEBUG, "check over: %d, size: %d", over, results.get_array_index());
            if (over)
              start = 0;
          }

          build_redundant_(need, now);
        }
        Func::sleep(SYSPARAM_NAMESERVER.heart_interval_, ngi.destroy_flag_);
      }
    }

    void LayoutManager::balance_()
    {
      int64_t total_capacity  = 0;
      int64_t total_use_capacity = 0;
      int64_t alive_server_nums = 0;
      int64_t need = 0;
      time_t  now = 0;
      std::multimap<int64_t, ServerCollect*> source;
      TfsSortedVector<ServerCollect*, ServerIdCompare> targets(MAX_PROCESS_NUMS, 1024, 0.1);
      NsRuntimeGlobalInformation& ngi = GFactory::get_runtime_info();
      while (!ngi.is_destroyed())
      {
        now = Func::get_monotonic_time();
        if (ngi.in_safe_mode_time(now))
          Func::sleep(SYSPARAM_NAMESERVER.safe_mode_time_, ngi.destroy_flag_);

        if (ngi.is_master())
        {
          while ((has_report_block_server_()) && (!ngi.is_destroyed()))
            usleep(1000);

          while (((need = has_space_in_task_queue_()) <= 0) && (!ngi.is_destroyed()))
            usleep(1000);

          while ((!(plan_run_flag_ & PLAN_TYPE_MOVE)) && (!ngi.is_destroyed()))
            usleep(100000);

          total_capacity = 0, total_use_capacity = 0, alive_server_nums = 0;
          get_server_manager().move_statistic_all_server_info(total_capacity,
              total_use_capacity, alive_server_nums);
          if (total_capacity > 0 && total_use_capacity > 0 && alive_server_nums > 0)
          {
            source.clear();
            targets.clear();
            double percent = calc_capacity_percentage(total_use_capacity, total_capacity);

            // find move src and dest ds list
            get_server_manager().move_split_servers(source, targets, percent);

            TBSYS_LOG(INFO, "need: %"PRI64_PREFIX"d, source size: %Zd, target: %Zd, percent: %e",
                need, source.size(), targets.size(), percent);

            const int32_t MAX_RETRY_COUNT = 3;
            int32_t ret = TFS_SUCCESS;
            bool complete = false;
            BlockCollect* block = NULL;
            std::vector<ServerCollect*> except;
            ServerCollect* hold[SYSPARAM_NAMESERVER.max_replication_];
            ArrayHelper<ServerCollect*> servers(SYSPARAM_NAMESERVER.max_replication_, hold);

            // we'd better start from the most needed ds
            std::multimap<int64_t, ServerCollect*>::const_reverse_iterator it = source.rbegin();
            for (; it != source.rend() && need > 0 && !targets.empty(); ++it, complete = false)
            {
              if (get_task_manager().has_space_do_task_in_machine(it->second->id(), false)
                  && !get_task_manager().exist(it->second->id()))
              {
                for (int32_t index = 0; index < MAX_RETRY_COUNT && !complete; ++index)
                {
                  block = NULL;
                  ret = it->second->choose_move_block_random(block);
                  if (TFS_SUCCESS == ret
                      && build_balance_task_(need, targets, it->second, block, now))
                  {
                    --need;
                    complete = true;
                  }
                }//end for ...
              }
            }//end for ...
          }
        }
        Func::sleep(SYSPARAM_NAMESERVER.heart_interval_, ngi.destroy_flag_);
      }
    }

    void LayoutManager::timeout_()
    {
      time_t now = 0;
      int32_t block_expand_index = 0;
      int32_t server_expand_index = 0;
      NsRuntimeGlobalInformation& ngi = GFactory::get_runtime_info();
      while (!ngi.is_destroyed())
      {
        now = Func::get_monotonic_time();

        get_task_manager().timeout(now);

        get_block_manager().timeout(now);

        get_block_manager().expand_ratio(block_expand_index);

        get_server_manager().expand_ratio(server_expand_index);

        get_gc_manager().gc(now);

        usleep(50000);
      }
    }

    void LayoutManager::check_all_server_isalive_()
    {
      time_t now = 0;
      uint64_t* servers = new uint64_t [MAX_PROCESS_NUMS];
      NsGlobalStatisticsInfo stat_info;
      ArrayHelper<uint64_t> helper(MAX_PROCESS_NUMS, servers);
      NsRuntimeGlobalInformation& ngi = GFactory::get_runtime_info();
      while (!ngi.is_destroyed())
      {
        helper.clear();
        memset(&stat_info, 0, sizeof(NsGlobalStatisticsInfo));
        now = Func::get_monotonic_time();
        if (ngi.in_safe_mode_time(now))
        {
          Func::sleep(SYSPARAM_NAMESERVER.safe_mode_time_, ngi.destroy_flag_);
        }

        //check dataserver is alive
        get_server_manager().get_dead_servers(helper, stat_info, now);

        // write global information
        GFactory::get_global_info().update(stat_info);
        //GFactory::get_global_info().dump(TBSYS_LOG_LEVEL_INFO);

        //remove dataserver
        uint64_t* value = NULL;
        int32_t size = helper.get_array_index();
        for (int32_t i = 0; i < size; ++i)
        {
          value = helper.pop();
          get_server_manager().remove(*value, now);
        }

        Func::sleep(SYSPARAM_NAMESERVER.heart_interval_, ngi.destroy_flag_);
      }
      tbsys::gDeleteA(servers);
    }

    void LayoutManager::add_block_in_all_server_()
    {
      time_t now = 0;
      uint64_t last = 0;
      bool   promote = true;
      bool   complete = false;
      const int32_t MAX_SLOT_NUMS = 32;
      ServerCollect* server = NULL;
      ServerCollect* servers[MAX_SLOT_NUMS];
      ArrayHelper<ServerCollect*> helper(MAX_SLOT_NUMS, servers);
      NsRuntimeGlobalInformation& ngi = GFactory::get_runtime_info();
      while (!ngi.is_destroyed())
      {
        helper.clear();
        now = Func::get_monotonic_time();
        complete = get_server_manager().get_range_servers(helper, last, MAX_SLOT_NUMS);
        if (!helper.empty())
        {
          server = *helper.at(helper.get_array_index() - 1);
          last = server->id();
        }

        touch_(promote, helper, now);

        if (complete)
        {
          last = 0;
          Func::sleep(SYSPARAM_NAMESERVER.heart_interval_, ngi.destroy_flag_);
        }
      }
    }

    void LayoutManager::check_all_server_report_block_()
    {
      time_t now = 0;
      int32_t new_add_nums  = 0;
      const int32_t MAX_SLOT_NUMS = 64;
      const int32_t SLEEP_TIME_US = 5000;
      NewClient* client = NULL;
      ServerCollect* last = NULL;
      ServerCollect* servers[MAX_SLOT_NUMS];
      ArrayHelper<ServerCollect*> helper(MAX_SLOT_NUMS, servers);
      NsRuntimeGlobalInformation& ngi = GFactory::get_runtime_info();
      while (!ngi.is_destroyed())
      {
        helper.clear();
        wait_report_block_server_mutex_.lock();
        new_add_nums = SYSPARAM_NAMESERVER.report_block_queue_size_ -  current_reporting_block_servers_.size();
        new_add_nums = std::min(MAX_SLOT_NUMS, new_add_nums);
        if (wait_report_block_servers_.empty())
        {
          wait_report_block_server_mutex_.unlock();
          usleep(SLEEP_TIME_US);
        }
        else
        {
          while (new_add_nums-- > 0
              && !wait_report_block_servers_.empty())
          {
            helper.push_back(wait_report_block_servers_.front());
            wait_report_block_servers_.pop_front();
          }
          wait_report_block_server_mutex_.unlock();
        }

        now = Func::get_monotonic_time();
        int32_t ret = TFS_SUCCESS;
        int32_t index = helper.get_array_index();
        while (index-- > 0)
        {
          last = *helper.at(index);
          assert(NULL != last);
          CallDsReportBlockRequestMessage req;
          req.set_server(ngi.owner_ip_port_);
          client = NewClientManager::get_instance().create_client();
          if (NULL != client)
            ret = post_msg_to_server(last->id(), client, &req, ns_async_callback);
          if (TFS_SUCCESS != ret)
            NewClientManager::get_instance().destroy_client(client);
          last->set_report_block_info(now, REPORT_BLOCK_STATUS_REPORTING);
          current_reporting_block_servers_.push_back(last);
        }
        usleep(100);
      }
    }

    /**
     * dataserver is the need to add testing of new block
     * @param[in] servers: dataserver object
     * @param[in] promote:  true: check writable block, false: no check
     * @return: return 0 if no need add, otherwise need block count
     */
    int LayoutManager::touch_(bool& promote, const common::ArrayHelper<ServerCollect*>& servers, const time_t now)
    {
      BlockCollect* block = NULL;
      ServerCollect* pserver = NULL;
      uint32_t new_block_id = 0;
      int32_t ret = TFS_SUCCESS;
      int64_t total_capacity = GFactory::get_global_info().total_capacity_ <= 0
              ? 1 : GFactory::get_global_info().total_capacity_;
      int64_t use_capacity = GFactory::get_global_info().use_capacity_ <= 0
        ? 0 : GFactory::get_global_info().use_capacity_;
      if (total_capacity <= 0)
        total_capacity = 0;
      double average_used_capacity = use_capacity / total_capacity;
      int32_t count = SYSPARAM_NAMESERVER.add_primary_block_count_;
      for (int32_t i = 0; i < servers.get_array_index(); ++i)
      {
        pserver = *servers.at(i);
        count = SYSPARAM_NAMESERVER.add_primary_block_count_;
        assert(NULL != pserver);
        pserver->touch(promote, count, average_used_capacity);
        //TBSYS_LOG(DEBUG, "%s touch, count: %d, index", CNetUtil::addrToString(pserver->id()).c_str(),count, i);
        if (count > 0
            && (GFactory::get_runtime_info().is_master())
            && (!GFactory::get_runtime_info().in_safe_mode_time(now)))
        {
          for (int32_t index = 0; index < count && TFS_SUCCESS == ret; ++index, new_block_id = 0)
          {
            block = add_new_block_(new_block_id, pserver, now);
            ret = (block != NULL) ? TFS_SUCCESS : EXIT_ADD_NEW_BLOCK_ERROR;
          }
        }
      }
      return TFS_SUCCESS;
    }

    int LayoutManager::add_new_block_helper_write_log_(const uint32_t block_id, const ArrayHelper<ServerCollect*>& servers, const time_t now)
    {
      int32_t ret = ((0 != block_id) && (!servers.empty())) ?  TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        BlockInfo info;
        info.block_id_ = block_id;
        info.seq_no_ = 1;
        std::vector<uint32_t> blocks;
        std::vector<uint64_t> tmp;
        blocks.push_back(block_id);
        for (int32_t i = 0; i < servers.get_array_index(); ++i)
        {
          ServerCollect* pserver = *servers.at(i);
          assert(NULL != pserver);
          tmp.push_back(pserver->id());
        }
        ret = block_oplog_write_helper(OPLOG_INSERT, info, blocks, tmp, now);
      }
      return ret;
    }

    int LayoutManager::add_new_block_helper_send_msg_(const uint32_t block_id, const ArrayHelper<ServerCollect*>& servers)
    {
      int32_t ret = ((0 != block_id) && (!servers.empty())) ?  TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        NewClient* client = NewClientManager::get_instance().create_client();
        ret = NULL != client ? TFS_SUCCESS : EXIT_CLIENT_MANAGER_CREATE_CLIENT_ERROR;
        if (TFS_SUCCESS == ret)
        {
          NewBlockMessage msg;
          msg.add_new_id(block_id);
          uint8_t send_id = 0;
          std::string all_servers, success_servers;
          uint64_t send_msg_success[SYSPARAM_NAMESERVER.max_replication_];
          ArrayHelper<uint64_t> send_msg_success_helper(SYSPARAM_NAMESERVER.max_replication_, send_msg_success);
          uint64_t send_msg_fail[SYSPARAM_NAMESERVER.max_replication_];
          ArrayHelper<uint64_t> send_msg_fail_helper(SYSPARAM_NAMESERVER.max_replication_, send_msg_fail);

          for (int32_t i = 0; i < servers.get_array_index() && TFS_SUCCESS == ret; ++i)
          {
            ServerCollect* pserver = *servers.at(i);
            assert(NULL != pserver);
            //send add new block message to dataserver
            ret = client->post_request(pserver->id(), &msg, send_id);
            if ( TFS_SUCCESS != ret)
            {
              send_msg_fail_helper.push_back(pserver->id());
              TBSYS_LOG(INFO, "send 'New block: %u' msg to server : %s fail",
                  block_id, CNetUtil::addrToString(pserver->id()).c_str());
            }
            else
            {
              send_msg_success_helper.push_back(pserver->id());
              TBSYS_LOG(INFO, "send 'New block: %u' msg to server : %s successful",
                  block_id, CNetUtil::addrToString(pserver->id()).c_str());
            }
          }

          ret = send_msg_success_helper.empty() ? EXIT_CREATE_BLOCK_SEND_MSG_ERROR: TFS_SUCCESS;
          // post message fail, rollback
          if (TFS_SUCCESS != ret)
          {
            print_servers(send_msg_success_helper, all_servers);
            print_servers(send_msg_fail_helper, success_servers);
            TBSYS_LOG(INFO, "add block: %u failed, we'll rollback, send msg successful: %s, failed: %s, alls: %s",
                block_id, all_servers.c_str(), success_servers.c_str(), all_servers.c_str());
          }
          else //有发消息成的
          {
            uint64_t success[SYSPARAM_NAMESERVER.max_replication_];
            ArrayHelper<uint64_t> success_helper(SYSPARAM_NAMESERVER.max_replication_, success);
            client->wait();
            NewClient::RESPONSE_MSG_MAP* sresponse = client->get_success_response();
            NewClient::RESPONSE_MSG_MAP* fresponse = client->get_fail_response();
            ret = send_msg_success_helper.get_array_index() == servers.get_array_index() ? TFS_SUCCESS : EXIT_SEND_RECV_MSG_COUNT_ERROR;
            if (TFS_SUCCESS == ret)
            {
              assert(NULL != sresponse);
              assert(NULL != fresponse);
              tbnet::Packet* packet = NULL;
              StatusMessage* message = NULL;
              for (NewClient::RESPONSE_MSG_MAP_ITER iter = sresponse->begin(); iter != sresponse->end(); ++iter)
              {
                packet = iter->second.second;
                assert(NULL != packet);
                if (packet->getPCode() == STATUS_MESSAGE)
                {
                  message =  dynamic_cast<StatusMessage*>(packet);
                  if (STATUS_MESSAGE_OK == message->get_status())
                  {
                    if (send_msg_success_helper.find(iter->second.first))
                      success_helper.push_back(iter->second.first);
                  }
                }
              }
              print_servers(success_helper, success_servers);
              ret = success_helper.get_array_index() != servers.get_array_index() ? EXIT_ADD_NEW_BLOCK_ERROR : TFS_SUCCESS;
              if (TFS_SUCCESS != ret)//add block failed, rollback
              {
                print_servers(send_msg_success_helper, all_servers);
                TBSYS_LOG(INFO, "add block: %u fail, we'll rollback, servers: %s, success: %s",
                    block_id, all_servers.c_str(), success_servers.c_str());
              }
              else
              {
                TBSYS_LOG(INFO, "add block: %u on servers: %s successful", block_id, success_servers.c_str());
              }
            }
          }
          NewClientManager::get_instance().destroy_client(client);
        }
      }
      return ret;
    }

    int LayoutManager::add_new_block_helper_build_relation_(BlockCollect* block, const common::ArrayHelper<ServerCollect*>& servers, const time_t now)
    {
      //build relation
      int32_t ret =  (NULL != block) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        for (int32_t i = 0; i < servers.get_array_index() && TFS_SUCCESS == ret; ++i)
        {
          bool writable = false;
          bool master   = false;
          ServerCollect* pserver = *servers.at(i);
          ret = get_block_manager().build_relation(block, writable, master, pserver, now, true);
          if (TFS_SUCCESS == ret)
            ret = get_server_manager().build_relation(pserver, block, writable, master);
          else
            TBSYS_LOG(INFO, "build relation fail between dataserver: %s and block: %u",
                CNetUtil::addrToString(pserver->id()).c_str(), block->id());
        }
      }
      return ret;
    }

    BlockCollect* LayoutManager::add_new_block_(uint32_t& block_id, ServerCollect* server, const time_t now)
    {
      return block_id != 0 ? add_new_block_helper_create_by_id_(block_id, now)
        : add_new_block_helper_create_by_system_(block_id, server, now);
    }

    BlockCollect* LayoutManager::add_new_block_helper_create_by_id_(const uint32_t block_id, const time_t now)
    {
      BlockCollect* block = NULL;
      int32_t ret =  (0 != block_id) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        ServerCollect* exist[SYSPARAM_NAMESERVER.max_replication_];
        ArrayHelper<ServerCollect*> helper(SYSPARAM_NAMESERVER.max_replication_, exist);
        ServerCollect* servers[SYSPARAM_NAMESERVER.max_replication_];
        ArrayHelper<ServerCollect*> news(SYSPARAM_NAMESERVER.max_replication_, servers);
        bool new_create_block_collect = false;
        block = get_block_manager().get(block_id);
        if (NULL == block)//block not found in nameserver meta
        {
          block = get_block_manager().insert(block_id, now, true);//create block && insert block map
          new_create_block_collect = NULL != block;
        }
        ret = (NULL != block) ? TFS_SUCCESS : EXIT_NO_BLOCK;
        if (TFS_SUCCESS == ret)//find or create block successful
        {
          get_block_manager().get_servers(helper, block);
          int32_t count = SYSPARAM_NAMESERVER.max_replication_ - helper.get_array_index();
          if (count > 0)
          {
            GCObject* pobject = NULL;
            get_server_manager().choose_create_block_target_server(helper, news, count);
            ret = helper.empty() ? EXIT_CHOOSE_CREATE_BLOCK_TARGET_SERVER_ERROR : TFS_SUCCESS;
            if (TFS_SUCCESS != ret)
            {
              TBSYS_LOG(INFO, "create block: %u by block id fail, dataserver is not enough", block_id);
              if (new_create_block_collect)
                get_block_manager().remove(pobject,block_id);
            }
            else//elect dataserver successful
            {
              if (!news.empty())
              {
                ret = add_new_block_helper_send_msg_(block_id, news);
                if (TFS_SUCCESS == ret)
                {
                  //build relation
                  ret = add_new_block_helper_build_relation_(block, news, now);
                  if (TFS_SUCCESS == ret)
                  {
                    add_new_block_helper_write_log_(block_id, news, now);
                    oplog_sync_mgr_.generation(block_id);
                  }
                }//end send message to dataserver successful
                else
                {
                  block = get_block_manager().get(block_id);
                  if ((new_create_block_collect)
                      || ((NULL != block)
                        && (!new_create_block_collect)
                        && (!block->is_creating())
                        && (block->get_servers_size() <= 0)))
                  {
                    get_block_manager().remove(pobject,block_id);
                  }
                }
              }
              if (NULL != pobject)
                get_gc_manager().add(pobject);
            }//end elect dataserver successful
          }//end if (count >0)
        }//end find or create block successful
      }//end if (bret)
      return ret == TFS_SUCCESS ? block : NULL;
    }

    BlockCollect* LayoutManager::add_new_block_helper_create_by_system_(uint32_t& block_id, ServerCollect* server, const time_t now)
    {
      BlockCollect* block = NULL;
      int32_t ret =  (0 == block_id) ? TFS_SUCCESS : EXIT_PARAMETER_ERROR;
      if (TFS_SUCCESS == ret)
      {
        ServerCollect* result[SYSPARAM_NAMESERVER.max_replication_];
        ArrayHelper<ServerCollect*> helper(SYSPARAM_NAMESERVER.max_replication_, result);
        ServerCollect* news[SYSPARAM_NAMESERVER.max_replication_];
        ArrayHelper<ServerCollect*> news_helper(SYSPARAM_NAMESERVER.max_replication_, news);
        if (NULL != server)
          helper.push_back(server);

        block_id = get_alive_block_id_();
        ret = (INVALID_BLOCK_ID == block_id) ? EXIT_BLOCK_ID_INVALID_ERROR : TFS_SUCCESS;
        if (TFS_SUCCESS == ret)
        {
          //add block collect object
          block = get_block_manager().insert(block_id, now, true);
          ret = (NULL != block) ? TFS_SUCCESS : EXIT_NO_BLOCK;
        }

        if (TFS_SUCCESS == ret)
        {
          int32_t count = SYSPARAM_NAMESERVER.max_replication_ - helper.get_array_index();
          if (count > 0)
          {
            get_server_manager().choose_create_block_target_server(helper, news_helper, count);
          }
          GCObject* pobject = NULL;
          ret = !helper.empty() ? TFS_SUCCESS : EXIT_CHOOSE_CREATE_BLOCK_TARGET_SERVER_ERROR;
          if (TFS_SUCCESS == ret)//add block collect object successful
          {
            ret = add_new_block_helper_send_msg_(block_id, helper);
            if (TFS_SUCCESS == ret)
            {
              //build relation
              ret = add_new_block_helper_build_relation_(block, helper, now);
              if (TFS_SUCCESS == ret)
              {
                add_new_block_helper_write_log_(block_id, helper, now);
              }
            }//end send message to dataserver successful
            else
            {
              get_block_manager().remove(pobject, block_id);//rollback
            }
          }
          else
          {
            get_block_manager().remove(pobject, block_id);//rollback
          }
          if (NULL != pobject)
            get_gc_manager().add(pobject);
        }
      }//end if (TFS_SUCCESS == ret) check parameter
      return TFS_SUCCESS == ret ? block : NULL;
    }

    bool LayoutManager::build_emergency_replicate_(int64_t& need, std::deque<BlockCollect*>& queue, const time_t now)
    {
      int32_t count = queue.size();
      BlockCollect* block = NULL;
      while (!queue.empty() && need > 0 && count-- > 0)
      {
        block = queue.front();
        queue.pop_front();
        if (!build_replicate_task_(need, block, now))
        {
          if (get_block_manager().need_replicate(block, now))
            queue.push_back(block);
        }
        else
        {
          --need;
        }
      }
      return true;
    }

    bool LayoutManager::check_emergency_replicate_(std::deque<BlockCollect*>& queue, ArrayHelper<BlockCollect*>& result,
        const int32_t count, const time_t now)
    {
      ServerCollect* server = NULL;
      while (NULL != (server = get_server_manager().pop_from_dead_queue(now)))
      {
        result.clear();
        bool complete = false;
        uint32_t start = 0;
        BlockCollect* block = NULL;
        do
        {
          result.clear();
          complete = server->get_range_blocks(result, start, count);
          for (int32_t i = 0; i < result.get_array_index(); ++i)
          {
            block = *result.at(i);
            assert(NULL != block);
            get_block_manager().relieve_relation(block, server, now);
            if (block->check_replicate(now))
              queue.push_back(block);
          }
          if (!result.empty())
            start = block->id();
        }
        while (!complete);
        server->update_last_time(now);
        get_gc_manager().add(server);
      }
      return true;
    }

    bool LayoutManager::build_replicate_task_(int64_t& need, const BlockCollect* block, const time_t now)
    {
      bool ret = ((NULL != block) && (plan_run_flag_ & PLAN_RUN_FLAG_REPLICATE) && (need > 0));
      if (ret)
      {
        PlanPriority priority = PLAN_PRIORITY_NONE;
        ServerCollect* servers[SYSPARAM_NAMESERVER.max_replication_];
        ArrayHelper<ServerCollect*> helper(SYSPARAM_NAMESERVER.max_replication_, servers);
        if ((ret = get_block_manager().need_replicate(helper, priority, block, now)))
        {
          ServerCollect* target = NULL;
          ServerCollect* source = NULL;
          get_server_manager().choose_replicate_source_server(source, helper);
          if (NULL == source)
          {
            TBSYS_LOG(INFO, "replicate block: %u cannot found source dataserver", block->id());
          }
          else
          {
            get_server_manager().choose_replicate_target_server(target, helper);
            if (NULL == target)
            {
              TBSYS_LOG(INFO, "replicate block: %u cannot found target dataserver", block->id());
            }
          }

          ret = ((NULL != source) && (NULL != target));
          if (ret)
          {
            std::vector<ServerCollect*> runer;
            runer.push_back(source);
            runer.push_back(target);
            ret = TFS_SUCCESS == get_task_manager().add(block->id(), runer, PLAN_TYPE_REPLICATE, now, priority);
          }
        }
      }
      return ret;
    }

    bool LayoutManager::build_compact_task_(const BlockCollect* block, const time_t now)
    {
      bool ret = ((NULL != block) && (plan_run_flag_ & PLAN_RUN_FLAG_COMPACT));
      if (ret)
      {
        ServerCollect* servers[SYSPARAM_NAMESERVER.max_replication_];
        ArrayHelper<ServerCollect*> helper(SYSPARAM_NAMESERVER.max_replication_, servers);
        ret = get_block_manager().need_compact(helper, block, now);
        if (ret)
        {
          ServerCollect* pserver = NULL;
          std::vector<ServerCollect*> runer;
          for (int32_t i = 0; i < helper.get_array_index(); ++i)
          {
            pserver = *helper.at(i);
            assert(NULL != pserver);
            runer.push_back(pserver);
          }
          ret = TFS_SUCCESS == get_task_manager().add(block->id(), runer, PLAN_TYPE_COMPACT, now);
        }
      }
      return ret;
    }

    bool LayoutManager::build_balance_task_(int64_t& need, common::TfsSortedVector<ServerCollect*,ServerIdCompare>& targets,
        const ServerCollect* source, const BlockCollect* block, const time_t now)
    {
      bool ret = ((NULL != block) && (NULL != source) && (plan_run_flag_ & PLAN_RUN_FLAG_MOVE) && (need > 0));
      if (ret)
      {
        ServerCollect* result= NULL;
        ServerCollect* servers[SYSPARAM_NAMESERVER.max_replication_];
        ArrayHelper<ServerCollect*> helper(SYSPARAM_NAMESERVER.max_replication_, servers);
        ret = get_block_manager().need_balance(helper, block, now);
        if (ret)
        {
          ret = helper.find(const_cast<ServerCollect*>(source));
          if (!ret)
          {
            TBSYS_LOG(INFO, "cannot choose move source server, block: %u, source: %s",
                block->id(), CNetUtil::addrToString(source->id()).c_str());
          }
          else
          {
            //helper.remove(const_cast<ServerCollect*>(source));
            get_server_manager().choose_move_target_server(result, targets, helper);
            ret = NULL != result;
            if (!ret)
            {
              TBSYS_LOG(INFO, "cannot choose move target server, block: %u, source: %s",
                  block->id(), CNetUtil::addrToString(source->id()).c_str());
            }
            else
            {
              std::vector<ServerCollect*> runer;
              runer.push_back(const_cast<ServerCollect*>(source));
              runer.push_back(result);
              ret = TFS_SUCCESS == get_task_manager().add(block->id(), runer, PLAN_TYPE_MOVE, now);
            }
          }
        }
      }
      return ret;
    }

    bool LayoutManager::build_redundant_(int64_t& need, const time_t now)
    {
      UNUSED(now);
      bool ret = false;
      const int32_t MAX_DELETE_NUMS = 10;
      if (need > MAX_DELETE_NUMS)
        need = MAX_DELETE_NUMS;
      int32_t count = need * 2;
      std::pair<uint32_t, uint64_t> output;
      while (get_block_manager().pop_from_delete_queue(output) && need > 0 && count-- > 0)
      {
        BlockCollect* block = get_block_manager().get(output.first);
        ServerCollect* server = get_server_manager().get(output.second);
        ret = ((NULL != block) && (NULL != server));
        if (ret)
        {
          if (get_task_manager().exist(output.first))
          {
            get_block_manager().push_to_delete_queue(output.first, output.second);
          }
          else
          {
            std::vector<ServerCollect*> runer;
            runer.push_back(server);
            ret = (TFS_SUCCESS == get_task_manager().add(block->id(), runer, PLAN_TYPE_DELETE, now));
            if (!ret)
            {
              get_block_manager().push_to_delete_queue(output.first, output.second);
            }
            else
            {
              --need;
            }
          }
        }
      }
      return true;
    }

    int64_t LayoutManager::has_space_in_task_queue_() const
    {
      return server_manager_.size() - task_manager_.get_running_server_size();
    }

    bool LayoutManager::has_report_block_server_() const
    {
      return !wait_report_block_servers_.empty() || !current_reporting_block_servers_.empty();
    }

    void LayoutManager::BuildPlanThreadHelper::run()
    {
      try
      {
        manager_.build_();
      }
      catch(std::exception& e)
      {
        TBSYS_LOG(ERROR, "catch exception: %s", e.what());
      }
      catch(...)
      {
        TBSYS_LOG(ERROR, "%s", "catch exception, unknow message");
      }
    }

    void LayoutManager::RunPlanThreadHelper::run()
    {
      try
      {
        manager_.get_task_manager().run();
      }
      catch(std::exception& e)
      {
        TBSYS_LOG(ERROR, "catch exception: %s", e.what());
      }
      catch(...)
      {
        TBSYS_LOG(ERROR, "%s", "catch exception, unknow message");
      }
    }

    void LayoutManager::CheckDataServerThreadHelper::run()
    {
      try
      {
        manager_.check_all_server_isalive_();
      }
      catch(std::exception& e)
      {
        TBSYS_LOG(ERROR, "catch exception: %s", e.what());
      }
      catch(...)
      {
        TBSYS_LOG(ERROR, "%s", "catch exception, unknow message");
      }
    }

    void LayoutManager::AddBlockInAllServerThreadHelper::run()
    {
      try
      {
        manager_.add_block_in_all_server_();
      }
      catch(std::exception& e)
      {
        TBSYS_LOG(ERROR, "catch exception: %s", e.what());
      }
      catch(...)
      {
        TBSYS_LOG(ERROR, "%s", "catch exception, unknow message");
      }
    }

    void LayoutManager::CheckDataServerReportBlockThreadHelper::run()
    {
      try
      {
        manager_.check_all_server_report_block_();
      }
      catch(std::exception& e)
      {
        TBSYS_LOG(ERROR, "catch exception: %s", e.what());
      }
      catch(...)
      {
        TBSYS_LOG(ERROR, "%s", "catch exception, unknow message");
      }
    }

    void LayoutManager::BuildBalanceThreadHelper::run()
    {
      try
      {
        manager_.balance_();
      }
      catch(std::exception& e)
      {
        TBSYS_LOG(ERROR, "catch exception: %s", e.what());
      }
      catch(...)
      {
        TBSYS_LOG(ERROR, "%s", "catch exception, unknow message");
      }
    }

    void LayoutManager::TimeoutThreadHelper::run()
    {
      try
      {
        manager_.timeout_();
      }
      catch(std::exception& e)
      {
        TBSYS_LOG(ERROR, "catch exception: %s", e.what());
      }
      catch(...)
      {
        TBSYS_LOG(ERROR, "%s", "catch exception, unknow message");
      }
    }
  } /** nameserver **/
}/** tfs **/
