/* drivers/android/vmem.c
 *
 * Copyright (C) 2007 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/* TODO:
 * 1. modify get_phy_addr ioctl to support multiple discontinguous physical page resolving
 * 2. set owner id
 */

#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/list.h>
#include <linux/debugfs.h>
#include <linux/android_vmem.h>
#include <linux/mempolicy.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>

#define VMEM_MAX_DEVICES 10
#define VMEM_MAX_ORDER 128
#define VMEM_MIN_ALLOC PAGE_SIZE

#define VMEM_DEBUG 1

#define VMEM_TYPE_KMALLOC 0
#define VMEM_TYPE_VMALLOC 1

/* indicates that a refernce to this file has been taken via get_vmem_file,
 * the file should not be released until put_vmem_file is called */
#define VMEM_FLAGS_BUSY 0x1
/* indicates that this is a suballocation of a larger master range */
#define VMEM_FLAGS_CONNECTED 0x1 << 1
/* indicates this is a master and not a sub allocation and that it is mmaped */
#define VMEM_FLAGS_MASTERMAP 0x1 << 2
/* submap and unsubmap flags indicate:
 * 00: subregion has never been mmaped
 * 10: subregion has been mmaped, reference to the mm was taken
 * 11: subretion has ben released, refernece to the mm still held
 * 01: subretion has been released, reference to the mm has been released
 */
#define VMEM_FLAGS_SUBMAP 0x1 << 3
#define VMEM_FLAGS_UNSUBMAP 0x1 << 4

#define virt_to_pfn(vaddr) (virt_to_phys(vaddr) >> PAGE_SHIFT)

#define VMEM_PROF
#ifdef VMEM_PROF
#include <linux/seq_file.h>
#include <linux/threads.h>
#include <linux/proc_fs.h>
//define PROF_MAX_PID PID_MAX_DEFAULT;
#define PROF_MAX_PID 3000
//#define VMEM_PID(id, index) vmem[id].bitmap[index].pid
static int vmem_usage[PROF_MAX_PID];
static int vmem_usage_max[PROF_MAX_PID];
static int vmem_max_usage = 0;
#endif
static void vmem_dump(void);

/* vmem_data will be kept in each fd->private_data */
struct vmem_data {
	/* the allocated kernel vaddr */
	unsigned int vaddr;
	/* the allocated size */
	unsigned int size;
	/* the allocated type: kmalloc or vmalloc */
	unsigned int type;
	/* see flags above for descriptions */
	unsigned int flags;
	/* protects this data field, if the mm_mmap sem will be held at the
	 * same time as this sem, the mm sem must be taken first (as this is
	 * the order for vma_open and vma_close ops */
	struct rw_semaphore sem;
	/* info about the mmaping process */
	struct vm_area_struct *vma;
	/* task struct of the mapping process */
	struct task_struct *task;
	/* process id of teh mapping process */
	pid_t pid;
	/* file descriptor of the master */
	int master_fd;
	/* file struct of the master */
	struct file *master_file;
	/* a list of currently available regions if this is a suballocation */
	struct list_head region_list;
	/* a linked list of data so we can access them for debugging */
	struct list_head list;
#if VMEM_DEBUG
	int ref;
#endif
};

struct vmem_bits {
	unsigned allocated:1;		/* 1 if allocated, 0 if free */
	unsigned order:7;		/* size of the region in vmem space */
#ifdef VMEM_PROF
	long pid;	/*vmem onwer*/
	long tgid;	/*vmem onwer*/
#endif
};

struct vmem_region_node {
	struct vmem_region region;
	struct list_head list;
};

