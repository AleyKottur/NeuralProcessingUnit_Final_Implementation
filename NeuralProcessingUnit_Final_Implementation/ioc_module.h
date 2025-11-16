#ifndef IOC_MODULE_H
#define IOC_MODULE_H

#include "config.h" 
#include "core_and_controller.h" 
#include <systemc.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

void read_vector_from_stream(std::ifstream& file, std::vector<data_t>& vec, size_t count, const std::string& name) {
    file >> std::skipws;

    vec.resize(count);
    size_t i = 0;
    try {
        for (; i < count; ++i) {
            if (!(file >> vec[i])) {
                throw std::runtime_error("File format error. Expected " + std::to_string(count) +
                    " values for " + name + ", but failed to read value #" + std::to_string(i + 1) + ".");
            }
            if (count > 100 && i % 100 == 99) {
                sc_core::sc_report_handler::report(sc_core::SC_INFO, "IOc",
                    (std::string("Reading progress: ") + name + " read " + std::to_string(i + 1) + " of " + std::to_string(count)).c_str(),
                    __FILE__, __LINE__);
            }
        }
        sc_core::sc_report_handler::report(sc_core::SC_INFO, "IOc",
            (std::string("Successfully read all ") + std::to_string(count) + " values for " + name).c_str(),
            __FILE__, __LINE__);
    }
    catch (const std::runtime_error& e) {
        throw;
    }
}


SC_MODULE(IOc) {
    sc_out<NWc*> controller_port;
    sc_out<bool> config_ready_o;

    SC_CTOR(IOc) {
        SC_THREAD(read_data_file);
    }

    void read_data_file() {
        wait(200, sc_core::SC_NS);

        NWc* controller = controller_port.read();

        if (!controller) {
            SC_REPORT_FATAL(name(), "NWc pointer is NULL after initial wait. Check binding in main.cpp.");
            return;
        }

        SC_REPORT_INFO(name(), "Controller pointer received. Attempting to read file...");

        config_ready_o.write(false);
        const char* filename = "weights.txt";
        std::ifstream weight_file(filename);

        if (!weight_file.is_open()) {
            std::string error_msg = "Cannot open file: ";
            error_msg += filename;
            error_msg += ". Ensure it is in the execution directory (e.g., Debug/Release) and not the source directory!";
            SC_REPORT_FATAL(name(), error_msg.c_str());
            return;
        }

        // --- Чтение данных ---
        int num_layers_with_inputs = 0;
        std::vector<int> layer_dims;
        std::vector<std::vector<data_t>> all_weights;
        std::vector<std::vector<data_t>> all_biases;
        std::vector<data_t> initial_inputs;

        try {
            if (!(weight_file >> num_layers_with_inputs) || num_layers_with_inputs < 2) {
                throw std::runtime_error("Invalid network architecture size format (expected N >= 2).");
            }
            layer_dims.resize(num_layers_with_inputs);

            sc_core::sc_report_handler::report(sc_core::SC_INFO, name(),
                (std::string("Reading ") + std::to_string(num_layers_with_inputs) + " layer dimensions.").c_str(),
                __FILE__, __LINE__);

            for (int i = 0; i < num_layers_with_inputs; ++i) {
                if (!(weight_file >> layer_dims[i]) || layer_dims[i] <= 0) {
                    throw std::runtime_error(std::string("Invalid layer dimension format for dimension #") + std::to_string(i) + ".");
                }
            }

            size_t num_weight_layers = layer_dims.size() - 1;
            all_weights.resize(num_weight_layers);
            all_biases.resize(num_weight_layers);

            for (size_t i = 0; i < num_weight_layers; ++i) {
                int input_size = layer_dims[i];
                int output_size = layer_dims[i + 1];

                size_t weight_count = (size_t)input_size * output_size;
                size_t bias_count = (size_t)output_size;

                read_vector_from_stream(weight_file, all_weights[i], weight_count, std::string("Weights Layer ") + std::to_string(i));
                read_vector_from_stream(weight_file, all_biases[i], bias_count, std::string("Biases Layer ") + std::to_string(i));
            }

            size_t input_count = (size_t)layer_dims[0];
            read_vector_from_stream(weight_file, initial_inputs, input_count, "Initial Inputs");

            weight_file.close();

            controller->set_network_config(layer_dims, all_weights, all_biases);
            controller->set_initial_inputs(initial_inputs);

        }
        catch (const std::runtime_error& e) {
            SC_REPORT_FATAL(name(), (std::string("FILE READ FAILED: ") + e.what()).c_str());
            if (weight_file.is_open()) weight_file.close();
            return;
        }

        SC_REPORT_INFO(name(), "Configuration loaded into NWc. Activating ready signal.");
        config_ready_o.write(true);
    }
};

#endif // IOC_MODULE_H