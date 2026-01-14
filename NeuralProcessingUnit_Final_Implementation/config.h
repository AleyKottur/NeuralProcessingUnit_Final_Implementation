#ifndef CONFIG_H
#define CONFIG_H

#include <systemc.h>
#include <vector>
#include <cmath>
#include <iomanip>

#define CLK_PERIOD_NS 10
const int NUM_CORES = 8;
const int MEM_SEGMENT_SIZE = 0x10000;

// Адресная карта
const int ADDR_I_BASE = 0x00000;
const int ADDR_W_BASE = 0x10000;
const int ADDR_B_BASE = 0x20000;
const int ADDR_O_BASE = 0x30000;

// Ограничение размера локальной памяти (LM) в словах
const int LOCAL_MEM_SIZE = 1024;

typedef int addr_t;
typedef float data_t;

class Bus_if : public virtual sc_interface {
public:
    virtual bool bus_write(addr_t addr, data_t data) = 0;
    virtual bool bus_read(addr_t addr, data_t& data) = 0;
};

// Интерфейс для LUT
class LUT_if : public virtual sc_interface {
public:
    virtual data_t get_activation(data_t input) = 0;
};

#endif