#define VMEM_DEBUG_MSGS 1
#if VMEM_DEBUG_MSGS
#define DLOG(fmt,args...) \
	do { printk(KERN_INFO "[%s:%s:%d] "fmt, __FILE__, __func__, __LINE__, \
		    ##args); } \
	while (0)
#else
#define DLOG(x...) do {} while (0)
#endif

struct vmem_info {
	struct miscdevice dev;
	/* physical start address of the remaped vmem space */
	unsigned long base;
	/* vitual start address of the remaped vmem space */
	unsigned char __iomem *vbase;
	/* total size of the vmem space */
	unsigned long size;
	/* number of entries in the vmem space */
	unsigned long num_entries;
	/* pfn of the garbage page in memory */
	unsigned long garbage_pfn;
	/* index of the garbage page in the vmem space */
	int garbage_index;
	/* indicates maps of this region should be cached, if a mix of
	 * cached and uncached is desired, set this and open the device with
	 * O_SYNC to get an uncached region */
	unsigned cached;
	unsigned buffered;
	/* for debugging, creates a list of vmem file structs, the
	 * data_list_sem should be taken before vmem_data->sem if both are
	 * needed */
	struct semaphore data_list_sem;
	struct list_head data_list;

	long (*ioctl)(struct file *, unsigned int, unsigned long);
	int (*release)(struct inode *, struct file *);
};

static struct vmem_info vmem;
static int id_count;

//#define VMEM_IS_FREE(id, index) !(vmem[id].bitmap[index].allocated)
//#define VMEM_ORDER(id, index) vmem[id].bitmap[index].order
//#define VMEM_BUDDY_INDEX(id, index) (index ^ (1 << VMEM_ORDER(id, index)))
//#define VMEM_NEXT_INDEX(id, index) (index + (1 << VMEM_ORDER(id, index)))
//#define VMEM_OFFSET(index) (index * VMEM_MIN_ALLOC)
//#define VMEM_START_ADDR(id, index) (VMEM_OFFSET(index) + vmem[id].base)
//#define VMEM_LEN(id, index) ((1 << VMEM_ORDER(id, index)) * VMEM_MIN_ALLOC)
//#define VMEM_END_ADDR(id, index) (VMEM_START_ADDR(id, index) + \
//	VMEM_LEN(id, index))
//#define VMEM_START_VADDR(id, index) (VMEM_OFFSET(id, index) + vmem[id].vbase)
//#define VMEM_END_VADDR(id, index) (VMEM_START_VADDR(id, index) + \
//	VMEM_LEN(id, index))
//#define VMEM_REVOKED(data) (data->flags & VMEM_FLAGS_REVOKED)
#define VMEM_IS_PAGE_ALIGNED(addr) (!((addr) & (~PAGE_MASK)))
//#define VMEM_IS_SUBMAP(data) ((data->flags & VMEM_FLAGS_SUBMAP) && \
//	(!(data->flags & VMEM_FLAGS_UNSUBMAP)))

static int vmem_release(struct inode *, struct file *);
static int vmem_mmap(struct file *, struct vm_area_struct *);
static int vmem_open(struct inode *, struct file *);
static long vmem_ioctl(struct file *, unsigned int, unsigned long);

struct file_operations vmem_fops = {
	.release = vmem_release,
	.mmap = vmem_mmap,
	.open = vmem_open,
	.unlocked_ioctl = vmem_ioctl,
};

static int get_id(struct file *file)
{
	return MINOR(file->f_dentry->d_inode->i_rdev);
}

int is_vmem_file(struct file *file)
{
	if (unlikely(!file || !file->f_dentry || !file->f_dentry->d_inode))
		return 0;
	if (unlikely(file->f_dentry->d_inode->i_rdev !=
	     MKDEV(MISC_MAJOR, vmem.dev.minor)))
		return 0;
	return 1;
}

static int has_allocation(struct file *file)
{
	struct vmem_data *data;
	/* check is_vmem_file first if not accessed via vmem_file_ops */

	if (unlikely(!file->private_data))
		return 0;
	data = (struct vmem_data *)file->private_data;
	if (unlikely(data->vaddr == NULL))
		return 0;
	return 1;
}

static int vmem_free(struct vmem_data *data, unsigned long vaddr)
{
	/* caller should hold the write lock on vmem_sem! */
	DLOG("free vaddr 0x%08x, type=%d\n", vaddr, data->type);

	/* free the memory according to types */
	if (data->type == VMEM_TYPE_KMALLOC)
		kfree(vaddr);
	else
		vfree(vaddr);
#ifdef VMEM_PROF
	/* We only prof proccess with pid below MAXPID */
/*
	if(!(VMEM_PID(id,index)>=PROF_MAX_PID || VMEM_PID(id,index)<0))
		vmem_usage[VMEM_PID(id,index)] -= (1 << VMEM_ORDER(id, index));
*/
#endif
	return 0;
}

static int vmem_release(struct inode *inode, struct file *file)
{
	struct vmem_data *data = (struct vmem_data *)file->private_data;
	struct vmem_region_node *region_node;
	struct list_head *elt, *elt2;
	int ret = 0;

	DLOG("vmem_release\n");

	down_write(&data->sem);

	/* if its not a conencted file and it has an allocation, free it */
	if (has_allocation(file)) {
		ret = vmem_free(data, data->vaddr);
	}

	file->private_data = NULL;

	up_write(&data->sem);
	kfree(data);
	if (vmem.release)
		ret = vmem.release(inode, file);

	return ret;
}

static int vmem_open(struct inode *inode, struct file *file)
{
	struct vmem_data *data;
	int ret = 0;

	DLOG("vmem_open\n");

	/* setup file->private_data to indicate its unmapped */
	/*  you can only open a vmem device one time */
	if (file->private_data != NULL) {
		printk("vmem: private data is not null\n");
		return -1;
	}
	data = kmalloc(sizeof(struct vmem_data), GFP_KERNEL);
	if (!data) {
		printk("vmem: unable to allocate memory for vmem metadata.");
		return -1;
	}
	data->flags = 0;
	data->vaddr = NULL;
	data->size = 0;
	data->task = NULL;
	data->vma = NULL;
	data->pid = 0;
	data->master_file = NULL;
#if VMEM_DEBUG
	data->ref = 0;
#endif
	INIT_LIST_HEAD(&data->region_list);
	init_rwsem(&data->sem);

	file->private_data = data;
	INIT_LIST_HEAD(&data->list);

	down(&vmem.data_list_sem);
	list_add(&data->list, &vmem.data_list);
	up(&vmem.data_list_sem);

	DLOG("vmem_open success\n");

	return ret;
}

/*
 * vmem_allocate
 * Allocate memory according to memory size.
 * Memory allocation < 1 page, use kmalloc, otherwise vmalloc is used.
 * 
 * input: vmem_data for malloc information
 *        len, memory size requested
 * return: the address of allocated buffer
 */
static void *vmem_allocate(struct vmem_data *data, unsigned long len)
{
	/* caller should hold the write lock on vmem_sem! */
	/* return the corresponding pdata[] entry */
	void *ret = NULL;

	if (len < 0)
		return -EINVAL;

	// TODO: if not cacheline aligned, return

	// if size < 1 page, use kmalloc, otherwise, use vmalloc
	if (len <= PAGE_SIZE) {
		ret = kmalloc(len, GFP_KERNEL);
		data->type = VMEM_TYPE_KMALLOC;
		DLOG("kmalloc for 0x%08x\n", len);
	} else {
		ret = vmalloc(len);
		data->type = VMEM_TYPE_VMALLOC;
		DLOG("vmalloc for 0x%08x\n", len);
	}	

	if (ret == NULL) {
		printk("vmem: no space left to allocate!\n");
		printk("PID:%d (%s) want %lu bytes\n",current->pid,current->comm, len);
		return -ENOMEM;
	}
	data->size = len;

	// TODO: profiling feature
//#ifdef VMEM_PROF
#if 0
	/* We only prof proccess with pid below MAXPID */
	if(current->pid >=PROF_MAX_PID){
		return best_fit;
	}
	vmem[id].bitmap[best_fit].pid = current->pid;
	vmem[id].bitmap[best_fit].tgid = current->tgid;
	vmem_usage[current->pid] += (1 << VMEM_ORDER(id, best_fit));
	vmem_usage_max[current->pid] = (vmem_usage_max[current->pid]<vmem_usage[current->pid])? vmem_usage[current->pid]:vmem_usage_max[current->pid];


	/*sub namespace for calculate all allocated pages*/
	{
		unsigned int used_pages = 0;
		curr = 0;
		while (curr < end) {
			if(!VMEM_IS_FREE(id, curr))
			{
				used_pages += (1 << VMEM_ORDER(id, curr));
			}
			curr = VMEM_NEXT_INDEX(id, curr);
		}
		vmem_max_usage = (vmem_max_usage < used_pages)? used_pages: vmem_max_usage;
	}
#else
/*
      {
         curr = 0;
         while (curr < end) {
         if(!VMEM_IS_FREE(id, curr))
         {
            u4CurrentSize += VMEM_LEN(id, curr);
         }
         curr = VMEM_NEXT_INDEX(id, curr);
      }
      if(u4CurrentSize > g_u4MaxSize)
      {
           g_u4MaxSize = u4CurrentSize;
       }
     }
*/
#endif

	return ret;
}

static pgprot_t phys_mem_access_prot(struct file *file, pgprot_t vma_prot)
{
#ifdef pgprot_noncached
	if (vmem.cached == 0 || file->f_flags & O_SYNC)
		return pgprot_noncached(vma_prot);
#endif
#ifdef pgprot_ext_buffered
	else if (vmem.buffered)
		return pgprot_ext_buffered(vma_prot);
#endif
	return vma_prot;
}

static unsigned long vmem_start_addr(struct vmem_data *data)
{
	return 0;
/*
	if (vmem[id].no_allocator)
		return VMEM_START_ADDR(id, 0);
	else
		return VMEM_START_ADDR(id, data->index);
*/
}

static void *vmem_start_vaddr(struct vmem_data *data)
{
	return data->vaddr;
}

static unsigned long vmem_len(struct vmem_data *data)
{
	return data->size;
}

/*
 * map a virtual address range to the garbage page
 */
static int vmem_map_garbage(struct vm_area_struct *vma,
			    struct vmem_data *data, unsigned long offset,
			    unsigned long len)
{
	int i, garbage_pages = len >> PAGE_SHIFT;

	DLOG("Remap to garbage: offset 0x%08x, pages 0x%08x\n", offset, garbage_pages);

	vma->vm_flags |= VM_IO | VM_RESERVED | VM_PFNMAP | VM_SHARED | VM_WRITE;
	for (i = 0; i < garbage_pages; i++) {
		if (vm_insert_pfn(vma, vma->vm_start + offset + (i * PAGE_SIZE),
		    vmem.garbage_pfn))
			return -EAGAIN;
	}
	return 0;
}

static int vmem_unmap_pfn_range(struct vm_area_struct *vma,
				struct vmem_data *data, unsigned long offset,
				unsigned long len)
{
	int garbage_pages;
	DLOG("unmap offset %lx len %lx\n", offset, len);

	BUG_ON(!VMEM_IS_PAGE_ALIGNED(len));

	garbage_pages = len >> PAGE_SHIFT;
	zap_page_range(vma, vma->vm_start + offset, len, NULL);
	vmem_map_garbage(vma, data, offset, len);
	return 0;
}

static inline unsigned int vmem_virt_to_pfn(struct vmem_data *data, unsigned int vaddr)
{
	if (data->type == VMEM_TYPE_KMALLOC)
		return virt_to_pfn(vaddr);
	else if (data->type == VMEM_TYPE_VMALLOC)
		return vmalloc_to_pfn(vaddr);
	else
		BUG_ON(1);
}

// by keene
// TODO: this function asks remapping range to be continuous
static int vmem_map_pfn_range(struct vm_area_struct *vma,
			      struct vmem_data *data, unsigned long offset,
			      unsigned long len)
{
	int num_pages;
	int i;
	unsigned int vaddr = data->vaddr;

	DLOG("[vmem_map_pfn_range] map offset %lx len %lx\n", offset, len);
	BUG_ON(!VMEM_IS_PAGE_ALIGNED(vma->vm_start));
	BUG_ON(!VMEM_IS_PAGE_ALIGNED(vma->vm_end));
	BUG_ON(!VMEM_IS_PAGE_ALIGNED(len));
	BUG_ON(!VMEM_IS_PAGE_ALIGNED(offset));

	num_pages = len / PAGE_SIZE;
	for (i = 0; i < num_pages; i++) {
		int offset = i*PAGE_SIZE;
		int start = vaddr + offset;
		unsigned int pfn = vmem_virt_to_pfn(data, start);

		DLOG("[vmem_map_pfn_range] vaddr: 0x%08x, paddr: 0x%08x\n", start, pfn << PAGE_SHIFT); 
		if (!pfn) {
			DLOG("ERROR: can't find paddr for vaddr 0x%08x!!!\n", start);
		}

		DLOG("[vmem_map_pfn_range] map paddr 0x%08x to user vaddr 0x%08x\n", pfn << PAGE_SHIFT, vma->vm_start + offset);
		if (io_remap_pfn_range(vma, vma->vm_start + offset, pfn, PAGE_SIZE, vma->vm_page_prot)) {
			DLOG("ERROR remapping paddr 0x%08x for size %d to vaddr 0x%08x fails!!!\n", pfn << PAGE_SHIFT, PAGE_SIZE, start);
			return -EAGAIN;
		}
	}
	return 0;
}

static int vmem_remap_pfn_range(struct vm_area_struct *vma,
			      struct vmem_data *data, unsigned long offset,
			      unsigned long len)
{
	/* hold the mm semp for the vma you are modifying when you call this */
	BUG_ON(!vma);
	zap_page_range(vma, vma->vm_start + offset, len, NULL);
	return vmem_map_pfn_range(vma, data, offset, len);
}

static void vmem_vma_open(struct vm_area_struct *vma)
{
	struct file *file = vma->vm_file;
	struct vmem_data *data = file->private_data;
	/* this should never be called as we don't support copying vmem
	 * ranges via fork */
	BUG_ON(!has_allocation(file));
	down_write(&data->sem);
	/* remap the garbage pages, forkers don't get access to the data */
	vmem_unmap_pfn_range(vma, data, 0, vma->vm_start - vma->vm_end);
	up_write(&data->sem);
}

static void vmem_vma_close(struct vm_area_struct *vma)
{
	struct file *file = vma->vm_file;
	struct vmem_data *data = file->private_data;

	DLOG("current %u ppid %u file %p count %d\n", current->pid,
	     current->parent->pid, file, file_count(file));
	if (unlikely(!is_vmem_file(file) || !has_allocation(file))) {
		printk(KERN_WARNING "vmem: something is very wrong, you are "
		       "closing a vm backing an allocation that doesn't "
		       "exist!\n");
		return;
	}
	down_write(&data->sem);
	if (data->vma == vma) {
		data->vma = NULL;
		if ((data->flags & VMEM_FLAGS_CONNECTED) &&
		    (data->flags & VMEM_FLAGS_SUBMAP))
			data->flags |= VMEM_FLAGS_UNSUBMAP;
	}
	/* the kernel is going to free this vma now anyway */
	up_write(&data->sem);
}

static struct vm_operations_struct vm_ops = {
	.open = vmem_vma_open,
	.close = vmem_vma_close,
};

static int vmem_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct vmem_data *data;
	void *vaddr;
	unsigned long vma_size =  vma->vm_end - vma->vm_start;
	int ret = 0;

	DLOG("vmem_mmap\n");

	if (vma->vm_pgoff || !VMEM_IS_PAGE_ALIGNED(vma_size)) {
#if VMEM_DEBUG
		printk(KERN_ERR "vmem: mmaps must be at offset zero, aligned"
				" and a multiple of pages_size.\n");
#endif
		return -EINVAL;
	}

	printk("1\n");

	data = (struct vmem_data *)file->private_data;
	down_write(&data->sem);
	/* check this file isn't already mmaped, for submaps check this file
	 * has never been mmaped */
	if ((data->flags & VMEM_FLAGS_MASTERMAP) ||
	    (data->flags & VMEM_FLAGS_SUBMAP) ||
	    (data->flags & VMEM_FLAGS_UNSUBMAP)) {
#if VMEM_DEBUG
		printk(KERN_ERR "vmem: you can only mmap a vmem file once, "
		       "this file is already mmaped. %x\n", data->flags);
#endif
		ret = -EINVAL;
		goto error;
	}

	printk("2\n");

	/* if file->private_data == unalloced, alloc*/
	if (data && data->vaddr == NULL) {
		vaddr = vmem_allocate(data, vma->vm_end - vma->vm_start);
		data->vaddr = vaddr;
	}

	DLOG("vmalloc allocation kernel vaddr: 0x%08x, size: 0x%08x\n", vaddr, vma->vm_end - vma->vm_start);

	/* either no space was available or an error occured */
	if (!has_allocation(file)) {
		ret = -EINVAL;
		printk("vmem: could not find allocation for map.\n");
		goto error;
	}

	if (vmem_len(data) < vma_size) {
#if VMEM_DEBUG
		printk(KERN_WARNING "vmem: mmap size [%lu] does not match"
		       "size of backing region [%lu].\n", vma_size,
		       vmem_len(data));
#endif
		ret = -EINVAL;
		goto error;
	}

	// TODO: find an appropriate value for offset in files
	vma->vm_pgoff = 0;
	//vma->vm_pgoff = vmem_start_addr(id, data) >> PAGE_SHIFT;
	vma->vm_page_prot = phys_mem_access_prot(file, vma->vm_page_prot);

	DLOG("vma->vm_page_prot = 0x%08x\n", vma->vm_page_prot);

	/*
	 * use a loop to remap vma
	 */ 
	if (vmem_map_pfn_range(vma, data, 0, vma_size)) {
		DLOG("ERROR: vmem_mmap failed!!!!\n");
		ret = -EAGAIN;
		goto error;
	}

	DLOG("vmem_mmap success!\n");

	data->flags |= VMEM_FLAGS_MASTERMAP;
	data->pid = current->pid;
	get_task_struct(current->group_leader);
	data->task = current->group_leader;

	vma->vm_ops = &vm_ops;
error:
	up_write(&data->sem);
	return ret;
}

int get_vmem_addr(struct file *file, unsigned long *start,
		  unsigned long *vstart, unsigned long *len)
{
	struct vmem_data *data;
	int id;

	if (!is_vmem_file(file) || !has_allocation(file)) {
		return -1;
	}

	data = (struct vmem_data *)file->private_data;
	if (data->vaddr == NULL) {
#if VMEM_DEBUG
		printk(KERN_INFO "vmem: requested vmem data from file with no "
		       "allocation.\n");
		return -1;
#endif
	}

	down_read(&data->sem);
	*start = vmem_start_addr(data);
	*len = vmem_len(data);
	*vstart = (unsigned long)vmem_start_vaddr(data);
	up_read(&data->sem);
#if VMEM_DEBUG
	down_write(&data->sem);
	data->ref++;
	up_write(&data->sem);
#endif
	return 0;
}

int get_vmem_file(int fd, unsigned long *start, unsigned long *vstart,
		  unsigned long *len, struct file **filp)
{
	struct file *file;

	file = fget(fd);
	if (unlikely(file == NULL)) {
		printk(KERN_INFO "vmem: requested data from file descriptor "
		       "that doesn't exist.");
		return -1;
	}

	if (get_vmem_addr(file, start, vstart, len))
		goto end;

	if (filp)
		*filp = file;
	return 0;
end:
	fput(file);
	return -1;
}

void put_vmem_file(struct file *file)
{
	struct vmem_data *data;
	int id;

	if (!is_vmem_file(file))
		return;
	data = (struct vmem_data *)file->private_data;
#if VMEM_DEBUG
	down_write(&data->sem);
	if (data->ref == 0) {
		printk("vmem: vmem_put > vmem_get %s (pid %d)\n",
		       vmem.dev.name, data->pid);
		BUG();
	}
	data->ref--;
	up_write(&data->sem);
#endif
	fput(file);
}

void flush_vmem_file(struct file *file, unsigned long offset, unsigned long len)
{
	struct vmem_data *data;
	int id;
	void *vaddr;
	struct vmem_region_node *region_node;
	struct list_head *elt;
	void *flush_start, *flush_end;

	if (!is_vmem_file(file) || !has_allocation(file)) {
		return;
	}

	data = (struct vmem_data *)file->private_data;
	if (!vmem.cached || file->f_flags & O_SYNC)
		return;

	down_read(&data->sem);
	vaddr = vmem_start_vaddr(data);
	/* if this isn't a submmapped file, flush the whole thing */
	if (unlikely(!(data->flags & VMEM_FLAGS_CONNECTED))) {
		dmac_flush_range(vaddr, vaddr + vmem_len(data));
		goto end;
	}
	/* otherwise, flush the region of the file we are drawing */
	list_for_each(elt, &data->region_list) {
		region_node = list_entry(elt, struct vmem_region_node, list);
		if ((offset >= region_node->region.offset) &&
		    ((offset + len) <= (region_node->region.offset +
			region_node->region.len))) {
			flush_start = vaddr + region_node->region.offset;
			flush_end = flush_start + region_node->region.len;
			dmac_flush_range(flush_start, flush_end);
			break;
		}
	}
end:
	up_read(&data->sem);
}

static void vmem_unlock_data_and_mm(struct vmem_data *data,
				    struct mm_struct *mm)
{
	up_write(&data->sem);
	if (mm != NULL) {
		up_write(&mm->mmap_sem);
		mmput(mm);
	}
}

static int vmem_lock_data_and_mm(struct file *file, struct vmem_data *data,
				 struct mm_struct **locked_mm)
{
	int ret = 0;
	struct mm_struct *mm = NULL;
	*locked_mm = NULL;
lock_mm:
        down_read(&data->sem);
        mm = get_task_mm(data->task);
        if (!mm) {
#if PMEM_DEBUG
                printk("pmem: can't remap task is gone!\n");
#endif
                up_read(&data->sem);
                return -1;
        }    
        up_read(&data->sem);

        if (mm) 
                down_write(&mm->mmap_sem);

	printk("1\n");
	down_write(&data->sem);
	/* check that the file didn't get mmaped before we could take the
	 * data sem, this should be safe b/c you can only submap each file
	 * once */
	if (!mm) {
		vmem_unlock_data_and_mm(data, mm);
		up_write(&data->sem);
		goto lock_mm;
	}
	printk("2\n");
	/* now check that vma.mm is still there, it could have been
	 * deleted by vma_close before we could get the data->sem */
	if ((data->flags & VMEM_FLAGS_UNSUBMAP) && (mm != NULL)) {
		/* might as well release this */
		if (data->flags & VMEM_FLAGS_SUBMAP) {
			put_task_struct(data->task);
			data->task = NULL;
			/* lower the submap flag to show the mm is gone */
			data->flags &= ~(VMEM_FLAGS_SUBMAP);
		}
		vmem_unlock_data_and_mm(data, mm);
		return -1;
	}
	printk("3\n");
	*locked_mm = mm;
	return ret;
}

int vmem_remap(struct vmem_region *region, struct file *file,
		      unsigned operation)
{
	int ret;
	struct vmem_region_node *region_node;
	struct mm_struct *mm = NULL;
	struct list_head *elt, *elt2;
	struct vmem_data *data = (struct vmem_data *)file->private_data;

	DLOG("[vmem_remap]\n");

	/* vmem region must be aligned on a page boundry */
	if (unlikely(!VMEM_IS_PAGE_ALIGNED(region->offset) ||
		 !VMEM_IS_PAGE_ALIGNED(region->len))) {
#if VMEM_DEBUG
		printk("vmem: request for unaligned vmem suballocation "
		       "%lx %lx\n", region->offset, region->len);
#endif
		return -EINVAL;
	}

	/* if userspace requests a region of len 0, there's nothing to do */
	if (region->len == 0)
		return 0;

	DLOG("[vmem_remap] remapping offset 0x%08x, len 0x%08x\n", region->offset, region->len);

	/* lock the mm and data */
	ret = vmem_lock_data_and_mm(file, data, &mm);
	if (ret) {
		DLOG("vmem_lock_data_and_mm failed: return %d\n", ret);
		return 0;
	}

	/* check that the requested range is within the src allocation */
	if (unlikely((region->offset > vmem_len(data)) ||
		     (region->len > vmem_len(data)) ||
		     (region->offset + region->len > vmem_len(data)))) {
#if VMEM_DEBUG
		printk(KERN_INFO "vmem: suballoc doesn't fit in src_file!\n");
#endif
		ret = -EINVAL;
		goto err;
	}

	if (operation == VMEM_MAP) {
		region_node = kmalloc(sizeof(struct vmem_region_node),
			      GFP_KERNEL);
		if (!region_node) {
			ret = -ENOMEM;
#if VMEM_DEBUG
			printk(KERN_INFO "No space to allocate metadata!");
#endif
			goto err;
		}
		region_node->region = *region;
		list_add(&region_node->list, &data->region_list);
	} else if (operation == VMEM_UNMAP) {
		int found = 0;
		list_for_each_safe(elt, elt2, &data->region_list) {
			region_node = list_entry(elt, struct vmem_region_node,
				      list);
			if (region->len == 0 ||
			    (region_node->region.offset == region->offset &&
			    region_node->region.len == region->len)) {
				list_del(elt);
				kfree(region_node);
				found = 1;
			}
		}
		if (!found) {
#if VMEM_DEBUG
			printk("vmem: Unmap region does not map any mapped "
				"region!");
#endif
			ret = -EINVAL;
			goto err;
		}
	}

	if (data->vma) {
		if (operation == VMEM_MAP)
			ret = vmem_remap_pfn_range(data->vma, data,
						   region->offset, region->len);
		else if (operation == VMEM_UNMAP)
			ret = vmem_unmap_pfn_range(data->vma, data,
						   region->offset, region->len);
	}

err:
	vmem_unlock_data_and_mm(data, mm);
	return ret;
}

static void vmem_get_size(struct vmem_region *region, struct file *file)
{
	struct vmem_data *data = (struct vmem_data *)file->private_data;

	if (!has_allocation(file)) {
		region->offset = 0;
		region->len = 0;
		return;
	} else {
		region->offset = vmem_start_addr(data);
		region->len = vmem_len(data);
	}
	DLOG("offset %lx len %lx\n", region->offset, region->len);
}


static long vmem_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct vmem_data *data;

	switch (cmd) {
	case VMEM_GET_PHYS:
		{
			struct vmem_region region;
			DLOG("[vmem_ioctl][VMEM_GET_PHYS]\n");
			if (!has_allocation(file)) {
				region.offset = 0;
				region.len = 0;
			} else {
				data = (struct vmem_data *)file->private_data;
				DLOG("User vaddr = 0x%08x\n", vmem_start_vaddr(data));
				region.offset = 0;
				region.len = 0;
				//region.offset = vmalloc_to_phys(vmem_start_vaddr(data));
				//region.len = vmem_len(data);
			}
//			printk(KERN_INFO "vmem: request for physical address of vmem region "
//					"from process %d.\n", current->pid);
			if (copy_to_user((void __user *)arg, &region,
						sizeof(struct vmem_region)))
				return -EFAULT;
			break;
		}
	case VMEM_MAP:
		{
			struct vmem_region region;
			DLOG("[vmem_ioctl][VMEM_MAP]\n");
			if (copy_from_user(&region, (void __user *)arg,
						sizeof(struct vmem_region)))
				return -EFAULT;
			data = (struct vmem_data *)file->private_data;
			return vmem_remap(&region, file, VMEM_MAP);
		}
		break;
	case VMEM_UNMAP:
		{
			struct vmem_region region;
			DLOG("[vmem_ioctl][VMEM_UNMAP]\n");
			if (copy_from_user(&region, (void __user *)arg,
						sizeof(struct vmem_region)))
				return -EFAULT;
			data = (struct vmem_data *)file->private_data;
			return vmem_remap(&region, file, VMEM_UNMAP);
			break;
		}
	case VMEM_GET_SIZE:
		{
			struct vmem_region region;
			DLOG("get_size\n");
			vmem_get_size(&region, file);
			if (copy_to_user((void __user *)arg, &region,
						sizeof(struct vmem_region)))
				return -EFAULT;
			break;
		}
	case VMEM_GET_TOTAL_SIZE:
		{
			struct vmem_region region;
			DLOG("get total size\n");
			region.offset = 0;
			get_id(file);
			region.len = vmem.size;
			if (copy_to_user((void __user *)arg, &region,
						sizeof(struct vmem_region)))
				return -EFAULT;
			break;
		}
	case VMEM_ALLOCATE:
		{
			if (has_allocation(file))
				return -EINVAL;
			data = (struct vmem_data *)file->private_data;
			data->vaddr = vmem_allocate(data, arg);
			break;
		}
	case VMEM_CACHE_FLUSH:
		{
			struct vmem_region region;
			DLOG("flush\n");
			if (copy_from_user(&region, (void __user *)arg,
					   sizeof(struct vmem_region)))
				return -EFAULT;
			flush_vmem_file(file, region.offset, region.len);
			break;
		}
	default:
		if (vmem.ioctl)
			return vmem.ioctl(file, cmd, arg);
		return -EINVAL;
	}
	DLOG("ioctl done\n");
	return 0;
}

