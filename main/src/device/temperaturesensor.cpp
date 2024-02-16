#include "temperaturesensor.h"
#include "system.h"
#include "logger.h"

CTemperatureSensor::CTemperatureSensor()
{
    m_matter_update_by_client_clus_tempmeasure_attr_measureval = false;
    m_measured_value = 0;
    m_measured_value_prev = 0;
}

bool CTemperatureSensor::matter_init_endpoint()
{
    esp_matter::node_t *root = GetSystem()->get_root_node();
    esp_matter::endpoint::temperature_sensor::config_t config_endpoint;
    config_endpoint.temperature_measurement.min_measured_value = -1000; // -10 * 100
    config_endpoint.temperature_measurement.max_measured_value = 8500;  // +85 * 100
    uint8_t flags = esp_matter::ENDPOINT_FLAG_DESTROYABLE;
    m_endpoint = esp_matter::endpoint::temperature_sensor::create(root, &config_endpoint, flags, nullptr);
    if (!m_endpoint) {
        GetLogger(eLogType::Error)->Log("Failed to create endpoint");
        return false;
    }

    return CDevice::matter_init_endpoint();
}

bool CTemperatureSensor::matter_config_attributes()
{
    return true;
}

void CTemperatureSensor::matter_on_change_attribute_value(esp_matter::attribute::callback_type_t type, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *value)
{
    if (cluster_id == chip::app::Clusters::TemperatureMeasurement::Id) {
        if (attribute_id == chip::app::Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Id) {
            if (m_matter_update_by_client_clus_tempmeasure_attr_measureval) {
                m_matter_update_by_client_clus_tempmeasure_attr_measureval = false;
            }
        }
    }
}

void CTemperatureSensor::matter_update_all_attribute_values()
{
    matter_update_clus_tempmeasure_attr_measureval();
}

void CTemperatureSensor::update_measured_value(float value)
{
    m_measured_value = (int16_t)(value * 100.f);
    if (m_measured_value_prev != m_measured_value) {
        GetLogger(eLogType::Info)->Log("Update measured temperature value as %g", value);
        matter_update_clus_tempmeasure_attr_measureval();
    }
    m_measured_value_prev = m_measured_value;
}

void CTemperatureSensor::matter_update_clus_tempmeasure_attr_measureval(bool force_update/*=false*/)
{
    esp_matter_attr_val_t target_value = esp_matter_nullable_int16(m_measured_value);
    matter_update_cluster_attribute_common(
        m_endpoint_id,
        chip::app::Clusters::TemperatureMeasurement::Id,
        chip::app::Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Id,
        target_value,
        &m_matter_update_by_client_clus_tempmeasure_attr_measureval,
        force_update
    );
}