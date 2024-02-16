#include "humiditysensor.h"
#include "system.h"
#include "logger.h"

CHumiditySensor::CHumiditySensor()
{
    m_matter_update_by_client_clus_relhummeasure_attr_measureval = false;
    m_measured_value = 0;
    m_measured_value_prev = 0;
}

bool CHumiditySensor::matter_init_endpoint()
{
    esp_matter::node_t *root = GetSystem()->get_root_node();
    esp_matter::endpoint::humidity_sensor::config_t config_endpoint;
    uint8_t flags = esp_matter::ENDPOINT_FLAG_DESTROYABLE;
    m_endpoint = esp_matter::endpoint::humidity_sensor::create(root, &config_endpoint, flags, nullptr);
    if (!m_endpoint) {
        GetLogger(eLogType::Error)->Log("Failed to create endpoint");
        return false;
    }

    return CDevice::matter_init_endpoint();
}

bool CHumiditySensor::matter_config_attributes()
{
    return true;
}

void CHumiditySensor::matter_on_change_attribute_value(esp_matter::attribute::callback_type_t type, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *value)
{
    if (cluster_id == chip::app::Clusters::RelativeHumidityMeasurement::Id) {
        if (attribute_id == chip::app::Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Id) {
            if (m_matter_update_by_client_clus_relhummeasure_attr_measureval) {
                m_matter_update_by_client_clus_relhummeasure_attr_measureval = false;
            }
        }
    }
}

void CHumiditySensor::matter_update_all_attribute_values()
{
    matter_update_clus_relhummeasure_attr_measureval();
}

void CHumiditySensor::update_measured_value(float value)
{
    m_measured_value = (uint16_t)(value * 100.f);
    if (m_measured_value_prev != m_measured_value) {
        GetLogger(eLogType::Info)->Log("Update measured relative humidity value as %g", value);
        matter_update_clus_relhummeasure_attr_measureval();
    }
    m_measured_value_prev = m_measured_value;
}

void CHumiditySensor::matter_update_clus_relhummeasure_attr_measureval(bool force_update/*=false*/)
{
    esp_matter_attr_val_t target_value = esp_matter_nullable_uint16(m_measured_value);
    matter_update_cluster_attribute_common(
        m_endpoint_id,
        chip::app::Clusters::RelativeHumidityMeasurement::Id,
        chip::app::Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Id,
        target_value,
        &m_matter_update_by_client_clus_relhummeasure_attr_measureval,
        force_update
    );
}