#include "scd41.h"
#include "logger.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>

#define SCD4X_I2C_ADDR                      0x62    /**< SCD4X I2C address */
#define SCD4X_SERIAL_NUMBER_WORD0           0xBE02  /**< SCD4X serial number */
#define SCD4X_SERIAL_NUMBER_WORD1           0x7F07  /**< SCD4X serial number 1 */
#define SCD4X_SERIAL_NUMBER_WORD2           0x3BFB  /**< SCD4X serial number 2 */
#define SCD4X_CRC8_INIT                     0xFF
#define SCD4X_CRC8_POLYNOMIAL               0x31
/* SCD4X Basic Commands */
#define SCD4X_START_PERIODIC_MEASURE        0x21B1  /**< start periodic measurement, signal update interval is 5 seconds. */
#define SCD4X_READ_MEASUREMENT              0xEC05  /**< read measurement */
#define SCD4X_STOP_PERIODIC_MEASURE         0x3F86  /**< stop periodic measurement command */
/* SCD4X On-chip output signal compensation */
#define SCD4X_SET_TEMPERATURE_OFFSET        0x241D  /**< set temperature offset */
#define SCD4X_GET_TEMPERATURE_OFFSET        0x2318  /**< get temperature offset */
#define SCD4X_SET_SENSOR_ALTITUDE           0x2427  /**< set sensor altitude */
#define SCD4X_GET_SENSOR_ALTITUDE           0x2322  /**< get sensor altitude */
#define SCD4X_SET_AMBIENT_PRESSURE          0xE000  /**< set ambient pressure */
/* SCD4X Field calibration */
#define SCD4X_PERFORM_FORCED_RECALIB        0x362F  /**< perform forced recalibration */
#define SCD4X_SET_AUTOMATIC_CALIB           0x2416  /**< set automatic self calibration enabled */
#define SCD4X_GET_AUTOMATIC_CALIB           0x2313  /**< get automatic self calibration enabled */
/* SCD4X Low power */
#define SCD4X_START_LOW_POWER_MEASURE       0x21AC  /**< start low power periodic measurement, signal update interval is approximately 30 seconds. */
#define SCD4X_GET_DATA_READY_STATUS         0xE4B8  /**< get data ready status */
/* SCD4X Advanced features */
#define SCD4X_PERSIST_SETTINGS              0x3615  /**< persist settings */
#define SCD4X_GET_SERIAL_NUMBER             0x3682  /**< get serial number */
#define SCD4X_PERFORM_SELF_TEST             0x3639  /**< perform self test */
#define SCD4X_PERFORM_FACTORY_RESET         0x3632  /**< perform factory reset */
#define SCD4X_REINIT                        0x3646  /**< reinit */
/* SCD4X Low power single shot */
#define SCD4X_MEASURE_SINGLE_SHOT           0x219D   ///< measure single shot */
#define SCD4X_MEASURE_SINGLE_SHOT_RHT_ONLY  0x2196   ///< measure single shot rht only */
#define SCD4X_POWER_DOWN                    0x36E0   ///< Put the sensor from idle to sleep to reduce current consumption. */
#define SCD4X_WAKE_UP                       0x36F6   ///< Wake up the sensor from sleep mode into idle mode. */

CScd41Ctrl* CScd41Ctrl::_instance = nullptr;

CScd41Ctrl::CScd41Ctrl()
{
    m_i2c_master = nullptr;
}

CScd41Ctrl::~CScd41Ctrl()
{

}

CScd41Ctrl* CScd41Ctrl::Instance()
{
    if (!_instance) {
        _instance = new CScd41Ctrl();
    }

    return _instance;
}

bool CScd41Ctrl::initialize(CI2CMaster *i2c_master, bool self_test/*=false*/)
{
    m_i2c_master = i2c_master;

    wakeup_module();
    stop_periodic_measure();

    uint64_t serial = 0;
    read_serial_number(&serial);
    GetLogger(eLogType::Info)->Log("Serial Number: 0x%" PRIX64 "", serial);

    if (self_test) {
        if (!perform_self_test()) {
            GetLogger(eLogType::Error)->Log("Failed self test");
        }
    }

    GetLogger(eLogType::Info)->Log("Initialized");
    return true;
}

bool CScd41Ctrl::release()
{
    return true;
}

