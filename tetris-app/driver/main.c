#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <asm/io.h>
#include <asm/delay.h>

/**
 * Size of the buffer used for storing characters
 */
#define BUFFER_SIZE        256
/**
 * Base address of the peripherals
 */
#define PERIPHERAL_BASE    0x3F000000UL
/**
 * Base address of the GPIO registers
 */
#define GPIO_BASE          (PERIPHERAL_BASE + 0x200000)
/**
 * Offset to the GPIO SET registers
 */
#define GPIO_SET_OFFSET    0x1c
/**
 * Offset to the GPIO CLEAR registers
 */
#define GPIO_CLR_OFFSET    0x28
/**
 * Offset to the GPIO LEVEL registers
 */
#define GPIO_LEV_OFFSET    0x34
/**
 * Offset to the GPIO PULL-UP/DOWN register
 */
#define GPIO_PUD_OFFSET    0x94
/**
 * Offset to the GPIO PULL-UP/DOWN clock register
 */
#define GPIO_PUDCLK_OFFSET 0x98
/**
 * Value for enabling the GPIO pull-up state
 */
#define GPIO_PUD_PULLUP    0x2
/**
 * Debounce time in milliseconds
 */
#define DEBOUNCE_TIME      200

/**
 * Pointer to GPIO control registers
 */
static int *gpio_ctr;
/**
 * Buffer for storing characters
 */
static char my_buf[BUFFER_SIZE];
/**
 * Pointer to the current position in the buffer
 */
static char *msg_ptr;
/**
 * Count of characters stored in the buffer*
 */
static int count = 0;

/**
 * Get the input value of a GPIO pin.
 *
 * @param gpio_ctr  Pointer to the GPIO controller.
 * @param gpio_nr   The GPIO number.
 * @param value     Pointer to store the input value (0 or 1).
 */
void get_gpio_input_value(void *gpio_ctr, int gpio_nr, int *value) {
    // Calculate register ID and position
    int reg_id = gpio_nr / 32;
    int pos = gpio_nr % 32;

    // Calculate the level register address
#define GPIO_LEV_OFFSET 0x34
    uint32_t *level_reg = (uint32_t * )(gpio_ctr + GPIO_LEV_OFFSET + 0x4 * reg_id);
    // Read the level register and extract the bit corresponding to the GPIO pin
    uint32_t level = *level_reg & (0x1 << pos);
    // Set the value based on the level
    *value = level ? 1 : 0;
}

/**
 * work_gpio12_callback - Callback function for the work queue associated with GPIO 12
 * @arg: Pointer to the work_struct object
 *
 * This function is the callback function for the work queue associated with GPIO 12.
 * It performs a debounce delay, reads the input value of GPIO 12, and updates the
 * my_buf array if the value is 0.
 */
void work_gpio12_callback(struct work_struct *arg) {
    int value;
    // Perform a debounce delay
    msleep(DEBOUNCE_TIME);
    // Read the input value of GPIO 12
    get_gpio_input_value(gpio_ctr, 12, &value);
    if (value == 0) {
        // Update my_buf array with 'r'
        my_buf[count++] = 'r';
        my_buf[count] = '\0';
    }
    pr_info("Key 'r' is pressed\n");
    return;
}

/**
 * work_gpio16_callback - Callback function for the work queue associated with GPIO 16
 * @arg: Pointer to the work_struct object
 *
 * This function is the callback function for the work queue associated with GPIO 16.
 * It performs a debounce delay, reads the input value of GPIO 16, and updates the
 * my_buf array if the value is 0.
 */
void work_gpio16_callback(struct work_struct *arg) {
    int value;
    // Perform a debounce delay
    msleep(DEBOUNCE_TIME);
    // Read the input value of GPIO 16
    get_gpio_input_value(gpio_ctr, 16, &value);
    if (value == 0) {
        // Update my_buf array with 'u'
        my_buf[count++] = 'u';
        my_buf[count] = '\0';
    }
    pr_info("Key 'u' is pressed\n");
    return;
}

