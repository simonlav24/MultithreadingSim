/* 046267 Computer Architecture - Spring 2021 - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>

void resetRegs(tcontext context) {
	for (int i = 0; i < REGS_COUNT; i++) {
		context.reg[i] = 0;
	}
}

class Thread {
public:
	int num;
	int offset;

	bool ready;
	bool active;
	int pc;
	tcontext context;

	int waitCycle;

	Thread(int num, int offset):num(num), offset(offset) {
		ready = true;
		active = true;
		pc = 0;
		resetRegs(context);
		waitCycle = 0;
	}
	Thread() {}

	void updateWaitCycles() {
		if (waitCycle > 0) {
			waitCycle--;
			if (waitCycle == 0)
				ready = true;
		}
	}
};

bool isAny(Thread* threads, bool active) {
	for (int i = 0; i < SIM_GetThreadsNum(); i++) {
		if (active && threads[i].active)
			return true;
		else if (!active && threads[i].ready)
			return true;
	}
	return false;
}

void updateWaitCycles4All() {
	for (int i = 0; i < SIM_GetThreadsNum(); i++) {
		threadPrograms[i].updateWaitCycles();
	}
}
	

void executeInst(Instruction inst, int threadId) {
	int param1, param2, base, offset, loaded, value;
	switch (inst.opcode) {
	case CMD_ADD:
		param1 = threadPrograms[threadId].context.reg[inst.src1_index];
		param2 = threadPrograms[threadId].context.reg[inst.src2_index_imm];
		threadPrograms[threadId].context.reg[inst.dst_index] = param1 + param2;
		break;
	case CMD_ADDI:
		param1 = threadPrograms[threadId].context.reg[inst.src1_index];
		param2 = inst.src2_index_imm;
		threadPrograms[threadId].context.reg[inst.dst_index] = param1 + param2;
		break;
	case CMD_SUB:
		param1 = threadPrograms[threadId].context.reg[inst.src1_index];
		param2 = threadPrograms[threadId].context.reg[inst.src2_index_imm];
		threadPrograms[threadId].context.reg[inst.dst_index] = param1 - param2;
		break;
	case CMD_SUBI:
		param1 = threadPrograms[threadId].context.reg[inst.src1_index];
		param2 = inst.src2_index_imm;
		threadPrograms[threadId].context.reg[inst.dst_index] = param1 - param2;
		break;
	case CMD_LOAD:
		base = threadPrograms[threadId].context.reg[inst.src1_index];
		offset = inst.src2_index_imm;
		loaded = 0;
		SIM_MemDataRead(base + offset, &loaded);
		threadPrograms[threadId].context.reg[inst.dst_index] = loaded;
		break;
	case CMD_STORE:
		base = threadPrograms[threadId].context.reg[inst.dst_index];
		offset = inst.src2_index_imm;
		value = threadPrograms[threadId].context.reg[inst.src1_index];
		SIM_MemDataWrite(base + offset, value);
		break;
	}
}

/////////////////////////////////////////
Thread* threadPrograms;

typedef enum {
	S_COMMAND = 0,
	S_IDLE,     
	S_EVENT,    
	S_OVERHEAD  
} state;

void CORE_BlockedMT() {
	threadPrograms = new Thread[SIM_GetThreadsNum()];
	int currentThreadId = 0;
	int totalCycles = 0;
	int	overheadingCounter = 0;
	state currentState = S_COMMAND;
	int	totalInst = 0;

	while (true) {
		// check if any active
		if (!isAny(threadPrograms, true))
			break;

		// check if current is ready
		if (!threadPrograms[currentThreadId].ready)
			currentState = S_IDLE;

		switch (currentState) {
		case S_COMMAND:
			// fetch instruction :
			int currentPc = threadPrograms[currentThreadId].pc;
			Instruction currentInst;
			SIM_MemInstRead(currentPc, &currentInst, currentThreadId);
			executeInst(currentInst, currentThreadId);
			threadPrograms[currentThreadId].pc++;
			//print(totalCycles, ":", currentThreadId, ":", instDict[currentInst.opcode])

			if (currentInst.opcode == CMD_STORE || currentInst.opcode == CMD_LOAD || currentInst.opcode == CMD_HALT]) {
				currentState = S_EVENT;
			}

			totalCycles ++;
			totalInst ++;
			updateWaitCycles4All();
			break;

		case S_EVENT:
			/// CONTINUE HERE D
			break;
		}
	}



}

void CORE_FinegrainedMT() {
}

double CORE_BlockedMT_CPI(){
	return 0;
}

double CORE_FinegrainedMT_CPI(){
	return 0;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
}
