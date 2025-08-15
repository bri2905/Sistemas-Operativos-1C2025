#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>

#define MAX_WAIT_TIME 3
#define MAX_HISTORY_LENGTH 100

void sched_halt(void);

void raise_priority(struct Env *e);

void lower_priority(struct Env *e);

struct sched_stats {
	uint32_t sched_calls;
	uint32_t env_runs[NENV];
	envid_t history[MAX_HISTORY_LENGTH];
	int history_index;
	int history_count;
	uint32_t env_start_time[NENV];
	uint32_t env_end_time[NENV];
	uint32_t global_time;
	uint8_t prev_env_status[NENV];
};

static struct sched_stats stats;

void
init_sched_stats(void)
{
	int i;
	stats.sched_calls = 0;
	stats.history_index = 0;
	stats.history_count = 0;
	stats.global_time = 0;

	for (i = 0; i < NENV; i++) {
		stats.env_runs[i] = 0;
		stats.env_start_time[i] = 0;
		stats.env_end_time[i] = 0;
		stats.prev_env_status[i] = ENV_FREE;
	}
}

void
record_env_run(struct Env *e)
{
	int env_idx = ENVX(e->env_id);

	stats.env_runs[env_idx]++;

	stats.history[stats.history_index] = e->env_id;
	stats.history_index = (stats.history_index + 1) % 100;
	if (stats.history_count < 100)
		stats.history_count++;

	if (stats.env_runs[env_idx] == 1) {
		stats.env_start_time[env_idx] = stats.global_time;
	}
}

void
record_env_exit(struct Env *e)
{
	if (e) {
		int env_idx = ENVX(e->env_id);

		if (stats.env_end_time[env_idx] == 0 &&
		    stats.env_start_time[env_idx] >= 0) {
			stats.env_end_time[env_idx] = stats.global_time;
		}
	}
}

void
print_sched_stats(void)
{
	int i;

	cprintf("\n╔═══════════════════════════════════════════╗\n");
	cprintf("║           SCHEDULER STATISTICS            ║\n");
	cprintf("╚═══════════════════════════════════════════╝\n\n");

	cprintf("Total scheduler calls: %u\n\n", stats.sched_calls);

	cprintf("╔══════════════════════════════════════════════════════════╗"
	        "\n");
	cprintf("║                PROCESS EXECUTION STATS                   "
	        "║\n");
	cprintf("╠═══════╦═══════╦════════════╦════════════╦════════════════╣"
	        "\n");
	cprintf("║ EnvID ║ Runs  ║   Start    ║    End     ║    Runtime     "
	        "║\n");
	cprintf("╠═══════╬═══════╬════════════╬════════════╬════════════════╣"
	        "\n");

	// contenido de la tabla
	for (i = 0; i < NENV; i++) {
		if (stats.env_runs[i] > 0) {
			uint32_t runtime = 0;
			if (stats.env_end_time[i] > 0 &&
			    stats.env_start_time[i] >= 0) {
				runtime = stats.env_end_time[i] -
				          stats.env_start_time[i];
			}


			uint32_t env_id = 0x00001000 + i;

			cprintf("║ %5x ║ %5d ║ %10d ║ %10d ║ %14d ║\n",
			        env_id,
			        stats.env_runs[i],
			        stats.env_start_time[i],
			        stats.env_end_time[i],
			        runtime);
		}
	}
	cprintf("╚═══════╩═══════╩════════════╩════════════╩════════════════╝\n"
	        "\n");

	cprintf("Last %d processes execution history:\n", stats.history_count);
	for (i = 0; i < stats.history_count; i++) {
		int idx = (stats.history_index - stats.history_count + i + 100) %
		          100;
		envid_t env_id = stats.history[idx];
		cprintf("%d: EnvID 0x%08x \n", i, env_id);
	}
	cprintf("\n");
}

// Choose a user environment to run and run it.
void
sched_yield(void)
{
	stats.global_time++;
	stats.sched_calls++;

	int i;
	for (i = 0; i < NENV; i++) {
		// Si un proceso cambió a un estado no activo y antes estaba
		// activo, registrarlo como finalizado
		if ((envs[i].env_status != ENV_RUNNABLE &&
		     envs[i].env_status != ENV_RUNNING) &&
		    (stats.prev_env_status[i] == ENV_RUNNABLE ||
		     stats.prev_env_status[i] == ENV_RUNNING)) {
			record_env_exit(&envs[i]);
		}

		stats.prev_env_status[i] = envs[i].env_status;
	}

#ifdef SCHED_ROUND_ROBIN
	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running. Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.

	int start;
	start = curenv ? ENVX(curenv->env_id) : 0;

	for (i = 1; i <= NENV; i++) {
		int idx = (start + i) % NENV;
		if (envs[idx].env_status == ENV_RUNNABLE) {
			record_env_run(&envs[idx]);
			env_run(&envs[idx]);
		}
	}

	if (curenv && curenv->env_status == ENV_RUNNING) {
		record_env_run(curenv);
		env_run(curenv);
	}

#endif

#ifdef SCHED_PRIORITIES
	// Implement simple priorities scheduling.
	//
	// Environments now have a "priority" so it must be consider
	// when the selection is performed.
	//
	// Be careful to not fall in "starvation" such that only one
	// environment is selected and run every time.

	int start = curenv ? ENVX(curenv->env_id) : 0;
	struct Env *max_e = NULL;

	for (i = 1; i <= NENV; i++) {
		int idx = (start + i) % NENV;
		if (envs[idx].env_status == ENV_RUNNABLE &&
		    (!max_e || envs[idx].env_priority > max_e->env_priority)) {
			max_e = &envs[idx];
		}
		if (envs[idx].env_wait_time >= MAX_WAIT_TIME) {
			raise_priority(&envs[idx]);
			envs[idx].env_wait_time = 0;
		}
		envs[idx].env_wait_time++;
	}
	if (max_e) {
		lower_priority(max_e);
		max_e->env_wait_time = 0;
		record_env_run(max_e);
		env_run(max_e);
	}
	if (curenv && curenv->env_status == ENV_RUNNING) {
		lower_priority(curenv);
		curenv->env_wait_time = 0;
		record_env_run(curenv);
		env_run(curenv);
	}

#endif

	// sched_halt never returns
	sched_halt();
}

void
raise_priority(struct Env *e)
{
	if (e->env_priority < 5) {
		e->env_priority++;
	}
}

void
lower_priority(struct Env *e)
{
	if (e->env_priority > 0) {
		e->env_priority--;
	}
}

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		// Mostrar estadísticas antes de entrar en el monitor
		print_sched_stats();

		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile("movl $0, %%ebp\n"
	             "movl %0, %%esp\n"
	             "pushl $0\n"
	             "pushl $0\n"
	             "sti\n"
	             "1:\n"
	             "hlt\n"
	             "jmp 1b\n"
	             :
	             : "a"(thiscpu->cpu_ts.ts_esp0));
}