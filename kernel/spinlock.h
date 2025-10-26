#ifndef KERNEL_SPINLOCK_H
#define KERNEL_SPINLOCK_H

// Mutual exclusion lock.
struct spinlock {
  uint locked;       

  // For debugging:
  char *name;        
  struct cpu *cpu;   // The cpu holding the lock.
};

#endif