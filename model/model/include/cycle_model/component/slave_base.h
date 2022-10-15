/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-14     lizhirui     the first version
 */

#pragma once
#include "common.h"
#include "bus.h"

namespace component
{
    class bus;
    
    class slave_base : public if_reset_t
    {
        private:
            bool check(uint32_t addr, uint32_t access_size)
            {
                if(!test_mode)
                {
                    assert(!(addr & (access_size - 1)));//align check
                    assert(addr < size);//boundary check
                    assert((size - addr) >= access_size);//boundary check
                }
                else
                {
                    if(addr & (access_size - 1))
                    {
                        has_error = true;
                    }
                    else if(!(addr < size))
                    {
                        has_error = true;
                    }
                    else if(!((size - addr) >= access_size))
                    {
                        has_error = true;
                    }
                }
                
                return !has_error;
            }
        
        protected:
            friend class bus;
            uint32_t size;
            bool test_mode;
            bool has_error;
            component::bus *bus_if;
            
            virtual void _write8(uint32_t addr, uint8_t value)
            {
            
            }
            
            virtual void _write16(uint32_t addr, uint16_t value)
            {
            
            }
            
            virtual void _write32(uint32_t addr, uint32_t value)
            {
            
            }
            
            virtual void _read8(uint32_t addr)
            {
            
            }
            
            virtual void _read16(uint32_t addr)
            {
            
            }
            
            virtual void _read32(uint32_t addr)
            {
            
            }
            
            virtual void _read_instruction(uint32_t addr)
            {
            
            }
            
            virtual void _init()
            {
            
            }
            
            void init(uint32_t size)
            {
                this -> size = size;
                _init();
            }
            
            virtual void _reset()
            {
            
            }
        
        public:
            slave_base(component::bus *bus_if)
            {
                this->bus_if = bus_if;
                test_mode = false;
                has_error = false;
                size = 0;
            }
            
            virtual void reset()
            {
                _reset();
            }
            
            bool check_align(uint32_t addr, uint32_t access_size)
            {
                return !(addr & (access_size - 1));
            }
            
            bool check_boundary(uint32_t addr, uint32_t access_size)
            {
                return (addr < size) && ((size - addr) >= access_size);
            }
            
            void entry_test_mode()
            {
                test_mode = true;
                has_error = false;
            }
            
            bool get_error()
            {
                return has_error;
            }
            
            void clear_error()
            {
                has_error = false;
            }
            
            void write8(uint32_t addr, uint8_t value)
            {
                if(check(addr, 1))
                {
                    _write8(addr, value);
                }
            }
            
            void write16(uint32_t addr, uint16_t value)
            {
                if(check(addr, 2))
                {
                    _write16(addr, value);
                }
            }
            
            void write32(uint32_t addr, uint32_t value)
            {
                if(check(addr, 4))
                {
                    _write32(addr, value);
                }
            }
            
            void read8(uint32_t addr)
            {
                if(check(addr, 1))
                {
                    _read8(addr);
                }
            }
            
            void read16(uint32_t addr)
            {
                if(check(addr, 2))
                {
                    _read16(addr);
                }
            }
            
            void read32(uint32_t addr)
            {
                if(check(addr, 4))
                {
                    _read32(addr);
                }
            }
            
            void read_instruction(uint32_t addr)
            {
                if(check(addr, 4))
                {
                    _read_instruction(addr);
                }
            }
    };
}