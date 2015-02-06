//
// Notice that we CANNOT use kmalloc/vmalloc and other allocation functions.
//

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include <asm/memory.h>
#include <linux/seq_file.h>      /* for seq_file */
#include <linux/kallsyms.h>
#include <linux/hash.h>
#include "linux/kmem_mon.h"

#ifdef CONFIG_MTPROF_KMEM

// compile these code to kernel
#define USING_STATIC
//#define USING_HASH
#define USING_LIST

//
// DEBUG & LOG macro {
//
//#define DEBUG
#ifdef DEBUG
#define LOG_D(string, args...) printk(KERN_ERR "[m]: "string,##args)
#else
#define LOG_D(string, args...) ((void)0)
#endif

//#define LOG_I(string, args...) printk(KERN_ERR "[m]: "string,##args)
//#define LOG_E(string, args...) printk(KERN_ERR "[m]: "string,##args)

#define LOG_E(string, args...) ((void)0)
#define LOG_I(string, args...) ((void)0)

// } DEBUG & LOG macro

#define kmem_mon_hash(address, size) (((u32)address / (u32)size) % size)

// Use this to debug
#define MAX_PID 0
#define DEBUG_PID 500

static DEFINE_MUTEX(kmem_mon_mutex);

static struct proc_dir_entry *proc_kmem_mon_dir = NULL;

//
// TODO: calculate the size of static array size.
//
// NOTICE: mem_info_table[0] is NOT used. It's the header.
//
static mem_info_t mem_info_table[MAX_KMEM_MON_NUM]; // mem nodes
static process_info_t process_table[MAX_PROCESS_NUM];
static caller_info_t caller_table[MAX_CALLER_TABLE];
static addr_info_t addr_table[MAX_ADDR_TABLE];

static int addr_idx_table[MAX_ADDR_IDX];

static char tmp[PAGE_SIZE];

static int last_mem_node = 0;
static int last_caller_node = 0;
static int last_addr_node = 0;
static int free_addr_head = 0;

//
// PROC Interface
// /proc/kmem_mon/switch
// /proc/kmem_mon/pid
//
// TODO: control this switch by compile option so we can enable monitor by default
// default is off
static int kmem_mon_switch = 0;
static int kmem_mon_caller_switch = 1;
static int kmem_mon_pid = 1;

// TODO: currently not used
static spinlock_t kmem_mon_alloc_lock = SPIN_LOCK_UNLOCKED;
static spinlock_t kmem_mon_free_lock = SPIN_LOCK_UNLOCKED;

static char* mem_class_string(MEM_CLASS_T mem_class);
static int allocate_mem_node();
static int find_mem_node(int start, MEM_CLASS_T mem_class, int new);
static void kmem_mon_alloc(MEM_CLASS_T mem_class, unsigned long caller,
	const void *addr, size_t bytes_req, size_t bytes_alloc);
static void kmem_mon_free(MEM_CLASS_T mem_class, unsigned long caller, 
	const void *addr, size_t bytes_free);



//
// Wrapper
//

// kmalloc
// ksize ONLY can be used in kmalloc and kmem_cache_alloc
void kmem_mon_kmalloc(unsigned long caller, const void *addr, int bytes_req, int bytes_alloc){
	kmem_mon_alloc(KMEM_MON_TYPE_KMALLOC, caller, addr, bytes_req, bytes_alloc);
}
void kmem_mon_kfree(unsigned long caller, const void *addr){
	if (unlikely(ZERO_OR_NULL_PTR(addr)))
		return;
	kmem_mon_free(KMEM_MON_TYPE_KMALLOC, caller, addr, ksize(addr));
}

// kmem_cache_alloc
void kmem_mon_kmem_cache_alloc(unsigned long caller, const void *addr, 
	size_t bytes_req, size_t bytes_alloc){
	kmem_mon_alloc(KMEM_MON_TYPE_KMEM_CACHE, caller, addr, bytes_req, bytes_alloc);
}
void kmem_mon_kmem_cache_free(unsigned long caller, const void *addr){
	if (unlikely(ZERO_OR_NULL_PTR(addr)))
		return;
	kmem_mon_free(KMEM_MON_TYPE_KMEM_CACHE, caller, addr, ksize(addr));
}


// PMEM
void kmem_mon_pmem_alloc(int req, int alloc){
	kmem_mon_alloc(KMEM_MON_TYPE_PMEM, NULL, NULL, req, alloc);
}
void kmem_mon_pmem_free(int size){
	kmem_mon_free(KMEM_MON_TYPE_PMEM, NULL, NULL, size);
}