/**
 * work_gpio20_callback - Callback function for the work queue associated with GPIO 20
 * @arg: Pointer to the work_struct object
 *
 * This function is the callback function for the work queue associated with GPIO 20.
 * It performs a debounce delay, reads the input value of GPIO 20, and updates the
 * my_buf array if the value is 0.
 */
void work_gpio20_callback(struct work_struct *arg) {
    int value;
    // Perform a debounce delay
    msleep(DEBOUNCE_TIME);
    // Read the input value of GPIO 20
    get_gpio_input_value(gpio_ctr, 20, &value);
    if (value == 0) {
        // Update my_buf array with 'l'
        my_buf[count++] = 'l';
        my_buf[count] = '\0';
    }
    pr_info("Key 'l' is pressed\n");
    return;
}

/**
 * work_gpio21_callback - Callback function for the work queue associated with GPIO 21
 * @arg: Pointer to the work_struct object
 *
 * This function is the callback function for the work queue associated with GPIO 21.
 * It performs a debounce delay, reads the input value of GPIO 21, and updates the
 * my_buf array if the value is 0.
 */
void work_gpio21_callback(struct work_struct *arg) {
    int value;
    // Perform a debounce delay
    msleep(DEBOUNCE_TIME);
    // Read the input value of GPIO 21
    get_gpio_input_value(gpio_ctr, 21, &value);
    if (value == 0) {
        // Update my_buf array with 'd'
        my_buf[count++] = 'd';
        my_buf[count] = '\0';
    }
    pr_info("Key 'd' is pressed\n");
    return;
}

/**
 * work_gpio26_callback - Callback function for the work queue associated with GPIO 26
 * @arg: Pointer to the work_struct object
 *
 * This function is the callback function for the work queue associated with GPIO 26.
 * It performs a debounce delay, reads the input value of GPIO 26, and updates the
 * my_buf array if the value is 0.
 */
void work_gpio26_callback(struct work_struct *arg) {
    int value;
    // Perform a debounce delay
    msleep(DEBOUNCE_TIME);
    // Read the input value of GPIO 21
    get_gpio_input_value(gpio_ctr, 26, &value);
    if (value == 0) {
        // Update my_buf array with 'f'
        my_buf[count++] = 'f';
        my_buf[count] = '\0';
    }
    pr_info("Key 'f' is pressed\n");
    return;
}

/**
 * @brief Declaration of works with their callback functions.
 *        These works are used for asynchronous processing.
 *
 * The DECLARE_WORK macro is typically used in an asynchronous programming context,
 * where works are units of work that can be scheduled and executed independently of the main program flow.
 * Each work is associated with a specific callback function that will be invoked when the work is scheduled to run.
 *
 * They are declared using the DECLARE_WORK macro, followed by the name of the work
 * and the name of the corresponding callback function.
 */
DECLARE_WORK(work_gpio12, work_gpio12_callback);
DECLARE_WORK(work_gpio16, work_gpio16_callback);
DECLARE_WORK(work_gpio20, work_gpio20_callback);
DECLARE_WORK(work_gpio21, work_gpio21_callback);
DECLARE_WORK(work_gpio26, work_gpio26_callback);

/**
 * gpio12_irq - Interrupt handler for GPIO 12
 * @irq: Interrupt number
 * @dev_id: Device ID (work queue pointer)
 *
 * This function is the interrupt handler for GPIO 12. It schedules the
 * work queue associated with GPIO 12 to be executed.
 *
 * Returns: IRQ_HANDLED indicating the interrupt was handled successfully.
 */
static irqreturn_t gpio12_irq(int irq, void *dev_id) {
    // Schedule the work queue associated with GPIO 12
    schedule_work(&work_gpio12);
    // Indicate that the interrupt was handled
    return IRQ_HANDLED;
}

/**
 * gpio16_irq - Interrupt handler for GPIO 16
 * @irq: Interrupt number
 * @dev_id: Device ID (work queue pointer)
 *
 * This function is the interrupt handler for GPIO 16. It schedules the
 * work queue associated with GPIO 16 to be executed.
 *
 * Returns: IRQ_HANDLED indicating the interrupt was handled successfully.
 */
