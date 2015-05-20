#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static thread_func start_process NO_RETURN;

static bool load (const char *cmd_line, void (**eip) (void), void **esp);

/* Data structure shared between process_execute() in the
   invoking thread and start_process() in the newly invoked
   thread. */
struct exec_info
  {
    const char *file_name;              /* Program to load. */
    struct semaphore load_done;         /* Up-ed when loading complete. */
    struct wait_status *wait_status;    /* Child process. */
    bool success;                       /* Program successfully loaded? */
  };

/* Starts a new thread running a user program loaded from
   FILENAME.  The new thread may be scheduled (and may even exit)
   before process_execute() returns.  Returns the new process's
   thread id, or TID_ERROR if the thread cannot be created. */
tid_t
process_execute (const char *file_name)
{
  struct exec_info exec;
  char thread_name[16];
  tid_t tid;

  /* Initialize exec_info. */
  exec.file_name = file_name;

  sema_init (&exec.load_done, 0);

  /* =======================
     Code to be added here to copy file_name to thread_name.
     Note that the file_name may contain command-line arguments
     that should not be included in the thread_name.
     ======================== */ 
   int i =0 ;
   for ( i =0 ; file_name[i] != ' ' && i < 16 ; i++ ){
	thread_name[i] = file_name[i] ;	
   }	


  /* start_process is the function to be called at the beginning of a new thread,
       &exec is a pointer to the auxiliary information */
  tid = thread_create (thread_name, PRI_DEFAULT, start_process, &exec);

  /* =======================
     Code to be added here to handle the case when tid != TID_ERROR.
     Several things to do here:
     1. Wait for the new user program to be loaded successfully by start_process.
     2. check whether the execution status is successful.
     3. If successful, put the wait_status of the child process into the children list.
     4. Otherwise, tid = TID_ERROR.
     ======================== */
     sema_down(&exec.load_done); //1
     if ( exec.success){ //2
     	struct thread *t = thread_current() ;
     	list_push_back(&t->children , &exec.wait_status->elem);//3
     	
     }else { return TID_ERROR ; } //4 
  return tid;
}

/* A thread function that loads a user process and starts it running. */
static void start_process (void *exec_)
{
  struct exec_info *exec = exec_;
  struct intr_frame if_;
  bool success;

  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;
  success = load (exec->file_name, &if_.eip, &if_.esp);

  /* =======================
     Code to be added here to handle the case when loading is successful.
     Several things to do here:
     1. If loading is successful, allocate kernel memory space to hold the wait_status, using malloc().
     2. Properly initialize the struct wait_status.
     3. Indicate in the execution status that loading is successful.
     4. Stop the parent process from waiting for the child process to be loaded.
     ======================== */
  if (success)
  {   
	struct thread *t = thread_current();
  	exec->wait_status = malloc (sizeof(&t->wait_status )); 
  	lock_init(&exec->wait_status->lock);//#2 
  	sema_init(&exec->wait_status-> dead , 0 ); // 0 live 1 dead
	exec->wait_status->ref_cnt = 2 ; // child and parent alive
  	exec->success = true ;  // #3
  	sema_up(&exec->load_done); // #4 
  }

  if (!success)
    thread_exit ();

  /* Start the user process by simulating a return from an
     interrupt, implemented by intr_exit (in
     threads/intr-stubs.S).  Because intr_exit takes all of its
     arguments on the stack in the form of a `struct intr_frame',
     we just point the stack pointer (%esp) to our stack frame
     and jump to it. */
  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
  NOT_REACHED ();
}

/* Releases one reference to CS and, if it is now unreferenced, frees it. */
static void release_child (struct wait_status *cs)
{
   lock_acquire(&cs->lock);
   cs->ref_cnt-- ;
   lock_release(&cs->lock);
   
   if (cs->ref_cnt-- == 0)
   free(cs);
	
	
}

/* Waits for thread TID to die and returns its exit status.  If
   it was terminated by the kernel (i.e. killed due to an
   exception), returns -1.  If TID is invalid or if it was not a
   child of the calling process, or if process_wait() has already
   been successfully called for the given TID, returns -1
   immediately, without waiting. */