// M4U
void kmem_mon_m4u_alloc(int req, int alloc){
	kmem_mon_alloc(KMEM_MON_TYPE_M4U, NULL, NULL, req, alloc);
}
void kmem_mon_m4u_dealloc(const unsigned int addr, const unsigned int req_size){
	// m4u does not provide direct interface for dealloc page size
	int page_num = (req_size + (addr & 0xfff)) / PAGE_SIZE;
	if((addr + req_size) & 0xfff) 
	    page_num++;
	kmem_mon_free(KMEM_MON_TYPE_M4U, NULL, (void *)addr, page_num * PAGE_SIZE);
}

// vmalloc
void kmem_mon_vmalloc(unsigned long caller, const void *addr, int bytes_req, int bytes_alloc){
	kmem_mon_alloc(KMEM_MON_TYPE_VMALLOC, caller, addr, bytes_req, bytes_alloc);
}
void kmem_mon_vfree(int size){
	kmem_mon_free(KMEM_MON_TYPE_VMALLOC, NULL, NULL, size);
}

// ashmem
// NOTICE: size need not be aligned?
// ONLY record  the requiring size
void kmem_mon_ashmem_mmap(int size){
	kmem_mon_alloc(KMEM_MON_TYPE_ASHMEM, NULL, NULL, size, size);
}
void kmem_mon_ashmem_release(int size){
	kmem_mon_free(KMEM_MON_TYPE_ASHMEM, NULL, NULL, size);
}

// ONLY record the requiring size
// TODO: account the failure
void kmem_mon_kmallocwrapper(unsigned long caller, int size){
	kmem_mon_alloc(KMEM_MON_TYPE_KMALLOCWRAPPER, caller, NULL, size, size);
}
void kmem_mon_kfreewrapper(const void *addr){
	if (unlikely(ZERO_OR_NULL_PTR(addr)))
		return;
	kmem_mon_free(KMEM_MON_TYPE_KMALLOC, NULL, addr, ksize(addr));
}

// For Unit Test
void kmem_mon_test(){
}

static int _cmdline(struct task_struct *task, char * buffer)
{
	int res = 0;
	unsigned int len;
	struct mm_struct *mm = get_task_mm(task);
	if (!mm)
		goto out;
	if (!mm->arg_end)
		goto out_mm;	/* Shh! No looking before we're done */

 	len = mm->arg_end - mm->arg_start;
 
	if (len > PAGE_SIZE)
		len = PAGE_SIZE;
 
	res = access_process_vm(task, mm->arg_start, buffer, len, 0);

	// If the nul at the end of args has been overwritten, then
	// assume application is using setproctitle(3).
	if (res > 0 && buffer[res-1] != '\0' && len < PAGE_SIZE) {
		len = strnlen(buffer, res);
		if (len < res) {
		    res = len;
		} else {
			len = mm->env_end - mm->env_start;
			if (len > PAGE_SIZE - res)
				len = PAGE_SIZE - res;
			res += access_process_vm(task, mm->env_start, buffer+res, len, 0);
			res = strnlen(buffer, res);
		}
	}
out_mm:
	mmput(mm);
out:
	return res;
}


// come from kernel/fs/proc/base.c
static int cmd_line(char *buffer){
	unsigned int len;

	struct mm_struct *mm = current->mm;

	if ((mm == NULL) || (current->flags & PF_KTHREAD)) {
		goto out;
	}
	
	if (!mm->arg_end){
		goto out;	/* Shh! No looking before we're done */
	}
	
 	len = mm->arg_end - mm->arg_start;
	if (len > MAX_CMD_LINE)
		len = MAX_CMD_LINE;

	LOG_I("len: %d\n", len);
 	strncpy(buffer, mm->arg_start, len);
	buffer[len -1] = '\0';
	LOG_I("cmdline: %s\n", buffer);
out:
	return len;
}

//
// NOTICE: mem_info_table[0] is NOT used. It's the header.
// >0: return the index of a new mem node
// -1: exceed the max node num
static int allocate_mem_node(){
	if(last_mem_node == MAX_KMEM_MON_NUM - 1)
		return -1;
	else
		return ++last_mem_node;
}

// find a matching mem node
// new: 1, if it does not exist, allocate a new node.
// return: -1, search & allocate fails; otherwise, new node id
static int find_mem_node(int start, MEM_CLASS_T mem_class, int new){
	int prev = start;
	int new_node;
	
	while(start != 0){
		if(mem_info_table[start].mem_class == mem_class) {
			return start;
		}
		else {
			prev = start;
			start = mem_info_table[start].next_mem_node;
		}
	}

	if(new){
		// If we come here, it means no mem node matches the mem class
		// so allocate a new node
		new_node = allocate_mem_node();
		if(new_node == -1)
			return -1;
		else {
			// construct the node list
			mem_info_table[prev].next_mem_node = new_node;

			return new_node;
		}
	} else
		return -1;
}

