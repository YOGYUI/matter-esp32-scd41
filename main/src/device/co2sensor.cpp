#include "co2sensor.h"
#include "system.h"
#include "logger.h"

CCO2Sensor::CCO2Sensor()
{
    m_matter_update_by_client_clus_co2measure_attr_measureval = false;
    m_measured_value = 0.f;
    m_measured_value_prev = 0.f;
}

bool CCO2Sensor::matter_init_endpoint()
{
    esp_matter::node_t *root = GetSystem()->get_root_node();
    esp_matter::endpoint::air_quality_sensor::config_t config_endpoint;
    uint8_t flags = esp_matter::ENDPOINT_FLAG_DESTROYABLE;
    m_endpoint = esp_matter::endpoint::air_quality_sensor::create(root, &config_endpoint, flags, nullptr);
    if (!m_endpoint) {
        GetLogger(eLogType::Error)->Log("Failed to create endpoint");
        return false;
    }
    return CDevice::matter_init_endpoint();
}

bool CCO2Sensor::matter_config_attributes()
{
    esp_err_t ret;
    esp_matter::cluster_t *cluster;
    esp_matter::attribute_t *attribute;
    esp_matter_attr_val_t val;

    // add <Carbon Dioxide Concentration Measurement> cluster
    cluster = esp_matter::cluster::get(m_endpoint, chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Id);
    if (!cluster) {
        esp_matter::cluster::carbon_dioxide_concentration_measurement::config_t cfg_cluster;
        cluster = esp_matter::cluster::carbon_dioxide_concentration_measurement::create(m_endpoint, &cfg_cluster, esp_matter::cluster_flags::CLUSTER_FLAG_SERVER);
        if (!cluster) {
            GetLogger(eLogType::Error)->Log("Failed to create <Carbon Dioxide Concentration Measurement> cluster");
            return false;
        }
    }

    // set feature map
    attribute = esp_matter::attribute::get(cluster, chip::app::Clusters::Globals::Attributes::FeatureMap::Id);
    if (attribute) {
        val = esp_matter_invalid(nullptr);
        ret = esp_matter::attribute::get_val(attribute, &val);
        if (ret != ESP_OK) {
            GetLogger(eLogType::Error)->Log("Failed to get FeatureMap attribute value (ret: %d)", ret);
            return false;
        }
        val.val.u32 |= 0x1; // MEA, Cluster supports numeric measurement of substance
        ret = esp_matter::attribute::set_val(attribute, &val);
        if (ret != ESP_OK) {
            GetLogger(eLogType::Error)->Log("Failed to set FeatureMap attribute value (ret: %d)", ret);
            return false;
        }
    }

    uint32_t attribute_id;
    uint8_t flags;

    // create <Measured Value> attribute
    attribute_id = chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Attributes::MeasuredValue::Id;
    attribute = esp_matter::attribute::get(cluster, attribute_id);
    if (!attribute) {
        flags = esp_matter::attribute_flags::ATTRIBUTE_FLAG_NULLABLE;
        attribute = esp_matter::attribute::create(cluster, attribute_id, flags, esp_matter_nullable_float(nullable<float>()));
        // attribute = esp_matter::attribute::create(cluster, attribute_id, flags, esp_matter_nullable_uint16(nullable<uint16_t>()));
        if (!attribute) {
            GetLogger(eLogType::Error)->Log("Failed to create <Measured Value> attribute");
            return false;
        }
    }

    // create <Min Measured Value> attribute & set value
    attribute_id = chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Attributes::MinMeasuredValue::Id;
    attribute = esp_matter::attribute::get(cluster, attribute_id);
    if (!attribute) {
        flags = esp_matter::attribute_flags::ATTRIBUTE_FLAG_NULLABLE;
        attribute = esp_matter::attribute::create(cluster, attribute_id, flags, esp_matter_nullable_float(nullable<float>()));
        // attribute = esp_matter::attribute::create(cluster, attribute_id, flags, esp_matter_nullable_uint16(nullable<uint16_t>()));
        if (!attribute) {
            GetLogger(eLogType::Error)->Log("Failed to create <Min Measured Value> attribute");
            return false;
        }
    }
    val = esp_matter_nullable_float(400.f);
    // val = esp_matter_nullable_uint16(400);
    ret = esp_matter::attribute::set_val(attribute, &val);
    if (ret != ESP_OK) {
        GetLogger(eLogType::Warning)->Log("Failed to set MinMeasuredValue attribute value (ret: %d)", ret);
    }

    // create <Max Measured Value> attribute & set value
    attribute_id = chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Attributes::MaxMeasuredValue::Id;
    attribute = esp_matter::attribute::get(cluster, attribute_id);
    if (!attribute) {
        flags = esp_matter::attribute_flags::ATTRIBUTE_FLAG_NULLABLE;
        attribute = esp_matter::attribute::create(cluster, attribute_id, flags, esp_matter_nullable_float(nullable<float>()));
        // attribute = esp_matter::attribute::create(cluster, attribute_id, flags, esp_matter_nullable_uint16(nullable<uint16_t>()));
        if (!attribute) {
            GetLogger(eLogType::Error)->Log("Failed to create <Max Measured Value> attribute");
            return false;
        }
    }
    val = esp_matter_nullable_float(5000.f);
    // val = esp_matter_nullable_uint16(5000);
    ret = esp_matter::attribute::set_val(attribute, &val);
    if (ret != ESP_OK) {
        GetLogger(eLogType::Warning)->Log("Failed to set MaxMeasuredValue attribute value (ret: %d)", ret);
    }

    // create <Measurement Unit> attribute & set value
    attribute_id = chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Attributes::MeasurementUnit::Id;
    attribute = esp_matter::attribute::get(cluster, chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Attributes::MeasurementUnit::Id);
    if (!attribute) {
        flags = esp_matter::attribute_flags::ATTRIBUTE_FLAG_NONE;
        attribute = esp_matter::attribute::create(cluster, attribute_id, flags, esp_matter_enum8(0));   // PPM
        if (!attribute) {
            GetLogger(eLogType::Error)->Log("Failed to create <Measurement Unit> attribute");
            return false;
        }
    }

    return true;
}

void CCO2Sensor::matter_on_change_attribute_value(esp_matter::attribute::callback_type_t type, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *value)
{
    if (cluster_id == chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Id) {
        if (attribute_id == chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Attributes::MeasuredValue::Id) {
            if (m_matter_update_by_client_clus_co2measure_attr_measureval) {
                m_matter_update_by_client_clus_co2measure_attr_measureval = false;
            }
        }
    }
}

void CCO2Sensor::matter_update_all_attribute_values()
{
    matter_update_clus_co2measure_attr_measureval();
}

void CCO2Sensor::update_measured_value(float value)
{
    m_measured_value = value;
    if (m_measured_value != m_measured_value_prev) {
        GetLogger(eLogType::Info)->Log("Update measured CO2 concentration value as %g", value);
        matter_update_clus_co2measure_attr_measureval();
    }
    m_measured_value_prev = m_measured_value;
}

void CCO2Sensor::matter_update_clus_co2measure_attr_measureval(bool force_update/*=false*/)
{
    esp_matter_attr_val_t target_value = esp_matter_nullable_float(m_measured_value);
    // esp_matter_attr_val_t target_value = esp_matter_nullable_uint16((uint16_t)m_measured_value);
    matter_update_cluster_attribute_common(
        m_endpoint_id,
        chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Id,
        chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Attributes::MeasuredValue::Id,
        target_value,
        &m_matter_update_by_client_clus_co2measure_attr_measureval,
        force_update
    );
}