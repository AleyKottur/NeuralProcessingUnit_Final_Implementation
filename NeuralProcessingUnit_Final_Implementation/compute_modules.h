#ifndef COMPUTE_MODULES_H
#define COMPUTE_MODULES_H

#include "config.h"
#include "memory_modules.h"

SC_MODULE(MUL) {
    sc_in<data_t> weight_i;
    sc_in<data_t> input_i;
    sc_out<data_t> product_o;

    SC_CTOR(MUL) {
        SC_METHOD(calculate);
        sensitive << weight_i << input_i;
    }

    void calculate() {
        product_o.write(weight_i.read() * input_i.read());
    }
};

SC_MODULE(Activation) {
    sc_in<data_t> net_input_i;
    sc_out<data_t> output_o;

    LM_LUT* lut_mem;

    SC_CTOR(Activation) {
        lut_mem = new LM_LUT("LUT_memory");

        SC_METHOD(activate);
        sensitive << net_input_i;
    }

    ~Activation() {
        delete lut_mem;
    }

    void activate() {
        data_t result = lut_mem->lookup(net_input_i.read());
        output_o.write(result);
    }
};

#endif // COMPUTE_MODULES_H