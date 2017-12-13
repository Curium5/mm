#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/slab.h> /* kmalloc */
#include <linux/mm.h>  /* mmap related stuff */

char *mem_ptr;
struct dentry  *file;
struct mmap_info {
  char *data;     /* データ */
  int vmas;       /* mmap回数 */
};

/* mmapされた回数を追跡 */
void mmap_open(struct vm_area_struct *vma)
{
  //struct mmap_info *info = vma->vm_private_data;
  //info->vmas++;
}

void mmap_close(struct vm_area_struct *vma)
{
  struct mmap_info *info = vma->vm_private_data;
  info->vmas--;
}

/* faultはメモリにアクセスされていないメモリ領域に初めてアクセスされたときに呼び出され、
 * カーネルとユーザー空間のメモリ間のマッピングを行う */
static int mmap_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
  //struct mmap_info *info = vma->vm_private_data;
  struct page *page;
  void *pageptr = NULL;

  /* アドレスが有効か? */
  if ((vmf->virtual_address) > vma->vm_end) {
    printk("invalid address\n");
    printk("address = %lu\n", vmf->virtual_address);
    return VM_FAULT_SIGBUS;
  }
  pageptr = mem_ptr;
  if (!pageptr) {
    printk("no data\n");
    return 0;
  }
  /* 引数として共有されるメモリ領域をとり、
   * このメモリ領域にアクセスするためにユーザ空間で使用できるstruct page *を返す */
  page = virt_to_page(pageptr);
  /* ページの参照カウントをインクリメントする */
  get_page(page);
  vmf->page = page;
  return 0;
}

struct vm_operations_struct mmap_vm_ops = {
  .open =     mmap_open,
  //.close =    mmap_close,
  .fault =    mmap_fault,
};

int my_mmap(struct file *filp, struct vm_area_struct *vma)
{
  vma->vm_ops = &mmap_vm_ops;
  vma->vm_flags |= (VM_DONTEXPAND | VM_DONTDUMP);
  vma->vm_private_data = mem_ptr;
  mmap_open(vma);
  return 0;
}

int my_close(struct inode *inode, struct file *filp)
{
  //struct mmap_info *info = filp->private_data;
  //free_page((unsigned long)info->data);
  //kfree(mem_ptr);
  //filp->private_data = NULL;
  return 0;
}

int my_open (struct inode *inode, struct file *filp)
{
  struct mmap_info *info; /* mmaped areaに関する情報 */
  if (!inode->i_private) {
    //info = kmalloc(sizeof(struct mmap_info), GFP_KERNEL);
    /* 新しいメモリの取得 */
    info->data = (char *)get_zeroed_page(GFP_KERNEL);
    //memset(info->data, 0, PAGE_SIZE);
    //memcpy(info->data, "hello from kernel", 18);
    inode->i_private = info;
  }
  /* all further references by mmap, read, write goes with this file pointer
   * mmap、read、writeによるこれ以上のすべての参照は、このファイルポインタとともに行われる */
  filp->private_data = inode->i_private;
  return 0;
}

static const struct file_operations my_fops = {
  //.open = my_open,
  .release = my_close,
  .mmap = my_mmap,
};

int my_trans_data(size_t len) {
  memcpy(mem_ptr, len, sizeof(size_t));
  return 0;
}

static int __init mmapfile_module_init(void)
{
  printk("Init mmap_file\n");
  file = debugfs_create_file("mmap_file", 0644, NULL, NULL, &my_fops);

  mem_ptr = kmalloc(PAGE_SIZE, GFP_KERNEL);
  memset(mem_ptr, 0, PAGE_SIZE);
  //memcpy(mem_ptr, "hello from kernel", 18);
  return 0;
}

static void __exit mmapfile_module_exit(void)
{
  debugfs_remove(file);
  kfree(mem_ptr);
}

module_init(mmapfile_module_init);
module_exit(mmapfile_module_exit);
MODULE_LICENSE("GPL");
