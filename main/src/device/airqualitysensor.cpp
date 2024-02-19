#include "airqualitysensor.h"
#include "system.h"
#include "logger.h"

CAirQualitySensor::CAirQualitySensor()
{
    m_matter_update_by_client_clus_co2measure_attr_measureval = false;
    m_matter_update_by_client_clus_tempmeasure_attr_measureval = false;
    m_matter_update_by_client_clus_relhummeasure_attr_measureval = false;
}

bool CAirQualitySensor::matter_init_endpoint()
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

bool CAirQualitySensor::matter_config_attributes()
{
    esp_err_t ret;
    esp_matter::cluster_t *cluster;
    esp_matter::attribute_t *attribute;
    esp_matter_attr_val_t val;

    // set air quality as GOOD
    cluster = esp_matter::cluster::get(m_endpoint, chip::app::Clusters::AirQuality::Id);
    if (cluster) {
        attribute = esp_matter::attribute::get(cluster, chip::app::Clusters::AirQuality::Attributes::AirQuality::Id);
        if (attribute) {
            val = esp_matter_invalid(nullptr);
            ret = esp_matter::attribute::get_val(attribute, &val);
            if (ret != ESP_OK) {
                GetLogger(eLogType::Error)->Log("Failed to get AirQuality attribute value (ret: %d)", ret);
                return false;
            }
            val.val.u8 = 1; // GOOD
            ret = esp_matter::attribute::set_val(attribute, &val);
            if (ret != ESP_OK) {
                GetLogger(eLogType::Error)->Log("Failed to set AirQuality attribute value (ret: %d)", ret);
                return false;
            }
        }
    }

    if (!create_temperature_measurement_cluster()) return false;
    if (!create_relative_humidity_measurement_cluster()) return false;
    if (!create_carbon_dioxide_concentration_measurement_cluster()) return false;

    return true;
}

bool CAirQualitySensor::create_temperature_measurement_cluster()
{
    esp_matter::cluster_t *cluster = esp_matter::cluster::get(m_endpoint, chip::app::Clusters::TemperatureMeasurement::Id);
    if (!cluster) {
        esp_matter::cluster::temperature_measurement::config_t cfg_tempmeasure_cluster;
        cluster = esp_matter::cluster::temperature_measurement::create(m_endpoint, &cfg_tempmeasure_cluster, esp_matter::cluster_flags::CLUSTER_FLAG_SERVER);
        if (!cluster) {
            GetLogger(eLogType::Error)->Log("Failed to create <Temperature Measurement> cluster");
            return false;
        }
    }

    return true;
}

bool CAirQualitySensor::create_relative_humidity_measurement_cluster()
{
    esp_matter::cluster_t *cluster = esp_matter::cluster::get(m_endpoint, chip::app::Clusters::RelativeHumidityMeasurement::Id);
    if (!cluster) {
        esp_matter::cluster::relative_humidity_measurement::config_t cfg_relhummeasure_cluster;
        cluster = esp_matter::cluster::relative_humidity_measurement::create(m_endpoint, &cfg_relhummeasure_cluster, esp_matter::cluster_flags::CLUSTER_FLAG_SERVER);
        if (!cluster) {
            GetLogger(eLogType::Error)->Log("Failed to create <Relative Humidity Measurement> cluster");
            return false;
        }
    }

    return true;
}

