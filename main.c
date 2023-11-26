#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

const void *gpio_mem = NULL;

// GPIO function select register offsets
const uintptr_t GPFSEL_BASE = 0x0;
const uintptr_t GPFSEL0 = GPFSEL_BASE + 0x0;
const uintptr_t GPFSEL1 = GPFSEL_BASE + 0x4;
const uintptr_t GPFSEL2 = GPFSEL_BASE + 0x8;

// GPIO pin output set register offsets
const uintptr_t GPSET_BASE = 0x1c;
const uintptr_t GPSET0 = GPSET_BASE + 0x0;
const uintptr_t GPSET1 = GPSET_BASE + 0x4;

// GPIO pin clear register offsets
const uintptr_t GPCLR_BASE = 0x28;
const uintptr_t GPCLR0 = GPCLR_BASE + 0x0;
const uintptr_t GPCLR1 = GPCLR_BASE + 0x4;

// GPIO pin levels
const uintptr_t GPLEV_BASE = 0x34;
const uintptr_t GPLEV0 = GPLEV_BASE + 0x0;
const uintptr_t GPLEV1 = GPLEV_BASE + 0x4;

typedef enum GpioPin {
    pin0, pin1, pin2, pin3,
    pin4, pin5, pin6, pin7,
    pin8, pin9, pin10, pin11,
    pin12, pin13, pin14, pin15,
    pin16, pin17, pin18, pin19,
    pin20, pin21, pin22, pin23,
    pin24, pin25, pin26, pin27,
    PIN_COUNT,
} GpioPin;

typedef enum FunctionCode {
    input = 0b000, 
    output = 0b001, 
    alt0 = 0b100, 
    alt1 = 0b101, 
    alt2 = 0b110, 
    alt3 = 0b111, 
    alt4 = 0b011, 
    alt5 = 0b010, 
} FunctionCode;

void setPinFunction(GpioPin pin, FunctionCode function_code) {
    assert(pin >= 0 && pin < PIN_COUNT);

    const u32 register_size = 4;
    const u32 pins_per_register = 10;
    const u32 bits_per_field = 3;
    
    const u32 register_num = pin / pins_per_register;
    const u32 register_pin = pin % pins_per_register;
    
    const u32 shift_amt = register_pin * bits_per_field;
    
    uintptr_t reg_addr = (uintptr_t)gpio_mem + GPFSEL_BASE + (register_num * register_size);
    volatile u32 *reg = (u32 *)reg_addr;
    u32 value = *reg;
    value &= ~(0b111 << shift_amt); // clear existing function code for this pin
    value |= function_code << shift_amt; // set new function code
    *reg = value;
}

void writePinBitNoRMW(GpioPin pin, uintptr_t base_addr) {
    assert(pin >= 0 && pin < PIN_COUNT);

    const u32 pins_per_register = 32;
    const u32 register_size = 4;
    
    const u32 register_number = pin / pins_per_register;
    const u32 register_pin = pin % pins_per_register;

    const uintptr_t reg_addr = base_addr + (register_number * register_size);
    
    volatile u32 *reg = (u32 *)reg_addr;
    u32 val = 1 << register_pin;
    *reg = val;
}

void setPin(GpioPin pin) {
    writePinBitNoRMW(pin, (uintptr_t)gpio_mem + GPSET_BASE);
}

void clearPin(GpioPin pin) {
    writePinBitNoRMW(pin, (uintptr_t)gpio_mem + GPCLR_BASE);
}

void *initGpioMem() {
    void *result;
    int fd = open("/dev/gpiomem", O_RDWR);
    
    if (fd == -1) {
        printf("Failed to open /dev/gpiomem Error: %s\n", strerror(errno));
        return NULL;
    }
    
    result = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    
    if (result == MAP_FAILED) {
        printf("Failed to mmap /dev/gpiomem Error: \"%s\"\n", strerror(errno));
        return NULL;
    }
    
    return result;
}

const struct timespec sleep_time = {0, 100000000};

int main(int argc, char **argv) {
    gpio_mem = initGpioMem();
    if (gpio_mem == NULL) {
        return 1;
    }

    setPinFunction(pin2, output);
    clearPin(pin2);

    for (int i = 0; i < 20; i++) {
        if (i % 2 == 1) {
            setPin(pin2);
        } else {
            clearPin(pin2);
        }
        nanosleep(&sleep_time, NULL);
    }
    
    return 0;
}
