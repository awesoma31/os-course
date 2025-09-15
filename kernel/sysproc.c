#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_dump(void)
{
  return dump();
}

extern struct proc proc[NPROC];

static struct proc* find_by_pid(int pid) {
  struct proc *p;
  for (p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    int found = (p->pid == pid && p->state != UNUSED);
    release(&p->lock);
    if (found) return p;
  }
  return 0;
}

static int is_ancestor_or_self(struct proc *caller, struct proc *target) {
  if (caller == target) return 1;
  struct proc *p = target;

  while (1) {
    acquire(&p->lock);
    struct proc *pp = p->parent;
    release(&p->lock);

    if (pp == 0) return 0;
    if (pp == caller) return 1;

    p = pp;
  }
}

uint64
sys_dump2(void)
{
  int pid;
  int regnum;
  uint64 uret;

  argint(0, &pid);
  argint(1, &regnum);
  argaddr(2, &uret);

  if (regnum < 2 || regnum > 11) return -3;

  struct proc *caller = myproc();
  struct proc *target = find_by_pid(pid);
  if (!target) return -2;

  if (!is_ancestor_or_self(caller, target))
    return -1;

  uint64 val = 0;
  acquire(&target->lock);
  struct trapframe *tf = target->trapframe;
  if (tf == 0) {
    release(&target->lock);
    return -2;
  }

  switch (regnum) {
    case 2:  val = tf->s2;  break;
    case 3:  val = tf->s3;  break;
    case 4:  val = tf->s4;  break;
    case 5:  val = tf->s5;  break;
    case 6:  val = tf->s6;  break;
    case 7:  val = tf->s7;  break;
    case 8:  val = tf->s8;  break;
    case 9:  val = tf->s9;  break;
    case 10: val = tf->s10; break;
    case 11: val = tf->s11; break;
    default:
      release(&target->lock);
      return -3;
  }
  release(&target->lock);

  if (copyout(caller->pagetable, uret, (char*)&val, sizeof(val)) < 0)
    return -4;

  return 0;
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