static irqreturn_t gpio16_irq(int irq, void *dev_id) {
    // Schedule the work queue associated with GPIO 16
    schedule_work(&work_gpio16);
    // Indicate that the interrupt was handled
    return IRQ_HANDLED;
}

/**
 * gpio20_irq - Interrupt handler for GPIO 20
 * @irq: Interrupt number
 * @dev_id: Device ID (work queue pointer)
 *
 * This function is the interrupt handler for GPIO 20. It schedules the
 * work queue associated with GPIO 20 to be executed.
 *
 * Returns: IRQ_HANDLED indicating the interrupt was handled successfully.
 */
static irqreturn_t gpio20_irq(int irq, void *dev_id) {
    // Schedule the work queue associated with GPIO 20
    schedule_work(&work_gpio20);
    // Indicate that the interrupt was handled
    return IRQ_HANDLED;
}

/**
 * gpio21_irq - Interrupt handler for GPIO 21
 * @irq: Interrupt number
 * @dev_id: Device ID (work queue pointer)
 *
 * This function is the interrupt handler for GPIO 21. It schedules the
 * work queue associated with GPIO 21 to be executed.
 *
 * Returns: IRQ_HANDLED indicating the interrupt was handled successfully.
 */
static irqreturn_t gpio21_irq(int irq, void *dev_id) {
    // Schedule the work queue associated with GPIO 21
    schedule_work(&work_gpio21);
    // Indicate that the interrupt was handled
    return IRQ_HANDLED;
}

/**
 * gpio26_irq - Interrupt handler for GPIO 26
 * @irq: Interrupt number
 * @dev_id: Device ID (work queue pointer)
 *
 * This function is the interrupt handler for GPIO 26. It schedules the
 * work queue associated with GPIO 26 to be executed.
 *
 * Returns: IRQ_HANDLED indicating the interrupt was handled successfully.
 */
static irqreturn_t gpio26_irq(int irq, void *dev_id) {
    // Schedule the work queue associated with GPIO 26
    schedule_work(&work_gpio26);
    // Indicate that the interrupt was handled
    return IRQ_HANDLED;
}

/**
 * set_gpio_pullup - Set pull-up resistor for a GPIO pin
 * @gpio_ctr: Pointer to the GPIO control register base address
 * @gpio_nr: GPIO pin number
 *
 * This function sets the pull-up resistor for the specified GPIO pin.
 * It uses the GPIO control register base address to access the pull-up
 * resistor control registers and sets the corresponding pull-up state
 * for the GPIO pin.
 */

void set_gpio_pullup(void *gpio_ctr, int gpio_nr) {
    // Calculate the register ID based on GPIO number
    int reg_id = gpio_nr / 32;
    // Calculate the position within the register
    int pos = gpio_nr % 32;

    // Pointer to the Pull-up control register
    uint32_t *pud_reg = (uint32_t * )(gpio_ctr + GPIO_PUD_OFFSET);
    // Pointer to the Pull-up clock control register
    uint32_t *pudclk_reg = (uint32_t * )(gpio_ctr + GPIO_PUDCLK_OFFSET + 0x4 * reg_id);

    // Set the pull-up state
    *pud_reg = GPIO_PUD_PULLUP;
    // Wait for a short delay
    udelay(1);
    // Assert the clock signal for the GPIO pin
    *pudclk_reg = (0x1 << pos);
    // Wait for a short delay
    udelay(1);
    // Clear the pull-up state
    *pud_reg = 0;
    // Clear the clock signal for the GPIO pin
    *pudclk_reg = 0;
}

/**
 * rpikey_open - Open the rpikey device file
 * @inode: Pointer to the inode structure
 * @file: Pointer to the file structure
 *
 * This function is called when the rpikey device file is opened.
 * It initializes the message pointer to the internal buffer.
 *
 * Returns 0 on success.
 */
