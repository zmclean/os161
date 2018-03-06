#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/wait.h>
#include <lib.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <copyinout.h>
#include <mips/trapframe.h>
#include <synch.h>
  /* this implementation of sys__exit does not do anything with the exit code */
  /* this needs to be fixed to get exit() and waitpid() working properly */

void sys__exit(int exitcode) {
	//thread_exit(exitcode);
	
  struct addrspace *as;
  struct proc *p = curproc;
 
  (void)exitcode;

  DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

  KASSERT(curproc->p_addrspace != NULL);
  as_deactivate();
 
   /* clear p_addrspace before calling as_destroy. Otherwise if
   * as_destroy sleeps (which is quite possible) when we
   * come back we'll be calling as_activate on a
   * half-destroyed address space. This tends to be
   * messily fatal.
   */
  as = curproc_setas(NULL);
  as_destroy(as);

  /* detach this thread from its process */
 /* note: curproc cannot be used after this call */
  proc_remthread(curthread);

  /* if this is the last user process in the system, proc_destroy()
     will wake up the kernel menu thread */
  proc_destroy(p);
  
  thread_exit();
  /* thread_exit() does not return, so we should never get here */
  panic("return from thread_exit in sys_exit\n");

}


/* stub handler for getpid() system call                */
int
sys_getpid(pid_t *retval)
{
	*retval = curproc->pid;
   return(0);
}

/* stub handler for waitpid() system call                */

int
sys_waitpid(pid_t pid,
	    userptr_t status,
	    int options,
	    pid_t *retval)
{
  int exitstatus;
  int result;
/*
  
     exit status of 0, regardless of the actual exit status of
     the specified process.   
     In fact, this will return 0 even if the specified process
     is still running, and even if it never existed in the first place.

     Fix this!
  */
  if (options != 0) {
    return(EINVAL);
  }
 
  exitstatus = 0;
  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
  return(0);
}

static struct semaphore *tsem;

void
uproc_thread (void *temp_tr, unsigned long k);

void
uproc_thread (void *temp_tr, unsigned long k) 
{
	as_activate();
	struct trapframe tf = *((struct trapframe*)temp_tr);	

	tf.tf_epc += 4;
	tf.tf_v0 = k;
	tf.tf_a3 = 0;
	kfree(temp_tr);

	mips_usermode(&tf);
	KASSERT(curproc->p_addrspace != NULL);

	thread_exit();
}

int
sys_fork(struct trapframe *tf, pid_t *retval)
{
	struct trapframe *temp_tf;
	struct proc *proc;
	struct addrspace *child_vmspace = NULL;
	int err;

	if (tsem==NULL) {
		tsem = sem_create("tsem", 0);
		if (tsem == NULL) {
			panic("Sem creation failed.\n");
		}
	}

	KASSERT(child_vmspace == NULL);
	as_copy(curproc->p_addrspace, &child_vmspace);
	if(child_vmspace == NULL) {
		kprintf("sys_fork: as_copy failed %s\n", strerror(ENOMEM));
		return ENOMEM;
	}

	proc = proc_create_fork("forkproc");
	DEBUG(DB_SYSCALL, "Syscall: sys_fork()\n");

	proc->p_addrspace = child_vmspace;
	proc->pid = 2;
	temp_tf = kmalloc(sizeof(struct trapframe));
	if(temp_tf == NULL) {
		return ENOMEM;
	}

	*temp_tf = *tf;
	err = thread_fork(curthread->t_name, proc, uproc_thread, temp_tf, *retval);

	if (err) {
		return err;
	}
	
	proc->pid +=1;
	*retval = proc->pid;
	return(0);
}
