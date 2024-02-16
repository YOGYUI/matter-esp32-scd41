#pragma once
#ifndef _HUMIDITY_SENSOR_H_
#define _HUMIDITY_SENSOR_H_

#include "device.h"

#ifdef __cplusplus
extern "C" {
#endif

class CHumiditySensor : public CDevice
{
public:
    CHumiditySensor();

    bool matter_init_endpoint() override;
    bool matter_config_attributes() override;
    void matter_on_change_attribute_value(
        esp_matter::attribute::callback_type_t type,
        uint32_t cluster_id,
        uint32_t attribute_id,
        esp_matter_attr_val_t *value
    ) override;
    void matter_update_all_attribute_values() override;

public:
    void update_measured_value(float value) override;

private:
    uint16_t m_measured_value;
    uint16_t m_measured_value_prev;
    bool m_matter_update_by_client_clus_relhummeasure_attr_measureval;

    void matter_update_clus_relhummeasure_attr_measureval(bool force_update = false);
};

#ifdef __cplusplus
};
#endif
#endif