static int rpikey_open(struct inode *inode, struct file *file) {
    msg_ptr = my_buf;
    return 0;
}

/**
 * rpikey_release - Release the rpikey device file
 * @inode: Pointer to the inode structure
 * @file: Pointer to the file structure
 *
 * This function is called when the rpikey device file is closed.
 *
 * Returns 0 on success.
 */
static int rpikey_release(struct inode *inode, struct file *file) {
    return 0;
}

/**
 * rpikey_read - Read data from the rpikey device
 * @filp: Pointer to the file structure
 * @buf: User-space buffer to store the read data
 * @size: Size of the buffer
 * @off: Pointer to the file offset
 *
 * This function is called when a read operation is performed on the rpikey device file.
 * It reads data from the internal buffer and copies it to the user-space buffer.
 *
 * Returns the number of bytes read on success, or a negative error code on failure.
 */
static ssize_t rpikey_read(struct file *filp, char __user *buf, size_t size, loff_t *off) {
    // Counter for the number of bytes read
    int bytes_read = 0;
    // Set msg_ptr to the beginning of the message buffer
    msg_ptr = my_buf;
    // Reset the count to zero for the next read operation
    count = 0;

    pr_info("The read callback is called\n");

    if (*msg_ptr == 0) {
        // Return 0 to indicate that there is no more data to read
        return 0;
    }

    // Iterate until either size or msg_ptr is exhausted
    while (size && *msg_ptr) {
        // Copy a single character from msg_ptr to the user space buffer buf
        put_user(*(msg_ptr), buf);
        // Set the current character in msg_ptr to null character
        *msg_ptr = '\0';
        // Move msg_ptr to the next character
        msg_ptr++;
        // Move user buf pointer to the next position
        buf++;
        // Decrement size to indicate the number of remaining bytes
        size--;
        // Increment the bytes_read counter
        bytes_read++;
    }

    // Return the number of bytes read to the caller
    return bytes_read;
}

/**
 * The struct file_operations structure is used to define file operations for a character device.
 * In this case, the key_fops structure is being initialized with function pointers
 * corresponding to different file operations.
 */
struct file_operations key_fops = {
        // Function to be called when the device is opened
        .open = rpikey_open,
        // Function to be called when data is read from the device
        .read = rpikey_read,
        // Function to be called when the device is released/closed
        .release = rpikey_release,
};

/**
 * The major number assigned to the character device
 * 0 major number will dynamically allocate an actual major number
 */
#define MAJOR_NUM 0
/**
 *  The name of the device node
 */
#define DEVICE_NAME "rpikey"
/**
 *  The name of the device class
 */
#define CLASS_NAME "rpikey_class"
/**
 * The base address of the peripheral registers
 */
#define PERIPHERAL_BASE 0x3F000000UL
/**
 * The base address of the GPIO registers
 */
#define GPIO_BASE (PERIPHERAL_BASE + 0x200000)

/*
 * A device class represents a group of devices that share common characteristics or functionality.
 * It provides a way to organize and manage devices of the same type.
 */
static struct class *rpikey_class = NULL;

/*
 * A device structure represents an instance of a device within the device class.
 * It represents a specific device node in the system, associated with the module.
 * The device node provides an interface through which user space programs can interact with the module.
 */
static struct device *rpikey_device = NULL;

/*
 * The majorNum variable is used to store the major number assigned to the character device
 * when it is registered with the kernel. The major number is a unique identifier for the device driver
 * and is used to associate the driver with the corresponding device nodes in the system.
 */
static int majorNum;

/**
 * rpikey_init - Initialize the rpikey module
 *
 * This function is called when the rpikey module is loaded. It initializes
 * the necessary resources and sets up GPIO pins and interrupts for key input.
 *
 * Return: 0 on success, or an error code on failure.
 */
