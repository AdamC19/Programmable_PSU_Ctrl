/**
 * 
 *
*/
#include <linux/init.h>             // Macros used to mark up functions e.g., __init __exit
#include <linux/module.h>           // Core header for loading LKMs into the kernel
#include <linux/moduleparam.h>
#include <linux/kernel.h>           // Contains types, macros, functions for the kernel
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>          // for copy to user function
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/kobject.h>          // using kobjects for the sysfs bindings
#include <linux/time.h>
#include <linux/timekeeping.h>
#include <linux/types.h>

#include "encoder.h"

#define DEVICE_NAME     "encoder"   ///< the device will appear at /dev/encoder using this value
#define CLASS_NAME      "enc"       ///< the device class -- this is a character device driver
 
MODULE_LICENSE("GPL");              ///< The license type -- this affects runtime behavior
MODULE_AUTHOR("Adam Cordingley");      ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("An quadrature encoder reader for the Beaglebone Black.");  ///< The description -- see modinfo
MODULE_VERSION("0.1"); 


static char enc0_name[16] = "enc0";
static char enc1_name[16] = "enc1";
static char enc2_name[16] = "enc2";
static char enc3_name[16] = "enc3";

static int encoder0_gpio = -1;
static int encoder1_gpio = -1;
static int encoder2_gpio = -1;
static int encoder3_gpio = -1;
static encoder_data_t encoders[4];

module_param(encoder0_gpio, int, 0664);
module_param(encoder1_gpio, int, 0664);
module_param(encoder2_gpio, int, 0664);
module_param(encoder3_gpio, int, 0664);

MODULE_PARM_DESC(encoder0_gpio, "The gpio info for quadrature encoder 0.");
MODULE_PARM_DESC(encoder1_gpio, "The gpio info for quadrature encoder 1.");
MODULE_PARM_DESC(encoder2_gpio, "The gpio info for quadrature encoder 2.");
MODULE_PARM_DESC(encoder3_gpio, "The gpio info for quadrature encoder 3.");

///////////////////////////////////////////////////////////////////////////////
// KOBJECT & SYSFS THINGS
///////////////////////////////////////////////////////////////////////////////
static ssize_t enc0_counts_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    return sprintf(buf, "%d\n", encoders[0].counts);
}
static ssize_t enc1_counts_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    return sprintf(buf, "%d\n", encoders[1].counts);
}
static ssize_t enc2_counts_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    return sprintf(buf, "%d\n", encoders[2].counts);
}
static ssize_t enc3_counts_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    return sprintf(buf, "%d\n", encoders[3].counts);
}

static ssize_t enc0_counts_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){
    sscanf(buf, "%d", &encoders[0].counts);
    return count;
}
static ssize_t enc1_counts_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){
    sscanf(buf, "%d", &encoders[1].counts);
    return count;
}
static ssize_t enc2_counts_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){
    sscanf(buf, "%d", &encoders[2].counts);
    return count;
}
static ssize_t enc3_counts_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){
    sscanf(buf, "%d", &encoders[3].counts);
    return count;
}

static ssize_t enc0_pulse_period_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    return sprintf(buf, "%d\n", encoders[0].pulse_period);
}
static ssize_t enc1_pulse_period_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    return sprintf(buf, "%d\n", encoders[1].pulse_period);
}
static ssize_t enc2_pulse_period_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    return sprintf(buf, "%d\n", encoders[2].pulse_period);
}
static ssize_t enc3_pulse_period_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    return sprintf(buf, "%d\n", encoders[3].pulse_period);
}

static ssize_t enc0_switch_state_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    ssize_t retval = sprintf(buf, "%d\n", encoders[0].switch_state);
    // once value is retrieved, set back to 0
    encoders[0].switch_state = 0;
    return retval;
}
static ssize_t enc1_switch_state_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    ssize_t retval = sprintf(buf, "%d\n", encoders[1].switch_state);
    // once value is retrieved, set back to 0
    encoders[1].switch_state = 0;
    return retval;
}
static ssize_t enc2_switch_state_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    ssize_t retval = sprintf(buf, "%d\n", encoders[2].switch_state);
    // once value is retrieved, set back to 0
    encoders[2].switch_state = 0;
    return retval;
}
static ssize_t enc3_switch_state_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    ssize_t retval = sprintf(buf, "%d\n", encoders[3].switch_state);
    // once value is retrieved, set back to 0
    encoders[3].switch_state = 0;
    return retval;
}


