#pragma once
#ifndef _CO2_SENSOR_H_
#define _CO2_SENSOR_H_

#include "device.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PPM = 0,    // parts per million
    PPB,        // parts per billion
    PPT,        // parts per trillion
    MGM3,       // milligram per m3
    UGM3,       // microgram per m3
    NGM3,       // nanogram per m3
    PM3,        // particles per m3
    BQM3        // becquerel per m3
} eMeasurementUnit;

class CAirQualitySensor : public CDevice
{
public:
    CAirQualitySensor();

    bool matter_init_endpoint() override;
    bool matter_config_attributes() override;
    void matter_on_change_attribute_value(
        esp_matter::attribute::callback_type_t type,
        uint32_t cluster_id,
        uint32_t attribute_id,
        esp_matter_attr_val_t *value
    ) override;
    void matter_update_all_attribute_values() override;

private:
    bool create_temperature_measurement_cluster();
    bool create_relative_humidity_measurement_cluster();
    bool create_carbon_dioxide_concentration_measurement_cluster();

public:
    bool set_carbon_dioxide_concentration_measurement_min_measured_value(float value);
    bool set_carbon_dioxide_concentration_measurement_max_measured_value(float value);
    bool set_carbon_dioxide_concentration_measurement_measurement_unit(int value);

    void update_measured_value_co2ppm(float value) override;
    void update_measured_value_temperature(float value) override;
    void update_measured_value_humidity(float value) override;

private:
    bool m_matter_update_by_client_clus_co2measure_attr_measureval;
    bool m_matter_update_by_client_clus_tempmeasure_attr_measureval;
    bool m_matter_update_by_client_clus_relhummeasure_attr_measureval;

    void matter_update_clus_co2measure_attr_measureval(bool force_update = false);
    void matter_update_clus_tempmeasure_attr_measureval(bool force_update = false);
    void matter_update_clus_relhummeasure_attr_measureval(bool force_update = false);
};

#ifdef __cplusplus
};
#endif
#endif