#ifdef USING_LIST
static int allocate_caller_node(){
	if(last_caller_node == MAX_CALLER_TABLE - 1)
		return -1;
	else
		return ++last_caller_node;
}
static int allocate_addr_node(){
	int free_node;
	
	if(last_addr_node == MAX_ADDR_TABLE - 1){
		// allocate from the free list
		if(free_addr_head != 0){
			LOG_I("last_addr_node: %d, free_addr_head = %d\n", last_addr_node, free_addr_head);
			free_node = free_addr_head;
			free_addr_head = addr_table[free_addr_head].next;
			return free_node;
		} else
			return -1;
	} else
		return ++last_addr_node;	
}

static int find_caller_node(int start, unsigned long caller, int new){
	int prev = start;
	int new_node;
	
	while(start != 0){
		if(caller_table[start].caller_addr == caller) {
			return start;
		}
		else {
			prev = start;
			start = caller_table[start].next_node;
		}
	}

	if(new){
		// If we come here, it means no mem node matches the mem class
		// so allocate a new node
		new_node = allocate_caller_node();
		if(new_node == -1)
			return -1;
		else {
			// construct the node list
			caller_table[prev].next_node = new_node;

			return new_node;
		}
	} else
		return -1;

}

static int insert_addr_free_list(int addr_node){
	// reset
	addr_table[addr_node].addr = 0;
	addr_table[addr_node].caller_hash = 0;
	addr_table[addr_node].next = 0;

	// TODO: unlikely
	if(free_addr_head == 0){
		free_addr_head = addr_node;
	} else {
		addr_table[addr_node].next = free_addr_head;
		free_addr_head = addr_node;
	}
}


#endif

//
// Using mutex in the free function may cause problems.
//
static void kmem_mon_free(MEM_CLASS_T mem_class, unsigned long caller, const void *addr, size_t bytes_free) {
	int i, pid, node;
	int addr_hash, caller_hash;
	int next;
	unsigned long flags;
	
	if(!kmem_mon_switch)
		return;
		
	pid = current->pid;

	// for debugging
	if(pid < MAX_PID){
		return;
	}

	if(pid > DEBUG_PID){
		LOG_I("f: %d %s, addr: %x, size: %d\n", current->pid, mem_class_string(mem_class), addr, bytes_free);
	}

	spin_lock_irqsave(&kmem_mon_alloc_lock, flags);
	//mutex_lock(&kmem_mon_mutex);
	// If a new pid
	if(process_table[pid].start_idx == 0){
		// something unexpected happens
		LOG_E("free before allocation\n");
	} else {
		node = find_mem_node(process_table[pid].start_idx, mem_class, FALSE);
		if(node == -1){
			// something unexpected happens
			LOG_E("process has no mem node\n");
			goto EXIT;
		}

		// update mem node
		mem_info_table[node].free_freq++;
		mem_info_table[node].total_bytes_free += bytes_free;
	}
#if 0
	if(addr != NULL) {
		addr_hash = kmem_mon_hash(addr, MAX_ADDR_TABLE);
		for(i = 0; i < MAX_ADDR_TABLE; i++){
			if(addr_table[addr_hash].addr == addr){
				break;
			}
			addr_hash = (addr_hash + 1) % MAX_ADDR_TABLE;
		}
		if(i < MAX_ADDR_TABLE){
			// update caller node
			caller_hash = addr_table[addr_hash].caller_hash;
			if(caller_table[caller_hash].pid != current->pid){
				//LOG_E("free others's allocation\n");
				//LOG_E("true pid: %d, false pid: %d\n", caller_table[caller_hash].pid, current->pid);
				goto EXIT;
			}

			caller_table[caller_hash].bytes_free += bytes_free;
			caller_table[caller_hash].freq_free++;
			// free this addr node
			addr_table[addr_hash].caller_hash = 0;
			addr_table[addr_hash].addr = 0;
		} else {
			LOG_E("free unkown addr: %x", addr);
		}
	} // if(addr != NULL) {
#endif

#ifdef USING_LIST
	if(addr != NULL){
		// search
		addr_hash = hash_32(addr, 12);
		
		next = addr_idx_table[addr_hash];
		if(next == 0){
			LOG_E("free new addr\n");
			goto EXIT;
		}
		
		while(next != 0){
			if(caller_table[addr_table[next].caller_hash].caller_addr == caller){
				break;
			} else
				LOG_I("unmatch\n");
			next = addr_table[next].next;
		}

		// delete
		if (next == 0) {
			LOG_E("cannot find this addr\n");
			goto EXIT;
		} else {
			LOG_I("free addr node: %d\n", next);
			// update caller table
			caller_hash = addr_table[next].caller_hash;
			if(caller_table[caller_hash].pid != current->pid){
				//LOG_E("free others's allocation\n");
				LOG_E("true pid: %d, false pid: %d\n", caller_table[caller_hash].pid, current->pid);
				goto EXIT;
			}

			caller_table[caller_hash].bytes_free += bytes_free;
			caller_table[caller_hash].freq_free++;			

			// delete addr node
			insert_addr_free_list(next);
		}
	}
#endif

EXIT:
	//return;
	//mutex_unlock(&kmem_mon_mutex);
	spin_unlock_irqrestore(&kmem_mon_alloc_lock, flags);
}

