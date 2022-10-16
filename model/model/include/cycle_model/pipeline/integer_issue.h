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
#include "config.h"
#include "../component/port.h"
#include "../component/ooo_issue_queue.h"
#include "../component/regfile.h"
#include "../component/store_buffer.h"
#include "dispatch_issue.h"
#include "integer_issue_readreg.h"
#include "integer_readreg.h"
#include "wb.h"
#include "commit.h"

namespace pipeline
{
    typedef struct integer_issue_feedback_pack_t : if_print_t
    {
        bool stall;
        
        virtual json get_json()
        {
            json t;
            t["stall"] = stall;
            return t;
        }
    }integer_issue_feedback_pack_t;
    
    class integer_issue : public if_print_t, public if_reset_t
    {
        private:
            typedef struct issue_queue_item_t : public if_print_t
            {
                bool enable = false;//this item has op
                uint32_t value = 0;
                bool valid = false;//this item has valid op
                bool last_uop = false;//this is the last uop of an ISA instruction
                bool hide = false;
                
                uint32_t rob_id = 0;
                uint32_t pc = 0;
                uint32_t imm = 0;
                bool has_exception = false;
                riscv_exception_t exception_id = riscv_exception_t::instruction_address_misaligned;
                uint32_t exception_value = 0;
                
                uint32_t rs1 = 0;
                arg_src_t arg1_src = arg_src_t::reg;
                bool rs1_need_map = false;
                uint32_t rs1_phy = 0;
                bool src1_ready = false;
                
                uint32_t rs2 = 0;
                arg_src_t arg2_src = arg_src_t::reg;
                bool rs2_need_map = false;
                uint32_t rs2_phy = 0;
                bool src2_ready = false;
                
                uint32_t rd = 0;
                bool rd_enable = false;
                bool need_rename = false;
                uint32_t rd_phy = 0;
                
                uint32_t csr = 0;
                op_t op = op_t::add;
                op_unit_t op_unit = op_unit_t::alu;
                
                union
                {
                    alu_op_t alu_op;
                    bru_op_t bru_op;
                    div_op_t div_op;
                    lsu_op_t lsu_op;
                    mul_op_t mul_op;
                    csr_op_t csr_op;
                }sub_op;
                
                virtual void print(std::string indent)
                {
                    std::string blank = "  ";
                    
                    std::cout << indent << "\tenable = " << outbool(enable);
                    std::cout << blank << "value = 0x" << fillzero(8) << outhex(value);
                    std::cout << blank << "valid = " << outbool(valid);
                    std::cout << blank << "last_uop = " << outbool(last_uop);
                    std::cout << blank << "rob_id = " << rob_id;
                    std::cout << blank << "pc = 0x" << fillzero(8) << outhex(pc);
                    std::cout << blank << "imm = 0x" << fillzero(8) << outhex(imm);
                    std::cout << blank << "has_exception = " << outbool(has_exception);
                    std::cout << blank << "exception_id = " << outenum(exception_id);
                    std::cout << blank << "exception_value = 0x" << fillzero(8) << outhex(exception_value) << std::endl;
                    
                    std::cout << indent << "\trs1 = " << rs1;
                    std::cout << blank << "arg1_src = " << outenum(arg1_src);
                    std::cout << blank << "rs1_need_map = " << outbool(rs1_need_map);
                    std::cout << blank << "rs1_phy = " << rs1_phy;
                    std::cout << blank << "src1_ready = " << outbool(src1_ready);
                    
                    std::cout << indent << "\trs2 = " << rs2;
                    std::cout << blank << "arg2_src = " << outenum(arg2_src);
                    std::cout << blank << "rs2_need_map = " << outbool(rs2_need_map);
                    std::cout << blank << "rs2_phy = " << rs2_phy;
                    std::cout << blank << "src2_ready = " << outbool(src2_ready);
                    
                    std::cout << blank << "rd = " << rd;
                    std::cout << blank << "rd_enable = " << outbool(rd_enable);
                    std::cout << blank << "need_rename = " << outbool(need_rename);
                    std::cout << blank << "rd_phy = " << rd_phy << std::endl;
                    
                    std::cout << indent << "\tcsr = 0x" << fillzero(8) << outhex(csr);
                    std::cout << blank << "op = " << outenum(op);
                    std::cout << blank << "op_unit = " << outenum(op_unit);
                    std::cout << blank << "sub_op = ";
                    
                    switch(op_unit)
                    {
                        case op_unit_t::alu:
                            std::cout << outenum(sub_op.alu_op);
                            break;
                        
                        case op_unit_t::bru:
                            std::cout << outenum(sub_op.bru_op);
                            break;
                        
                        case op_unit_t::csr:
                            std::cout << outenum(sub_op.csr_op);
                            break;
                        
                        case op_unit_t::div:
                            std::cout << outenum(sub_op.div_op);
                            break;
                        
                        case op_unit_t::lsu:
                            std::cout << outenum(sub_op.lsu_op);
                            break;
                        
                        case op_unit_t::mul:
                            std::cout << outenum(sub_op.mul_op);
                            break;
                        
                        default:
                            std::cout << "<Unsupported>";
                            break;
                    }
                    
                    std::cout << std::endl;
                }
                
                virtual json get_json()
                {
                    json t;
                    t["enable"] = enable;
                    t["value"] = value;
                    t["valid"] = valid;
                    t["last_uop"] = last_uop;
                    t["rob_id"] = rob_id;
                    t["pc"] = pc;
                    t["imm"] = imm;
                    t["has_exception"] = has_exception;
                    t["exception_id"] = outenum(exception_id);
                    t["exception_value"] = exception_value;
                    t["rs1"] = rs1;
                    t["arg1_src"] = arg1_src;
                    t["rs1_need_map"] = rs1_need_map;
                    t["rs1_phy"] = rs1_phy;
                    t["src1_ready"] = src1_ready;
                    t["rs2"] = rs2;
                    t["arg2_src"] = outenum(arg2_src);
                    t["rs2_need_map"] = rs2_need_map;
                    t["rs2_phy"] = rs2_phy;
                    t["src2_ready"] = src2_ready;
                    t["rd"] = rd;
                    t["rd_enable"] = rd_enable;
                    t["need_rename"] = need_rename;
                    t["rd_phy"] = rd_phy;
                    t["csr"] = csr;
                    t["op"] = outenum(op);
                    t["op_unit"] = outenum(op_unit);
                    
                    switch(op_unit)
                    {
                        case op_unit_t::alu:
                            t["sub_op"] = outenum(sub_op.alu_op);
                            break;
                        
                        case op_unit_t::bru:
                            t["sub_op"] = outenum(sub_op.bru_op);
                            break;
                        
                        case op_unit_t::csr:
                            t["sub_op"] = outenum(sub_op.csr_op);
                            break;
                        
                        case op_unit_t::div:
                            t["sub_op"] = outenum(sub_op.div_op);
                            break;
                        
                        case op_unit_t::lsu:
                            t["sub_op"] = outenum(sub_op.lsu_op);
                            break;
                        
                        case op_unit_t::mul:
                            t["sub_op"] = outenum(sub_op.mul_op);
                            break;
                        
                        default:
                            t["sub_op"] = "<Unsupported>";
                            break;
                    }
                    
                    return t;
                }
            }issue_queue_item_t;
        