/**  Use these helper macros to define the name and access levels of the kobj_attributes
 *  The kobj_attribute has an attribute attr (name and mode), show and store function pointers
 *  The count variable is associated with the numberPresses variable and it is to be exposed
 *  with mode 0666 using the numberPresses_show and numberPresses_store functions above
 */
static struct kobj_attribute enc0_count_attr = __ATTR(counts, 0664, enc0_counts_show, enc0_counts_store);
static struct kobj_attribute enc1_count_attr = __ATTR(counts, 0664, enc1_counts_show, enc1_counts_store);
static struct kobj_attribute enc2_count_attr = __ATTR(counts, 0664, enc2_counts_show, enc2_counts_store);
static struct kobj_attribute enc3_count_attr = __ATTR(counts, 0664, enc3_counts_show, enc3_counts_store);

static struct kobj_attribute enc0_pulse_period_attr = __ATTR(pulse_period, 0444, enc0_pulse_period_show, NULL);     ///< 
static struct kobj_attribute enc1_pulse_period_attr = __ATTR(pulse_period, 0444, enc1_pulse_period_show, NULL);     ///< 
static struct kobj_attribute enc2_pulse_period_attr = __ATTR(pulse_period, 0444, enc2_pulse_period_show, NULL);     ///< 
static struct kobj_attribute enc3_pulse_period_attr = __ATTR(pulse_period, 0444, enc3_pulse_period_show, NULL);     ///< 

static struct kobj_attribute enc0_switch_state_attr  = __ATTR(switch_state, 0444, enc0_switch_state_show, NULL);     ///< 
static struct kobj_attribute enc1_switch_state_attr  = __ATTR(switch_state, 0444, enc1_switch_state_show, NULL);     ///< 
static struct kobj_attribute enc2_switch_state_attr  = __ATTR(switch_state, 0444, enc2_switch_state_show, NULL);     ///< 
static struct kobj_attribute enc3_switch_state_attr  = __ATTR(switch_state, 0444, enc3_switch_state_show, NULL);     ///< 

/**  The ebb_attrs[] is an array of attributes that is used to create the attribute group below.
 *  The attr property of the kobj_attribute is used to extract the attribute struct
 */
static struct attribute *enc0_attrs[] = {
      &enc0_count_attr.attr,                  ///< The number of button presses
      &enc0_pulse_period_attr.attr,                  ///< 
      &enc0_switch_state_attr.attr,                   ///< 
      NULL,
};
static struct attribute *enc1_attrs[] = {
      &enc1_count_attr.attr,                  ///< The number of button presses
      &enc1_pulse_period_attr.attr,                  ///< 
      &enc1_switch_state_attr.attr,                   ///< 
      NULL,
};
static struct attribute *enc2_attrs[] = {
      &enc2_count_attr.attr,                  ///< The number of button presses
      &enc2_pulse_period_attr.attr,                  ///< 
      &enc2_switch_state_attr.attr,                   ///< 
      NULL,
};
static struct attribute *enc3_attrs[] = {
      &enc3_count_attr.attr,                  ///< The number of button presses
      &enc3_pulse_period_attr.attr,                  ///< 
      &enc3_switch_state_attr.attr,                   ///< 
      NULL,
};
 
/**  The attribute group uses the attribute array and a name, which is exposed on sysfs -- in this
 *  case it is gpio115, which is automatically defined in the ebbButton_init() function below
 *  using the custom kernel parameter that can be passed when the module is loaded.
 */
static struct attribute_group enc0_attr_group = {
      .name  = encoders[0].name,                 ///< The name is generated in ebbButton_init()
      .attrs = enc0_attrs,                ///< The attributes array defined just above
};
static struct attribute_group enc1_attr_group = {
      .name  = encoders[1].name,                 ///< The name is generated in ebbButton_init()
      .attrs = enc1_attrs,                ///< The attributes array defined just above
};
static struct attribute_group enc2_attr_group = {
      .name  = encoders[2].name,                 ///< The name is generated in ebbButton_init()
      .attrs = enc2_attrs,                ///< The attributes array defined just above
};
static struct attribute_group enc3_attr_group = {
      .name  = encoders[3].name,                 ///< The name is generated in ebbButton_init()
      .attrs = enc3_attrs,                ///< The attributes array defined just above
};

