#ifndef _ENCODER_H_
#define _ENCODER_H_

#include <linux/interrupt.h>

#define MSG_BUFFER_SIZE 256

typedef irq_handler_t (*irq_handler_func)(unsigned int irq, void* dev_id, struct pt_regs* regs);

typedef struct encoder_data {
    char name[16];
    int gpio_a;
    int gpio_b;
    int gpio_sw;
    int invert_dir;
    int direction;
    int64_t counts;
    uint64_t timestamp;
    int64_t pulse_period;
    int switch_state;
    irq_handler_func a_irq_handler;
    irq_handler_func sw_irq_handler;
    int irq_a_number;
    int irq_sw_number;
}encoder_data_t;

static int     encoder_dev_open(struct inode *, struct file *);
static int     encoder_dev_release(struct inode *, struct file *);
static ssize_t encoder_dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t encoder_dev_write(struct file *, const char *, size_t, loff_t *);

// static irq_handler_t encoder_a_irq_handler(unsigned int irq, void* dev_id, struct pt_regs* regs);
// static irq_handler_t encoder_sw_irq_handler(unsigned int irq, void* dev_id, struct pt_regs* regs);

static irq_handler_t enc_0_a_irq_handler(unsigned int irq, void* dev_id, struct pt_regs* regs);
static irq_handler_t enc_0_sw_irq_handler(unsigned int irq, void* dev_id, struct pt_regs* regs);

static irq_handler_t enc_1_a_irq_handler(unsigned int irq, void* dev_id, struct pt_regs* regs);
static irq_handler_t enc_1_sw_irq_handler(unsigned int irq, void* dev_id, struct pt_regs* regs);

static irq_handler_t enc_2_a_irq_handler(unsigned int irq, void* dev_id, struct pt_regs* regs);
static irq_handler_t enc_2_sw_irq_handler(unsigned int irq, void* dev_id, struct pt_regs* regs);

static irq_handler_t enc_3_a_irq_handler(unsigned int irq, void* dev_id, struct pt_regs* regs);
static irq_handler_t enc_3_sw_irq_handler(unsigned int irq, void* dev_id, struct pt_regs* regs);

#endif