//
// NOTICE: 
// 1. It's dangeours to use mutex, spin_lock, or printk in this function
// 2. kmem_mon_alloc is called BEFORE kmem_mon_init
//
static void kmem_mon_alloc(MEM_CLASS_T mem_class, unsigned long caller, const void *addr, 
	size_t bytes_req, size_t bytes_alloc){
	int i, pid, mem_node, ret, len;
	int caller_hash, addr_hash;
	int caller_node, addr_node, prev, next;
	unsigned long flags;
	
	if(!kmem_mon_switch)
		return;
	
	pid = current->pid;

	// for debugging
	if(pid < MAX_PID){
		return;
	}

	if(pid > DEBUG_PID){
		LOG_I("a: %d, %s, addr %x, size: %d\n", current->pid, mem_class_string(mem_class), addr, bytes_alloc);
	}

	spin_lock_irqsave(&kmem_mon_alloc_lock, flags);
	//mutex_lock(&kmem_mon_mutex);
	if(pid < MAX_PROCESS_NUM){
		// If a new pid
		if(process_table[pid].start_idx == 0){
			// fill process_info_t 

			mem_node = allocate_mem_node();
			if(mem_node == -1) {
				// something unexpected happens
				LOG_E("allocate mem node fails\n");
				goto EXIT;
			}
			else
				process_table[pid].start_idx = mem_node;
			
			process_table[pid].pid = current->pid;			
			process_table[pid].tgid = current->tgid;
			//LOG_I("c->c: %d %s\n", sizeof(current->comm), current->comm);
			strncpy(process_table[pid].comm, current->comm, sizeof(current->comm));
			
			//memset(cmdline, 0, sizeof(cmdline));
			//ret = cmd_line(&(process_table[pid].cmdline[0]));
#if 0
			len = sizeof(tmp) > sizeof(process_table[pid].cmdline) ? sizeof(process_table[pid].cmdline) : sizeof(tmp);
			_cmdline(current, tmp);
			strncpy(process_table[pid].cmdline, tmp, len);
#endif

		} else {
			// walk around the list to find mem class belong to the process
			// allocate a new node or return a existing node
			mem_node = find_mem_node(process_table[pid].start_idx, mem_class, TRUE);
			if(mem_node == -1){
				LOG_E("increntable find_mem_node fails\n");
				goto EXIT;
			}
		}
	} else {
		// exceed process table
		LOG_E("exceed process table");
		goto EXIT;
	}
	
#if 1
	//
	// fill kmem_mon_info_t
	// NOTICE: need not fill next_mem_node. It's done in find_mem_node
	//
	mem_info_table[mem_node].mem_class = mem_class;
	// total
	mem_info_table[mem_node].total_bytes_alloc += bytes_alloc;
	mem_info_table[mem_node].total_bytes_req += bytes_req;

	mem_info_table[mem_node].alloc_freq++;

	// peak
	if(mem_info_table[mem_node].peak_every_req < bytes_req){
		mem_info_table[mem_node].peak_every_req = bytes_req;
		mem_info_table[mem_node].peak_caller = caller;
	}
#endif

#ifdef USING_HASH
	if(caller != NULL){
		// find a suitable caller node
		caller_hash = kmem_mon_hash(caller, MAX_CALLER_TABLE);
		//caller_hash = (caller_hash + pid) % MAX_CALLER_TABLE;
		LOG_D("first: caller_hash: %d, caller: %x\n", caller_hash, caller);

		for(i = 0; i < MAX_CALLER_TABLE; i++){
			// a completely new caller
			if(!caller_table[caller_hash].caller_addr){
				LOG_D("caller_table idx: %d is allocated\n", caller_hash);
				break;
			}
			// a existing caller: pid exists.
			else if(caller_table[caller_hash].caller_addr == caller && caller_table[caller_hash].pid == pid){
				LOG_D("find existing caller & pid\n");
				break;
			}
			
			//LOG_D("c: \n");
			LOG_D("c_h: %d, c: %x, c_t[c_h].c_a: %x, p = %d, c_t[c_h].p = %d\n", 
				caller_hash, caller, caller_table[caller_hash].caller_addr, pid, caller_table[caller_hash].pid);
			
			caller_hash = (caller_hash + 1) % MAX_CALLER_TABLE;
		}
		if(i < MAX_CALLER_TABLE) {
			caller_table[caller_hash].caller_addr = caller;
			caller_table[caller_hash].pid = pid;
			caller_table[caller_hash].bytes_req += bytes_req;
			caller_table[caller_hash].bytes_alloc += bytes_alloc;
			caller_table[caller_hash].freq_alloc++;
			caller_table[caller_hash].mem_class = mem_class;
			//add the allocation address
			if(addr != NULL){
				addr_hash = kmem_mon_hash(addr, MAX_ADDR_TABLE);
				for(i = 0; i < MAX_ADDR_TABLE; i++){
					if(!addr_table[addr_hash].caller_hash){
						break;
					}
					addr_hash = (addr_hash + 1) % MAX_ADDR_TABLE;
				}
				if(i < MAX_ADDR_TABLE){
					addr_table[addr_hash].caller_hash = caller_hash;
					addr_table[addr_hash].addr = addr;
				} else {
					LOG_E("a lost addr: %x\n", addr);
				}
			}
		} else {
			// dump caller table
			LOG_E("a lost caller: %x\n", caller);
		}
	}
#endif // #ifdef USING_HASH

#ifdef USING_LIST
	if(caller != NULL){
		// TODO: use hash
		// new caller
		if(mem_info_table[mem_node].caller_start_idx == 0){
			caller_node = allocate_caller_node();
			if(caller_node == -1){
				LOG_E("allocate new caller node fail\n");
				goto EXIT;
			} else
				mem_info_table[mem_node].caller_start_idx  = caller_node;
		} else {
			// find caller node
			caller_node = find_caller_node(mem_info_table[mem_node].caller_start_idx, caller, TRUE);
			if(caller_node == -1){
				LOG_E("increntable find_caller_node fails\n");
				goto EXIT;
			}
		}
		caller_hash = caller_node;
		
		// fulfil caller node
		caller_table[caller_hash].caller_addr = caller;
		caller_table[caller_hash].pid = pid;
		caller_table[caller_hash].bytes_req += bytes_req;
		caller_table[caller_hash].bytes_alloc += bytes_alloc;
		caller_table[caller_hash].freq_alloc++;
		caller_table[caller_hash].mem_class = mem_class;
#if 0
		//add the allocation address
		if(addr != NULL){
			addr_hash = kmem_mon_hash(addr, MAX_ADDR_TABLE);
			for(i = 0; i < MAX_ADDR_TABLE; i++){
				if(!addr_table[addr_hash].caller_hash){
					break;
				}
				addr_hash = (addr_hash + 1) % MAX_ADDR_TABLE;
			}
			if(i < MAX_ADDR_TABLE){
				addr_table[addr_hash].caller_hash = caller_hash;
				addr_table[addr_hash].addr = addr;
			} else {
				LOG_E("a lost addr: %x\n", addr);
			}
		}
#endif

#if 1
		if(addr != NULL){
			addr_hash = hash_32(addr, 12);
			// TODO: the address should not be in the list
			
			// allocate
			addr_node = allocate_addr_node();			
			if(addr_node == -1){
				LOG_E("allocate addr node fail\n");
				goto EXIT;
			}
			addr_table[addr_node].addr = addr;
			addr_table[addr_node].caller_hash = caller_hash;
			
			// insert
			next = addr_idx_table[addr_hash];
			if(next == 0){
				addr_idx_table[addr_hash] = addr_node;
			} else {
				while(next != 0){
					prev = next;
					next = addr_table[prev].next;
				}
				addr_table[prev].next = addr_node;
			}
		}
#endif
	}
#endif

EXIT:
	//return;
	//mutex_unlock(&kmem_mon_mutex);
	spin_unlock_irqrestore(&kmem_mon_alloc_lock, flags);
}