int
process_wait (tid_t child_tid)
{
  /* =======================
      Code to be added here to perform the following
      1. Go through each child one by one.
      2. Check whether a child process is the one to be waited for.
      3. Wait for the child process to become dead.
      4. Obtain exit code from the wait_status of the child process.
      5. Call release_child().
      6. Return the exit code.
      ======================== */
  struct thread *t = thread_current(); 
  struct list_elem *e;
   for (e = list_begin (&t->children); e != list_end (&t->children);
           e = list_next (e)) // #1
        {
          struct wait_status *child_wait_status = list_entry (e, struct wait_status, elem); 
  			 if (&child_wait_status->tid == child_tid){ //#2
  			 	sema_down ( &child_wait_status->dead );//#3
  			 //	(&child.wait_status)->exit_code = 1 ;
				list_remove(e) ;   
  			 	release_child (&child_wait_status);//#5
  			 	return &child_wait_status->exit_code;//#6
  			 }  	
        }	


  return -1;
}

/* Free the current process's resources. */
void
process_exit (void)
{
  struct thread *cur = thread_current ();
  uint32_t *pd;

  /* Close executable (and allow writes). */
  file_close (cur->bin_file);

  if (cur->wait_status != NULL)
  {
      struct wait_status *cs = cur->wait_status;
      printf ("%s: exit(%d)\n", cur->name, cs->exit_code);
  }

  /* =======================
      Code to be added here to perform the following
      1. Notify the parent process that this process is dead.
      2. Release a reference to the wait_status of this process.
      3. Go through the child list.
      4. Release a reference to the wait_status of each child process.
      ======================== */
  sema_up(&cur->wait_status->dead) ; 
  release_child ( &cur->wait_status);
   struct list_elem *e;
   for (e = list_begin (&cur->children); e != list_end (&cur->children);
           e = list_next (e)) // #1
        {
           struct wait_status *child_wait_status = list_entry (e, struct wait_status, elem);
           release_child (child_wait_status);	
        }
  
  /* Destroy the current process's page directory and switch back
     to the kernel-only page directory. */
  pd = cur->pagedir;
  if (pd != NULL)
    {
      /* Correct ordering here is crucial.  We must set
         cur->pagedir to NULL before switching page directories,
         so that a timer interrupt can't switch back to the
         process page directory.  We must activate the base page
         directory before destroying the process's page
         directory, or our active page directory will be one
         that's been freed (and cleared). */
      cur->pagedir = NULL;
      pagedir_activate (NULL);
      pagedir_destroy (pd);
    }
}

/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void
process_activate (void)
{
  struct thread *t = thread_current ();

  /* Activate thread's page tables. */
  pagedir_activate (t->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_update ();
}

/* We load ELF binaries.  The following definitions are taken
   from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* For use with ELF types in printf(). */
#define PE32Wx PRIx32   /* Print Elf32_Word in hexadecimal. */
#define PE32Ax PRIx32   /* Print Elf32_Addr in hexadecimal. */
#define PE32Ox PRIx32   /* Print Elf32_Off in hexadecimal. */
#define PE32Hx PRIx16   /* Print Elf32_Half in hexadecimal. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
   This appears at the very beginning of an ELF binary. */
struct Elf32_Ehdr
  {
    unsigned char e_ident[16];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
  };

/* Program header.  See [ELF1] 2-2 to 2-4.
   There are e_phnum of these, starting at file offset e_phoff
   (see [ELF1] 1-6). */
struct Elf32_Phdr
  {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
  };

/* Values for p_type.  See [ELF1] 2-3. */
#define PT_NULL    0            /* Ignore. */
#define PT_LOAD    1            /* Loadable segment. */
#define PT_DYNAMIC 2            /* Dynamic linking info. */
#define PT_INTERP  3            /* Name of dynamic loader. */
#define PT_NOTE    4            /* Auxiliary info. */
#define PT_SHLIB   5            /* Reserved. */
#define PT_PHDR    6            /* Program header table. */
#define PT_STACK   0x6474e551   /* Stack segment. */

/* Flags for p_flags.  See [ELF3] 2-3 and 2-4. */
#define PF_X 1          /* Executable. */
#define PF_W 2          /* Writable. */
#define PF_R 4          /* Readable. */

static bool setup_stack (const char *cmd_line, void **esp);

static bool validate_segment (const struct Elf32_Phdr *, struct file *);
static bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
                          uint32_t read_bytes, uint32_t zero_bytes,
                          bool writable);