static struct kobject *enc_kobj;

///////////////////////////////////////////////////////////////////////////////
// MODULE INIT AND EXIT
///////////////////////////////////////////////////////////////////////////////

/**
 * check if all specifiec GPIO numbers in the enc struct are valid
 */
static int check_encoder_gpios(encoder_data_t* enc){
    if (!gpio_is_valid(enc->gpio_a)){
        printk(KERN_INFO "ENCODER: invalid channel A GPIO\n");
        return -ENODEV;
    }
    if (!gpio_is_valid(enc->gpio_b)){
        printk(KERN_INFO "ENCODER: invalid channel B GPIO\n");
        return -ENODEV;
    }
    if (!gpio_is_valid(enc->gpio_sw)){
        printk(KERN_INFO "ENCODER: invalid switch GPIO\n");
        return -ENODEV;
    }
    return 0;
}

/**
 * ensure that irq handler function pointers are set in the encoder_data struct
 * before calling this function.
 */
static int init_encoder_gpio(encoder_data_t* enc, int gpio_info){
    enc->invert_dir = (gpio_info >> 24) & 0xFF;
    enc->gpio_a = (gpio_info >> 16) & 0xFF;
    enc->gpio_b = (gpio_info >> 8) & 0xFF;
    enc->gpio_sw = gpio_info & 0xFF;
    if(check_encoder_gpios(enc)){
        return -ENODEV;
    }
    gpio_request(enc->gpio_a, "sysfs");
    gpio_direction_input(enc->gpio_a);
    gpio_export(enc->gpio_a, false);
    enc->irq_a_number = gpio_to_irq(enc->gpio_a);
    int result = request_irq(enc->irq_a_number,
                            (irq_handler_t)enc->a_irq_handler,
                            IRQF_TRIGGER_RISING,
                            "enc_a_gpio_handler",
                            NULL);

    gpio_request(enc->gpio_b, "sysfs");
    gpio_direction_input(enc->gpio_b);
    gpio_export(enc->gpio_b, false);
    
    // Switch
    gpio_request(enc->gpio_sw, "sysfs");
    gpio_direction_input(enc->gpio_sw);
    gpio_set_debounce(enc->gpio_sw, 50); // 50ms debounce delay
    gpio_export(enc->gpio_sw, false);
    enc->irq_sw_number = gpio_to_irq(enc->gpio_sw);
    result += request_irq(enc->irq_sw_number,
                        (irq_handler_t)enc->sw_irq_handler,
                        IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                        "enc_sw_gpio_handler",
                        NULL);
    return result;
}

/**
 * call for each encoder on exit
 */
static void cleanup_encoder_gpios(encoder_data_t* enc){
    free_irq(enc->irq_a_number, NULL);
    free_irq(enc->irq_sw_number, NULL);
    gpio_unexport(enc->gpio_a);
    gpio_unexport(enc->gpio_b);
    gpio_unexport(enc->gpio_sw);
    gpio_free(enc->gpio_a);
    gpio_free(enc->gpio_b);
    gpio_free(enc->gpio_sw);
}


