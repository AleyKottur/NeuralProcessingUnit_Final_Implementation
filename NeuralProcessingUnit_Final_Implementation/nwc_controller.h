#ifndef NWC_MODULE_H
#define NWC_MODULE_H

#include "config.h"

SC_MODULE(NW_Controller) {
    sc_in_clk clk_i;
    sc_in<bool> config_ready_i;

    // Управление ядрами
    sc_out<bool> core_start[NUM_CORES];
    sc_out<int>  core_input_size[NUM_CORES];
    sc_out<addr_t> core_w_addr[NUM_CORES];
    sc_out<addr_t> core_i_addr[NUM_CORES];
    sc_out<addr_t> core_b_addr[NUM_CORES];
    sc_out<addr_t> core_o_addr[NUM_CORES];

    sc_in<bool> core_done[NUM_CORES];

    // Конфигурация
    std::vector<int>* layer_dims_ref;

    SC_CTOR(NW_Controller) {
        SC_THREAD(control_logic);
        sensitive << clk_i.pos();
    }

    void set_dims_ptr(std::vector<int>*ptr) { layer_dims_ref = ptr; }

    void control_logic() {
        for (int i = 0; i < NUM_CORES; i++) core_start[i].write(false);

        while (!config_ready_i.read()) wait();

        SC_REPORT_INFO("NWc", "Start Computing Layers...");

        addr_t base_w = ADDR_W_BASE;
        addr_t base_b = ADDR_B_BASE;

        addr_t current_input_base = ADDR_I_BASE;
        addr_t current_output_base = ADDR_O_BASE;

        for (size_t l = 0; l < layer_dims_ref->size() - 1; ++l) {
            int n_in = (*layer_dims_ref)[l];
            int n_out = (*layer_dims_ref)[l + 1];

            std::cout << "Computing Layer " << l << " (" << n_in << "->" << n_out << ")" << std::endl;

            int computed_neurons = 0;

            while (computed_neurons < n_out) {
                int active_cores = 0;

                // Распределяем задачи по ядрам
                for (int c = 0; c < NUM_CORES && (computed_neurons + c) < n_out; ++c) {

                    // Расчет адресов для конкретного нейрона
                    // Веса лежат линейно: [Neuron0_Weights][Neuron1_Weights]...
                    addr_t w_addr = base_w + (computed_neurons + c) * n_in;
                    addr_t b_addr = base_b + (computed_neurons + c);
                    addr_t o_addr = current_output_base + (computed_neurons + c);

                    core_input_size[c].write(n_in);
                    core_w_addr[c].write(w_addr);
                    core_i_addr[c].write(current_input_base); // Все читают один вектор входа
                    core_b_addr[c].write(b_addr);
                    core_o_addr[c].write(o_addr);

                    core_start[c].write(true);
                    active_cores++;
                }

                bool all_done = false;
                while (!all_done) {
                    all_done = true;
                    for (int c = 0; c < active_cores; c++) {
                        if (!core_done[c].read()) all_done = false;
                    }
                    wait();
                }

                // Сброс сигналов старта
                for (int c = 0; c < active_cores; c++) core_start[c].write(false);
                wait();

                computed_neurons += active_cores;
            }

            // Сдвигаем базовые адреса для следующего слоя
            base_w += n_in * n_out;
            base_b += n_out;

            std::swap(current_input_base, current_output_base);
        }

        SC_REPORT_INFO("NWc", "Inference Finished.");

        std::cout << "Results are stored in Memory at address: " << std::hex << current_input_base << std::endl;

        sc_stop();
    }
};

#endif