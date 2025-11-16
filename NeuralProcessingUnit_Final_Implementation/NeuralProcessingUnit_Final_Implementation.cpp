#include "ioc_module.h"
#include "core_and_controller.h" 
#include <systemc.h>
#include <iostream>

int sc_main(int argc, char* argv[]) {

    sc_clock clk_signal("clk", 10, sc_core::SC_NS, 0.5, 0, sc_core::SC_NS, false);
    sc_signal<bool> config_ready_sig;

    sc_signal<NWc*> controller_ptr_sig;

    IOc ioc("ioc");
    NWc controller("controller");

    NWc* nwc_ptr = &controller;

    controller.clk_i(clk_signal);

    controller.config_ready_i(config_ready_sig);
    ioc.config_ready_o(config_ready_sig);
    ioc.controller_port(controller_ptr_sig);

    controller_ptr_sig.write(nwc_ptr);

    sc_core::sc_start();

    std::cout << "\nSimulation finished." << std::endl;

    return 0;
}