#ifndef BUS_MODULE_H
#define BUS_MODULE_H
#include "config.h"
#include "memory_modules.h"

SC_MODULE(SystemBus), public Bus_if{
    sc_port<Memory_if> port_ram_i;
    sc_port<Memory_if> port_ram_w;
    sc_port<Memory_if> port_ram_b;
    sc_port<Memory_if> port_ram_o;
    sc_in_clk clk_i;
    sc_mutex bus_mutex;

    SC_CTOR(SystemBus) {}

    bool bus_write(addr_t addr, data_t data) override {
        bus_mutex.lock();
        bool success = false;
        // Маршрутизация
        if (addr >= ADDR_I_BASE && addr < ADDR_I_BASE + MEM_SEGMENT_SIZE) success = port_ram_i->mem_write(addr - ADDR_I_BASE, data);
        else if (addr >= ADDR_W_BASE && addr < ADDR_W_BASE + MEM_SEGMENT_SIZE) success = port_ram_w->mem_write(addr - ADDR_W_BASE, data);
        else if (addr >= ADDR_B_BASE && addr < ADDR_B_BASE + MEM_SEGMENT_SIZE) success = port_ram_b->mem_write(addr - ADDR_B_BASE, data);
        else if (addr >= ADDR_O_BASE && addr < ADDR_O_BASE + MEM_SEGMENT_SIZE) success = port_ram_o->mem_write(addr - ADDR_O_BASE, data);
        wait(1, SC_NS);
        bus_mutex.unlock();
        return success;
    }

    bool bus_read(addr_t addr, data_t& data) override {
        bus_mutex.lock();
        bool success = false;
        if (addr >= ADDR_I_BASE && addr < ADDR_I_BASE + MEM_SEGMENT_SIZE) success = port_ram_i->mem_read(addr - ADDR_I_BASE, data);
        else if (addr >= ADDR_W_BASE && addr < ADDR_W_BASE + MEM_SEGMENT_SIZE) success = port_ram_w->mem_read(addr - ADDR_W_BASE, data);
        else if (addr >= ADDR_B_BASE && addr < ADDR_B_BASE + MEM_SEGMENT_SIZE) success = port_ram_b->mem_read(addr - ADDR_B_BASE, data);
        else if (addr >= ADDR_O_BASE && addr < ADDR_O_BASE + MEM_SEGMENT_SIZE) success = port_ram_o->mem_read(addr - ADDR_O_BASE, data);
        wait(1, SC_NS);
        bus_mutex.unlock();
        return success;
    }
};
#endif