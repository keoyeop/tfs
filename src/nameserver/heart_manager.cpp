/*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id: heart_manager.cpp 983 2011-10-31 09:59:33Z duanfei $
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
#include <Mutex.h>
#include "ns_define.h"
#include "nameserver.h"
#include "common/error_msg.h"
#include "common/config_item.h"
#include "common/client_manager.h"
#include "heart_manager.h"
#include "global_factory.h"

using namespace tfs::common;
using namespace tfs::message;
using namespace tbsys;

namespace tfs
{
  namespace nameserver
  {

    HeartManagement::HeartManagement(NameServer& m) :
      meta_mgr_(m),
      keepalive_queue_size_(10240),
      report_block_queue_size_(2),
      keepalive_queue_header_(*this),
      report_block_queue_header_(*this)
    {

    }

    HeartManagement::~HeartManagement()
    {
    }

    int HeartManagement::initialize(const int32_t keepalive_thread_count, const int32_t keepalive_queue_size,
                     const int32_t report_block_thread_count, const int32_t report_block_queue_size)
    {
      keepalive_queue_size_ = keepalive_queue_size;
      report_block_queue_size_ = report_block_queue_size;
      keepalive_threads_.setThreadParameter(keepalive_thread_count, &keepalive_queue_header_, this);
      report_block_threads_.setThreadParameter(report_block_thread_count, &report_block_queue_header_, this);
      keepalive_threads_.start();
      report_block_threads_.start();
      return TFS_SUCCESS;
    }

    void HeartManagement::wait_for_shut_down()
    {
      keepalive_threads_.wait();
      report_block_threads_.wait();
    }

    void HeartManagement::destroy()
    {
      keepalive_threads_.stop(true);
      report_block_threads_.stop(true);
    }

    /**
     * push do lot of things.
     * first call base_type::push check if current processing queue size > max_queue_size_
     * if true cannot processing this heart message, directly response to client with busy repsonse.
     * pay special attention to free the message...
     */
    int HeartManagement::push(common::BasePacket* msg)
    {
      int32_t iret = NULL != msg ? TFS_SUCCESS : EXIT_GENERAL_ERROR;
      if (TFS_SUCCESS == iret)
      {
        bool handled = false;
        int32_t pcode = msg->getPCode();
        int32_t status = 0;
        uint64_t server = 0;
        //normal or login or logout heartbeat message, just push, cannot blocking!
        if (pcode == SET_DATASERVER_MESSAGE)
        {
          SetDataserverMessage* message = dynamic_cast<SetDataserverMessage*>(msg);
          server = message->get_ds().id_;
          status = message->get_ds().status_;
          handled = keepalive_threads_.push(msg, keepalive_queue_size_, false);
        }
        else if (pcode == REQ_REPORT_BLOCKS_TO_NS_MESSAGE)
        {
          //dataserver report block heartbeat message, cannot blocking!
          ReportBlocksToNsRequestMessage* message = dynamic_cast<ReportBlocksToNsRequestMessage*>(msg);
          server = message->get_server();
          handled = report_block_threads_.push(msg, report_block_queue_size_, false);
        }
        else
        {
          TBSYS_LOG(INFO, "pcode: %d invalid", pcode);
        }
        iret = handled ? TFS_SUCCESS : EXIT_GENERAL_ERROR;
        if (TFS_SUCCESS != iret)
        {
          //threadpool busy..cannot handle it
          iret = msg->reply_error_packet(TBSYS_LOG_LEVEL(WARN), STATUS_MESSAGE_ERROR,
              "nameserver heartbeat busy! cannot accept this request from : %s, type: %s, status: %d",
              tbsys::CNetUtil::addrToString(server).c_str(),
              pcode == SET_DATASERVER_MESSAGE ? "heartbeat" :
              pcode == REQ_REPORT_BLOCKS_TO_NS_MESSAGE ? "report block" : "unknown", status);
          // already repsonse, now can free this message object.
          msg->free();
        }
      }
      return iret;
    }

    // event handler
    bool HeartManagement::KeepAliveIPacketQueueHeaderHelper::handlePacketQueue(tbnet::Packet *packet, void *args)
    {
      UNUSED(args);
      bool bret = packet != NULL;
      if (bret)
      {
        //if return TFS_SUCCESS, packet had been delete in this func
        //if handlePacketQueue return true, tbnet will delete this packet
        manager_.keepalive(packet);
      }
      return bret;
    }

    bool HeartManagement::ReportBlockIPacketQueueHeaderHelper::handlePacketQueue(tbnet::Packet *packet, void *args)
    {
      UNUSED(args);
      bool bret = packet != NULL;
      if (bret)
      {
        //if return TFS_SUCCESS, packet had been delete in this func
        //if handlePacketQueue return true, tbnet will delete this packet
        manager_.report_block(packet);
      }
      return bret;
    }

    int HeartManagement::keepalive(tbnet::Packet* packet)
    {
      int32_t iret = NULL != packet ? TFS_SUCCESS : EXIT_GENERAL_ERROR;
      if (TFS_SUCCESS == iret)
      {
        #ifdef TFS_NS_DEBUG
        tbutil::Time begin = tbutil::Time::now();
        #endif
        SetDataserverMessage* message = dynamic_cast<SetDataserverMessage*> (packet);
        assert(SET_DATASERVER_MESSAGE == packet->getPCode());
        RespHeartMessage *result_msg = new RespHeartMessage();
        result_msg->set_heart_interval(SYSPARAM_NAMESERVER.heart_interval_);
        const DataServerStatInfo& ds_info = message->get_ds();
        time_t now = Func::get_monotonic_time();

        iret = meta_mgr_.get_layout_manager().get_client_request_server().keepalive(ds_info, now);
        result_msg->set_status(TFS_SUCCESS == iret ? HEART_MESSAGE_OK : HEART_MESSAGE_FAILED);
        if (TFS_SUCCESS == iret
            && DATASERVER_STATUS_DEAD == ds_info.status_)
        {
          //dataserver exit
          TBSYS_LOG(INFO, "dataserver: %s exit", CNetUtil::addrToString(ds_info.id_).c_str());
        }
        iret = message->reply(result_msg);
        TBSYS_LOG(DEBUG, "dataserver: %s %s %s, ret: %d", CNetUtil::addrToString(ds_info.id_).c_str(),
          DATASERVER_STATUS_DEAD == ds_info.status_ ? "exit" : DATASERVER_STATUS_ALIVE  == ds_info.status_ ? "keepalive" :
          "unknow", TFS_SUCCESS == iret ? "successful" : "failed", iret);
        #ifdef TFS_NS_DEBUG
        tbutil::Time end = tbutil::Time::now() - begin;
        TBSYS_LOG(INFO, "dataserver: %s keepalive consume times: %"PRI64_PREFIX"d(us)",
          CNetUtil::addrToString(ds_info.id_).c_str(), end.toMicroSeconds());
        #endif
      }
      return iret;
    }

    int HeartManagement::report_block(tbnet::Packet* packet)
    {
      uint64_t server = 0;
      int32_t block_nums = 0, expires_block_nums = 0;
      int32_t iret = NULL != packet ? TFS_SUCCESS : EXIT_GENERAL_ERROR;
      if (TFS_SUCCESS == iret)
      {
        #ifdef TFS_NS_DEBUG
        tbutil::Time begin = tbutil::Time::now();
        #endif
        ReportBlocksToNsRequestMessage* message = dynamic_cast<ReportBlocksToNsRequestMessage*> (packet);
        assert(REQ_REPORT_BLOCKS_TO_NS_MESSAGE == packet->getPCode());
        server = message->get_server();
        NsRuntimeGlobalInformation& ngi = GFactory::get_runtime_info();
        ReportBlocksToNsResponseMessage* result_msg = new ReportBlocksToNsResponseMessage();
        result_msg->set_server(ngi.owner_ip_port_);
        time_t now = Func::get_monotonic_time();

			  iret = meta_mgr_.get_layout_manager().get_client_request_server().report_block(server, now, message->get_blocks(), result_msg->get_blocks());
        int8_t status = iret == EIXT_SERVER_OBJECT_NOT_FOUND ? REPORT_BLOCK_SERVER_OBJECT_NOT_FOUND  :
                     iret == EXIT_UPDATE_RELATION_ERROR ? REPORT_BLOCK_UPDATE_RELATION_ERROR : REPORT_BLOCK_OTHER_ERROR;
        result_msg->set_status(status);
        if (TFS_SUCCESS != iret)
        {
          TBSYS_LOG(INFO, "dataserver %s report block failed, ret: %d", tbsys::CNetUtil::addrToString(server).c_str(), iret);
        }
        block_nums = message->get_blocks().size();
        expires_block_nums = result_msg->get_blocks().size();
        #ifdef TFS_NS_DEBUG
        tbutil::Time end = tbutil::Time::now() - begin;
        TBSYS_LOG(INFO, "dataserver: %s report block consume times: %"PRI64_PREFIX"d(us)",
          CNetUtil::addrToString(server).c_str(), end.toMicroSeconds());
        #endif
			  iret = message->reply(result_msg);
      }
      TBSYS_LOG(INFO, "dataserver: %s report block %s, ret: %d, blocks: %d, expires: %d",
         CNetUtil::addrToString(server).c_str(), TFS_SUCCESS == iret ? "successful" : "failed",
         iret, block_nums, expires_block_nums);
      return iret;
    }

    CheckOwnerIsMasterTimerTask::CheckOwnerIsMasterTimerTask(LayoutManager* mm) :
      meta_mgr_(mm)
    {

    }

    void CheckOwnerIsMasterTimerTask::master_lost_vip(NsRuntimeGlobalInformation& ngi)
    {
      if (!tbsys::CNetUtil::isLocalAddr(ngi.vip_))
      {
        TBSYS_LOG(WARN, "%s", "the master ns role modify,i'm going to be the slave ns");
        tbutil::Mutex::Lock lock(ngi);
        ngi.owner_role_ = NS_ROLE_SLAVE;
        ngi.sync_oplog_flag_ = NS_SYNC_DATA_FLAG_NO;
        ngi.set_switch_time();
        meta_mgr_->destroy_plan();
      }
      return;
    }

    void CheckOwnerIsMasterTimerTask::check_when_slave_hold_vip(NsRuntimeGlobalInformation& ngi)
    {
      int iret = -1;
      tbnet::Packet* rmsg = NULL;
      MasterAndSlaveHeartMessage mashm;
      NsStatus other_side_status = NS_STATUS_OTHERSIDEDEAD;
      NsSyncDataFlag ns_sync_flag = NS_SYNC_DATA_FLAG_NO;

      mashm.set_ip_port(ngi.owner_ip_port_);
      mashm.set_role(ngi.owner_role_);
      mashm.set_status(ngi.owner_status_);
      mashm.set_flags(HEART_GET_DATASERVER_LIST_FLAGS_NO);
      ngi.dump(TBSYS_LOG_LEVEL(DEBUG));

      NewClient* client = NewClientManager::get_instance().create_client();
      iret = send_msg_to_server(ngi.other_side_ip_port_, client, &mashm, rmsg);
      if (TFS_SUCCESS != iret) // otherSide dead
      {
        ns_switch(NS_STATUS_OTHERSIDEDEAD, NS_SYNC_DATA_FLAG_NO);
      }
      else
      {
        if (rmsg->getPCode() == MASTER_AND_SLAVE_HEART_RESPONSE_MESSAGE)
        {
          MasterAndSlaveHeartResponseMessage* tmsg = dynamic_cast<MasterAndSlaveHeartResponseMessage*> (rmsg);
          if (tmsg->get_role() == NS_ROLE_SLAVE)
          {
            if (ngi.other_side_status_ != tmsg->get_status()) // update otherside status
            {
              other_side_status = static_cast<NsStatus> (tmsg->get_status());
              if ((other_side_status == NS_STATUS_INITIALIZED) && (ngi.sync_oplog_flag_ != NS_SYNC_DATA_FLAG_YES))
              {
                ns_sync_flag = NS_SYNC_DATA_FLAG_YES;
              }
            }
            ns_switch(other_side_status, ns_sync_flag);
          }
          else // otherside role == master, but vip in local
          {
            if (tbsys::CNetUtil::isLocalAddr(ngi.vip_))// make sure vip in local
            {
              TBSYS_LOG(WARN, "%s", "the master ns role modify,i'm going to be the master ns");
              tbutil::Mutex::Lock lock(ngi);
              ngi.owner_role_ = NS_ROLE_MASTER;
              ngi.other_side_role_ = NS_ROLE_SLAVE;
              ngi.sync_oplog_flag_ = NS_SYNC_DATA_FLAG_YES;
              ngi.set_switch_time();
              meta_mgr_->destroy_plan();
              meta_mgr_->register_report_servers();
              ns_force_modify_other_side();
            }
          }
        }
      }
      NewClientManager::get_instance().destroy_client(client);
      return;
    }

    void CheckOwnerIsMasterTimerTask::runTimerTask()
    {
      NsRuntimeGlobalInformation& ngi = GFactory::get_runtime_info();
      ngi.dump(TBSYS_LOG_LEVEL(DEBUG));
      if (!tbsys::CNetUtil::isLocalAddr(ngi.vip_)) //vip != local ip
      {
        if (ngi.owner_role_ == NS_ROLE_MASTER)
        {
          master_lost_vip(ngi);
        }
      }
      else //vip == local ip
      {
        if (ngi.owner_role_ == NS_ROLE_MASTER)//master
        {
          if (ngi.other_side_role_ == NS_ROLE_MASTER)
          {
            if (tbsys::CNetUtil::isLocalAddr(ngi.vip_)) // make sure owner role master,set otherside role == NS_ROLE_SLAVE
            {
              ns_force_modify_other_side();
            }
          }
          // owner role == master and otherside role == master
        }
        else //i am hold vip but i am slave now
        {
          check_when_slave_hold_vip(ngi);
        }
      }
      return;
    }

    void CheckOwnerIsMasterTimerTask::ns_switch(const NsStatus& other_side_status, const NsSyncDataFlag& ns_sync_flag)
    {
      NsRuntimeGlobalInformation& ngi = GFactory::get_runtime_info();
      TBSYS_LOG(WARN, "the master ns: %s mybe dead, i'm going to be the master ns: %s",
          tbsys::CNetUtil::addrToString(ngi.other_side_ip_port_).c_str(), tbsys::CNetUtil::addrToString(
            ngi.owner_ip_port_).c_str());
      tbutil::Mutex::Lock lock(ngi);
      ngi.owner_role_ = NS_ROLE_MASTER;
      ngi.other_side_role_ = NS_ROLE_SLAVE;
      ngi.other_side_status_ = other_side_status;
      ngi.sync_oplog_flag_ = ns_sync_flag;
      meta_mgr_->destroy_plan();
      ngi.set_switch_time();
      meta_mgr_->get_oplog_sync_mgr().notify_all();
      meta_mgr_->register_report_servers();
      TBSYS_LOG(INFO, "%s", "notify all oplog thread");
      return;
    }

    void CheckOwnerIsMasterTimerTask::ns_force_modify_other_side()
    {
      MasterAndSlaveHeartMessage mashm;
      NsRuntimeGlobalInformation& ngi = GFactory::get_runtime_info();
      int iret = -1;
      tbnet::Packet* rmsg = NULL;

      mashm.set_ip_port(ngi.owner_ip_port_);
      mashm.set_role(NS_ROLE_SLAVE);
      mashm.set_status(ngi.other_side_status_);
      mashm.set_flags(HEART_GET_DATASERVER_LIST_FLAGS_NO);
      mashm.set_force_flags(HEART_FORCE_MODIFY_OTHERSIDE_ROLE_FLAGS_YES);
      bool success = false;
      int32_t count = 0;
      do
      {
        count++;
        rmsg = NULL;
        NewClient* client = NewClientManager::get_instance().create_client();
        iret = send_msg_to_server(ngi.other_side_ip_port_, client, &mashm, rmsg);
        if (TFS_SUCCESS == iret)
        {
          MasterAndSlaveHeartResponseMessage* tmsg = dynamic_cast<MasterAndSlaveHeartResponseMessage*> (rmsg);
          if (tmsg->getPCode() == MASTER_AND_SLAVE_HEART_RESPONSE_MESSAGE)
          {
            if (tmsg->get_role() == NS_ROLE_SLAVE)
            {
              if (ngi.other_side_status_ != tmsg->get_status()) // update otherside status
              {
                NsStatus other_side_status = static_cast<NsStatus> (tmsg->get_status());
                if ((other_side_status == NS_STATUS_INITIALIZED) && (ngi.sync_oplog_flag_ != NS_SYNC_DATA_FLAG_YES))
                {
                  ngi.sync_oplog_flag_ = NS_SYNC_DATA_FLAG_YES;
                  meta_mgr_->get_oplog_sync_mgr().notify_all();
                  TBSYS_LOG(INFO, "%s", "notify all oplog thread");
                }
              }
              success = true;
            }
          }
        }
        NewClientManager::get_instance().destroy_client(client);
      }
      while ((count < 3)
             && !success);
      return;
    }

    MasterHeartTimerTask::MasterHeartTimerTask(LayoutManager* mm) :
      meta_mgr_(mm)
    {

    }

    void MasterHeartTimerTask::runTimerTask()
    {
      NsRuntimeGlobalInformation& ngi = GFactory::get_runtime_info();
      if ((ngi.owner_role_ == NS_ROLE_MASTER)
          && (ngi.owner_status_ >= NS_STATUS_INITIALIZED))
      {
        tbnet::Packet* rmsg = NULL;
        MasterAndSlaveHeartMessage mashm;
        mashm.set_ip_port(ngi.owner_ip_port_);
        mashm.set_role(ngi.owner_role_);
        mashm.set_status(ngi.owner_status_);
        mashm.set_flags(HEART_GET_DATASERVER_LIST_FLAGS_NO);
        ngi.dump(TBSYS_LOG_LEVEL(DEBUG));
        bool slave_is_alive = false;
        int32_t count = 0;
        int32_t iret = TFS_ERROR;
        do
        {
          ++count;
          rmsg = NULL;
          NewClient* client = NewClientManager::get_instance().create_client();
          iret = send_msg_to_server(ngi.other_side_ip_port_,client, &mashm, rmsg);
          if (TFS_SUCCESS == iret)
          {
            if (rmsg->getPCode() == MASTER_AND_SLAVE_HEART_RESPONSE_MESSAGE)
            {
              MasterAndSlaveHeartResponseMessage* tmsg = dynamic_cast<MasterAndSlaveHeartResponseMessage*> (rmsg);
              if (tmsg->get_role() == NS_ROLE_SLAVE)//other side role == slave
              {
                if (ngi.other_side_status_ != tmsg->get_status()) // update otherside status
                {
                  tbutil::Mutex::Lock lock(ngi);
                  if (ngi.other_side_status_ != tmsg->get_status())
                  {
                    ngi.other_side_status_ = static_cast<NsStatus> (tmsg->get_status());
                    if ((ngi.other_side_status_ == NS_STATUS_INITIALIZED) && (ngi.sync_oplog_flag_
                          < NS_SYNC_DATA_FLAG_YES))
                    {
                      ngi.sync_oplog_flag_ = NS_SYNC_DATA_FLAG_YES;
                      meta_mgr_->get_oplog_sync_mgr().notify_all();
                      TBSYS_LOG(INFO, "%s", "notify all oplog thread");
                    }
                  }
                }
                else
                {
                  if ((ngi.other_side_status_ == NS_STATUS_INITIALIZED) && (ngi.sync_oplog_flag_
                      < NS_SYNC_DATA_FLAG_YES))
                  {
                    tbutil::Mutex::Lock lock(ngi);
                    if ((ngi.other_side_status_ == NS_STATUS_INITIALIZED) && (ngi.sync_oplog_flag_
                          < NS_SYNC_DATA_FLAG_YES))
                    {
                      ngi.sync_oplog_flag_ = NS_SYNC_DATA_FLAG_YES;
                      meta_mgr_->get_oplog_sync_mgr().notify_all();
                      TBSYS_LOG(INFO, "%s", "notify all oplog thread");
                    }
                  }
                }
                slave_is_alive = true;
              }
              else
              {
                TBSYS_LOG(WARN, "do master and slave heart fail: owner role: %s, other side role: %s", ngi.owner_role_
                  == NS_ROLE_MASTER ? "master" : "slave", tmsg->get_role() == NS_ROLE_MASTER ? "master" : "slave");
              }
            }
          }
          NewClientManager::get_instance().destroy_client(client);
        }
        while ((count < 0x03)
              && (!slave_is_alive));

        if (!slave_is_alive)
        {
          //slave dead
          TBSYS_LOG(WARN, "slave: %s dead", tbsys::CNetUtil::addrToString(ngi.other_side_ip_port_).c_str());
          tbutil::Mutex::Lock lock(ngi);
          ngi.sync_oplog_flag_ = NS_SYNC_DATA_FLAG_NO;
          ngi.other_side_status_ = NS_STATUS_UNINITIALIZE;
        }
      }
    }

    SlaveHeartTimerTask::SlaveHeartTimerTask(LayoutManager* mm, tbutil::TimerPtr& timer) :
      meta_mgr_(mm), timer_(timer)
    {

    }

    void SlaveHeartTimerTask::runTimerTask()
    {
      NsRuntimeGlobalInformation& ngi = GFactory::get_runtime_info();
      if ((ngi.owner_role_ == NS_ROLE_SLAVE)
          && (ngi.owner_status_ >= NS_STATUS_INITIALIZED))
      {
        tbnet::Packet* rmsg = NULL;
        MasterAndSlaveHeartMessage mashm;
        mashm.set_ip_port(ngi.owner_ip_port_);
        mashm.set_role(ngi.owner_role_);
        mashm.set_status(ngi.owner_status_);
        mashm.set_flags(HEART_GET_DATASERVER_LIST_FLAGS_NO);
        ngi.dump(TBSYS_LOG_LEVEL(DEBUG));
        int32_t count = 0;
        int32_t iret = TFS_ERROR;
        bool master_is_alive = false;
        bool switch_flag = false;
        do
        {
          ++count;
          rmsg = NULL;
          NewClient* client = NewClientManager::get_instance().create_client();
          iret = send_msg_to_server(ngi.other_side_ip_port_,client, &mashm, rmsg);
          if (TFS_SUCCESS == iret)
          {
            if (rmsg->getPCode() == MASTER_AND_SLAVE_HEART_RESPONSE_MESSAGE)
            {
              MasterAndSlaveHeartResponseMessage* tmsg = dynamic_cast<MasterAndSlaveHeartResponseMessage*> (rmsg);
              if (tmsg->get_role() == NS_ROLE_MASTER) //i am slave
              {
                if (ngi.other_side_status_ != tmsg->get_status()) // update otherside status
                {
                  tbutil::Mutex::Lock lock(ngi);
                  ngi.other_side_status_ = static_cast<NsStatus> (tmsg->get_status());
                }
                master_is_alive = true;
              }
              else
              {
                TBSYS_LOG(WARN, "do master and slave heart fail: owner role: %s, other side role: %s", ngi.owner_role_
                  == NS_ROLE_MASTER ? "master" : "slave", tmsg->get_role() == NS_ROLE_MASTER ? "master" : "slave");
              }
            }
          }
          NewClientManager::get_instance().destroy_client(client);
          if (!master_is_alive)
          {
            if (tbsys::CNetUtil::isLocalAddr(ngi.vip_)) // vip == local ip
            {
              TBSYS_LOG(WARN, "%s", "the master ns is dead,i'm going to be the master ns");
              tbutil::Mutex::Lock lock(ngi);
              ngi.owner_role_ = NS_ROLE_MASTER;
              ngi.other_side_role_ = NS_ROLE_SLAVE;
              ngi.other_side_status_ = NS_STATUS_OTHERSIDEDEAD;
              ngi.set_switch_time();
              meta_mgr_->destroy_plan();
              meta_mgr_->register_report_servers();
              switch_flag = true;
            }
          }
        }
        while ((count < 0x03)
                && (!master_is_alive));

        if ((!master_is_alive)// master is dead and vip != local ip
            && (!switch_flag))
        {
          //make sure master dead
          {
            tbutil::Mutex::Lock lock(ngi);
            ngi.other_side_status_ = NS_STATUS_OTHERSIDEDEAD;
          }

          do
          {
            TBSYS_LOG(DEBUG, "%s", "the master ns is dead,check vip...");
            if (tbsys::CNetUtil::isLocalAddr(ngi.vip_)) // vip == local ip
            {
              TBSYS_LOG(WARN, "%s", "the master ns is dead,I'm going to be the master ns");
              tbutil::Mutex::Lock lock(ngi);
              ngi.owner_role_ = NS_ROLE_MASTER;
              ngi.other_side_role_ = NS_ROLE_SLAVE;
              ngi.other_side_status_ = NS_STATUS_OTHERSIDEDEAD;
              ngi.set_switch_time();
              meta_mgr_->destroy_plan();
              meta_mgr_->register_report_servers();
              break;
            }
            usleep(0x01);
          }
          while ((ngi.other_side_status_ != NS_STATUS_INITIALIZED)
                  && (ngi.owner_status_ == NS_STATUS_INITIALIZED));
        }
      }
    }

    MasterAndSlaveHeartManager::MasterAndSlaveHeartManager(LayoutManager* mm, tbutil::TimerPtr& timer) :
      meta_mgr_(mm), timer_(timer)
    {

    }

    MasterAndSlaveHeartManager::~MasterAndSlaveHeartManager()
    {

    }

    int MasterAndSlaveHeartManager::initialize()
    {
      work_thread_.setThreadParameter(1, this, this);
      work_thread_.start();
      return TFS_SUCCESS;
   }

    int MasterAndSlaveHeartManager::wait_for_shut_down()
    {
      work_thread_.wait();
      return TFS_SUCCESS;
    }

    int MasterAndSlaveHeartManager::destroy()
    {
      work_thread_.stop(true);
      return TFS_SUCCESS;
    }

    int MasterAndSlaveHeartManager::push(common::BasePacket* message, const int32_t max_queue_size, const bool block)
    {
      return work_thread_.push(message, max_queue_size, block);
    }

    bool MasterAndSlaveHeartManager::handlePacketQueue(tbnet::Packet *packet, void *args)
    {
      bool bret = packet != NULL;
      if (bret)
      {
        common::BasePacket* message = dynamic_cast<common::BasePacket*>(packet);
        int32_t iret = TFS_SUCCESS;
        if (message->getPCode() == HEARTBEAT_AND_NS_HEART_MESSAGE)
        {
          iret = do_heartbeat_and_ns_msg(message, args);//check heartbeat and nameserver heart message
        }
        else
        {
          NsRuntimeGlobalInformation& ngi = GFactory::get_runtime_info();
          if (ngi.owner_role_ == NS_ROLE_MASTER) //master
            iret = do_master_msg(message, args);
          else if (ngi.owner_role_ == NS_ROLE_SLAVE) //slave
            iret = do_slave_msg(message, args);
        }
      }
      return bret;
    }

    int MasterAndSlaveHeartManager::do_master_msg(common::BasePacket* message, void*)
    {
      int32_t iret = NULL != message ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        NsRuntimeGlobalInformation& ngi = GFactory::get_runtime_info();
        ngi.dump(TBSYS_LOG_LEVEL(DEBUG));
        MasterAndSlaveHeartMessage* mashm = dynamic_cast<MasterAndSlaveHeartMessage*> (message);
        if ((mashm->get_force_flags() == HEART_FORCE_MODIFY_OTHERSIDE_ROLE_FLAGS_YES)
            && (ngi.other_side_ip_port_ == mashm->get_ip_port())
            && (ngi.owner_role_ != mashm->get_role()))
        {
          tbutil::Mutex::Lock lock(ngi);
          ngi.owner_role_ = static_cast<NsRole> (mashm->get_role());
          ngi.other_side_role_ = ngi.owner_role_ == NS_ROLE_MASTER ? NS_ROLE_SLAVE : NS_ROLE_MASTER;
          ngi.owner_status_ = static_cast<NsStatus> (mashm->get_status());
          ngi.sync_oplog_flag_ = ngi.owner_role_ == NS_ROLE_MASTER ? NS_SYNC_DATA_FLAG_YES : NS_SYNC_DATA_FLAG_NO;
          if (ngi.sync_oplog_flag_ == NS_SYNC_DATA_FLAG_YES)
            meta_mgr_->get_oplog_sync_mgr().notify_all();

          TBSYS_LOG(DEBUG, "other side modify owner status, owner status: %s, notify all oplog thread",
              ngi.owner_status_ == NS_STATUS_ACCEPT_DS_INFO ? "acceptdsinfo" : ngi.owner_status_
                  == NS_STATUS_INITIALIZED ? "initialized"
                  : ngi.other_side_status_ == NS_STATUS_OTHERSIDEDEAD ? "other side dead" : "unknow");
        }
        else
        {
          if (mashm->get_role() != NS_ROLE_SLAVE)
          {
            TBSYS_LOG(WARN, "do master and slave heart fail: owner role: %s, other side role: %s", ngi.owner_role_
                == NS_ROLE_MASTER ? "master" : "slave", mashm->get_role() == NS_ROLE_MASTER ? "master" : "slave");
            iret = TFS_ERROR;
          }
          if (TFS_SUCCESS == iret)
          {
            if (ngi.other_side_status_ != mashm->get_status()) //update otherside status
            {
              tbutil::Mutex::Lock lock(ngi);
              ngi.other_side_status_ = static_cast<NsStatus> (mashm->get_status());
              if ((ngi.other_side_status_ == NS_STATUS_INITIALIZED) && (ngi.sync_oplog_flag_ != NS_SYNC_DATA_FLAG_YES))
              {
                ngi.sync_oplog_flag_ = NS_SYNC_DATA_FLAG_YES;
                meta_mgr_->get_oplog_sync_mgr().notify_all();
                TBSYS_LOG(INFO, "%s", "notify all oplog thread");
              }
            }
            else
            {
              if ((ngi.other_side_status_ == NS_STATUS_INITIALIZED) && (ngi.sync_oplog_flag_ < NS_SYNC_DATA_FLAG_YES))
              {
                tbutil::Mutex::Lock lock(ngi);
                ngi.sync_oplog_flag_ = NS_SYNC_DATA_FLAG_YES;
                meta_mgr_->get_oplog_sync_mgr().notify_all();
                TBSYS_LOG(INFO, "%s", "notify all oplog thread");
              }
              else if ((ngi.other_side_status_ >= NS_STATUS_ACCEPT_DS_INFO) && (ngi.sync_oplog_flag_
                  < NS_SYNC_DATA_FLAG_YES))
              {
                tbutil::Mutex::Lock lock(ngi);
                ngi.sync_oplog_flag_ = NS_SYNC_DATA_FLAG_READY;
              }
            }
            ngi.dump(TBSYS_LOG_LEVEL(DEBUG));
          }
        }
        if (TFS_SUCCESS == iret)
        {
          MasterAndSlaveHeartResponseMessage *mashrm = new MasterAndSlaveHeartResponseMessage();
          mashrm->set_ip_port(ngi.owner_ip_port_);
          mashrm->set_role(ngi.owner_role_);
          mashrm->set_status(ngi.owner_status_);
          mashrm->set_flags(mashm->get_flags());

          if (mashm->get_flags() == HEART_GET_DATASERVER_LIST_FLAGS_YES)
          {
            TBSYS_LOG(INFO, "%s", "ns(slave) register");
            meta_mgr_->get_oplog_sync_mgr().get_file_queue_thread()->update_queue_information_header();
            VUINT64 ds_list;
            meta_mgr_->get_alive_server(ds_list);
            mashrm->set_ds_list(ds_list);
          }
          iret = message->reply(mashrm);
        }
      }
      return iret;
    }

    int MasterAndSlaveHeartManager::do_slave_msg(common::BasePacket* message, void*)
    {
      int32_t iret = NULL != message ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        NsRuntimeGlobalInformation& ngi = GFactory::get_runtime_info();
        MasterAndSlaveHeartMessage* mashm = dynamic_cast<MasterAndSlaveHeartMessage*> (message);
        if ((mashm->get_force_flags() == HEART_FORCE_MODIFY_OTHERSIDE_ROLE_FLAGS_YES)
            && (ngi.other_side_ip_port_ == mashm->get_ip_port())
            && (ngi.owner_role_ != mashm->get_role()))
        {
          tbutil::Mutex::Lock lock(ngi);
          ngi.owner_role_ = static_cast<NsRole> (mashm->get_role());
          ngi.other_side_role_ = ngi.owner_role_ == NS_ROLE_MASTER ? NS_ROLE_SLAVE : NS_ROLE_MASTER;
          ngi.owner_status_ = static_cast<NsStatus> (mashm->get_status());
          ngi.sync_oplog_flag_ = ngi.owner_role_ == NS_ROLE_MASTER ? NS_SYNC_DATA_FLAG_YES : NS_SYNC_DATA_FLAG_NO;
          if (ngi.sync_oplog_flag_ == NS_SYNC_DATA_FLAG_YES)
            meta_mgr_->get_oplog_sync_mgr().notify_all();
          TBSYS_LOG(DEBUG, "other side modify owner status, owner status: %s, notify all oplog thread",
              ngi.owner_status_ == NS_STATUS_ACCEPT_DS_INFO ? "acceptdsinfo" : ngi.owner_status_
              == NS_STATUS_INITIALIZED ? "initialized"
              : ngi.other_side_status_ == NS_STATUS_OTHERSIDEDEAD ? "other side dead" : "unknow");
        }
        else
        {
          if (mashm->get_role() != NS_ROLE_MASTER)
          {
            TBSYS_LOG(WARN, "do master and slave heart fail: owner role: %s, other side role: %s", ngi.owner_role_
                == NS_ROLE_MASTER ? "master" : "slave", mashm->get_role() == NS_ROLE_MASTER ? "master" : "slave");
            iret = TFS_ERROR;
          }
          else
          {
            if (ngi.other_side_status_ != mashm->get_status()) //update otherside status
            {
              tbutil::Mutex::Lock lock(ngi);
              ngi.other_side_status_ = static_cast<NsStatus> (mashm->get_status());
            }
          }
        }
        if (TFS_SUCCESS == iret)
        {
          MasterAndSlaveHeartResponseMessage *mashrm = new MasterAndSlaveHeartResponseMessage();
          mashrm->set_ip_port(ngi.owner_ip_port_);
          mashrm->set_role(ngi.owner_role_);
          mashrm->set_status(ngi.owner_status_);
          mashrm->set_flags(HEART_GET_DATASERVER_LIST_FLAGS_NO);
          iret = message->reply(mashrm);
        }
      }
      return iret;
    }

    int MasterAndSlaveHeartManager::do_heartbeat_and_ns_msg(common::BasePacket* message, void*)
    {
      int32_t iret = NULL != message && message->getPCode() == HEARTBEAT_AND_NS_HEART_MESSAGE ? TFS_SUCCESS : TFS_ERROR;
      if (TFS_SUCCESS == iret)
      {
        NsRuntimeGlobalInformation& ngi = GFactory::get_runtime_info();
        HeartBeatAndNSHeartMessage* hbam = dynamic_cast<HeartBeatAndNSHeartMessage*> (message);
        int32_t ns_switch_flag = hbam->get_ns_switch_flag();
        TBSYS_LOG(DEBUG, "ns_switch_flag: %s, status: %d",
            hbam->get_ns_switch_flag() == NS_SWITCH_FLAG_NO ? "no" : "yes", hbam->get_ns_status());
        HeartBeatAndNSHeartMessage* mashrm = new HeartBeatAndNSHeartMessage();
        mashrm->set_ns_switch_flag_and_status(0 /*no use*/ , ngi.owner_status_);
        message->reply(mashrm);

        if (ns_switch_flag == NS_SWITCH_FLAG_YES)
        {
          TBSYS_LOG(WARN, "ns_switch_flag: %s, status: %d", hbam->get_ns_switch_flag() == NS_SWITCH_FLAG_NO ? "no"
              : "yes", hbam->get_ns_status());
          do
          {
            TBSYS_LOG(DEBUG, "%s", "the master ns is dead,check vip...");
            if (tbsys::CNetUtil::isLocalAddr(ngi.vip_)) // vip == local ip
            {
              TBSYS_LOG(WARN, "%s", "the master ns is dead,i'm going to be the master ns");
              tbutil::Mutex::Lock lock(ngi);
              ngi.owner_role_ = NS_ROLE_MASTER;
              ngi.owner_status_ = NS_STATUS_INITIALIZED;
              ngi.other_side_role_ = NS_ROLE_SLAVE;
              ngi.other_side_status_ = NS_STATUS_OTHERSIDEDEAD;
              ngi.set_switch_time();
              meta_mgr_->destroy_plan();
              break;
            }
            usleep(0x01);
          }
          while ((ngi.other_side_status_ != NS_STATUS_INITIALIZED)
                && (ngi.owner_status_ == NS_STATUS_INITIALIZED));
        }
      }
      return iret;
    }
  }/** nameserver **/
} /** tfs **/
