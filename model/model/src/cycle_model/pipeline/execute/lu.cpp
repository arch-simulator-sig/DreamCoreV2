/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-15     lizhirui     the first version
 */

#include "common.h"
#include "cycle_model/pipeline/execute/lu.h"
#include "cycle_model/component/handshake_dff.h"
#include "cycle_model/pipeline/lsu_readreg_execute.h"
#include "cycle_model/pipeline/execute_wb.h"
#include "cycle_model/component/bus.h"
#include "cycle_model/component/store_buffer.h"
#include "cycle_model/component/slave/clint.h"
#include "cycle_model/component/age_compare.h"

namespace cycle_model::pipeline::execute
{
    lu::lu(global_inst *global, uint32_t id, component::handshake_dff<lsu_readreg_execute_pack_t> *readreg_lu_hdff, component::port<execute_wb_pack_t> *lu_wb_port, component::bus *cacheable_bus, component::bus *noncacheable_bus, component::store_buffer *store_buffer, component::retired_store_buffer *retired_store_buffer, component::slave::clint *clint, component::load_queue *load_queue, component::wait_table *wait_table, component::pma_unit *pma_unit) : tdb(TRACE_EXECUTE_LSU)
    {
        this->global = global;
        this->id = id;
        this->readreg_lu_hdff = readreg_lu_hdff;
        this->lu_wb_port = lu_wb_port;
        this->cacheable_bus = cacheable_bus;
        this->noncacheable_bus = noncacheable_bus;
        this->store_buffer = store_buffer;
        this->retired_store_buffer = retired_store_buffer;
        this->clint = clint;
        this->load_queue = load_queue;
        this->wait_table = wait_table;
        this->pma_unit = pma_unit;
        this->lu::reset();
    }
    
    void lu::reset()
    {
        this->l2_stall = false;
        this->l2_last_stall = false;
        this->l2_last_last_stall = false;
        this->l2_rev_pack = lsu_readreg_execute_pack_t();
        this->l2_addr = 0;
    }
    
