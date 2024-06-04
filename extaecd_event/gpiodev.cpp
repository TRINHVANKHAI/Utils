#include "gpiodev.h"

GpioDev::GpioDev(const char *chipname, int line_num)
{
    printf("%s\n", __func__);
    init(chipname, line_num);
}

GpioDev::~GpioDev()
{
    printf("%s\n", __func__);
    deinit();
}

int GpioDev::init(const char *chipname, int line_num)
{
    int ret;

    chip = gpiod_chip_open_by_name(chipname);
    if (!chip) {
        perror("Open chip failed\n");
        return -1;
    }

    line = gpiod_chip_get_line(chip, line_num);
    if (!line) {
        perror("Get line failed\n");
        gpiod_chip_close(chip);
        return -2;
    }

    ret = gpiod_line_request_output(line, CONSUMER, 0);
    if (ret < 0) {
        perror("Request line as output failed\n");
        gpiod_line_release(line);
        gpiod_chip_close(chip);
    }

    return 0;

}

void GpioDev::deinit()
{
    gpiod_line_release(line);
    gpiod_chip_close(chip);
}

int GpioDev::setValue()
{
    return gpiod_line_set_value(line, 1);
}

int GpioDev::clearValue()
{
    return gpiod_line_set_value(line, 0);
}

int GpioDev::toggleValue()
{
    m_state = (m_state == 1) ? 0 : 1;
    return gpiod_line_set_value(line, m_state);
}
