#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/list.h>
#include <asm/uaccess.h>

#define BUFFER_SIZE 256*128
#define PROC_NAME "pid"
#define MAX_PIDS 128

static int pid_values[MAX_PIDS];
static int pid_count = 0;
static char state = '\0';

static const char * const task_state_array[] = {
    "R (running)",
    "S (sleeping)",
    "D (disk sleep)",
    "T (stopped)",
    "t (tracing stop)",
    "X (dead)",
    "Z (zombie)",
    "P (parked)",
    "I (idle)"
};

static ssize_t pid_read(struct file *file, char __user *usr_buf, size_t count, loff_t *pos);
static ssize_t pid_write(struct file *file, const char __user *usr_buf, size_t count, loff_t *pos);

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
    struct task_struct *task = NULL;
    struct pid *pid_struct;
    char *buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    int len = 0;
    int i;

    if (!buffer) {
        pr_err("Failed to allocate buffer memory\n");
        return -ENOMEM;
    }

    if (*pos > 0) {
        kfree(buffer);
        return 0; // Neu da duoc doc thi khong doc nua
    }

    len += snprintf(buffer + len, BUFFER_SIZE - len, "+------------------+--------------------------------+-------------------+\n");
    len += snprintf(buffer + len, BUFFER_SIZE - len, "| PID              | Command                        | State             |\n");
    len += snprintf(buffer + len, BUFFER_SIZE - len, "+------------------+--------------------------------+-------------------+\n");

    if (pid_count > 0){
        for (i = 0; i < pid_count; i++){
            const char *state_description;
            if (len >= BUFFER_SIZE) break; // Ngan tran bo dem

            pid_struct = find_vpid(pid_values[i]);
            if (!pid_struct) {
                len += snprintf(buffer + len, BUFFER_SIZE - len, "| %-16d | %-30s | %-17s |\n", pid_values[i], "Invalid PID", "N/A");
                len += snprintf(buffer + len, BUFFER_SIZE - len, "+------------------+--------------------------------+-------------------+\n");
                continue;
            }

            task = pid_task(pid_struct, PIDTYPE_PID);
            if (!task) {
                len += snprintf(buffer + len, BUFFER_SIZE - len, "| %-16d | %-30s | %-17s |\n", pid_values[i], "No task struct found", "N/A");
                len += snprintf(buffer + len, BUFFER_SIZE - len, "+------------------+--------------------------------+-------------------+\n");
                continue;
            }

            state_description = task_state_array[task_state_index(task)];

            len += snprintf(buffer + len, BUFFER_SIZE - len, "| %-16d | %-30s | %-17s |\n", task->pid, task->comm, state_description);
            len += snprintf(buffer + len, BUFFER_SIZE - len, "+------------------+--------------------------------+-------------------+\n");
        }
    } else {
        for_each_process(task) {
            const char *state_description;
            if (len >= BUFFER_SIZE) break; // Ngan tran bo dem

            if (state != '\0' && task_state_to_char(task) != state) {
                continue; // Khong trung trang thai thi bo qua
            }

            state_description = task_state_array[task_state_index(task)];

            len += snprintf(buffer + len, BUFFER_SIZE - len, "| %-16d | %-30s | %-17s |\n",
                            task->pid, task->comm, state_description);
            len += snprintf(buffer + len, BUFFER_SIZE - len, "+------------------+--------------------------------+-------------------+\n");
        }
    }

    if (copy_to_user(usr_buf, buffer, len)) {
        pr_err("Failed to copy data to user space\n");
        kfree(buffer);
        return -EFAULT; // Loi khong the copy tu kernel sang buffer
    }

    kfree(buffer);
    *pos = len; // Cap nhat vi tri da doc
    return len; // Tra ve so byte da doc
}

static ssize_t pid_write(struct file *file, const char __user *usr_buf, size_t count, loff_t *pos)
{
    char *k_mem;
    char *token;
    int pid;
    int i = 0;

    k_mem = kmalloc(count + 1, GFP_KERNEL); // cap phat vung nho
    if (!k_mem){
        printk (KERN_ERR "Out of memory\n"); // Khong du bo nho de cap phat
        return -ENOMEM; // Tra ve ma loi Out of memory
    }

    if(copy_from_user(k_mem, usr_buf, count)){ // sao chep du lieu tu buffer cua user vao kmem
        kfree(k_mem);
        printk (KERN_ERR "Bad address\n");
        return -EFAULT; // loi bad address
    }

    k_mem[count] = '\0'; //Danh dau ket thuc chuoi de su dung strsep
    pid_count = 0;

    if (strchr("RSDTItXZP", k_mem[0])) {
        state = k_mem[0];
        pr_info("State filter set to: %c\n", state);
    } else {
        token = strsep(&k_mem, " ,");  //Phan cach bang space hoac ,
        while (token != NULL && i < MAX_PIDS) {
            if (kstrtoint(token, 10, &pid) == 0) pid_values[pid_count++] = pid;
            token = strsep(&k_mem, " ,");
        }
    }

    kfree(k_mem);
    return count; // so bytes da ghi
}

module_init(pid_start);
module_exit(pid_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group 05: Kernel Module for Process Management");
MODULE_DESCRIPTION("A basic module for handling and managing PID details.");
