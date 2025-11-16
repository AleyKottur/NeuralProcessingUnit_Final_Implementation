#ifndef MEMORY_MODULES_H
#define MEMORY_MODULES_H

#include "config.h"

SC_MODULE(Memory), public Bus_Master_if{

    std::vector<data_t> mem;

    SC_CTOR(Memory) {
        mem.resize(GLOBAL_MEMORY_SIZE);
    }

    bool write_data(addr_t addr, data_t data) override {
        if (addr >= mem.size()) {
            SC_REPORT_ERROR(name(), "Invalid memory write address");
            return false;
        }
        mem[addr] = data;
        return true;
    }

    bool read_data(addr_t addr, data_t& data) override {
        if (addr >= mem.size()) {
            SC_REPORT_ERROR(name(), "Invalid memory read address");
            return false;
        }
        data = mem[addr];
        return true;
    }
};

SC_MODULE(LM_LUT) {
    data_t lut_table[LUT_SIZE];

    SC_CTOR(LM_LUT) {
        // Заполнение LUT через косталь :)
        float step = (LUT_MAX_VAL - LUT_MIN_VAL) / (LUT_SIZE - 1);
        for (int i = 0; i < LUT_SIZE; ++i) {
            float x = LUT_MIN_VAL + i * step;
            lut_table[i] = 1.0f / (1.0f + std::exp(-x));
        }
    }

    data_t lookup(data_t input_val) {
        if (input_val < LUT_MIN_VAL) return lut_table[0];
        if (input_val > LUT_MAX_VAL) return lut_table[LUT_SIZE - 1];

        float range = LUT_MAX_VAL - LUT_MIN_VAL;
        float index_f = (input_val - LUT_MIN_VAL) / range * (LUT_SIZE - 1);

        int index = static_cast<int>(std::round(index_f));
        return lut_table[index];
    }
};

SC_MODULE(LM_Mem) {
    data_t local_weights[LOCAL_MEMORY_SIZE];
    data_t local_biases[LOCAL_MEMORY_SIZE];

    SC_CTOR(LM_Mem) {}

    void write_weight(int index, data_t val) {
        if (index < LOCAL_MEMORY_SIZE) local_weights[index] = val;
    }
    void write_bias(int index, data_t val) {
        if (index < LOCAL_MEMORY_SIZE) local_biases[index] = val;
    }

    data_t read_weight(int index) {
        return (index < LOCAL_MEMORY_SIZE) ? local_weights[index] : 0.0f;
    }
    data_t read_bias(int index) {
        return (index < LOCAL_MEMORY_SIZE) ? local_biases[index] : 0.0f;
    }
};

#endif // MEMORY_MODULES_H