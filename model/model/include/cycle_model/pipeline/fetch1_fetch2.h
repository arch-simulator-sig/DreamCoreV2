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
#include "pipeline_common.h"
#include "../component/branch_predictor_base.h"

namespace cycle_model::pipeline
{
    typedef struct fetch1_fetch2_op_info_t
    {
        inst_common_info_t inst_common_info;
        bool enable = false;
        uint64_t id = 0;
        uint32_t pc = 0;
        uint32_t value = 0;
        bool has_exception = false;
        riscv_exception_t exception_id = riscv_exception_t::instruction_address_misaligned;
        uint32_t exception_value = 0;
        component::branch_predictor_info_pack_t branch_predictor_info_pack;
    
        virtual void print(std::string indent)
        {
            std::string blank = "    ";
        
            std::cout << blank << "enable = " << outbool(enable);
            std::cout << indent << "pc = 0x" << fillzero(8) << outhex(pc);
            std::cout << blank << "value = 0x" << fillzero(8) << outhex(value);
            std::cout << blank << "has_exception = " << outbool(has_exception);
            std::cout << blank << "exception_id = " << outenum(exception_id);
            std::cout << blank << "exception_value = 0x" << fillzero(8) << outhex(exception_value) << std::endl;
        }
    
        virtual json get_json()
        {
            json t;
            t["inst_common_info"] = inst_common_info.get_json();
            t["enable"] = enable;
            t["pc"] = pc;
            t["value"] = value;
            t["has_exception"] = has_exception;
            t["exception_id"] = outenum(exception_id);
            t["exception_value"] = exception_value;
            t["branch_predictor_info_pack"] = branch_predictor_info_pack.get_json();
            return t;
        }
    }fetch1_fetch2_op_info_t;
    
    typedef struct fetch1_fetch2_pack_t : public if_print_t
    {
        fetch1_fetch2_op_info_t op_info[FETCH_WIDTH];
    
        virtual void print(std::string indent)
        {
            std::string blank = "    ";
        
            for(uint32_t i = 0;i < FETCH_WIDTH;i++)
            {
                std::cout << indent << "Item " << i << ":" << std::endl;
                op_info[i].print(indent + "\t");
                std::cout << std::endl;
            }
        }
    
        virtual json get_json()
        {
            json ret = json::array();
        
            for(uint32_t i = 0;i < FETCH_WIDTH;i++)
            {
                ret.push_back(op_info[i].get_json());
            }
        
            return ret;
        }
    }fetch1_fetch2_pack_t;
}