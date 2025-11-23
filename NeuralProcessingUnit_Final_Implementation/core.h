#ifndef CORE_H
#define CORE_H

#include "config.h"
#include "memory_modules.h"

SC_MODULE(Core) {
    sc_in_clk clk_i;

    // Управляющие сигналы
    sc_in<bool> start_i;
    sc_in<int>  layer_input_size_i;
    sc_in<addr_t> weights_addr_start_i;
    sc_in<addr_t> inputs_addr_start_i;
    sc_in<addr_t> bias_addr_i;
    sc_in<addr_t> output_addr_i;
    sc_out<bool> done_o;

    // Порт к шине
    sc_port<Bus_if> bus_port;

    // Внутренний LUT
    LUT_Module* internal_lut;

    // Локальная память
    std::vector<data_t> lm_weights;
    std::vector<data_t> lm_inputs;
    data_t lm_bias;

    SC_CTOR(Core) {

        internal_lut = new LUT_Module("Internal_LUT");

        // Инициализация размеров LM
        lm_weights.resize(LOCAL_MEM_SIZE);
        lm_inputs.resize(LOCAL_MEM_SIZE);

        SC_THREAD(compute_thread);
        sensitive << clk_i.pos();
        async_reset_signal_is(start_i, false);
    }

    ~Core() {
        delete internal_lut;
    }

    void compute_thread() {
        while (true) {
            done_o.write(false);
            wait();

            while (!start_i.read()) wait();

            // Чтение конфигурации
            int N = layer_input_size_i.read();
            addr_t w_addr_base = weights_addr_start_i.read();
            addr_t i_addr_base = inputs_addr_start_i.read();
            addr_t b_addr = bias_addr_i.read();
            addr_t o_addr = output_addr_i.read();

            if (N > LOCAL_MEM_SIZE) {
                SC_REPORT_ERROR(name(), "Layer size exceeds Local Memory capacity!");
            }

            // Загрузка весов в LM
            for (int k = 0; k < N; ++k) {
                data_t val;
                bus_port->bus_read(w_addr_base + k, val);
                lm_weights[k] = val;
                wait();
            }

            // Загрузка входов в LM
            for (int k = 0; k < N; ++k) {
                data_t val;
                bus_port->bus_read(i_addr_base + k, val);
                lm_inputs[k] = val;
                wait();
            }

            // Загрузка смещения
            bus_port->bus_read(b_addr, lm_bias);
            wait();

            data_t acc = 0.0f;
            for (int k = 0; k < N; ++k) {
                // Прямой доступ к локальным массивам
                acc += lm_weights[k] * lm_inputs[k];
                if (k % 4 == 0) wait();
            }
            acc += lm_bias;

            // Используем внутренний LUT
            data_t res = internal_lut->get_activation(acc);
            wait();

            // Result -> Bus -> Global Memory
            bus_port->bus_write(o_addr, res);
            wait();

            // Завершение
            done_o.write(true);

            while (start_i.read()) wait();
        }
    }
};

#endif