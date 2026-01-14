#define _CRT_SECURE_NO_WARNINGS

#include <systemc.h>
#include "config.h"
#include "bus_module.h"
#include "memory_modules.h"
#include "ioc_module.h"
#include "nwc_controller.h"
#include "core.h"
#include <vector>

int sc_main(int argc, char* argv[]) {
    // Счётчик
    sc_clock clk("clk", CLK_PERIOD_NS, SC_NS);

    // Выделение памяти
    RAM_Module mem_W("Memory_Weights");
    RAM_Module mem_I("Memory_Inputs");
    RAM_Module mem_B("Memory_Biases");
    RAM_Module mem_O("Memory_Outputs");

    // Шина
    SystemBus bus("SystemBus");
    bus.clk_i(clk);

    bus.port_ram_w(mem_W);
    bus.port_ram_i(mem_I);
    bus.port_ram_b(mem_B);
    bus.port_ram_o(mem_O);

    // Контроллеры
    IO_Controller ioc("IO_Controller");
    NW_Controller nwc("NW_Controller");

    std::vector<int> layer_info;
    ioc.set_dims_ptr(&layer_info);
    nwc.set_dims_ptr(&layer_info);

    // Ядра
    Core* cores[NUM_CORES];
    for (int i = 0; i < NUM_CORES; ++i) {
        char name[20];
        sprintf(name, "Core_%d", i);
        cores[i] = new Core(name);
        cores[i]->clk_i(clk);
        cores[i]->bus_port(bus);
    }

    sc_signal<bool> config_ready_sig;

    sc_signal<bool> core_start_sig[NUM_CORES];
    sc_signal<int>  core_in_size_sig[NUM_CORES];
    sc_signal<addr_t> core_w_addr_sig[NUM_CORES];
    sc_signal<addr_t> core_i_addr_sig[NUM_CORES];
    sc_signal<addr_t> core_b_addr_sig[NUM_CORES];
    sc_signal<addr_t> core_o_addr_sig[NUM_CORES];
    sc_signal<bool> core_done_sig[NUM_CORES];

    ioc.bus_port(bus);
    ioc.config_ready_o(config_ready_sig);

    nwc.clk_i(clk);
    nwc.config_ready_i(config_ready_sig);

    for (int i = 0; i < NUM_CORES; ++i) {
        nwc.core_start[i](core_start_sig[i]);
        nwc.core_input_size[i](core_in_size_sig[i]);
        nwc.core_w_addr[i](core_w_addr_sig[i]);
        nwc.core_i_addr[i](core_i_addr_sig[i]);
        nwc.core_b_addr[i](core_b_addr_sig[i]);
        nwc.core_o_addr[i](core_o_addr_sig[i]);
        nwc.core_done[i](core_done_sig[i]);

        cores[i]->start_i(core_start_sig[i]);
        cores[i]->layer_input_size_i(core_in_size_sig[i]);
        cores[i]->weights_addr_start_i(core_w_addr_sig[i]);
        cores[i]->inputs_addr_start_i(core_i_addr_sig[i]);
        cores[i]->bias_addr_i(core_b_addr_sig[i]);
        cores[i]->output_addr_i(core_o_addr_sig[i]);
        cores[i]->done_o(core_done_sig[i]);
    }

    // Запуск
    std::cout << "Starting NPU Simulation (Load-Store Architecture)..." << std::endl;
    sc_start();

    // Вывод результатов
    if (!layer_info.empty()) {
        size_t num_weight_layers = layer_info.size() - 1;
        int output_size = layer_info.back();
        RAM_Module* result_mem_ptr = (num_weight_layers % 2 != 0) ? &mem_O : &mem_I;

        std::cout << "Final Results: ";
        for (int i = 0; i < output_size; ++i) {
            data_t val;
            result_mem_ptr->mem_read(i, val);
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }

    for (int i = 0; i < NUM_CORES; i++) delete cores[i];
    return 0;
}