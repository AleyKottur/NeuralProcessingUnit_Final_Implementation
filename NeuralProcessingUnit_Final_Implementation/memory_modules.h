#ifndef MEMORY_MODULES_H
#define MEMORY_MODULES_H
#include "config.h"

class Memory_if : public virtual sc_interface {
public:
    virtual bool mem_write(addr_t addr, data_t data) = 0;
    virtual bool mem_read(addr_t addr, data_t& data) = 0;
};

SC_MODULE(RAM_Module), public Memory_if{
    std::vector<data_t> storage;
    SC_CTOR(RAM_Module) { storage.resize(MEM_SEGMENT_SIZE, 0.0f); }

    bool mem_write(addr_t addr, data_t data) override {
        if (addr < storage.size()) { storage[addr] = data; return true; }
        return false;
    }
    bool mem_read(addr_t addr, data_t& data) override {
        if (addr < storage.size()) { data = storage[addr]; return true; }
        return false;
    }
    void debug_dump(addr_t start_addr, int count, const char* label) {}
};

// LUT Module
SC_MODULE(LUT_Module), public LUT_if{
    data_t lut_table[256];
    const float LUT_MIN = -10.0f;
    const float LUT_MAX = 10.0f;

    SC_CTOR(LUT_Module) {
        float step = (LUT_MAX - LUT_MIN) / 255.0f;
        for (int i = 0; i < 256; ++i) {
            float x = LUT_MIN + i * step;
            lut_table[i] = 1.0f / (1.0f + std::exp(-x));
        }
    }

    data_t get_activation(data_t input_val) override {
        if (input_val < LUT_MIN) return lut_table[0];
        if (input_val > LUT_MAX) return lut_table[255];
        float range = LUT_MAX - LUT_MIN;
        float index_f = (input_val - LUT_MIN) / range * 255.0f;
        return lut_table[(int)std::round(index_f)];
    }
};
#endif