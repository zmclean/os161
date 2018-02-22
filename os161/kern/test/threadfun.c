#include <types.h>
#include <lib.h>
#include <thread.h>
#include <synch.h>
#include <test.h>
#include <spinlock.h>
static struct semaphore *tsem = NULL;
static struct lock *lock = NULL;
static struct spinlock spl; 
static int counter = 0;
static int threadcount = 0;
//
// Sem and Lock Initilization
//
static
void
init_items(void)
{
	if (tsem == NULL) {
		tsem = sem_create("tsem", 0);
		if (tsem == NULL) {
			panic("threadfun: sem_create failed\n");
		}
	}
	if (lock == NULL) {
		lock = lock_create("lock");
		if (lock == NULL) {
			panic("threadfun: lock_create failed\n");
		}
	}
	spinlock_init(&spl);
}

//
// Programs
//
//My Lab 5 and 6 function
static
void
printDigits(void *junk, unsigned long num)
{
	(void)junk;
	(void)num;

	kprintf("%d ", counter);

	lock_acquire(lock);
	counter++;
	lock_release(lock);

	V(tsem);
	threadcount--;
}

//Project 1 Programs
//
static
void
unsafethread(void* junk, unsigned long num) 
{
	(void)junk;

	//kprintf("%d ", counter);
	for(int i = 0; i < (int)num; i++)
	{
		counter++;
	}

	V(tsem);
	threadcount--;
}

static
void
safethread(void* junk, unsigned long num)
{
	(void)junk;
	
	for (int i = 0; i < (int)num; i++)
	{
		lock_acquire(lock);	
		counter++;
		lock_release(lock);
	}

	V(tsem);
	threadcount--;
}

static
void
spinlockthread(void* junk, unsigned long num)
{
	(void)junk;

	for (int i = 0; i < (int)num; i++)
	{
		spinlock_acquire(&spl);
		counter++;
		spinlock_release(&spl);
	}

	V(tsem);
	threadcount--;
	
}

// Gets the arguments and puts them into int form
int * getarguments(int nargs, char **args) 
{
	(void)nargs;
	static int values[2];
	char *tempargs = args[1];

	values[0] = atoi(tempargs);
	tempargs = args[2];
	values[1] = atoi(tempargs);

	kprintf("\nValue 1: %d\nValue 2: %d\n", values[0], values[1]);

	return values;
}

//
//Runs the thread tests
//
static
void
runthreads(int count, int incrcount, int testchoice)
{
	char name[16];
	int tsemdecr, result;
	int threadscreated = 0;
	bool purge = false;
	
	void (*myfunc)();
	
	// Chooses which thread type to run.
	if (testchoice == 0)
	{
		myfunc = printDigits;
	}
	else if (testchoice == 1)
	{
		myfunc = unsafethread;
	}
	else if (testchoice == 2)
	{
		myfunc = safethread;
	}
	else
	{
		myfunc = spinlockthread;
	}

	while(threadscreated != count)
	{
		// "Thread Creation" case.
		// Creates a thread up to 44 times at a time. It will then enter a purge 
		// case, then exit once the thread count reaches zero again. This is to 
		// not overload the os161.
		if (threadcount < 44 && !purge)
		{
			result = thread_fork(name, NULL, myfunc, NULL, incrcount);
			if (result) 
			{
				panic("threadfun: thread_fork failed %s)\n", strerror(result));
			}
			threadcount++;
			threadscreated++;
		}
		// "Purge Exit" case.
		// Enters if the int representation of the current amount of threads has
		// been decreased to zero, and purge is still true. This sets purge to
		// false, which forces the loop out of the purge case and back to normal
		// operation.  
		else if (threadcount == 0 && purge)
		{
			purge = false;
		}
		// "Purge" case.
		// Enters if the amount of threads created (and possible still running)
		// is equal to or greater thatn 44. Entering this is simply to waste time,
		// allowing threads to run their course before creating new ones.
		// This also sets the "purge" variable to true, and decrements the int
		// representation of current threads by 1 each time.
		else
		{
			purge = true;
		}
	}

	// Loops an amount of times equal to the amount of threads created,
	// and decrements the semaphore count each time
	tsemdecr = 0;
	while (tsemdecr <= threadscreated - 1)
	{
		P(tsem);
		tsemdecr++;
	}

	kprintf("\nNumber is now: %d\n\n", counter);
	counter = 0;

	// An attempt to clean the locks
	/*
	spinlock_cleanup(&spl);
	sem_destroy(tsem);
	lock_destroy(lock);
	*/
}

//
// Tests
//
// These are what are called at the menu command line
// Each function gets the command line arguments, splits them into a count and an increment values via the use of the "getarguments" function,
// 	then puts them into the runthreads function. It also puts into the runthreads function an int value that will determine which thread program
// 	the runthread function will perform.

//Simple digit print from lab
int
digitThreadTest(int nargs, char **args)
{
	(void)nargs;
	int count;
	
	int *values = getarguments(nargs, args);
	count = values[0];

	kprintf("Num should be: %d\n", count);
	kprintf("\nStart Fun Thread.\n");

	init_items();
	runthreads(count, 0, 0);
	
	kprintf("\nFun thread done.\n");
	
	
	return 0;
}

//The unsafe thread program caller. This is the one that will cause value corruption.
int unsafethreadcounter(int nargs, char **args)
{
	(void)nargs;
	int count;
	int incarg;

	int *values = getarguments(nargs, args);
	
	count = values[0];
	incarg = values[1];
	
	kprintf("Num should be: %d\n", (count * incarg));

	init_items();
	runthreads(count, incarg, 1);
	
	return 0;
}

//The locked thread program caller. Safely increments the counter without corrupt via a lock.
//Slower than spinlock and unsafe, but less processor intensive.
int lockthreadcounter(int nargs, char **args)
{
	(void)nargs;

	int count;
	int incarg;
	int *values = getarguments(nargs, args);

	count = values[0];
	incarg = values[1];

	kprintf("Num should be: %d\n", (count * incarg));

	init_items();
	runthreads(count, incarg, 2);

	return 0;
}
//Spinlock thread caller. Safely increments the counter without corruption via a spinlock.
//Faster than a lock, but more processor intensive.
int spinlockthreadcounter(int nargs, char **args)
{
	(void)nargs;
	int count;
	int incarg;

	int *values = getarguments(nargs, args);

	count = values[0];
	incarg = values[1];

	kprintf("Num should be: %d\n", (count * incarg));
	
	init_items();
	runthreads(count, incarg, 3);

	return 0;
}