char* mem_class_string(MEM_CLASS_T mem_class){
	switch(mem_class){
		case KMEM_MON_TYPE_KMALLOC:
			return "kmalloc";
		case KMEM_MON_TYPE_KMEM_CACHE:
			return "kmem_cache";
		case KMEM_MON_TYPE_PAGES:
			return "pages";
		case KMEM_MON_TYPE_PMEM:
			return "pmem";
		case KMEM_MON_TYPE_M4U:
			return "m4u";
		case KMEM_MON_TYPE_VMALLOC:
			return "vmalloc";
		case KMEM_MON_TYPE_ASHMEM:
			return "ashmem";
		case KMEM_MON_TYPE_KMALLOCWRAPPER:
			return "kmallocwrapper";
		default:
			return "unkown";
	}
}

//
// seq
// as_start -> as_show -> as_next -> as_next -> ... -> as_stop -> as_start -> as_stop
//

// If return non-NULL value, next() will be invoked.
static void *as_start(struct seq_file *m, loff_t *pos)
{
	int i;
	loff_t n = *pos;
	
	if (!n) {
		// beginning of a new sequence
		seq_puts(m, "\n    pid tgid  cmdline process_name  type  req   alloc free alloc_freq free_freq peak_alloc   peak_caller\n");
		
	}

	for (i = 0; i < MAX_PROCESS_NUM; i++) {
		if(process_table[i].start_idx != 0)
			n--;
		if(n < 0)
			return (void *)(i+1);
	}

	return 0;
}

