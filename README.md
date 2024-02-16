# Matter CO2 Sensor Example (ESP32 + SCD41)
Matter 이산화탄소 농도 측정 센서 예제 프로젝트<br>
다음 Matter 클러스터들에 대한 코드 구현 방법을 알아본다
- Carbon Dioxide Concentration Measurement (Cluster Id: `0x040D`)

Software (Matter)
---
3개의 Endpoint가 아래와 같이 생성된다. (센서 모듈이 CO2 농도 뿐만 아니라 온도와 상대 습도 데이터도 제공함)

1. Endpoint ID `1`<br>
    Device Type: Air Quality Sensor (Classification ID: `0x002C`)<br>
    [Clusters]
    - Air Quality (Cluster ID: `0x005B`)
    - Carbon Dioxide Concentration Measurement (Cluster ID: `0x040D`)<br>
        [Attributes]
        - Measured Value (Attribute ID: `0x0000`)
        - Min Measured Value (Attribute ID: `0x0001`)
        - Max Measured Value (Attribute ID: `0x0002`)
        - Measurement Unit (Attribute ID: `0x0008`)
2. Endpoint ID `2`<br>
    Device Type: Temperature Sensor (Classification ID: `0x0302`)<br>
    [Clusters]
    - Temperature Measurement (Cluster ID: `0x0402`)<br>
        [Attributes]
        - Measured Value (Attribute ID: `0x0000`)
        - Min Measured Value (Attribute ID: `0x0001`)
        - Max Measured Value (Attribute ID: `0x0002`)
3. Endpoint ID `3`<br>
    Device Type: Humidity Sensor (Classification ID: `0x0307`) <br>
    [Clusters]
    - Relative Humidity Measurement (Cluster ID: `0x0405`)<br>
        [Attributes]
        - Measured Value (Attribute ID: `0x0000`)
        - Min Measured Value (Attribute ID: `0x0001`)
        - Max Measured Value (Attribute ID: `0x0002`)


Hardware
---
[SCD41](https://sensirion.com/media/documents/48C4B7FB/64C134E7/Sensirion_SCD4x_Datasheet.pdf): I2C 통신 방식의 CO2 센서 IC 사용<br>
<p style="text-align:center"><img src="https://github.com/DFRobot/DFRobot_SCD4X/raw/main/resources/images/SCD41.png" width="300"></p><br>

I2C GPIO 핀번호 변경은 /main/include/definition.h에서 아래 항목을 수정<br>
default: `SDA` = GPIO`18` / `SCL` = GPIO`19`
```c
#define GPIO_PIN_I2C_SCL 19
#define GPIO_PIN_I2C_SDA 18
```

SDK Version
---
- esp-idf: [v5.1.2](https://github.com/espressif/esp-idf/tree/v5.1.2)
- esp-matter: [fe4f9f69634b060744f06560b7afdaf25d96ba37](https://github.com/espressif/esp-matter/commit/fe4f9f69634b060744f06560b7afdaf25d96ba37)
- connectedhomeip: [d38a6496c3abeb3daf6429b1f11172cfa0112a1a](https://github.com/project-chip/connectedhomeip/tree/d38a6496c3abeb3daf6429b1f11172cfa0112a1a)
  - Matter 1.1 released (2023.05.18)
  - Matter 1.2 released (2023.10.23)

SDK Source Code Modification
---
- esp_matter_cluster.h
    ```cpp
    namespace carbon_dioxide_concentration_measurement {
    typedef struct config {
        uint16_t cluster_revision;
        config() : cluster_revision(3) {}
    } config_t;

    // 아래 한줄 추가
    cluster_t *create(endpoint_t *endpoint, config_t *config, uint8_t flags);
    } /* carbon_dioxide_concentration_measurement */
    ```
- esp_matter_attribute.cpp
    ```cpp
    static esp_err_t get_attr_val_from_data(esp_matter_attr_val_t *val, EmberAfAttributeType attribute_type,
                                        uint16_t attribute_size, uint8_t *value,
                                        const EmberAfAttributeMetadata * attribute_metadata)
    {
        switch (attribute_type) {
        /*
        * 기존 코드 유지
        */
        
        // 아래 블록 추가
        case ZCL_SINGLE_ATTRIBUTE_TYPE: {
            using Traits = chip::app::NumericAttributeTraits<float>;
            Traits::StorageType attribute_value;
            memcpy((float *)&attribute_value, value, sizeof(Traits::StorageType));
            if (attribute_metadata->IsNullable()) {
                if (Traits::IsNullValue(attribute_value)) {
                    *val = esp_matter_nullable_float(nullable<float>());
                } else {
                    *val = esp_matter_nullable_float(attribute_value);
                }
            } else {
                *val = esp_matter_float(attribute_value);
            }
            break;
        }
        
        default:
            *val = esp_matter_invalid(NULL);
            break;
        }

        return ESP_OK;
    }
    ```

Helper Scripts
---
SDK 클론 및 설치
```shell
$ source ./scripts/install_sdk.sh
```
SDK (idf.py) 준비
```shell
$ source ./scripts/prepare_sdk.sh
```

Build & Flash Firmware
---
1. Factory Partition (Matter DAC)
    ```shell
    $ source ./scripts/flash_factory_dac_provider.sh
    ```
2. Configure project
    ```shell
    $ idf.py set-target esp32
    ```
3. Build Firmware
    ```shell
    $ idf.py build
    ```
4. Flash Firmware
    ```shell
    $ idf.py -p ${seiral_port} flash monitor
    ```

QR Code for commisioning
---
![qrcode.png](./resource/DACProvider/qrcode.png)

References
---
[Matter 이산화탄소 농도 측정 클러스터 개발 예제 (ESP32)](https://yogyui.tistory.com/entry/PROJ-Matter-CO2-%EC%84%BC%EC%84%9C-%EA%B0%9C%EB%B0%9C-%EC%98%88%EC%A0%9C-ESP32)<br>