#if VMEM_DEBUG
static ssize_t debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t debug_read(struct file *file, char __user *buf, size_t count,
			  loff_t *ppos)
{
	struct list_head *elt, *elt2;
	struct vmem_data *data;
	struct vmem_region_node *region_node;
	int id = (int)file->private_data;
	const int debug_bufmax = 4096;
	static char buffer[4096];
	int n = 0;

	DLOG("debug open\n");
	n = scnprintf(buffer, debug_bufmax,
		      "pid #: mapped regions (offset, len) (offset,len)...\n");

	down(&vmem.data_list_sem);
	list_for_each(elt, &vmem.data_list) {
		data = list_entry(elt, struct vmem_data, list);
		down_read(&data->sem);
		n += scnprintf(buffer + n, debug_bufmax - n, "pid %u:",
				data->pid);
		list_for_each(elt2, &data->region_list) {
			region_node = list_entry(elt2, struct vmem_region_node,
				      list);
			n += scnprintf(buffer + n, debug_bufmax - n,
					"(%lx,%lx) ",
					region_node->region.offset,
					region_node->region.len);
		}
		n += scnprintf(buffer + n, debug_bufmax - n, "\n");
		up_read(&data->sem);
	}
	up(&vmem.data_list_sem);

	n++;
	buffer[n] = 0;
	return simple_read_from_buffer(buf, count, ppos, buffer, n);
}

