#ifndef CONFIG_H
#define CONFIG_H

#include <systemc.h>
#include <vector>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>

// --- Аппаратные ограничения нейропроцессора ---
#define CLK_PERIOD_NS 10 // Период тактового сигнала в наносекундах (100 МГц)

// Параметры параллелизма и памяти
const int NUM_CORES = 4; // Количество ядер для параллельных вычислений
const int GLOBAL_MEMORY_SIZE = 1024 * 1024; // Размер общей памяти (в float-ах)
const int LOCAL_MEMORY_SIZE = 16 * 1024; // Размер локальной памяти на ядро (в float-ах)

// Параметры LUT (Look-Up Table) для Сигмоиды
const int LUT_SIZE = 256;
const float LUT_MIN_VAL = -10.0f;
const float LUT_MAX_VAL = 10.0f;

// Общие типы данных и интерфейсы
typedef sc_uint<20> addr_t; // Тип адреса
typedef float data_t;       // Тип данных (float)

// Интерфейс для доступа к общей памяти (Шине)
struct Bus_Master_if : public sc_interface {
    virtual bool write_data(addr_t addr, data_t data) = 0;
    virtual bool read_data(addr_t addr, data_t& data) = 0;
};

#endif // CONFIG_H