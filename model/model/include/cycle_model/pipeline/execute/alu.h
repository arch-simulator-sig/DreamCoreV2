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
#include "../integer_readreg_execute.h"
#include "../execute_wb.h"
#include "../execute.h"
#include "../commit.h"
#include "bru_define.h"
#include "sau_define.h"

namespace cycle_model::pipeline::execute
{
    class alu : public if_reset_t
    {
        private:
            global_inst *global;
            uint32_t id = 0;
            component::handshake_dff<integer_readreg_execute_pack_t> *readreg_alu_hdff;
            component::port<execute_wb_pack_t> *alu_wb_port;
            trace::trace_database tdb;
        
        public:
            alu(global_inst *global, uint32_t id, component::handshake_dff<integer_readreg_execute_pack_t> *readreg_alu_hdff, component::port<execute_wb_pack_t> *alu_wb_port);
            virtual void reset();
            execute_feedback_channel_t run(const bru_feedback_pack_t &bru_feedback_pack, const sau_feedback_pack_t &sau_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack);
    };
}