/* Loads an ELF executable from FILE_NAME into the current thread.
   Stores the executable's entry point into *EIP
   and its initial stack pointer into *ESP.
   Returns true if successful, false otherwise. */
bool
load (const char *cmd_line, void (**eip) (void), void **esp)
{
  struct thread *t = thread_current ();
  char file_name[NAME_MAX + 2];
  struct Elf32_Ehdr ehdr;
  struct file *file = NULL;
  off_t file_ofs;
  bool success = false;
  char *cp;
  int i;

  /* Allocate and activate page directory. */
  t->pagedir = pagedir_create ();
  if (t->pagedir == NULL)
    goto done;
  process_activate ();

  /* Extract file_name from command line. */
  while (*cmd_line == ' ')
    cmd_line++;
  strlcpy (file_name, cmd_line, sizeof file_name);

  cp = strchr (file_name, ' ');
  if (cp != NULL)
    *cp = '\0';

  /* Open executable file. */
  t->bin_file = file = filesys_open (file_name);
  if (file == NULL)
    {
      printf ("load: %s: open failed\n", file_name);
      goto done;
    }

  file_deny_write (file);

  /* Read and verify executable header. */
  if (file_read (file, &ehdr, sizeof ehdr) != sizeof ehdr
      || memcmp (ehdr.e_ident, "\177ELF\1\1\1", 7)
      || ehdr.e_type != 2
      || ehdr.e_machine != 3
      || ehdr.e_version != 1
      || ehdr.e_phentsize != sizeof (struct Elf32_Phdr)
      || ehdr.e_phnum > 1024)
    {
      printf ("load: %s: error loading executable\n", file_name);
      goto done;
    }

  /* Read program headers. */
  file_ofs = ehdr.e_phoff;
  for (i = 0; i < ehdr.e_phnum; i++)
    {
      struct Elf32_Phdr phdr;

      if (file_ofs < 0 || file_ofs > file_length (file))
        goto done;
      file_seek (file, file_ofs);

      if (file_read (file, &phdr, sizeof phdr) != sizeof phdr)
        goto done;
      file_ofs += sizeof phdr;
      switch (phdr.p_type)
        {
        case PT_NULL:
        case PT_NOTE:
        case PT_PHDR:
        case PT_STACK:
        default:
          /* Ignore this segment. */
          break;
        case PT_DYNAMIC:
        case PT_INTERP:
        case PT_SHLIB:
          goto done;
        case PT_LOAD:
          if (validate_segment (&phdr, file))
            {
              bool writable = (phdr.p_flags & PF_W) != 0;
              uint32_t file_page = phdr.p_offset & ~PGMASK;
              uint32_t mem_page = phdr.p_vaddr & ~PGMASK;
              uint32_t page_offset = phdr.p_vaddr & PGMASK;
              uint32_t read_bytes, zero_bytes;
              if (phdr.p_filesz > 0)
                {
                  /* Normal segment.
                     Read initial part from disk and zero the rest. */
                  read_bytes = page_offset + phdr.p_filesz;
                  zero_bytes = (ROUND_UP (page_offset + phdr.p_memsz, PGSIZE)
                                - read_bytes);
                }
              else
                {
                  /* Entirely zero.
                     Don't read anything from disk. */
                  read_bytes = 0;
                  zero_bytes = ROUND_UP (page_offset + phdr.p_memsz, PGSIZE);
                }
              if (!load_segment (file, file_page, (void *) mem_page,
                                 read_bytes, zero_bytes, writable))
                goto done;
            }
          else
            goto done;
          break;
        }
    }

  /* Set up stack. */
  if (!setup_stack (cmd_line, esp))
    goto done;

  /* Start address. */
  *eip = (void (*) (void)) ehdr.e_entry;

  success = true;

 done:
  /* We arrive here whether the load is successful or not. */
  return success;
}

/* load() helpers. */

static bool install_page (void *upage, void *kpage, bool writable);

/* Checks whether PHDR describes a valid, loadable segment in
   FILE and returns true if so, false otherwise. */