static int __init rpikey_init(void) {
    int ret;
    // Register a character device
    majorNum = register_chrdev(MAJOR_NUM, DEVICE_NAME, &key_fops);
    if (majorNum < 0) {
        pr_alert("The rpikey failed to register a major number\n");
        return majorNum;
    }

    // Create a device class and device
    rpikey_class = class_create(THIS_MODULE, CLASS_NAME);
    rpikey_device = device_create(rpikey_class, NULL, MKDEV(majorNum, 0), NULL, DEVICE_NAME);

    // Request GPIO pins
    gpio_request(12, "gpio12");
    gpio_request(13, "gpio13");
    gpio_request(20, "gpio20");
    gpio_request(21, "gpio21");
    gpio_request(26, "gpio26");

    // Set GPIO pins as input
    gpio_direction_input(12);
    gpio_direction_input(16);
    gpio_direction_input(20);
    gpio_direction_input(21);
    gpio_direction_input(26);

    // Remap GPIO peripheral base address
    gpio_ctr = ioremap(GPIO_BASE, 0x1000);

    // Set GPIO pins pull-up
    set_gpio_pullup(gpio_ctr, 12);
    set_gpio_pullup(gpio_ctr, 16);
    set_gpio_pullup(gpio_ctr, 20);
    set_gpio_pullup(gpio_ctr, 21);
    set_gpio_pullup(gpio_ctr, 26);

    // Request interrupts for GPIO pins
    ret = request_irq(gpio_to_irq(12), gpio12_irq, IRQF_TRIGGER_FALLING, "rpikey", &work_gpio12);
    if (ret < 0) {
        pr_alert("%s: request_irq failed with %d\n", __func__, ret);
    }
    ret = request_irq(gpio_to_irq(16), gpio16_irq, IRQF_TRIGGER_FALLING, "rpikey", &work_gpio16);
    if (ret < 0) {
        pr_alert("%s: request_irq failed with %d\n", __func__, ret);
    }
    ret = request_irq(gpio_to_irq(20), gpio20_irq, IRQF_TRIGGER_FALLING, "rpikey", &work_gpio20);
    if (ret < 0) {
        pr_alert("%s: request_irq failed with %d\n", __func__, ret);
    }
    ret = request_irq(gpio_to_irq(21), gpio21_irq, IRQF_TRIGGER_FALLING, "rpikey", &work_gpio21);
    if (ret < 0) {
        pr_alert("%s: request_irq failed with %d\n", __func__, ret);
    }
    ret = request_irq(gpio_to_irq(26), gpio26_irq, IRQF_TRIGGER_FALLING, "rpikey", &work_gpio26);
    if (ret < 0) {
        pr_alert("%s: request_irq failed with %d\n", __func__, ret);
    }

    return 0;
}

/**
 * rpikey_exit - Clean up and exit the rpikey module
 *
 * This function is called when the rpikey module is unloaded. It releases
 * the allocated resources, frees GPIO pins, destroys the device and class,
 * unregisters the character device, and frees the requested interrupts.
 */
static void __exit rpikey_exit(void) {
    // Unmap GPIO peripheral base address
    iounmap(gpio_ctr);

    // Free GPIO pins
    gpio_free(12);
    gpio_free(16);
    gpio_free(20);
    gpio_free(21);
    gpio_free(26);

    // Destroy device and class
    device_destroy(rpikey_class, MKDEV(majorNum, 0));
    class_unregister(rpikey_class);
    class_destroy(rpikey_class);

    // Unregister character device
    unregister_chrdev(majorNum, DEVICE_NAME);

    // Free interrupts for GPIO pins
    free_irq(gpio_to_irq(12), &work_gpio12);
    free_irq(gpio_to_irq(16), &work_gpio16);
    free_irq(gpio_to_irq(20), &work_gpio20);
    free_irq(gpio_to_irq(21), &work_gpio21);
    free_irq(gpio_to_irq(26), &work_gpio26);
}

// Specifies the function to be called when the module is loaded
module_init(rpikey_init);
// Specifies the function to be called when the module is unloaded
module_exit(rpikey_exit);
// Specifies the license for the module
MODULE_LICENSE("GPL");
