#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>

int init_tfluna(const struct i2c_dt_spec* dev_i2c);

float get_dist();