static void *as_next(struct seq_file *m, void *p, loff_t *pos)
{
	int i = (int)p;
	++*pos;
	
	for (; i < MAX_PROCESS_NUM; i++) {
		if(process_table[i].start_idx != 0)
			return (void *)(i+1);
	}
	
	return NULL;
}

static void as_stop(struct seq_file *m, void *p)
{
}

static int as_show(struct seq_file *m, void *p)
{
	int cur_process = (int)p-1;

	int next_mem_node = process_table[cur_process].start_idx;
	mem_info_t *show;

#ifdef CONFIG_KALLSYMS
	char *mod_name;
	const char *name;
	unsigned long offset = 0, size;
	char name_buf[128];
#endif

	// walk around the process list
	while(next_mem_node != 0){
		show = &(mem_info_table[next_mem_node]);

#ifdef CONFIG_KALLSYMS
		name = kallsyms_lookup((int)show->peak_caller, &size, &offset, &mod_name,
			       name_buf);
// with peak caller info
// but we can only know the peak caller info of kmalloc, kmem_cache_alloc and etc
#if 0
		seq_printf(m, "%5d %5d %-30s %-16s %-20s %10d %10d %10d %10d %10d %10d  %s+0x%lx\n", 
			process_table[cur_process].pid, 
			process_table[cur_process].tgid,
			process_table[cur_process].cmdline,
			process_table[cur_process].comm, 
			mem_class_string(show->mem_class),
			show->total_bytes_req,
			show->total_bytes_alloc,
			show->total_bytes_free,
			show->alloc_freq,
			show->free_freq,
			show->peak_every_req,
			name,
			offset);
#endif

		seq_printf(m, "%5d %5d %-30s %-16s %-20s %10d %10d %10d %10d %10d %10d\n", 
			process_table[cur_process].pid, 
			process_table[cur_process].tgid,
			process_table[cur_process].cmdline,
			process_table[cur_process].comm, 
			mem_class_string(show->mem_class),
			show->total_bytes_req,
			show->total_bytes_alloc,
			show->total_bytes_free,
			show->alloc_freq,
			show->free_freq,
			show->peak_every_req);


#ifdef DEBUG
		LOG_D("caller symbol: %s\n", name);
		if(name == NULL)
			LOG_D("caller: 0x%lx\n", show->peak_caller);
#endif // #ifdef DEBUG

#else
	LOG_I("CONFIG_KALLSYMS is not defined ");
#endif // #ifdef CONFIG_KALLSYMS

		next_mem_node = show->next_mem_node;
	}

	return 0;
}

struct seq_operations kmem_mon_seq_op = {
	.start	= as_start,
	.next	= as_next,
	.stop	= as_stop,
	.show	= as_show,
};

static int kmem_mon_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &kmem_mon_seq_op);
}