    std::tuple<execute_feedback_channel_t, lu_feedback_pack_t> lu::run(const execute::bru_feedback_pack_t &bru_feedback_pack, const execute::sau_feedback_pack_t &sau_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack, bool need_lu_feedback_only)
    {
        //level 2 pipeline: get memory data
        execute_wb_pack_t send_pack;
        lu_feedback_pack_t lu_feedback_pack;
        bool load_stall = false;
        auto l2_bus = l2_is_cacheable ? cacheable_bus : noncacheable_bus;
        
        if(!need_lu_feedback_only)
        {
            this->l2_last_stall = this->l2_stall;
        }
        
        lu_feedback_pack.replay = l2_conflict_found;
        lu_feedback_pack.replay_following_only = false;
        lu_feedback_pack.rob_id = l2_rev_pack.rob_id;
        lu_feedback_pack.rob_id_stage = l2_rev_pack.rob_id_stage;
        
        if(l2_rev_pack.enable && l2_conflict_found && !need_lu_feedback_only)
        {
            load_queue->add_replay_num(l2_rev_pack.load_queue_id);
            wait_table->set_wait_bit(l2_rev_pack.pc);
        }
        
        if(l2_rev_pack.enable && !l2_conflict_found && !commit_feedback_pack.flush && (!bru_feedback_pack.flush ||
          (component::age_compare(l2_rev_pack.rob_id, l2_rev_pack.rob_id_stage) >= component::age_compare(bru_feedback_pack.rob_id, bru_feedback_pack.rob_id_stage)))
          && (!sau_feedback_pack.flush || (component::age_compare(l2_rev_pack.rob_id, l2_rev_pack.rob_id_stage) > component::age_compare(sau_feedback_pack.rob_id, sau_feedback_pack.rob_id_stage))))
        {
            verify_only(l2_rev_pack.valid);
            send_pack.inst_common_info = l2_rev_pack.inst_common_info;
            send_pack.enable = l2_rev_pack.enable;
            send_pack.value = l2_rev_pack.value;
            send_pack.valid = l2_rev_pack.valid;
            send_pack.last_uop = l2_rev_pack.last_uop;
            send_pack.rob_id = l2_rev_pack.rob_id;
            send_pack.rob_id_stage = l2_rev_pack.rob_id_stage;
            send_pack.pc = l2_rev_pack.pc;
            send_pack.imm = l2_rev_pack.imm;
            send_pack.has_exception = l2_rev_pack.has_exception;
            send_pack.exception_id = l2_rev_pack.exception_id;
            send_pack.exception_value = l2_rev_pack.exception_value;
            send_pack.branch_predictor_info_pack = l2_rev_pack.branch_predictor_info_pack;
            
            send_pack.rs1 = l2_rev_pack.rs1;
            send_pack.arg1_src = l2_rev_pack.arg1_src;
            send_pack.rs1_need_map = l2_rev_pack.rs1_need_map;
            send_pack.rs1_phy = l2_rev_pack.rs1_phy;
            send_pack.src1_value = l2_rev_pack.src1_value;
            
            send_pack.rs2 = l2_rev_pack.rs2;
            send_pack.arg2_src = l2_rev_pack.arg2_src;
            send_pack.rs2_need_map = l2_rev_pack.rs2_need_map;
            send_pack.rs2_phy = l2_rev_pack.rs2_phy;
            send_pack.src2_value = l2_rev_pack.src2_value;
            
            send_pack.rd = l2_rev_pack.rd;
            send_pack.rd_enable = l2_rev_pack.rd_enable;
            send_pack.need_rename = l2_rev_pack.need_rename;
            send_pack.rd_phy = l2_rev_pack.rd_phy;
            
            send_pack.csr = l2_rev_pack.csr;
            send_pack.load_queue_id = l2_rev_pack.load_queue_id;
            send_pack.op = l2_rev_pack.op;
            send_pack.op_unit = l2_rev_pack.op_unit;
            memcpy((void *)&send_pack.sub_op, (void *)&l2_rev_pack.sub_op, sizeof(l2_rev_pack.sub_op));
            
            if(!l2_rev_pack.has_exception)
            {
                component::store_buffer_item_t item;
                uint32_t bus_value = 0;
                uint32_t feedback_value = 0;
                
                switch(l2_rev_pack.sub_op.lu_op)
                {
                    case lu_op_t::lb:
                        load_stall = !l2_bus->get_data_value(&bus_value);
                        feedback_value = (bus_value & (~l2_feedback_mask)) | l2_feedback_value;
                        send_pack.rd_value = sign_extend(feedback_value, 8);
                        break;
                    
                    case lu_op_t::lbu:
                        load_stall = !l2_bus->get_data_value(&bus_value);
                        feedback_value = (bus_value & (~l2_feedback_mask)) | l2_feedback_value;
                        send_pack.rd_value = feedback_value;
                        break;
                    
                    case lu_op_t::lh:
                        load_stall = !l2_bus->get_data_value(&bus_value);
                        feedback_value = (bus_value & (~l2_feedback_mask)) | l2_feedback_value;
                        send_pack.rd_value = sign_extend(feedback_value, 16);
                        break;
                    
                    case lu_op_t::lhu:
                        load_stall = !l2_bus->get_data_value(&bus_value);
                        feedback_value = (bus_value & (~l2_feedback_mask)) | l2_feedback_value;
                        send_pack.rd_value = feedback_value;
                        break;
                    
                    case lu_op_t::lw:
                        load_stall = !l2_bus->get_data_value(&bus_value);
                        feedback_value = (bus_value & (~l2_feedback_mask)) | l2_feedback_value;
                        send_pack.rd_value = feedback_value;
                        break;
                }
                
                if(load_stall)
                {
                    send_pack = execute_wb_pack_t();
                    lu_feedback_pack.replay = true;
                    lu_feedback_pack.replay_following_only = true;
                }
                else if(this->l2_last_last_stall)
                {
                    lu_feedback_pack.replay = true;
                    lu_feedback_pack.replay_following_only = true;
                }
            }
        }
        else if(!need_lu_feedback_only && l2_rev_pack.enable && !l2_rev_pack.has_exception)
        {
            l2_bus->revoke_data_request();
        }
    
        if(!need_lu_feedback_only)
        {
            this->l2_last_last_stall = !load_stall && this->l2_last_stall;
            this->l2_stall = load_stall;//generate l2_stall signal
        }
        
        lu_feedback_pack.stall = load_stall;
        
        if(!need_lu_feedback_only)
        {
            lu_wb_port->set(send_pack);
        }
        
        //level 1 pipeline: calculate effective address
        lsu_readreg_execute_pack_t rev_pack;
        
        if(!need_lu_feedback_only)
        {
            if(!commit_feedback_pack.flush)
            {
                if(!l2_stall)
                {
                    if(readreg_lu_hdff->pop(&rev_pack))
                    {
                        if(bru_feedback_pack.flush && (component::age_compare(rev_pack.rob_id, rev_pack.rob_id_stage) < component::age_compare(bru_feedback_pack.rob_id, bru_feedback_pack.rob_id_stage)))
                        {
                            l2_addr = 0;
                            l2_rev_pack = {};
                            goto exit;
                        }
        
                        if(sau_feedback_pack.flush && (component::age_compare(rev_pack.rob_id, rev_pack.rob_id_stage) <= component::age_compare(sau_feedback_pack.rob_id, sau_feedback_pack.rob_id_stage)))
                        {
                            l2_addr = 0;
                            l2_rev_pack = {};
                            goto exit;
                        }
                        
                        l2_addr = 0;
                        l2_rev_pack.inst_common_info = rev_pack.inst_common_info;
                        l2_rev_pack.enable = rev_pack.enable;
                        l2_rev_pack.value = rev_pack.value;
                        l2_rev_pack.valid = rev_pack.valid;
                        l2_rev_pack.last_uop = rev_pack.last_uop;
                        l2_rev_pack.rob_id = rev_pack.rob_id;
                        l2_rev_pack.rob_id_stage = rev_pack.rob_id_stage;
                        l2_rev_pack.pc = rev_pack.pc;
                        l2_rev_pack.imm = rev_pack.imm;
                        l2_rev_pack.has_exception = rev_pack.has_exception;
                        l2_rev_pack.exception_id = rev_pack.exception_id;
                        l2_rev_pack.exception_value = rev_pack.exception_value;
                        l2_rev_pack.branch_predictor_info_pack = rev_pack.branch_predictor_info_pack;
        
                        l2_rev_pack.rs1 = rev_pack.rs1;
                        l2_rev_pack.arg1_src = rev_pack.arg1_src;
                        l2_rev_pack.rs1_need_map = rev_pack.rs1_need_map;
                        l2_rev_pack.rs1_phy = rev_pack.rs1_phy;
                        l2_rev_pack.src1_value = rev_pack.src1_value;
        
                        l2_rev_pack.rs2 = rev_pack.rs2;
                        l2_rev_pack.arg2_src = rev_pack.arg2_src;
                        l2_rev_pack.rs2_need_map = rev_pack.rs2_need_map;
                        l2_rev_pack.rs2_phy = rev_pack.rs2_phy;
                        l2_rev_pack.src2_value = rev_pack.src2_value;
        
                        l2_rev_pack.rd = rev_pack.rd;
                        l2_rev_pack.rd_enable = rev_pack.rd_enable;
                        l2_rev_pack.need_rename = rev_pack.need_rename;
                        l2_rev_pack.rd_phy = rev_pack.rd_phy;
        
                        l2_rev_pack.csr = rev_pack.csr;
                        l2_rev_pack.store_buffer_id = rev_pack.store_buffer_id;
                        l2_rev_pack.load_queue_id = rev_pack.load_queue_id;
                        l2_rev_pack.op = rev_pack.op;
                        l2_rev_pack.op_unit = rev_pack.op_unit;
                        memcpy(&l2_rev_pack.sub_op, &rev_pack.sub_op, sizeof(rev_pack.sub_op));
                        
                        if(rev_pack.enable)
                        {
                            verify_only(rev_pack.valid);
                            verify_only(rev_pack.op_unit == op_unit_t::lu);
                            clint_sync_list[rev_pack.rob_id].mtime = clint->get_mtime();
                            clint_sync_list[rev_pack.rob_id].mtimecmp = clint->get_mtimecmp();
                            clint_sync_list[rev_pack.rob_id].msip = clint->get_msip();
                            
                            if(!rev_pack.has_exception)
                            {
                                l2_addr = rev_pack.src1_value + rev_pack.imm;
                                l2_is_cacheable = pma_unit->is_cacheable(l2_addr);
                                l2_rev_pack.exception_value = l2_addr;
                                auto bus = l2_is_cacheable ? cacheable_bus : noncacheable_bus;
                                uint32_t l2_retired_feedback_value = 0;
                                uint32_t l2_retired_feedback_mask = 0;
                                
                                switch(rev_pack.sub_op.lu_op)
                                {
                                    case lu_op_t::lb:
                                    case lu_op_t::lbu:
                                        l2_rev_pack.has_exception = !component::bus::check_align(l2_addr, 1);
                                        l2_rev_pack.exception_id = !component::bus::check_align(l2_addr, 1) ? riscv_exception_t::load_address_misaligned : riscv_exception_t::load_access_fault;
                                        
                                        if(!l2_rev_pack.has_exception)
                                        {
                                            bus->read8(l2_addr);
                                        }
                                        
                                        l2_length = 1;
                                        
                                        {
                                            std::tie(l2_retired_feedback_value, l2_retired_feedback_mask) = retired_store_buffer->get_feedback_value(l2_addr, 1);
                                            auto feedback_param = store_buffer->get_feedback_value(l2_addr, 1, rev_pack.rob_id, rev_pack.rob_id_stage);
                                            
                                            if(feedback_param == std::nullopt)
                                            {
                                                l2_conflict_found = true;
                                            }
                                            else
                                            {
                                                l2_conflict_found = false;
                                                std::tie(l2_feedback_value, l2_feedback_mask) = feedback_param.value();
                                            }
                                        }
                                        
                                        break;
                                        
                                    case lu_op_t::lh:
                                    case lu_op_t::lhu:
                                        l2_rev_pack.has_exception = !component::bus::check_align(l2_addr, 2);
                                        l2_rev_pack.exception_id = !component::bus::check_align(l2_addr, 2) ? riscv_exception_t::load_address_misaligned : riscv_exception_t::load_access_fault;
        
                                        if(!l2_rev_pack.has_exception)
                                        {
                                            bus->read16(l2_addr);
                                        }
                                        
                                        l2_length = 2;
                                        
                                        {
                                            std::tie(l2_retired_feedback_value, l2_retired_feedback_mask) = retired_store_buffer->get_feedback_value(l2_addr, 2);
                                            auto feedback_param = store_buffer->get_feedback_value(l2_addr, 2, rev_pack.rob_id, rev_pack.rob_id_stage);
        
                                            if(feedback_param == std::nullopt)
                                            {
                                                l2_conflict_found = true;
                                            }
                                            else
                                            {
                                                l2_conflict_found = false;
                                                std::tie(l2_feedback_value, l2_feedback_mask) = feedback_param.value();
                                            }
                                        }
                                        
                                        break;
                                        
                                    case lu_op_t::lw:
                                        l2_rev_pack.has_exception = !component::bus::check_align(l2_addr, 4);
                                        l2_rev_pack.exception_id = !component::bus::check_align(l2_addr, 4) ? riscv_exception_t::load_address_misaligned : riscv_exception_t::load_access_fault;
        
                                        if(!l2_rev_pack.has_exception)
                                        {
                                            bus->read32(l2_addr);
                                        }
                                        
                                        l2_length = 4;
            
                                        {
                                            std::tie(l2_retired_feedback_value, l2_retired_feedback_mask) = retired_store_buffer->get_feedback_value(l2_addr, 4);
                                            auto feedback_param = store_buffer->get_feedback_value(l2_addr, 4, rev_pack.rob_id, rev_pack.rob_id_stage);
        
                                            if(feedback_param == std::nullopt)
                                            {
                                                l2_conflict_found = true;
                                            }
                                            else
                                            {
                                                l2_conflict_found = false;
                                                std::tie(l2_feedback_value, l2_feedback_mask) = feedback_param.value();
                                            }
                                        }
                                        
                                        break;
                                        
                                    default:
                                        verify_only(0);
                                        break;
                                }
                                
                                //combine retired feedback param to feedback param
                                for(size_t i = 0;i < sizeof(uint32_t) * 8;i += 8)
                                {
                                    if((l2_retired_feedback_mask & (0xff << i)) && !(l2_feedback_mask & (0xff << i)))
                                    {
                                        l2_feedback_value |= (l2_retired_feedback_value & (0xff << i));
                                        l2_feedback_mask |= (0xff << i);
                                    }
                                }
                                
                                component::load_queue_item_t load_queue_item;
                                load_queue_item.addr_valid = true;
                                load_queue_item.pc = rev_pack.pc;
                                load_queue_item.rob_id = rev_pack.rob_id;
                                load_queue_item.rob_id_stage = rev_pack.rob_id_stage;
                                load_queue_item.addr = l2_addr;
                                load_queue_item.size = (rev_pack.sub_op.lu_op == lu_op_t::lb || rev_pack.sub_op.lu_op == lu_op_t::lbu) ? 1 :
                                                       (rev_pack.sub_op.lu_op == lu_op_t::lh || rev_pack.sub_op.lu_op == lu_op_t::lhu) ? 2 : 4;
                                load_queue_item.checkpoint_id_valid = rev_pack.branch_predictor_info_pack.checkpoint_id_valid;
                                load_queue_item.checkpoint_id = rev_pack.branch_predictor_info_pack.checkpoint_id;
                                load_queue->set_item(rev_pack.load_queue_id, load_queue_item);
                                load_queue->write_conflict(rev_pack.load_queue_id, false);
                            }
                        }
                    }
                    else
                    {
                        l2_addr = 0;
                        l2_rev_pack = lsu_readreg_execute_pack_t();
                    }
                }
            }
            else
            {
                l2_addr = 0;
                l2_rev_pack = lsu_readreg_execute_pack_t();
            }
        }

        exit:
        execute_feedback_channel_t feedback_pack;
        feedback_pack.enable = send_pack.enable && send_pack.valid && send_pack.need_rename && !send_pack.has_exception;
        feedback_pack.phy_id = send_pack.rd_phy;
        feedback_pack.value = send_pack.rd_value;
        feedback_pack.rob_id = send_pack.rob_id;
        feedback_pack.rob_id_stage = send_pack.rob_id_stage;
        return {feedback_pack, lu_feedback_pack};
    }
    
    json lu::get_json()
    {
        json t;
        
        t["l2_stall"] = this->l2_stall;
        t["l2_addr"] = this->l2_addr;
        t["l2_rev_pack"] = this->l2_rev_pack.get_json();
        return t;
    }
}