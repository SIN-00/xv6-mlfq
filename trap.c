#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;
void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }

    struct proc *p = myproc();
    if (p && p->state == RUNNING) {
      p->cpu_burst++;
      p->cpu_used++;
    }
    acquire(&ptable.lock);
    for(p = &ptable.proc[NPROC-1]; p >= ptable.proc; p--){
      if(p->state == RUNNABLE) {
        p->cpu_wait++;
      }
      if(p->state == SLEEPING) {
        p->io_wait_time++;
      }
        if(p->cpu_wait >= 250 && p->q_level > 0&&p->pid >3) {
          p->q_level--;  // Aging
          p->cpu_wait = 0;
#ifdef DEBUG
	   cprintf("PID: %d Aging\n",
                        p->pid);
#endif
        }
      }
    release(&ptable.lock);

    lapiceoi();
    break;

  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    cprintf("pid %d %s: trap %d err %d on cpu %d eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }
 
     if(myproc() && myproc()->end_time != -1 && myproc()->cpu_used >= myproc()->end_time) {
#ifdef DEBUG
	     cprintf("PID: %d uses %d ticks in mlfq[%d], total(%d/%d)\n",
            myproc()->pid, myproc()->cpu_burst, myproc()->q_level,
            myproc()->cpu_used, myproc()->end_time);
#endif
#ifdef DEBUG
            cprintf("PID: %d, used %d ticks, terminated\n", myproc()->pid, myproc()->cpu_used);
#endif

      myproc()->killed = 1;
      exit();
    }

    if(myproc() && myproc()->cpu_burst >= time_quantum[myproc()->q_level]) {
      if(myproc()->pid > 3){
            #ifdef DEBUG
	    cprintf("PID: %d uses %d ticks in mlfq[%d], total(%d/%d)\n",
              myproc()->pid, myproc()->cpu_burst, myproc()->q_level,
              myproc()->cpu_used, myproc()->end_time);
#endif
      }
      yield();  
    }
    if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}



