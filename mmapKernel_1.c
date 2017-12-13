#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/slab.h> /* kmalloc */
#include <linux/mm.h>  /* mmap related stuff */
#include <linux/string.h>

char *map_data;
struct dentry  *file;
struct mmap_info {
  char *data;     /* データ */
  int reference;       /* mmap回数 */
};

/* mmapされた回数を追跡 */
void mmap_open(struct vm_area_struct *vma)
{
  //struct mmap_info *info = vma->vm_private_data;
  //info->reference++;
  //struct mmap_info *info = (struct mmap_info *)vma->vm_private_data;
  //info->reference++;
}

void mmap_close(struct vm_area_struct *vma)
{
  //struct mmap_info *info = vma->vm_private_data;
  //info->reference--;
  struct mmap_info *info = (struct mmap_info *)vma->vm_private_data;
  info->reference--;
}

/* faultはメモリにアクセスされていないメモリ領域に初めてアクセスされたときに呼び出され、
 * カーネルとユーザー空間のメモリ間のマッピングを行う */
static int mmap_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
  //struct mmap_info *info = vma->vm_private_data;
  struct page *page;
  void *pageptr = NULL;

  printk("vmf->virtual_address : %lu\n", vmf->virtual_address);
  printk("vmf->virtual_address : %s\n", vmf->virtual_address);
  
  if ((vmf->virtual_address) > vma->vm_end) {
    printk("invalid address\n");
    printk("address = %lu\n", vmf->virtual_address);
    return VM_FAULT_SIGBUS;
  }
  pageptr = map_data;
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
  vma->vm_private_data = map_data;
  mmap_open(vma);
  return 0;
}

int my_close(struct inode *inode, struct file *filp)
{
  //struct mmap_info *info = filp->private_data;
  //free_page((unsigned long)info->data);
  //kfree(map_data);
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
  int ext;
  //for(  ; sizeof(size_t) < PAGE_SIZE ; sizeof(size_t) +=  sizeof(size_t)){
  memcpy(map_data + ext, len, sizeof(size_t));

  //}

  return 0;
}

static int __init mmapfile_module_init(void)
{
  int i;
  char msg[20] = "hello from kernel 1";
  msg[19] = '\n';
 
  printk("----------Init mmapKernel_1 module----------\n");
  file = debugfs_create_file("mmap_file", 0644, NULL, NULL, &my_fops);


  map_data = kmalloc(PAGE_SIZE, GFP_KERNEL);
  memset(map_data, 0, PAGE_SIZE);
  //for(i = 20 ; i < PAGE_SIZE ; i+=20 )
  memcpy(map_data, msg, 20);
  printk("memcpy ok\n");
  memcpy(map_data+20, "hello from kernel 2", 20);
  printk("size_t %d\n", strlen(msg));
  printk("address : %lu\n", map_data);
  printk("address : %s\n", map_data);
  return 0;
}

static void __exit mmapfile_module_exit(void)
{
  debugfs_remove(file);
  kfree(map_data);
}

module_init(mmapfile_module_init);
module_exit(mmapfile_module_exit);
MODULE_LICENSE("GPL");