static int __init encoder_init(void){
    printk(KERN_INFO "ENCODER: Initializing the encoder LKM\n");
    
    strcpy(encoders[0].name, enc0_name);
    strcpy(encoders[1].name, enc1_name);
    strcpy(encoders[2].name, enc2_name);
    strcpy(encoders[3].name, enc3_name);

    // create the kobject sysfs entry at /sys/kernel/enc 
    enc_kobj = kobject_create_and_add("enc", kernel_kobj); // kernel_kobj points to /sys/kernel
    if(!enc_kobj){
        printk(KERN_ALERT "ENCODER: failed to create kobject mapping\n");
        return -ENOMEM;
    }
    // add the attributes to /sys/kernel/enc/ -- for example, /sys/kernel/enc/enc0/counts
    int result = sysfs_create_group(enc_kobj, &enc0_attr_group);
    result += sysfs_create_group(enc_kobj, &enc1_attr_group);
    result += sysfs_create_group(enc_kobj, &enc2_attr_group);
    result += sysfs_create_group(enc_kobj, &enc3_attr_group);
    if(result) {
        printk(KERN_ALERT "ENCODER: failed to create sysfs group\n");
        kobject_put(enc_kobj);                          // clean up -- remove the kobject sysfs entry
        return result;
    }

    // ==== GPIO THINGS ====
    if(encoder0_gpio > 0){
        encoders[0].a_irq_handler = enc_0_a_irq_handler;
        encoders[0].sw_irq_handler = enc_0_sw_irq_handler;
        result += init_encoder_gpio(&encoders[0], encoder0_gpio);
    }
    if(encoder1_gpio > 0){
        encoders[1].a_irq_handler = enc_1_a_irq_handler;
        encoders[1].sw_irq_handler = enc_1_sw_irq_handler;
        result += init_encoder_gpio(&encoders[1], encoder1_gpio);
    }
    if(encoder2_gpio > 0){
        encoders[2].a_irq_handler = enc_2_a_irq_handler;
        encoders[2].sw_irq_handler = enc_2_sw_irq_handler;
        result += init_encoder_gpio(&encoders[2], encoder2_gpio);
    }
    if(encoder3_gpio > 0){
        encoders[3].a_irq_handler = enc_3_a_irq_handler;
        encoders[3].sw_irq_handler = enc_3_sw_irq_handler;
        result += init_encoder_gpio(&encoders[3], encoder3_gpio);
    }
    
    // ==== END GPIO THINGS ====

    return result;
}

static void __exit encoder_exit(void){
    kobject_put(enc_kobj);                   // clean up -- remove the kobject sysfs entry
    cleanup_encoder_gpios(&encoders[0]);
    cleanup_encoder_gpios(&encoders[1]);
    cleanup_encoder_gpios(&encoders[2]);
    cleanup_encoder_gpios(&encoders[3]);
    // device_destroy(encoder_class, MKDEV(major_num, 0));
    // class_unregister(encoder_class);
    // class_destroy(encoder_class);
    // unregister_chrdev(major_num, DEVICE_NAME);
    printk(KERN_INFO "ENCODER: Exiting...\n");
}


///////////////////////////////////////////////////////////////////////////////
// GPIO HANDLING THINGS
///////////////////////////////////////////////////////////////////////////////

static irq_handler_t handle_quadrature(encoder_data_t* enc){
    uint64_t timestamp = ktime_get_ns();
    if(gpio_get_value(enc->gpio_b) == enc->invert_dir){
        enc->direction = 1;
    }else{
        enc->direction = -1;
    }
    enc->counts += enc->direction;
    enc->pulse_period = timestamp - enc->timestamp;
    enc->timestamp = timestamp;
    return (irq_handler_t) IRQ_HANDLED;  // Announce that the IRQ has been handled correctly
}

static irq_handler_t handle_button(encoder_data_t* enc){
    enc->switch_state = gpio_get_value(enc->gpio_sw);
    return (irq_handler_t) IRQ_HANDLED;  // Announce that the IRQ has been handled correctly
}


static irq_handler_t enc_0_a_irq_handler(unsigned int irq, void* dev_id, struct pt_regs* regs){
    return handle_quadrature(&encoders[0]);
}
static irq_handler_t enc_0_sw_irq_handler(unsigned int irq, void* dev_id, struct pt_regs* regs){
    return handle_button(&encoders[0]);
}

static irq_handler_t enc_1_a_irq_handler(unsigned int irq, void* dev_id, struct pt_regs* regs){
    return handle_quadrature(&encoders[1]);
}
static irq_handler_t enc_1_sw_irq_handler(unsigned int irq, void* dev_id, struct pt_regs* regs){
    return handle_button(&encoders[1]);
}

static irq_handler_t enc_2_a_irq_handler(unsigned int irq, void* dev_id, struct pt_regs* regs){
    return handle_quadrature(&encoders[2]);
}
static irq_handler_t enc_2_sw_irq_handler(unsigned int irq, void* dev_id, struct pt_regs* regs){
    return handle_button(&encoders[2]);
}

static irq_handler_t enc_3_a_irq_handler(unsigned int irq, void* dev_id, struct pt_regs* regs){
    return handle_quadrature(&encoders[3]);
}
static irq_handler_t enc_3_sw_irq_handler(unsigned int irq, void* dev_id, struct pt_regs* regs){
    return handle_button(&encoders[3]);
}

module_init(encoder_init);
module_exit(encoder_exit);