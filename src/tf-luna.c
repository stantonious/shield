
#include <stdlib.h>
#include <stdio.h>
#include "tf-luna.h"

#define I2C_SLAVE_ADDR 0x10
#define LIDAR_ADDR 16
#define PACKET_SIZE 3
#define REG_DIST_L 0
#define REG_DIST_H 1
#define REG_TEMP_L 4
#define REG_TEMP_H 5

const struct i2c_dt_spec* g_tfluna_dev_i2c;
int init_tfluna(const struct i2c_dt_spec *dev_i2c)
{
    g_tfluna_dev_i2c = dev_i2c;
    printk("Tfluna Init \n\r");
    uint8_t dev_reg[0x23] = {0};
    int ret = i2c_read_dt(dev_i2c, &dev_reg[0], 0x23);
    if (ret != 0)
    {
        printk("TF Luna Failed to write/read I2C device address %x at Reg. %x \n\r",
               dev_i2c->addr, dev_reg[0x22]);
        return 1;
    }
    printk("Tfluna Got data %x \n\r", dev_reg[0x22]);
    return 0;
}

uint32_t get_total_dist(uint8_t dist_l, uint8_t dist_h)
{
    return (dist_h * 0xFF) + dist_l;
}

float get_dist()
{

    uint8_t dist_h, dist_l;
    int rc = i2c_reg_read_byte_dt(g_tfluna_dev_i2c, REG_DIST_H, &dist_h);

    if (rc)
    {
        printk("Unable to read reg %x\n", REG_DIST_H);
    }

    rc = i2c_reg_read_byte_dt(g_tfluna_dev_i2c, REG_DIST_L, &dist_l);

    if (rc)
    {
        printk("Unable to read reg %x\n", REG_DIST_L);
    }

    return get_total_dist(dist_l,dist_h);
}
/*

float scale_sample(uint16_t sample)
{
    return 0.;
}

float read_sample(const struct i2c_dt_spec *dev_i2c)
{
    uint8_t sensor_regs[1] = {MLX90614_OBJ1_REG};
    uint8_t temp_reading[2] = {0};
    int ret = i2c_write_read_dt(dev_i2c, &sensor_regs[0], 1, &temp_reading[0], 2);
    if (ret != 0)
    {
        printk("Failed to write/read I2C device address %x at Reg. %x \n\r",
               dev_i2c->addr, sensor_regs[0]);
    }

    return scale_sample(*(uint16_t *)&temp_reading[0]);
}
*/