static struct file_operations debug_fops = {
	.read = debug_read,
	.open = debug_open,
};
#endif

#if 0
static struct miscdevice vmem_dev = {
	.name = "vmem",
	.fops = &vmem_fops,
};
#endif

static void vmem_dump()
{
/*
	int curr =0;
	int vmem_end = vmem[0].num_entries;
	int id = 0;
	int free_pages=0;
	int used_pages = 0;
	int i;
	struct task_struct* tsk;
	
	for(i =0; i< PROF_MAX_PID; i ++)
		vmem_usage[i] = 0;

	while(curr < vmem_end){
		if (VMEM_IS_FREE(id, curr)) {
			free_pages += (1 << VMEM_ORDER(id, curr));
		}
		else
		{
			used_pages += (1 << VMEM_ORDER(id, curr));
			vmem_usage[VMEM_PID(id,curr)] += (1 << VMEM_ORDER(id, curr));
		}
		curr = VMEM_NEXT_INDEX(id, curr);
	}
	vmem_max_usage = (vmem_max_usage < used_pages)? used_pages: vmem_max_usage;
	printk("VMEM Size:%lu : free_pages : %d : used_pages: %d : MAX_usage_pages: %d\n",
			vmem[0].size/PAGE_SIZE, free_pages, used_pages, vmem_max_usage);
	for(i =0; i< PROF_MAX_PID; i ++){
		for(i =0; i< PROF_MAX_PID; i ++){
			if(vmem_usage_max[i] == 0)
				continue;
			vmem_usage_max[i] = (vmem_usage_max[i]<vmem_usage[i])?
				vmem_usage[i]:vmem_usage_max[i];
			tsk = find_task_by_vpid(i); 
			if(tsk!=NULL)
				printk("PID : %d : TGID : %d : Pages : %d : MAX : %d : %s\n",
						i, tsk->tgid, vmem_usage[i], vmem_usage_max[i], tsk->comm);
		}
	}
*/
}

