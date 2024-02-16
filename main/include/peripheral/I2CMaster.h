#pragma once
#ifndef _I2C_MASTER_H_
#define _I2C_MASTER_H_

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

class CI2CMaster
{
public:
    CI2CMaster();
    virtual ~CI2CMaster();
    static CI2CMaster* Instance();

public:
    bool initialize(int port, int gpio_scl, int gpio_sda, uint32_t clk_speed);
    bool release();

    bool write_bytes(uint8_t dev_addr, uint8_t *data, size_t data_len, uint32_t timeout_ms = 1000);
    bool read_bytes(uint8_t dev_addr, uint8_t *data, size_t data_len, uint32_t timeout_ms = 1000);
    bool write_and_read_bytes(uint8_t dev_addr, uint8_t *data_write, size_t data_write_len, uint8_t *data_read, size_t data_read_len, uint32_t timeout_ms = 1000);

private:
    static CI2CMaster *_instance;
    int m_port;
    bool m_initialized;
};

inline CI2CMaster* GetI2CMaster() {
    return CI2CMaster::Instance();
}

#ifdef __cplusplus
}
#endif
#endif
