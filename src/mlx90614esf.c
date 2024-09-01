
#include <stdlib.h>
#include <stdio.h>
#include "mlx90614esf.h"

#define MLX90614_TEMP_REG 0x00U
#define MLX90614_CONF_REG 0x01U
#define MLX90614_NORMAL_MODE 0x00
#define MLX90614_NONNORMAL_MODE 0x01
#define MLX90614_ADDR (0x5AU)
#define MLX90614_AMBIENT_REG 0x06U
#define MLX90614_OBJ1_REG 0x07U
#define MLX90614_OBJ2_REG 0x08U

const struct i2c_dt_spec *g_temp_dev_i2c;

void print_float(float f)
{
    // static char buf[10];
    // gcvt(f, 6, buf); // need to import stdio.h
    // printf("%s", buf);
}

int init_mlx90614esf(const struct i2c_dt_spec *dev_i2c)
{
    g_temp_dev_i2c = dev_i2c;
    uint8_t sensor_regs[1] = {MLX90614_OBJ1_REG};
    uint8_t temp_reading[2] = {0};
    int ret = i2c_write_read_dt(dev_i2c, &sensor_regs[0], 1, &temp_reading[0], 2);
    if (ret != 0)
    {
        printk("Failed to write/read I2C device address %x at Reg. %x \n\r",
               dev_i2c->addr, sensor_regs[0]);
        return 1;
    }
    return 0;
}

float scale_sample(uint16_t sample)
{

    if (sample & 0x80000U)
    {
        printf("Error bit set");
    }
    double c = (sample / 50.0) - 273.15;
    double f = (1.8 * c) + 32.0;

    // printk("Temp c:%d f:%d\n",(int)c,(int)f);

    return f;
}

float read_sample()
{
    uint8_t sensor_regs[1] = {MLX90614_OBJ1_REG};
    uint8_t temp_reading[2] = {0};
    int ret = i2c_write_read_dt(g_temp_dev_i2c, &sensor_regs[0], 1, &temp_reading[0], 2);
    if (ret != 0)
    {
        printk("Failed to write/read I2C device address %x at Reg. %x \n\r",
               g_temp_dev_i2c->addr, sensor_regs[0]);
    }

    return scale_sample(*(uint16_t *)&temp_reading[0]);
}