#ifdef VMEM_PROF
struct task_struct *find_task_by_vpid(pid_t vnr);
static int vmem_prof_show(char *page, char **start, off_t off,
				  int count, int *eof, void *data)
{
/*
	int curr =0;
	int vmem_end = vmem[0].num_entries;
	int id = 0;
	int free_pages=0;
	int used_pages = 0;
	int i;
	int len = 0;
	char *buf = page;
	char *end = page + PAGE_SIZE;
	struct task_struct* tsk;

	for(i =0; i< PROF_MAX_PID; i ++)
		vmem_usage[i] = 0;

	while(curr < vmem_end){
		if (VMEM_IS_FREE(id, curr)) {
			free_pages += (1 << VMEM_ORDER(id, curr));
		}
		else
		{
			used_pages += (1 << VMEM_ORDER(id, curr));
			vmem_usage[VMEM_PID(id,curr)] += (1 << VMEM_ORDER(id, curr));
		}
		curr = VMEM_NEXT_INDEX(id, curr);
	}
	vmem_max_usage = (vmem_max_usage < used_pages)? used_pages: vmem_max_usage;
	buf += snprintf(buf, end - buf,"VMEM Size:%lu : free_pages : %d : used_pages: %d : MAX_usage_pages: %d\n", vmem[0].size/PAGE_SIZE, free_pages, used_pages, vmem_max_usage);
	for(i =0; i< PROF_MAX_PID; i ++){
		if(vmem_usage_max[i] == 0)
			continue;
		vmem_usage_max[i] = (vmem_usage_max[i]<vmem_usage[i])? vmem_usage[i]:vmem_usage_max[i];
		tsk = find_task_by_vpid(i); 
		if(tsk!=NULL)
			buf += snprintf(buf, end - buf, "PID : %d : TGID : %d : Pages : %d : MAX : %d : %s\n", i, tsk->tgid, vmem_usage[i], vmem_usage_max[i], tsk->comm);
		//else 
		//		buf += snprintf(buf, end - buf, "PID : -1 : TGID : -2 : Pages : %d : MAX : %d : Dead\n",  vmem_usage[i], vmem_usage_max[i] );
	}
	
	*start = page + off;
	len = buf - page;
	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len  : count;     	
*/
}

