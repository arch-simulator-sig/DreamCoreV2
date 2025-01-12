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
#include "../component/branch_predictor_base.h"

namespace cycle_model::pipeline
{
    typedef struct fetch2_decode_pack_t : public if_print_t
    {
        inst_common_info_t inst_common_info;
        bool enable = false;
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
    }fetch2_decode_pack_t;
}