bool CScd41Ctrl::read_serial_number(uint64_t *serial)
{
    if (!m_i2c_master) {
        GetLogger(eLogType::Error)->Log("I2C Controller is null");
        return false;
    }

    uint8_t data_write[2] = {
        (uint8_t)(SCD4X_GET_SERIAL_NUMBER >> 8),
        (uint8_t)(SCD4X_GET_SERIAL_NUMBER & 0xFF)
    };
    uint8_t data_read[9] = {0, };
    uint8_t crc_expected;

    if (!m_i2c_master->write_and_read_bytes(SCD4X_I2C_ADDR, data_write, sizeof(data_write), data_read, sizeof(data_read), 5000))
        return false;
    uint16_t word1 = ((uint16_t)data_read[0] << 8) | (uint16_t)data_read[1];
    crc_expected = calculate_crc(word1);
    if (crc_expected != data_read[2]) {
        GetLogger(eLogType::Error)->Log("CRC mismatch (%02X - %02X)", crc_expected, data_read[2]);
        return false;
    }
    uint16_t word2 = ((uint16_t)data_read[3] << 8) | (uint16_t)data_read[4];
    crc_expected = calculate_crc(word2);
    if (crc_expected != data_read[5]) {
        GetLogger(eLogType::Error)->Log("CRC mismatch (%02X - %02X)", crc_expected, data_read[5]);
        return false;
    }
    uint16_t word3 = ((uint16_t)data_read[6] << 8) | (uint16_t)data_read[7];
    crc_expected = calculate_crc(word3);
    if (crc_expected != data_read[8]) {
        GetLogger(eLogType::Error)->Log("CRC mismatch (%02X - %02X)", crc_expected, data_read[8]);
        return false;
    }
    *serial = ((uint64_t)word1 << 32) | ((uint64_t)word2 << 16) | (uint64_t)word3;

    return true;
}

bool CScd41Ctrl::perform_self_test()
{
    if (!m_i2c_master) {
        GetLogger(eLogType::Error)->Log("I2C Controller is null");
        return false;
    }

    uint8_t data_write[2] = {
        (uint8_t)(SCD4X_PERFORM_SELF_TEST >> 8),
        (uint8_t)(SCD4X_PERFORM_SELF_TEST & 0xFF)
    };
    if (!m_i2c_master->write_bytes(SCD4X_I2C_ADDR, data_write, sizeof(data_write))) {
        return false;
    }
    vTaskDelay(10000 / portTICK_PERIOD_MS);

    uint8_t data_read[3] = {0,};
    if (!m_i2c_master->read_bytes(SCD4X_I2C_ADDR, data_read, sizeof(data_read))) {
        return false;
    }

    if (data_read[0] != 0) {
        GetLogger(eLogType::Error)->Log("Malfunction detected (%02X)", data_read[0]);
        return false;
    }

    GetLogger(eLogType::Info)->Log("Passed self test (no malfunction detected)");
    return true;
}

bool CScd41Ctrl::reinit_module()
{
    if (!m_i2c_master) {
        GetLogger(eLogType::Error)->Log("I2C Controller is null");
        return false;
    }

    if (!stop_periodic_measure()) {
        return false;
    }

    uint8_t data_write[2] = {
        (uint8_t)(SCD4X_REINIT >> 8),
        (uint8_t)(SCD4X_REINIT & 0xFF)
    };
    if (!m_i2c_master->write_bytes(SCD4X_I2C_ADDR, data_write, sizeof(data_write))) {
        return false;
    }
    vTaskDelay(20 / portTICK_PERIOD_MS);

    return true;
}

bool CScd41Ctrl::wakeup_module()
{
    if (!m_i2c_master) {
        GetLogger(eLogType::Error)->Log("I2C Controller is null");
        return false;
    }

    uint8_t data_write[2] = {
        (uint8_t)(SCD4X_WAKE_UP >> 8),
        (uint8_t)(SCD4X_WAKE_UP & 0xFF)
    };
    if (!m_i2c_master->write_bytes(SCD4X_I2C_ADDR, data_write, sizeof(data_write))) {
        return false;
    }
    vTaskDelay(20 / portTICK_PERIOD_MS);

    return true;
}

bool CScd41Ctrl::sleep_module()
{
    if (!m_i2c_master) {
        GetLogger(eLogType::Error)->Log("I2C Controller is null");
        return false;
    }

    uint8_t data_write[2] = {
        (uint8_t)(SCD4X_POWER_DOWN >> 8),
        (uint8_t)(SCD4X_POWER_DOWN & 0xFF)
    };
    if (!m_i2c_master->write_bytes(SCD4X_I2C_ADDR, data_write, sizeof(data_write))) {
        return false;
    }

    return true;
}

bool CScd41Ctrl::perform_factory_reset()
{
    if (!m_i2c_master) {
        GetLogger(eLogType::Error)->Log("I2C Controller is null");
        return false;
    }

    uint8_t data_write[2] = {
        (uint8_t)(SCD4X_PERFORM_FACTORY_RESET >> 8),
        (uint8_t)(SCD4X_PERFORM_FACTORY_RESET & 0xFF)
    };
    if (!m_i2c_master->write_bytes(SCD4X_I2C_ADDR, data_write, sizeof(data_write))) {
        return false;
    }
    vTaskDelay(1200 / portTICK_PERIOD_MS);

    return true;
}