            component::port<dispatch_issue_pack_t> *dispatch_integer_issue_port;
            component::port<integer_issue_readreg_pack_t> *integer_issue_readreg_port;
            
            component::ooo_issue_queue<issue_queue_item_t> issue_q;
            bool busy = false;
            dispatch_issue_pack_t hold_rev_pack;
            
            uint32_t alu_index = 0;
            uint32_t bru_index = 0;
            uint32_t csr_index = 0;
            uint32_t div_index = 0;
            uint32_t mul_index = 0;
            
            bool alu_busy[ALU_UNIT_NUM];
            bool bru_busy[BRU_UNIT_NUM];
            bool csr_busy[CSR_UNIT_NUM];
            bool div_busy[DIV_UNIT_NUM];
            bool mul_busy[MUL_UNIT_NUM];
        
            uint32_t alu_busy_countdown[ALU_UNIT_NUM];
            uint32_t bru_busy_countdown[BRU_UNIT_NUM];
            uint32_t csr_busy_countdown[CSR_UNIT_NUM];
            uint32_t div_busy_countdown[DIV_UNIT_NUM];
            uint32_t mul_busy_countdown[MUL_UNIT_NUM];
            
            uint32_t wakeup_countdown_src1[INTEGER_ISSUE_QUEUE_SIZE];
            bool wakeup_valid_src1[INTEGER_ISSUE_QUEUE_SIZE];
        
            uint32_t wakeup_countdown_src2[INTEGER_ISSUE_QUEUE_SIZE];
            bool wakeup_valid_src2[INTEGER_ISSUE_QUEUE_SIZE];
            
            trace::trace_database tdb;
        
        public:
            integer_issue(component::port<dispatch_issue_pack_t> *dispatch_integer_issue_port, component::port<integer_issue_readreg_pack_t> *integer_issue_readreg_port);
            virtual void reset();
            integer_issue_feedback_pack_t run(integer_readreg_feedback_pack_t integer_readreg_feedback_pack, commit_feedback_pack_t commit_feedback_pack);
            virtual void print(std::string indent);
            virtual json get_json();
    };
}