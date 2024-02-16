#include "I2CMaster.h"
#include "driver/i2c.h"
#include "logger.h"

CI2CMaster* CI2CMaster::_instance = nullptr;

CI2CMaster::CI2CMaster()
{
    m_initialized = false;
    m_port = 0;
}

CI2CMaster::~CI2CMaster()
{
}

CI2CMaster* CI2CMaster::Instance()
{
    if (!_instance) {
        _instance = new CI2CMaster();
    }

    return _instance;
}

bool CI2CMaster::initialize(int port, int gpio_scl, int gpio_sda, uint32_t clk_speed)
{
    esp_err_t ret;
    
    m_initialized = false;

    m_port = port;
    i2c_config_t i2c_conf;
    i2c_conf.mode = I2C_MODE_MASTER;
    i2c_conf.sda_io_num = gpio_sda;
    i2c_conf.scl_io_num = gpio_scl;
    i2c_conf.sda_pullup_en = GPIO_PULLUP_ENABLE,
    i2c_conf.scl_pullup_en = GPIO_PULLUP_ENABLE,
    i2c_conf.master.clk_speed = clk_speed,
    i2c_conf.clk_flags = 0;

    ret = i2c_param_config((i2c_port_t)m_port, &i2c_conf);
    if (ret != ESP_OK) {
        GetLogger(eLogType::Error)->Log("Failed to config i2c parameter (ret: %d)", ret);
        return false;
    }
    ret = i2c_driver_install((i2c_port_t)m_port, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) {
        GetLogger(eLogType::Error)->Log("Failed to install i2c driver (ret: %d)", ret);
        return false;
    }
    m_initialized = true;
    GetLogger(eLogType::Info)->Log("Initialized (port num: %d, gpio scl: %d, gpio sda: %d, clock: %u)", m_port, gpio_scl, gpio_sda, clk_speed);
    return true;
}

bool CI2CMaster::release()
{
    esp_err_t ret;

    m_initialized = false;
    i2c_port_t port = (i2c_port_t)m_port;
    ret = i2c_driver_delete(port);
    if (ret != ESP_OK) {
        GetLogger(eLogType::Error)->Log("Failed to delete i2c driver (ret: %d)", ret);
        return false;
    }

    return true;
}

bool CI2CMaster::write_bytes(uint8_t dev_addr, uint8_t *data, size_t data_len, uint32_t timeout_ms/*=1000*/)
{
    if (!m_initialized) {
        GetLogger(eLogType::Error)->Log("Not initialized");
        return false;
    }

    esp_err_t ret;
    ret = i2c_master_write_to_device(
        (i2c_port_t)m_port, 
        dev_addr, 
        data, 
        data_len, 
        timeout_ms / portTICK_PERIOD_MS
    );
    if (ret != ESP_OK) {
        GetLogger(eLogType::Error)->Log("Failed to write (ret: %d)", ret);
        return false;
    }

    return true;
}

bool CI2CMaster::read_bytes(uint8_t dev_addr, uint8_t *data, size_t data_len, uint32_t timeout_ms/*=1000*/)
{
    if (!m_initialized) {
        GetLogger(eLogType::Error)->Log("Not initialized");
        return false;
    }

    esp_err_t ret;
    ret = i2c_master_read_from_device(
        (i2c_port_t)m_port, 
        dev_addr, 
        data, 
        data_len, 
        timeout_ms / portTICK_PERIOD_MS
    );
    if (ret != ESP_OK) {
        GetLogger(eLogType::Error)->Log("Failed to read (ret: %d)", ret);
        return false;
    }

    return true;
}

bool CI2CMaster::write_and_read_bytes(uint8_t dev_addr, uint8_t *data_write, size_t data_write_len, uint8_t *data_read, size_t data_read_len, uint32_t timeout_ms/*=1000*/)
{
    if (!m_initialized) {
        GetLogger(eLogType::Error)->Log("Not initialized");
        return false;
    }

    esp_err_t ret;
    ret = i2c_master_write_read_device(
        (i2c_port_t)m_port, 
        dev_addr, 
        data_write, 
        data_write_len, 
        data_read,
        data_read_len, 
        timeout_ms / portTICK_PERIOD_MS
    );
    if (ret != ESP_OK) {
        GetLogger(eLogType::Error)->Log("Failed to write and read (ret: %d)", ret);
        return false;
    }
    return true;
}