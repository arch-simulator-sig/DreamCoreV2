/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-15     lizhirui     the first version
 */

#pragma once
#include "common.h"
#include "../../component/handshake_dff.h"
#include "../../component/port.h"
#include "../../component/bus.h"
#include "../../component/store_buffer.h"
#include "../../component/retired_store_buffer.h"
#include "../../component/slave/clint.h"
#include "../../component/load_queue.h"
#include "../../component/wait_table.h"
#include "../../component/pma_unit.h"
#include "../lsu_readreg_execute.h"
#include "../execute_wb.h"
#include "../execute.h"
#include "../commit.h"
#include "bru_define.h"
#include "lu_define.h"
#include "sau_define.h"

namespace cycle_model::pipeline::execute
{
    class lu : if_reset_t
    {
        private:
            global_inst *global;
            uint32_t id = 0;
            component::handshake_dff<lsu_readreg_execute_pack_t> *readreg_lu_hdff;
            component::port<execute_wb_pack_t> *lu_wb_port;
            component::bus *cacheable_bus;
            component::bus *noncacheable_bus;
            component::store_buffer *store_buffer;
            component::retired_store_buffer *retired_store_buffer;
            component::wait_table *wait_table;
            component::pma_unit *pma_unit;
            
            bool l2_stall = false;
            bool l2_last_stall = false;
            bool l2_last_last_stall = false;
            lsu_readreg_execute_pack_t l2_rev_pack;
            uint32_t l2_addr = 0;
            uint32_t l2_feedback_value = 0;
            uint32_t l2_feedback_mask = 0;
            bool l2_conflict_found = false;
            uint32_t l2_length = 0;
            bool l2_is_cacheable = false;
            trace::trace_database tdb;
            
            component::slave::clint *clint;//only for debug
        
        public:
            typedef struct clint_sync_info_t
            {
                uint64_t mtime;
                uint64_t mtimecmp;
                uint32_t msip;
            }clint_sync_info;
            
            clint_sync_info_t clint_sync_list[ROB_SIZE] = {{0, 0, 0}};
            component::load_queue *load_queue;
            
            lu(global_inst *global, uint32_t id, component::handshake_dff<lsu_readreg_execute_pack_t> *readreg_lu_hdff, component::port<execute_wb_pack_t> *lu_wb_port, component::bus *cacheable_bus, component::bus *noncacheable_bus, component::store_buffer *store_buffer, component::retired_store_buffer *retired_store_buffer, component::slave::clint *clint, component::load_queue *load_queue, component::wait_table *wait_table, component::pma_unit *pma_unit);
            virtual void reset();
            std::tuple<execute_feedback_channel_t, lu_feedback_pack_t> run(const bru_feedback_pack_t &bru_feedback_pack, const sau_feedback_pack_t &sau_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack, bool need_lu_feedback_only);
            virtual json get_json();
    };
}