bool CAirQualitySensor::create_carbon_dioxide_concentration_measurement_cluster()
{
    esp_err_t ret;
    esp_matter::cluster_t *cluster;
    esp_matter::attribute_t *attribute;
    esp_matter_attr_val_t val;
    uint32_t attribute_id;
    uint8_t flags;

    cluster = esp_matter::cluster::get(m_endpoint, chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Id);
    if (!cluster) {
        esp_matter::cluster::carbon_dioxide_concentration_measurement::config_t cfg_co2measure_cluster;
        cluster = esp_matter::cluster::carbon_dioxide_concentration_measurement::create(m_endpoint, &cfg_co2measure_cluster, esp_matter::cluster_flags::CLUSTER_FLAG_SERVER);
        if (!cluster) {
            GetLogger(eLogType::Error)->Log("Failed to create <Carbon Dioxide Concentration Measurement> cluster");
            return false;
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
            if (!attribute) {
                GetLogger(eLogType::Error)->Log("Failed to create <Min Measured Value> attribute");
                return false;
            }
        }

        // create <Max Measured Value> attribute & set value
        attribute_id = chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Attributes::MaxMeasuredValue::Id;
        attribute = esp_matter::attribute::get(cluster, attribute_id);
        if (!attribute) {
            flags = esp_matter::attribute_flags::ATTRIBUTE_FLAG_NULLABLE;
            attribute = esp_matter::attribute::create(cluster, attribute_id, flags, esp_matter_nullable_float(nullable<float>()));
            if (!attribute) {
                GetLogger(eLogType::Error)->Log("Failed to create <Max Measured Value> attribute");
                return false;
            }
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
    }

    return true;
}

bool CAirQualitySensor::set_carbon_dioxide_concentration_measurement_min_measured_value(float value)
{
    esp_matter::cluster_t *cluster = esp_matter::cluster::get(m_endpoint, chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Id);
    if (!cluster) {
        GetLogger(eLogType::Error)->Log("Failed to get CarbonDioxideConcentrationMeasurement cluster");
        return false;
    }
    esp_matter::attribute_t *attribute = esp_matter::attribute::get(cluster, chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Attributes::MinMeasuredValue::Id);
    if (!attribute) {
        GetLogger(eLogType::Error)->Log("Failed to get MinMeasuredValue attribute");
        return false;
    }
    esp_matter_attr_val_t val = esp_matter_nullable_float(value);
    esp_err_t ret = esp_matter::attribute::set_val(attribute, &val);
    if (ret != ESP_OK) {
        GetLogger(eLogType::Error)->Log("Failed to set MinMeasuredValue attribute value (ret: %d)", ret);
        return false;
    }

    return true;
}

bool CAirQualitySensor::set_carbon_dioxide_concentration_measurement_max_measured_value(float value)
{
    esp_matter::cluster_t *cluster = esp_matter::cluster::get(m_endpoint, chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Id);
    if (!cluster) {
        GetLogger(eLogType::Error)->Log("Failed to get CarbonDioxideConcentrationMeasurement cluster");
        return false;
    }
    esp_matter::attribute_t *attribute = esp_matter::attribute::get(cluster, chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Attributes::MaxMeasuredValue::Id);
    if (!attribute) {
        GetLogger(eLogType::Error)->Log("Failed to get MaxMeasuredValue attribute");
        return false;
    }
    esp_matter_attr_val_t val = esp_matter_nullable_float(value);
    esp_err_t ret = esp_matter::attribute::set_val(attribute, &val);
    if (ret != ESP_OK) {
        GetLogger(eLogType::Warning)->Log("Failed to set MaxMeasuredValue attribute value (ret: %d)", ret);
    }

    return true;
}

bool CAirQualitySensor::set_carbon_dioxide_concentration_measurement_measurement_unit(int value)
{
    esp_matter::cluster_t *cluster = esp_matter::cluster::get(m_endpoint, chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Id);
    if (!cluster) {
        GetLogger(eLogType::Error)->Log("Failed to get CarbonDioxideConcentrationMeasurement cluster");
        return false;
    }
    esp_matter::attribute_t *attribute = esp_matter::attribute::get(cluster, chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Attributes::MeasurementUnit::Id);
    if (!attribute) {
        GetLogger(eLogType::Error)->Log("Failed to get MeasurementUnit attribute");
        return false;
    }
    esp_matter_attr_val_t val = esp_matter_enum8(value);
    esp_err_t ret = esp_matter::attribute::set_val(attribute, &val);
    if (ret != ESP_OK) {
        GetLogger(eLogType::Error)->Log("Failed to set MeasurementUnit attribute value (ret: %d)", ret);
        return false;
    }

    return true;
}

void CAirQualitySensor::matter_on_change_attribute_value(esp_matter::attribute::callback_type_t type, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *value)
{
    if (cluster_id == chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Id) {
        if (attribute_id == chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Attributes::MeasuredValue::Id) {
            if (m_matter_update_by_client_clus_co2measure_attr_measureval) {
                m_matter_update_by_client_clus_co2measure_attr_measureval = false;
            }
        }
    } else if (cluster_id == chip::app::Clusters::TemperatureMeasurement::Id) {
        if (attribute_id == chip::app::Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Id) {
            if (m_matter_update_by_client_clus_tempmeasure_attr_measureval) {
                m_matter_update_by_client_clus_tempmeasure_attr_measureval = false;
            }
        }
    } else if (cluster_id == chip::app::Clusters::RelativeHumidityMeasurement::Id) {
        if (attribute_id == chip::app::Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Id) {
            if (m_matter_update_by_client_clus_relhummeasure_attr_measureval) {
                m_matter_update_by_client_clus_relhummeasure_attr_measureval = false;
            }
        }
    }
}

void CAirQualitySensor::matter_update_all_attribute_values()
{
    matter_update_clus_co2measure_attr_measureval();
    matter_update_clus_tempmeasure_attr_measureval();
    matter_update_clus_relhummeasure_attr_measureval();
}

void CAirQualitySensor::update_measured_value_co2ppm(float value)
{
    m_measured_value_co2ppm = value;
    if (m_measured_value_co2ppm != m_measured_value_co2ppm_prev) {
        GetLogger(eLogType::Info)->Log("Update measured CO2 concentration value as %g", value);
        matter_update_clus_co2measure_attr_measureval();
    }
    m_measured_value_co2ppm_prev = m_measured_value_co2ppm;
}

void CAirQualitySensor::update_measured_value_temperature(float value)
{
    m_measured_value_temperature = (int16_t)(value * 100.f);
    if (m_measured_value_temperature_prev != m_measured_value_temperature) {
        GetLogger(eLogType::Info)->Log("Update measured temperature value as %g", value);
        matter_update_clus_tempmeasure_attr_measureval();
    }
    m_measured_value_temperature_prev = m_measured_value_temperature;
}

void CAirQualitySensor::update_measured_value_humidity(float value)
{
    m_measured_value_humidity = (uint16_t)(value * 100.f);
    if (m_measured_value_humidity_prev != m_measured_value_humidity) {
        GetLogger(eLogType::Info)->Log("Update measured relative humidity value as %g", value);
        matter_update_clus_relhummeasure_attr_measureval();
    }
    m_measured_value_humidity_prev = m_measured_value_humidity;
}

void CAirQualitySensor::matter_update_clus_co2measure_attr_measureval(bool force_update/*=false*/)
{
    esp_matter_attr_val_t target_value = esp_matter_nullable_float(m_measured_value_co2ppm);
    matter_update_cluster_attribute_common(
        m_endpoint_id,
        chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Id,
        chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Attributes::MeasuredValue::Id,
        target_value,
        &m_matter_update_by_client_clus_co2measure_attr_measureval,
        force_update
    );
}

void CAirQualitySensor::matter_update_clus_tempmeasure_attr_measureval(bool force_update/*=false*/)
{
    esp_matter_attr_val_t target_value = esp_matter_nullable_int16(m_measured_value_temperature);
    matter_update_cluster_attribute_common(
        m_endpoint_id,
        chip::app::Clusters::TemperatureMeasurement::Id,
        chip::app::Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Id,
        target_value,
        &m_matter_update_by_client_clus_tempmeasure_attr_measureval,
        force_update
    );
}

void CAirQualitySensor::matter_update_clus_relhummeasure_attr_measureval(bool force_update/*=false*/)
{
    esp_matter_attr_val_t target_value = esp_matter_nullable_uint16(m_measured_value_humidity);
    matter_update_cluster_attribute_common(
        m_endpoint_id,
        chip::app::Clusters::RelativeHumidityMeasurement::Id,
        chip::app::Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Id,
        target_value,
        &m_matter_update_by_client_clus_relhummeasure_attr_measureval,
        force_update
    );
}