#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>

int init_mlx90614esf(const struct i2c_dt_spec* dev_i2c);

float scale_sample(uint16_t sample);

float read_sample();