bool CScd41Ctrl::start_periodic_measure()
{
    if (!m_i2c_master) {
        GetLogger(eLogType::Error)->Log("I2C Controller is null");
        return false;
    }

    uint8_t data_write[2] = {
        (uint8_t)(SCD4X_START_PERIODIC_MEASURE >> 8),
        (uint8_t)(SCD4X_START_PERIODIC_MEASURE & 0xFF)
    };
    if (!m_i2c_master->write_bytes(SCD4X_I2C_ADDR, data_write, sizeof(data_write))) {
        return false;
    }

    return true;
}

bool CScd41Ctrl::stop_periodic_measure()
{
    if (!m_i2c_master) {
        GetLogger(eLogType::Error)->Log("I2C Controller is null");
        return false;
    }

    uint8_t data_write[2] = {
        (uint8_t)(SCD4X_STOP_PERIODIC_MEASURE >> 8),
        (uint8_t)(SCD4X_STOP_PERIODIC_MEASURE & 0xFF)
    };
    if (!m_i2c_master->write_bytes(SCD4X_I2C_ADDR, data_write, sizeof(data_write))) {
        return false;
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);

    return true;
}

bool CScd41Ctrl::measure_single_shot()
{
    if (!m_i2c_master) {
        GetLogger(eLogType::Error)->Log("I2C Controller is null");
        return false;
    }

    uint8_t data_write[2] = {
        (uint8_t)(SCD4X_MEASURE_SINGLE_SHOT >> 8),
        (uint8_t)(SCD4X_MEASURE_SINGLE_SHOT & 0xFF)
    };
    if (!m_i2c_master->write_bytes(SCD4X_I2C_ADDR, data_write, sizeof(data_write))) {
        return false;
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    return true;
}

bool CScd41Ctrl::read_measurement(uint16_t *co2ppm, float *temperature, float *humidity)
{
    if (!m_i2c_master) {
        GetLogger(eLogType::Error)->Log("I2C Controller is null");
        return false;
    }
    uint8_t data_write[2] = {
        (uint8_t)(SCD4X_READ_MEASUREMENT >> 8),
        (uint8_t)(SCD4X_READ_MEASUREMENT & 0xFF)
    };
    uint8_t data_read[9] = {0, };
    if (!m_i2c_master->write_and_read_bytes(SCD4X_I2C_ADDR, data_write, sizeof(data_write), data_read, sizeof(data_read)))
        return false;
    
    if (co2ppm) {
        *co2ppm = ((uint16_t)data_read[0] << 8) | (uint16_t)data_read[1];
    }

    if (temperature) {
        uint16_t temp1 = ((uint16_t)data_read[3] << 8) | (uint16_t)data_read[4];
        *temperature = -45.f + 175.f * (float)temp1 / (float)((uint32_t)1 << 16);
    }

    if (humidity) {
        uint16_t temp2 = ((uint16_t)data_read[6] << 8) | (uint16_t)data_read[7];
        *humidity = 100.f * (float)temp2 / (float)((uint32_t)1 << 16);
    }

    return true;
}

bool CScd41Ctrl::is_measurement_data_ready()
{
    if (!m_i2c_master) {
        GetLogger(eLogType::Error)->Log("I2C Controller is null");
        return false;
    }

    uint8_t data_write[2] = {
        (uint8_t)(SCD4X_GET_DATA_READY_STATUS >> 8),
        (uint8_t)(SCD4X_GET_DATA_READY_STATUS & 0xFF)
    };
    uint8_t data_read[3] = {0, };
    if (!m_i2c_master->write_and_read_bytes(SCD4X_I2C_ADDR, data_write, sizeof(data_write), data_read, sizeof(data_read)))
        return false;

    uint16_t result = ((uint16_t)data_read[0] << 8) | (uint16_t)data_read[1];
    if ((result & 0x07FF) == 0x0000)
        return false;

    return true;
}

uint8_t CScd41Ctrl::calculate_crc(uint16_t data)
{
    uint8_t result = SCD4X_CRC8_INIT;
    uint8_t crc_bit;
    uint8_t buf[2] = {
        (uint8_t)((data >> 8) & 0xFF), 
        (uint8_t)(data & 0xFF)
    };

    for (uint16_t i = 0; i < 2; ++i) {
        result ^= (buf[i]);
        for (crc_bit = 8; crc_bit > 0; --crc_bit) {
            if (result & 0x80)
                result = (result << 1) ^ SCD4X_CRC8_POLYNOMIAL;
            else
                result = (result << 1);
        }
    }
    
    return result;
}