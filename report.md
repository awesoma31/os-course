## Цель работы
Цель работы состояла в том, чтобы разобраться с межпроцессным взаимодействием, а также, как добавлять новые системные вызовы в ядро. Нужно было реализовать простую пользовательскую программу pingpong, написать системный вызов dump для вывода содержимого регистров текущего процесса и системный вызов dump2 для возврата значения регистра произвольного процесса в пространство пользователя.

## Ход работы

### Часть 1
Реализована пользовательская программа pingpong в **user/pingpong.c**. Она создаёт пайп, делает fork и организует обмен строками "ping" и "pong" между дочерним и родительским процессами:

```C
#include "kernel/types.h"
#include "user/user.h"

int main(void) {
  int pipka[2]; // pipe
  char buf[8];

  if (pipe(pipka) < 0) {
    fprintf(2, "pipe error\n");
    exit(1);
  }

  int pid = fork();
  if (pid == 0) { // ch
    if (read(pipka[0], buf, 4) != 4) {
      fprintf(2, "child read error\n");
      exit(1);
    }
    printf("%d: got %s\n", getpid(), buf);

    if (write(pipka[1], "pong", 4) != 4) {
      fprintf(2, "child write error\n");
      exit(1);
    }

    close(pipka[0]);
    close(pipka[1]);
    exit(0);
  } else { // parent -- batyok
    if (write(pipka[1], "ping", 4) != 4) {
      fprintf(2, "parent write error\n");
      exit(1);
    }

    wait(0);  
    
    if (read(pipka[0], buf, 4) != 4) {
      fprintf(2, "parent read error\n");
      exit(1);
    }
    printf("%d: got %s\n", getpid(), buf);

    close(pipka[0]);
    close(pipka[1]);
    exit(0);
  }
}
```

### Часть 2,3

В файле **kernel/defs.h** были добавлены объявления новых функций ядра:
```c
// dump
int dump(void);
int dump2(int, int, uint64*);
```

В **kernel/proc.c** реализован системный вызов dump. Он получает текущий процесс и последовательно печатает содержимое регистров s2–s11:

```C
int dump() {
  struct proc* p = myproc();
  struct trapframe *tf = p->trapframe;

  uint64* cur_reg = &tf->s2;
  for (int i = 0; i < 10; i++) {
    printf("s%d = %d\n", i+2, (uint)*cur_reg++);
  }

  return 0;
}
```

Для реализации dump2 в **kernel/proc.c** добавлены функции поиска процесса по pid и проверки, является ли вызывающий процесс предком целевого.

```C
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
```

Использование блокировки ```p->lock``` необходимо, чтобы избежать race condition при доступе к полям процесса. Функция ```is_ancestor_or_self``` поднимается по цепочке родителей, чтобы проверить, может ли вызывающий процесс читать регистры:

```C
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
```

Основная функция ```dump2``` проверяет корректность аргументов, ищет подходящий процесс и возвращает регистр в user space с помощью copyout:

```C
int dump2(int pid, int regnum, uint64 *uret) {
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

  if (copyout(caller->pagetable, (uint64)uret, (char*)&val, sizeof(val)) < 0)
    return -4;

  return 0;
}
```

Здесь использование copyout обязательно, так как ядро не может напрямую писать в адресное пространство процесса. Возможные ошибки возвращаются кодами от -1 до -4.

В **kernel/syscall.c** добавлены объявления и подключение функций в таблицу системных вызовов:

```C
extern uint64 sys_dump(void);
extern uint64 sys_dump2(void);

[SYS_dump]    sys_dump,
[SYS_dump2]   sys_dump2,
```

В **kernel/syscall.h** зарегистрированы номера вызовов:

```C    
#define SYS_dump   22
#define SYS_dump2  23
```

В **kernel/sysproc.c** реализованы обёртки, которые извлекают аргументы и вызывают функции ядра:

```C
uint64 sys_dump(void) {
  return dump();
}

uint64 sys_dump2(void) {
  int pid;
  int regnum;
  uint64 uret;

  argint(0, &pid);
  argint(1, &regnum);
  argaddr(2, &uret);

  return dump2(pid, regnum, (uint64*)uret);
}
```

В **user/user.h** были добавлены объявления системных вызовов для пользовательских программ:

```C
int dump(void);
int dump2(int pid, int reg, uint64 *ret);
```

В **user/usys.pl** добавлены генерацию asm кода для вызова system call:

```C
entry("dump");
entry("dump2");
```

### Выводы

В ходе лабораторной работы была реализована пользовательская программа, использующая базовые системные вызовы для организации обмена данными между процессами. На её примере стало ясно, как в Xv6 работают pipe. Далее были добавлены новые системные вызовы dump и dump2. Для dump достаточно было получить trapframe текущего процесса и напечатать значения регистров. Для dump2 пришлось учесть вопросы безопасности и синхронизации: доступ разрешён только к себе и потомкам, а обращение к данным процесса защищается локом p->lock. Результат возвращается в пространство пользователя через copyout, так как ядро не может напрямую писать в память приложения. Таким образом, работа позволила понять полную цепочку интеграции системного вызова: от объявления в user space и генерации перехода до обработки в ядре и доступа к структурам процесса. Это закрепило понимание различий между user space и kernel space.