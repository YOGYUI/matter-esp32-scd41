#pragma once
#ifndef _SCD41_H_
#define _SCD41_H_

#include "I2CMaster.h"

#ifdef __cplusplus
extern "C" {
#endif

class CScd41Ctrl
{
public:
    CScd41Ctrl();
    virtual ~CScd41Ctrl();
    static CScd41Ctrl* Instance();

public:
    bool initialize(CI2CMaster *i2c_master, bool self_test = false);
    bool release();

    bool reinit_module();
    bool wakeup_module();
    bool sleep_module();
    bool perform_self_test();
    bool perform_factory_reset();

    bool start_periodic_measure();
    bool stop_periodic_measure();

    bool measure_single_shot();
    bool read_measurement(uint16_t *co2ppm, float *temperature, float *humidity);
    bool is_measurement_data_ready();

private:
    static CScd41Ctrl *_instance;
    CI2CMaster *m_i2c_master;

    bool read_serial_number(uint64_t *serial);
    uint8_t calculate_crc(uint16_t data);
};

inline CScd41Ctrl* GetScd41Ctrl() {
    return CScd41Ctrl::Instance();
}

#ifdef __cplusplus
}
#endif
#endif