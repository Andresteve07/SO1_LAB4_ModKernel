#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/semaphore.h> //uso de semáforos para solucionar problemas de concurrencia, esto es, varios procesos intentan usar el mismo dispositivo al mismo timepo. Sincronización. 
#include <asm/uaccess.h> //Copia al espacio de usario y desde el espacio de usuario.
#include <linux/string.h>

#define MAX 100

static dev_t first; // Global variable for the first device number
static struct cdev c_dev; // Global variable for the character device structure
static struct class *cl; // Global variable for the device class
//static char c;

struct fake_device{
	char data[MAX];
	struct semaphore sem;
} virtual_device;

int ret;//se va a usar para guardar los valores de retorno de las funciones, esto es porque el stack del kernel es muy pequeño, por lo tanto declarar variables por todos lados en las funciones de nuestro modulo se come el stack muy rápido y genera problemas.
#define DEVICE_NAME	"mod_enc"//nombre--->aparece en /proc/devices


static int my_open(struct inode *i, struct file *f)
{
	/*int index;	
	index=0;
	while(index<MAX){
		virtual_device.data[index]='a';
		index++;
	}*/
	printk(KERN_INFO "encriptador_code: open()\n");
	if(down_interruptible(&virtual_device.sem)!=0){
	printk(KERN_ALERT "encriptador_code: no se pudo acceser al dispositivo durante la apertura.\n");
	return -1;
	}
	printk(KERN_ALERT "encriptador_code: Dispositivo abierto.\n");
	return 0;
}
static int my_close(struct inode *i, struct file *f)
{
	up(&virtual_device.sem);
	printk(KERN_INFO "encriptador_code: archivo cerrado.\n");
	return 0;
}

static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
	ssize_t bytes;
	printk(KERN_INFO "encriptador_code: read()\n");
	//ret=copy_to_user(buf, virtual_device.data, len);	

	bytes = len < (MAX-(*off)) ? len : (MAX-(*off));
    if(copy_to_user(buf, virtual_device.data, bytes)){
        return -EFAULT;
    }
    (*off) += bytes;
    return bytes;
}

static ssize_t my_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
	size_t index;
	printk(KERN_INFO "encriptador_code: write()\n");
	if(len > MAX) // se agrega esta sentencia para evitar la advertencia de overflow al compilar
		len = MAX;
	if (copy_from_user(virtual_device.data, buf, len)) {
 		return -EFAULT;
 	}
 	*off += len;
	printk(KERN_INFO "encriptador_code: se escribio:%s.\n",virtual_device.data);
	printk(KERN_INFO "encriptador_code: se actualizo solo: len=%i??\n",len);
	
	index=0;
	while(index<len){
		virtual_device.data[index]=virtual_device.data[index]+1;//Copia el la información encriptada
		index++;
	}
	
	while(index<MAX){
		virtual_device.data[index]='\0';
		index++;
	}
 	return len;
		
}
static struct file_operations pugs_fops =
{
	.owner = THIS_MODULE,
	.open = my_open,
	.release = my_close,
	.read = my_read,
	.write = my_write
};

static int __init ofcd_init(void) /* Constructor */
{
	struct device *dev_ret;

	printk(KERN_INFO "encriptador_code: ofcd registrado.");
	if ((ret = alloc_chrdev_region(&first, 0, 1, DEVICE NAME)) < 0)
	{
		return ret;
	}
	if (IS_ERR(cl = class_create(THIS_MODULE, "encriptador")))
	{
		unregister_chrdev_region(first, 1);
		return PTR_ERR(cl);
	}
	if (IS_ERR(dev_ret = device_create(cl, NULL, first, NULL, DEVICE_NAME)))
	{
		class_destroy(cl);
		unregister_chrdev_region(first, 1);
		return PTR_ERR(dev_ret);
	}

	cdev_init(&c_dev, &pugs_fops);
	if ((ret = cdev_add(&c_dev, first, 1)) < 0)
	{
		device_destroy(cl, first);
		class_destroy(cl);
		unregister_chrdev_region(first, 1);
		return ret;
	}
	sema_init(&virtual_device.sem, 1); //semáforo binario.
	return 0;
}

static void __exit ofcd_exit(void) /* Destructor */
{
	cdev_del(&c_dev);
	device_destroy(cl, first);
	class_destroy(cl);
	unregister_chrdev_region(first, 1);
	printk(KERN_INFO "encriptador_code: modulo descargado.");
}

module_init(ofcd_init);
module_exit(ofcd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Esteban Andrés Morales <andresteve07@gmail.com>");
MODULE_DESCRIPTION("TPNro4");