static bool
validate_segment (const struct Elf32_Phdr *phdr, struct file *file)
{
  /* p_offset and p_vaddr must have the same page offset. */
  if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK))
    return false;

  /* p_offset must point within FILE. */
  if (phdr->p_offset > (Elf32_Off) file_length (file))
    return false;

  /* p_memsz must be at least as big as p_filesz. */
  if (phdr->p_memsz < phdr->p_filesz)
    return false;

  /* The segment must not be empty. */
  if (phdr->p_memsz == 0)
    return false;

  /* The virtual memory region must both start and end within the
     user address space range. */
  if (!is_user_vaddr ((void *) phdr->p_vaddr))
    return false;
  if (!is_user_vaddr ((void *) (phdr->p_vaddr + phdr->p_memsz)))
    return false;

  /* The region cannot "wrap around" across the kernel virtual
     address space. */
  if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
    return false;

  /* Disallow mapping page 0.
     Not only is it a bad idea to map page 0, but if we allowed
     it then user code that passed a null pointer to system calls
     could quite likely panic the kernel by way of null pointer
     assertions in memcpy(), etc. */
  if (phdr->p_vaddr < PGSIZE)
    return false;

  /* It's okay. */
  return true;
}

/* Loads a segment starting at offset OFS in FILE at address
   UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
   memory are initialized, as follows:

        - READ_BYTES bytes at UPAGE must be read from FILE
          starting at offset OFS.

        - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.

   The pages initialized by this function must be writable by the
   user process if WRITABLE is true, read-only otherwise.

   Return true if successful, false if a memory allocation error
   or disk read error occurs. */
static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable)
{
  ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (ofs % PGSIZE == 0);

  file_seek (file, ofs);
  while (read_bytes > 0 || zero_bytes > 0)
    {
      /* Calculate how to fill this page.
         We will read PAGE_READ_BYTES bytes from FILE
         and zero the final PAGE_ZERO_BYTES bytes. */
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;

      /* Get a page of memory. */
      uint8_t *kpage = palloc_get_page (PAL_USER);
      if (kpage == NULL)
        return false;

      /* Load this page. */
      if (file_read (file, kpage, page_read_bytes) != (int) page_read_bytes)
        {
          palloc_free_page (kpage);
          return false;
        }
      memset (kpage + page_read_bytes, 0, page_zero_bytes);

      /* Add the page to the process's address space. */
      if (!install_page (upage, kpage, writable))
        {
          palloc_free_page (kpage);
          return false;
        }

      /* Advance. */
      read_bytes -= page_read_bytes;
      zero_bytes -= page_zero_bytes;
      upage += PGSIZE;
    }
  return true;
}

/* Pushes the SIZE bytes in BUF onto the stack in KPAGE, whose
   page-relative stack pointer is *OFS, and then adjusts *OFS
   appropriately.  The bytes pushed are rounded to a 32-bit
   boundary.

   If successful, returns a pointer to the newly pushed object.
   On failure, returns a null pointer. */
static void * push (uint8_t *kpage, size_t *ofs, const void *buf, size_t size)
{
	return NULL;
}

/* Sets up command line arguments in KPAGE, which will be mapped
   to UPAGE in user space.  The command line arguments are taken
   from CMD_LINE, separated by spaces.  Sets *ESP to the initial
   stack pointer for the process. */
static bool
init_cmd_line (uint8_t *kpage, uint8_t *upage, const char *cmd_line,
               void **esp)
{
    return false;
}

/* Create a minimal stack for T by mapping a page at the
   top of user virtual memory.  Fills in the page using CMD_LINE
   and sets *ESP to the stack pointer. */
static bool
setup_stack (const char *cmd_line, void **esp)
{
  uint8_t *kpage;
  bool success = false;

  /* palloc_get_page is defined in threads/palloc.c. Obtain a single free page,
  from the user pool, and the page is filled with zeroes. */
  kpage = palloc_get_page (PAL_USER | PAL_ZERO);
  if (kpage != NULL)
    {
      uint8_t *upage = ((uint8_t *) PHYS_BASE) - PGSIZE;
      if (install_page (upage, kpage, true))
      {
          success = true;
          *esp = PHYS_BASE - 12;
      }
      else
        palloc_free_page (kpage);
    }
  return success;
}

/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE to the page table.
   If WRITABLE is true, the user process may modify the page;
   otherwise, it is read-only.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   Returns true on success, false if UPAGE is already mapped or
   if memory allocation fails. */
static bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable));
}
