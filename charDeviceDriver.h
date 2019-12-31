#define DEVICE_NAME "opsysmem" // The device will appear at /dev/opsysmem using this value
#define CLASS_NAME "c"         // The device class -- this is a character device driver

#define MAX_MESSAGE_SIZE 4 * 1024
#define SLEEP 0
#define DEBUG 0

// --- Data structures ---
typedef struct Node
{
    unsigned int length;
    char *string;
    struct Node *next;
} Node;

typedef struct list
{
    struct Node *node;
} list;

static struct semaphore sem;                         // Global semaphore
static int majorNumber;                              // Stores the device number -- determined automatically
struct class *opsysmemClass = NULL;                  // Pointer to device class
static list l;                                       // Global list, protected by semaphore
static int current_size = 0;                         // Global list size
static unsigned int MAX_LIST_SIZE = 2 * 1024 * 1024; // Max list size

// --- Device functions ---
static int dev_open(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
static long dev_ioctl(struct file *file,
                      unsigned int ioctl_num,
                      unsigned long ioctl_param);
static int dev_release(struct inode *, struct file *);

// --- List functions ---
static Node *pop(list *l, unsigned int index);
static int push(list *l, Node *n);
static int remove_element(list *l, unsigned int index);
static void init(list *l);
static void destroy2(list *l);

// --- fops struct --
static struct file_operations fops = {
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .unlocked_ioctl = dev_ioctl,
    .release = dev_release,
};
