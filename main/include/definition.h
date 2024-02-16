#ifndef _DEFINITION_H_
#define _DEFINITION_H_
#pragma once

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif
#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#define PRODUCT_NAME            "YOGYUI-MATTER"

#define GPIO_PIN_DEFAULT_BTN    0
#define GPIO_PIN_I2C_SCL        19
#define GPIO_PIN_I2C_SDA        18

#define I2C_PORT_NUM            0
#define I2C_MASTER_FREQ         400000

#define TASK_STACK_DEPTH        4096

#endif