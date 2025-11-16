#ifndef CORE_AND_CONTROLLER_H
#define CORE_AND_CONTROLLER_H

#include "config.h"
#include "compute_modules.h"
#include "memory_modules.h"

#include <systemc.h>
#include <vector>
#include <iostream>
#include <string>
#include <algorithm>

// Ядро (Core)
SC_MODULE(Core) {
    sc_in<bool> clk_i;

    sc_in<bool> start_compute_i; // Начало вычислений
    sc_in<int> neuron_idx_i;     // Индекс нейрона
    sc_in<data_t> input_data_i;  // Входы

    sc_out<data_t> output_data_o; // Выходы 
    sc_out<bool> output_valid_o;  // Сигнал готовности данных

    LM_Mem* local_mem;
    MUL* multiplier;
    Activation* activation;

    sc_signal<data_t> weight_s;    // Вес для MUL
    sc_signal<data_t> product_s;   // Выход MUL (W*I)
    sc_signal<data_t> net_input_s; // Вход для Activation (сумма)

    // Параметры слоя (устанавливаются NWc)
    int layer_input_size;

    SC_CTOR(Core) {
        local_mem = new LM_Mem("LM_Mem");
        multiplier = new MUL("MUL");
        activation = new Activation("Activation");

        multiplier->weight_i(weight_s);
        multiplier->input_i(input_data_i);
        multiplier->product_o(product_s);

        activation->net_input_i(net_input_s);
        activation->output_o(output_data_o);

        SC_THREAD(process_mac_unit);
        sensitive << clk_i.pos();
    }

    ~Core() {
        delete local_mem;
        delete multiplier;
        delete activation;
    }

    // Настройка ядра (вызывается NWc)
    void configure(int input_s) {
        layer_input_size = input_s;
    }

    // Загрузка данных в LM_Mem
    void load_weights(const std::vector<data_t>&weights) {
        for (size_t i = 0; i < weights.size(); ++i) {
            local_mem->write_weight(i, weights[i]);
        }
    }
    void load_biases(const std::vector<data_t>&biases) {
        for (size_t i = 0; i < biases.size(); ++i) {
            local_mem->write_bias(i, biases[i]);
        }
    }

    // Основной процесс
    void process_mac_unit() {
        data_t accumulator;
        int current_neuron_idx;

        while (true) {
            output_valid_o.write(false);

            do {
                wait();
                current_neuron_idx = neuron_idx_i.read();
            } while (!start_compute_i.read());

            accumulator = 0.0f;

            for (int i = 0; i < layer_input_size; ++i) {
                data_t w_val = local_mem->read_weight(current_neuron_idx * layer_input_size + i);
                weight_s.write(w_val);

                wait();

                accumulator += product_s.read();
            }

            data_t bias = local_mem->read_bias(current_neuron_idx);
            accumulator += bias;

            net_input_s.write(accumulator);

            wait();

            output_valid_o.write(true);

            while (start_compute_i.read()) {
                wait();
            }
        }
    }
};

SC_MODULE(NWc) {
    sc_in_clk clk_i;
    sc_in<bool> config_ready_i;

    std::vector<int> layer_dims;
    std::vector<std::vector<data_t>> all_weights;
    std::vector<std::vector<data_t>> all_biases;
    std::vector<data_t> current_inputs;

    Core* cores[NUM_CORES];
    sc_signal<data_t> core_input_signals[NUM_CORES];
    sc_signal<bool> core_start_signals[NUM_CORES];
    sc_signal<int> core_neuron_idx_signals[NUM_CORES];
    sc_signal<data_t> core_output_data_signals[NUM_CORES];
    sc_signal<bool> core_output_valid_signals[NUM_CORES];

    SC_CTOR(NWc) {
        for (int i = 0; i < NUM_CORES; ++i) {
            std::stringstream ss;
            ss << "Core_" << i;
            cores[i] = new Core(ss.str().c_str());

            cores[i]->clk_i(clk_i);

            cores[i]->start_compute_i(core_start_signals[i]);
            cores[i]->neuron_idx_i(core_neuron_idx_signals[i]);
            cores[i]->input_data_i(core_input_signals[i]);
            cores[i]->output_data_o(core_output_data_signals[i]);
            cores[i]->output_valid_o(core_output_valid_signals[i]);
        }

        SC_THREAD(controller_main);
    }

    void set_network_config(const std::vector<int>&dims,
        const std::vector<std::vector<data_t>>&weights,
        const std::vector<std::vector<data_t>>&biases) {
        layer_dims = dims;
        all_weights = weights;
        all_biases = biases;
    }

    void set_initial_inputs(const std::vector<data_t>&inputs) {
        current_inputs = inputs;
    }

    // Основной процесс
    void controller_main() {

        while (!config_ready_i.read()) {
            SC_REPORT_INFO(name(), "Waiting for configuration ready signal...");
            wait(1, sc_core::SC_NS);
        }
        SC_REPORT_INFO(name(), "Configuration received. Starting computation.");

        for (int current_layer = 0; current_layer < all_weights.size(); ++current_layer) {

            int input_size = layer_dims[current_layer];
            int output_size = layer_dims[current_layer + 1];

            SC_REPORT_INFO(name(), (std::string("Starting Layer ") + std::to_string(current_layer) +
                ": " + std::to_string(input_size) + " -> " + std::to_string(output_size)).c_str());

            for (int i = 0; i < NUM_CORES; ++i) {
                cores[i]->configure(input_size);
                cores[i]->load_weights(all_weights[current_layer]);
                cores[i]->load_biases(all_biases[current_layer]);
            }
            wait(10, sc_core::SC_NS);

            std::vector<data_t> layer_outputs(output_size);
            int neurons_computed = 0;

            while (neurons_computed < output_size) {
                int num_active_cores = std::min(NUM_CORES, output_size - neurons_computed);

                for (int i = 0; i < num_active_cores; ++i) {
                    core_neuron_idx_signals[i].write(neurons_computed + i);
                    core_start_signals[i].write(true);
                }

                for (int mac_step = 0; mac_step < input_size; ++mac_step) {
                    wait(clk_i.posedge_event());

                    data_t current_input_val = current_inputs[mac_step];

                    for (int i = 0; i < num_active_cores; ++i) {
                        core_input_signals[i].write(current_input_val);
                    }
                }

                wait(30, sc_core::SC_NS);

                for (int i = 0; i < num_active_cores; ++i) {
                    gotem:
                    if (core_output_valid_signals[i].read()) {
                        layer_outputs[neurons_computed + i] = core_output_data_signals[i].read();
                    }
                    else {
                        std::cout << std::endl << "Core output was not valid!" << std::endl;
                        goto gotem;
                    }
                }

                for (int i = 0; i < num_active_cores; ++i) {
                    core_start_signals[i].write(false);
                }
                wait(clk_i.posedge_event());

                neurons_computed += num_active_cores;
            }

            current_inputs = layer_outputs;
        }

        SC_REPORT_INFO(name(), "--- Network computation finished ---");
        std::cout << "Final Output Vector (Size " << current_inputs.size() << "):" << std::endl;
        for (size_t i = 0; i < current_inputs.size(); ++i) {
            std::cout << "Out[" << i << "] = " << current_inputs[i] << std::endl;
        }

        sc_stop();
    }
};

#endif // CORE_AND_CONTROLLER_H