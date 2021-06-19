/* 046267 Computer Architecture - Spring 2021 - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>
#include <iostream>

#define DEBUG if(true) std::cout

// class for thread data
class Thread {
public:
	int num;
	int offset;
	bool ready;
	bool active;
	int pc;
	int reg[REGS_COUNT];
	int waitCycle;

	Thread(int num, int offset):num(num), offset(offset) {
		ready = true;
		active = true;
		pc = 0;
		for (int i = 0; i < REGS_COUNT; i++) {
			reg[i] = 0;
		}
		waitCycle = 0;
	}
	Thread() {}

	// initializer
	void init() {
		ready = true;
		active = true;
		pc = 0;
		for (int i = 0; i < REGS_COUNT; i++) {
			reg[i] = 0;
		}
		waitCycle = 0;
	}

	// decrement wait cycles in store/load
	void updateWaitCycles() {
		if (waitCycle > 0) {
			waitCycle--;
			if (waitCycle == 0)
				ready = true;
		}
	}

	// debug printing
	void printRegs() {
		for (int i = 0; i < REGS_COUNT; i++) {
			printf("%d ", reg[i]);
		}
		printf("\n");
	}
};

// struct for holding stats
typedef struct {
	double cpiBlocked;
	double cpiFineGrained;
}Stats;

// initiate globals
Stats stats;
Thread* threadProgramsBlocked;
Thread* threadProgramsFineGrained;

// checks if any of the threads are active or ready
bool isAny(Thread* threads, bool active) {
	for (int i = 0; i < SIM_GetThreadsNum(); i++) {
		if (active && threads[i].active)
			return true;
		else if (!active && threads[i].ready)
			return true;
	}
	return false;
}

// update wait cycles for all threads
void updateWaitCycles4All(Thread* threadPrograms) {
	for (int i = 0; i < SIM_GetThreadsNum(); i++) {
		threadPrograms[i].updateWaitCycles();
	}
}
	
// execute instruction 
void executeInst(Instruction inst, int threadId, Thread* threadPrograms) {
	int param1, param2, base, offset, loaded, value;
	switch (inst.opcode) {
	case CMD_ADD:
		param1 = threadPrograms[threadId].reg[inst.src1_index];
		param2 = threadPrograms[threadId].reg[inst.src2_index_imm];
		threadPrograms[threadId].reg[inst.dst_index] = param1 + param2;
		break;
	case CMD_ADDI:
		param1 = threadPrograms[threadId].reg[inst.src1_index];
		param2 = inst.src2_index_imm;
		threadPrograms[threadId].reg[inst.dst_index] = param1 + param2;
		break;
	case CMD_SUB:
		param1 = threadPrograms[threadId].reg[inst.src1_index];
		param2 = threadPrograms[threadId].reg[inst.src2_index_imm];
		threadPrograms[threadId].reg[inst.dst_index] = param1 - param2;
		break;
	case CMD_SUBI:
		param1 = threadPrograms[threadId].reg[inst.src1_index];
		param2 = inst.src2_index_imm;
		threadPrograms[threadId].reg[inst.dst_index] = param1 - param2;
		break;
	case CMD_LOAD:
		base = threadPrograms[threadId].reg[inst.src1_index];
		offset = inst.src2_index_imm;
		loaded = 0;
		SIM_MemDataRead(base + offset, &loaded);
		threadPrograms[threadId].reg[inst.dst_index] = loaded;
		break;
	case CMD_STORE:
		base = threadPrograms[threadId].reg[inst.dst_index];
		offset = inst.src2_index_imm;
		value = threadPrograms[threadId].reg[inst.src1_index];
		SIM_MemDataWrite(base + offset, value);
		break;
	default:
		break;
	}
}

// state machine enums
typedef enum {
	S_COMMAND = 0,
	S_IDLE,     
	S_EVENT,    
	S_OVERHEAD  
} state;

void CORE_BlockedMT() {
	// allocate and initialize threads
	threadProgramsBlocked = new Thread[SIM_GetThreadsNum()];
	for (int i = 0; i < SIM_GetThreadsNum(); i++) {
		threadProgramsBlocked[i].init();
	}
	int currentThreadId = 0;
	int totalCycles = 0;
	int	overheadingCounter = 0;
	state currentState = S_COMMAND;
	int	totalInst = 0;
	int currentPc;
	Instruction currentInst;
	bool anyReady = false;
	int candidateThread;

	// main os loop
	while (true) {
		// check if any active
		if (!isAny(threadProgramsBlocked, true))
			break;

		// check if current is ready
		if (!threadProgramsBlocked[currentThreadId].ready)
			currentState = S_IDLE;

		switch (currentState) {
		case S_COMMAND:
			// fetch instruction
			currentPc = threadProgramsBlocked[currentThreadId].pc;
			SIM_MemInstRead(currentPc, &currentInst, currentThreadId);

			// execute instruction
			executeInst(currentInst, currentThreadId, threadProgramsBlocked);
			threadProgramsBlocked[currentThreadId].pc++;
			
			// if store/load/halt trigger event for context switch
			if (currentInst.opcode == CMD_STORE || currentInst.opcode == CMD_LOAD || currentInst.opcode == CMD_HALT) {
				currentState = S_EVENT;
			}

			// count cycles
			totalCycles ++;
			totalInst ++;
			updateWaitCycles4All(threadProgramsBlocked);
			break;

		case S_EVENT:
			// thread wait cycles based on instruction
			threadProgramsBlocked[currentThreadId].ready = false;
			
			switch (currentInst.opcode) {
			case CMD_STORE:
				threadProgramsBlocked[currentThreadId].waitCycle = SIM_GetStoreLat();
				break;
			
			case CMD_LOAD:
				threadProgramsBlocked[currentThreadId].waitCycle = SIM_GetLoadLat();
				break;
			
			case CMD_HALT:
				threadProgramsBlocked[currentThreadId].active = false;
				break;
			default:
				break;
			}

			// try to context switch
			anyReady = isAny(threadProgramsBlocked, false);

			if (anyReady) {
				for (int i = 0; i < SIM_GetThreadsNum(); i++) {
					candidateThread = (currentThreadId + i + 1) % SIM_GetThreadsNum();
					if (threadProgramsBlocked[candidateThread].ready) {
						currentThreadId = candidateThread;
						currentState = S_OVERHEAD;
						overheadingCounter = SIM_GetSwitchCycles();
						break;
					}
				}
			}
			break;

		case S_OVERHEAD:
			totalCycles++;
			updateWaitCycles4All(threadProgramsBlocked);
			overheadingCounter--;
			if (overheadingCounter == 0)
				currentState = S_COMMAND;
			break;

		case S_IDLE:
			totalCycles++;
			updateWaitCycles4All(threadProgramsBlocked);

			if (threadProgramsBlocked[currentThreadId].ready) {
				currentState = S_COMMAND;
				continue;
			}

			anyReady = isAny(threadProgramsBlocked, false);
			if (anyReady) {
				for (int i = 0; i < SIM_GetThreadsNum(); i++) {
					candidateThread = (currentThreadId + i + 1) % SIM_GetThreadsNum();
					if (threadProgramsBlocked[candidateThread].ready) {
						if (currentThreadId == candidateThread)
							currentState = S_COMMAND;
						else
							currentThreadId = candidateThread;
						currentState = S_OVERHEAD;
						overheadingCounter = SIM_GetSwitchCycles();
						break;
					}
				}
			}
			break;
		default:
			break;
		}
	}

	// record
	stats.cpiBlocked = (double)totalCycles / (double)totalInst;

}

void CORE_FinegrainedMT() {
	// allocate and initialize threads
	threadProgramsFineGrained = new Thread[SIM_GetThreadsNum()];
	for (int i = 0; i < SIM_GetThreadsNum(); i++) {
		threadProgramsFineGrained[i].init();
	}

	int currentThreadId = 0;
	int totalCycles = 0;
	int totalInst = 0;
	state currentState = S_COMMAND;
	int currentPc;
	Instruction currentInst;
	bool anyReady;
	int candidateThread;

	while (true) {
		// check if any active
		if (!isAny(threadProgramsFineGrained, true))
			break;

		switch (currentState) {
		case S_COMMAND:
			// fetch instruction :
			currentPc = threadProgramsFineGrained[currentThreadId].pc;
			SIM_MemInstRead(currentPc, &currentInst, currentThreadId);
			executeInst(currentInst, currentThreadId, threadProgramsFineGrained);
			threadProgramsFineGrained[currentThreadId].pc++;

			if (currentInst.opcode == CMD_STORE || currentInst.opcode == CMD_LOAD || currentInst.opcode == CMD_HALT)
				threadProgramsFineGrained[currentThreadId].ready = false;
			// thread wait
			switch (currentInst.opcode) {
			case CMD_STORE:
				threadProgramsFineGrained[currentThreadId].waitCycle = SIM_GetStoreLat() + 1;
				break;

			case CMD_LOAD:
				threadProgramsFineGrained[currentThreadId].waitCycle = SIM_GetLoadLat() + 1;
				break;

			case CMD_HALT:
				threadProgramsFineGrained[currentThreadId].active = false;
				break;
			default:
				break;
			}
			
			totalCycles++;
			totalInst++;
			updateWaitCycles4All(threadProgramsFineGrained);

			// switch on
			anyReady = isAny(threadProgramsFineGrained, false);
			if (anyReady) {
				for (int i = 0; i < SIM_GetThreadsNum(); i++) {
					candidateThread = (currentThreadId + i + 1) % SIM_GetThreadsNum();
					if (threadProgramsFineGrained[candidateThread].ready) {
						currentThreadId = candidateThread;
						break;
					}
				}
			}
			else
				currentState = S_IDLE;
			break;

		case S_IDLE:

			totalCycles++;
			updateWaitCycles4All(threadProgramsFineGrained);

			if (threadProgramsFineGrained[currentThreadId].ready) {
				currentState = S_COMMAND;
				continue;
			}
			anyReady = isAny(threadProgramsFineGrained, false);
			if (anyReady) {
				for (int i = 0; i < SIM_GetThreadsNum(); i++) {
					candidateThread = (currentThreadId + i + 1) % SIM_GetThreadsNum();
					if (threadProgramsFineGrained[candidateThread].ready) {
						currentState = S_COMMAND;
						currentThreadId = candidateThread;
						break;
					}
				}
			}
			break;
		default:
			break;
		}
	}
	// add to record
	stats.cpiFineGrained = (double)totalCycles / (double)totalInst;
}

double CORE_BlockedMT_CPI(){
	delete[] threadProgramsBlocked;
	return stats.cpiBlocked;
}

double CORE_FinegrainedMT_CPI(){
	delete[] threadProgramsFineGrained;
	return stats.cpiFineGrained;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
	for (int i = 0; i < REGS_COUNT; i++) {
		context[threadid].reg[i] = threadProgramsBlocked[threadid].reg[i];
	}
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
	for (int i = 0; i < REGS_COUNT; i++) {
		context[threadid].reg[i] = threadProgramsFineGrained[threadid].reg[i];
	}
}
