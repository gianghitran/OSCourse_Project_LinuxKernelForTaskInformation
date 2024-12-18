#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/list.h>
#include <asm/uaccess.h>

#define BUFFER_SIZE 256
#define PROC_NAME "pid"
// #define PROC_COUNT "pid_count"

static int pid_value = -1;

static ssize_t pid_read(struct file *file, char *buf, size_t count, loff_t *pos);
static ssize_t pid_write(struct file *file, const char __user *usr_buf, size_t count, loff_t *pos);
//static ssize_t proc_count_read(struct file *file, char *buf, size_t count, loff_t *pos);

static struct proc_ops proc_file_fops =
{
    .proc_read = pid_read,
    .proc_write = pid_write,
};

static int __init pid_start(void)
{
    if (proc_create(PROC_NAME, 0666, NULL, &proc_file_fops) == NULL) {
        pr_err("Error: Could not initialize /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }
    pr_info("/proc/%s created successfully\n", PROC_NAME);
    return 0;
}

static void __exit pid_exit(void)
{
    remove_proc_entry(PROC_NAME, NULL);
    pr_info("/proc/%s removed successfully\n", PROC_NAME);
}

static ssize_t pid_read(struct file *file, char __user *usr_buf, size_t count, loff_t *pos)
{
    /*
    Truyen vao user_buff, kich thuoc vung nho,...
    Doc thong tin tien trinh. Tra ve kich thuoc vung nho
    */
    struct task_struct *task = NULL;
    struct pid *pid_struct;
    char buffer[BUFFER_SIZE];
    int len = 0;

    if (*pos > 0 || pid_value == -1)
        return 0; // pid_value khong hop le

    // Tim pid_value
    pid_struct = find_vpid(pid_value);
    if (!pid_struct) {
        pr_err("Invalid PID: %d\n", pid_value);
        return 0; // Khong tim thay PID
    }

    task = pid_task(pid_struct, PIDTYPE_PID);
    if (!task) {
        pr_err("No task struct found for PID: %d\n", pid_value);
        return 0; // Khong tim thay task_struct cua PID
    }

    // Lay thong tin tu task_struct va luu vao buffer
    len = snprintf(buffer, BUFFER_SIZE, "Command: %s\nPID: %d\nState: %c\n", task->comm, task->pid, task_state_to_char(task));

    if (copy_to_user(usr_buf, buffer, len)) {
        pr_err("Failed to copy data to user space\n");
        return -EFAULT; // Loi khong the copy tu kernel sang buffer
    }

    *pos = len; // Cap nhat vi tri da doc
    return len; // Tra ve so byte da doc
}

static ssize_t pid_write(struct file *file, const char __user *usr_buf, size_t count, loff_t *pos)
{
   /*
    Truyen vao user_buff, kich thuoc vung nho,...
    Tra ve kich thuoc vung nho. In ra thong tin tien trinh
    */
    char * k_mem;

    k_mem = kmalloc(count, GFP_KERNEL); // cấp phát vùng nhớ với kích thước count

    if (!k_mem){
        printk (KERN_ERR "Out of memory\n"); // Khong du bo nho de cap phat
        return -ENOMEM; // Tra ve ma loi Out of memory
    }
    if(copy_from_user(k_mem, usr_buf, count)){ // sao chép dữ liệu từ buffer của user vào kmem
        kfree(k_mem);
        // copy_from_user trả về giá trị khác 0 (tức là có lỗi trong quá trình sao chép)
        printk (KERN_ERR "Bad address\n");
        return -EFAULT; // loi bad address
    }

    //cout: so bytes muon ghi vao tep

    printk (KERN_INFO "%s\n", k_mem);
    sscanf(k_mem, "%d", &pid_value); // Lưu giá trị PID vào pid_value
    kfree(k_mem);
    return count; // so bytes da ghi
}

module_init(pid_start);
module_exit(pid_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group 05: Kernel Module for Process Management");
MODULE_DESCRIPTION("A basic module for handling and managing PID details.");
