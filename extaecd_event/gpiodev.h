#ifndef GPIODEV_H
#define GPIODEV_H
#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>

#ifndef	CONSUMER
#define	CONSUMER	"Consumer"
#endif

class GpioDev
{
public:
    GpioDev(const char *chipname, int line_num);
    ~GpioDev();
    int setValue();
    int clearValue();
    int toggleValue();
private:
    int init(const char *chipname, int line_num);
    void deinit();
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    int m_state;
};

#endif // GPIODEV_H