void vmem_dist_prof(void)
{
/*
        int curr =0;
        int vmem_end = vmem[0].num_entries;
        int id = 0; 
        struct task_struct *tsk;

	printk("VMEM base: 0x%x\n", (unsigned int)vmem[0].base);
        printk("index:size:pid:command line\n");
        while(curr < vmem_end){
                if (VMEM_IS_FREE(id, curr)) {
                        printk("[VMEM]:%d:%d:0:NULL\n", curr, VMEM_ORDER(id, curr));
                }
                else
                {
                        tsk = find_task_by_vpid(VMEM_PID(id,curr));
                        printk("[VMEM]:%d:%d:%d:%s\n", curr, VMEM_ORDER(id, curr), (int)VMEM_PID(id,curr), tsk->comm);
                }
                curr = VMEM_NEXT_INDEX(id, curr);
        }
*/
}

static int __init init_vmem_procfs(void)
{
	struct proc_dir_entry *pe;
	pe = create_proc_entry("vmem", 0444, NULL);
	if (pe) {
		pe->read_proc = vmem_prof_show;
	}
	if (!pe)
		return -ENOMEM;
	return 0;
}
#endif
int vmem_setup(struct android_vmem_platform_data *pdata,
	       long (*ioctl)(struct file *, unsigned int, unsigned long),
	       int (*release)(struct inode *, struct file *))
{
	int err = 0;
	int i, index = 0;

	vmem.cached = pdata->cached;
	vmem.buffered = pdata->buffered;
	vmem.base = pdata->start;
	vmem.size = pdata->size;
	vmem.ioctl = ioctl;
	vmem.release = release;
	init_MUTEX(&vmem.data_list_sem);
	INIT_LIST_HEAD(&vmem.data_list);
	vmem.dev.name = pdata->name;
	vmem.dev.minor = MISC_DYNAMIC_MINOR;
	vmem.dev.fops = &vmem_fops;
	printk("%s: %d init\n", pdata->name, pdata->cached);

	err = misc_register(&vmem.dev);
	if (err) {
		printk("Unable to register vmem driver!: %d\n", err);
		goto err_cant_register_device;
	}

	vmem.garbage_pfn = page_to_pfn(alloc_page(GFP_KERNEL));

#if VMEM_DEBUG
	debugfs_create_file(pdata->name, S_IFREG | S_IRUGO, NULL, (void *)NULL,
			    &debug_fops);
#endif
	return 0;
err_cant_register_device:
	return -1;
}

static int vmem_probe(struct platform_device *pdev)
{
	struct android_vmem_platform_data *pdata;

	printk("in vmem_probe\n");

	if (!pdev || !pdev->dev.platform_data) {
		printk(KERN_ALERT "Unable to probe vmem!\n");
		return -1;
	}
	pdata = pdev->dev.platform_data;
	return vmem_setup(pdata, NULL, NULL);
}


static int vmem_remove(struct platform_device *pdev)
{
	__free_page(pfn_to_page(vmem.garbage_pfn));
	misc_deregister(&vmem.dev);
	return 0;
}

static struct platform_driver vmem_driver = {
	.probe = vmem_probe,
	.remove = vmem_remove,
	.driver = { .name = "android_vmem" }
};


static int __init vmem_init(void)
{
#ifdef VMEM_PROF
	if(init_vmem_procfs()<0)
		printk("vmem_procfs init faild\n");
#endif
	return platform_driver_register(&vmem_driver);
}

static void __exit vmem_exit(void)
{
	platform_driver_unregister(&vmem_driver);
}

module_init(vmem_init);
module_exit(vmem_exit);