static struct file_operations proc_kmem_mon_operations = {
	.open		= kmem_mon_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

//
// /proc/kmem_mon/switch
//
static int switch_read(char *page, char **start, off_t off,
			       int count, int *eof, void *data) {
	char *p = page;
	int len = 0;
	LOG_D("Enter switch_read\n");
	p += sprintf(p, "%d\n", kmem_mon_switch);

	*start = page + off;

	len = p - page;
	if (len > off)
	    len -= off;
	else
	    len = 0;

	return len < count ? len  : count;
}

static int switch_write (struct file *file, const char *buffer,
					unsigned long count, void *data)
{
	char switch_buf = 0;

	LOG_D("Enter switch_write\n");

	if(copy_from_user((void *)&switch_buf, (const void __user *)buffer, sizeof(char))) {
		LOG_E("switch_write copy_from_user fails\n");
		return 0;
	}

	switch(switch_buf) {
	case '0':
		kmem_mon_switch = 0;
		break;
	case '1':
		kmem_mon_switch = 1;
		break;
	default:
		LOG_E("unknown proc parameter: %c\n", switch_buf);
		break;
	}

    return count;
}

//
//
//
static void *caller_start(struct seq_file *m, loff_t *pos) {
	int i;
	loff_t n = *pos;
	
	if (!n) {
		// beginning of a new sequence
		seq_puts(m, "\n caller bytes_req bytes_alloc bytes_free freq_alloc freq_free \n");
	}

	// move index to the start position
	for (i = 0; i < MAX_CALLER_TABLE; i++) {
		if(caller_table[i].caller_addr != 0)
			n--;
		if(n < 0)
			return (void *)(i+1);
	}

	return 0;

}

static void *caller_next(struct seq_file *m, void *p, loff_t *pos) {
	int i = (int)p;
	++*pos;
	
	for (; i < MAX_CALLER_TABLE; i++) {
		if(caller_table[i].caller_addr != 0)
			return (void *)(i+1);
	}
	
	return NULL;

}

static void caller_stop(struct seq_file *m, void *p) {

}

static int caller_show(struct seq_file *m, void *p) {
		int cur_caller = (int)p-1;
	
#ifdef CONFIG_KALLSYMS
		char *mod_name;
		const char *name;
		unsigned long offset = 0, size;
		char name_buf[128];
#endif
		if(caller_table[cur_caller].pid == kmem_mon_pid){	
#ifdef CONFIG_KALLSYMS
			name = kallsyms_lookup((int)(caller_table[cur_caller].caller_addr), &size, &offset, &mod_name,
					   name_buf);

			seq_printf(m, "%10d %10d %10d %10d %10d %10s %10s %s+0x%lx\n", 
				caller_table[cur_caller].bytes_req,
				caller_table[cur_caller].bytes_alloc,
				caller_table[cur_caller].bytes_free,
				caller_table[cur_caller].freq_alloc,
				caller_table[cur_caller].freq_free,
				mem_class_string(caller_table[cur_caller].mem_class),
				mod_name,
				name,
				offset);
#else
		LOG_I("CONFIG_KALLSYMS is not defined ");
#endif // #ifdef CONFIG_KALLSYMS
		}		
	
		return 0;

}

struct seq_operations caller_seq_op = {
	.start	= caller_start,
	.next	= caller_next,
	.stop	= caller_stop,
	.show	= caller_show,
};

static int caller_open(struct inode *inode, struct file *file) {
	return seq_open(file, &caller_seq_op);
}

static struct file_operations proc_caller_operations = {
	.open		= caller_open, // NOT NECESSARY
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release		= seq_release,
};

//
// /proc/kmem_mon/pid
//
static int pid_read(char *page, char **start, off_t off,
			       int count, int *eof, void *data) {
	char *p = page;
	int len = 0;
	LOG_D("Enter pid_read\n");
	p += sprintf(p, "%d\n", kmem_mon_pid);

	*start = page + off;

	len = p - page;
	if (len > off)
	    len -= off;
	else
	    len = 0;

	return len < count ? len  : count;
}

static int pid_write(struct file *file, const char *buffer,
					unsigned long count, void *data) {
	char pid_buf[MAX_PID_LEN];
	int i = 0;
	
	LOG_D("Enter switch_write\n");

	if(copy_from_user((void *)&pid_buf, (const void __user *)buffer, sizeof(pid_buf))) {
		LOG_E("pid_write copy_from_user fails\n");
		return 0;
	}

	pid_buf[MAX_PID_LEN - 1] = 0;
	LOG_I("write pid: %s", pid_buf);
	kmem_mon_pid = 0;
	for (i = 0; i < MAX_PID_LEN; i++) {
		if(pid_buf[i] <= '9' && pid_buf[i] >= '0'){
			kmem_mon_pid = kmem_mon_pid * 10 + (pid_buf[i] - '0');
			LOG_I("p_b: %c\n", pid_buf[i]);
		} else if (pid_buf[i] == 0)
			break;
	}
	LOG_I("kmem_mon_pid: %d\n", kmem_mon_pid);
    return count;
}

static int __init kmem_mon_init(void)
{
	int rc = 0;
	struct proc_dir_entry *switch_file;
	struct proc_dir_entry *pid_file;
	struct proc_dir_entry *caller_file;

	//
	// calculate array size 
	//
	int size = sizeof(mem_info_table) + sizeof(process_table) + sizeof(addr_idx_table)
		+ sizeof(caller_table) + sizeof(addr_table) + sizeof(tmp);
	LOG_I("total array size: %d bytes\n", size);
	
	// create /proc/kmem_mon
	proc_kmem_mon_dir = proc_mkdir("kmem_mon", NULL);
	
	//
	// create /proc/kmem_mon/general
	//
	proc_create("general", S_IRUGO, proc_kmem_mon_dir, &proc_kmem_mon_operations);

	// /proc/kmem_mon/switch
	switch_file = create_proc_entry("switch", 0, proc_kmem_mon_dir);
	if (switch_file) {
		switch_file->read_proc = switch_read;
		switch_file->write_proc = switch_write;
	} else
		LOG_E("create_proc_entry switch file fails\n");

	// /proc/kmem_mon/pid
	pid_file = create_proc_entry("pid", 0, proc_kmem_mon_dir);
	if (pid_file) {
		pid_file->read_proc = pid_read;
		pid_file->write_proc = pid_write;
	} else
		LOG_E("create_proc_entry pid file fails\n");

	// /proc/kmem_mon/caller
	proc_create("caller", S_IRUGO, proc_kmem_mon_dir, &proc_caller_operations);
	LOG_I("kmem_mon_init\n");
	return rc;
}

static void __exit kmem_mon_exit(void)
{
	return;
	//
	// need free static memory?
	//
	
	//
	// clear proc entry
	//
    remove_proc_entry("general", proc_kmem_mon_dir);
    remove_proc_entry("kmem_mon", NULL);

	LOG_I("kmem_mon_exit");
}

#ifdef USING_MODULE
//
// This module should be installed earlier.
//
module_init(kmem_mon_init);
module_exit(kmem_mon_exit);

MODULE_LICENSE("GPL");
#else

__initcall(kmem_mon_init);
#endif

#else
// kmalloc
// ksize ONLY can be used in kmalloc and kmem_cache_alloc
void kmem_mon_kmalloc(unsigned long caller, const void *addr, int bytes_req, int bytes_alloc){
}
void kmem_mon_kfree(unsigned long caller, const void *addr){
}

// kmem_cache_alloc
void kmem_mon_kmem_cache_alloc(unsigned long caller, const void *addr, 
	size_t bytes_req, size_t bytes_alloc){
}
void kmem_mon_kmem_cache_free(unsigned long caller, const void *addr){
}


// PMEM
void kmem_mon_pmem_alloc(int req, int alloc){
}
void kmem_mon_pmem_free(int size){
}

// M4U
void kmem_mon_m4u_alloc(int req, int alloc){
}
void kmem_mon_m4u_dealloc(const unsigned int addr, const unsigned int req_size){
}

// vmalloc
void kmem_mon_vmalloc(unsigned long caller, const void *addr, int bytes_req, int bytes_alloc){
}
void kmem_mon_vfree(int size){
}

// ashmem
// NOTICE: size need not be aligned?
// ONLY record  the requiring size
void kmem_mon_ashmem_mmap(int size){
}
void kmem_mon_ashmem_release(int size){
}

// ONLY record the requiring size
// TODO: account the failure
void kmem_mon_kmallocwrapper(unsigned long caller, int size){
}
void kmem_mon_kfreewrapper(const void *addr){
}

#endif //#ifdef CONFIG_MTPROF_KMEM

EXPORT_SYMBOL(kmem_mon_kmalloc);
EXPORT_SYMBOL(kmem_mon_kfree);
EXPORT_SYMBOL(kmem_mon_pmem_alloc);
EXPORT_SYMBOL(kmem_mon_pmem_free);
EXPORT_SYMBOL(kmem_mon_m4u_alloc);
EXPORT_SYMBOL(kmem_mon_m4u_dealloc);
EXPORT_SYMBOL(kmem_mon_vmalloc);
EXPORT_SYMBOL(kmem_mon_vfree);
EXPORT_SYMBOL(kmem_mon_ashmem_mmap);
EXPORT_SYMBOL(kmem_mon_ashmem_release);
EXPORT_SYMBOL(kmem_mon_kmallocwrapper);
EXPORT_SYMBOL(kmem_mon_kfreewrapper);
