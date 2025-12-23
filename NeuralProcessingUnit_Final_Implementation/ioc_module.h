#ifndef IOC_MODULE_H
#define IOC_MODULE_H

#include "config.h"
#include <fstream>
#include <string>
#include <amsi.h>

// TEST

SC_MODULE(IO_Controller) {
    sc_out<bool> config_ready_o;
    sc_port<Bus_if> bus_port;

    std::vector<int>* layer_dims_ref;

    SC_CTOR(IO_Controller) {
        SC_THREAD(load_data);
    }

    void set_dims_ptr(std::vector<int>*ptr) { layer_dims_ref = ptr; }

    void load_data() {
        config_ready_o.write(false);
        wait(10, SC_NS);

        std::ifstream file("input.txt"); // Используем input.txt
        if (!file.is_open()) {
            SC_REPORT_ERROR("IOc", "Cannot open input.txt");
            return;
        }

        std::string junk;
        int num_layers;

        while (file.peek() == '[' || file.peek() == 's') file >> junk;

        if (!(file >> num_layers)) {
            SC_REPORT_ERROR("IOc", "Failed to read num layers");
            return;
        }

        layer_dims_ref->resize(num_layers);
        for (int i = 0; i < num_layers; i++) file >> (*layer_dims_ref)[i];

        // Глобальные счетчики адресов
        addr_t curr_w_addr = ADDR_W_BASE;
        addr_t curr_b_addr = ADDR_B_BASE;
        addr_t curr_i_addr = ADDR_I_BASE;

        for (int i = 0; i < num_layers - 1; ++i) {
            int input_size = (*layer_dims_ref)[i];
            int output_size = (*layer_dims_ref)[i + 1];

            // Чтение весов (Size: input * output)
            for (int w = 0; w < input_size * output_size; ++w) {
                data_t val;
                while (file.peek() == '[' || isalpha(file.peek())) file >> junk;
                file >> val;
                bus_port->bus_write(curr_w_addr++, val);
            }

            // Чтение биасов
            for (int b = 0; b < output_size; ++b) {
                data_t val;
                while (file.peek() == '[' || isalpha(file.peek())) file >> junk;
                file >> val;
                bus_port->bus_write(curr_b_addr++, val);
            }
        }

        int first_layer_size = (*layer_dims_ref)[0];
        for (int k = 0; k < first_layer_size; ++k) {
            data_t val;
            while (file.peek() == '[' || isalpha(file.peek())) file >> junk;
            file >> val;
            bus_port->bus_write(curr_i_addr++, val);
        }

        SC_REPORT_INFO("IOc", "Data loaded into W, B, I Memories via Bus.");
        config_ready_o.write(true);
    }
};

#endif