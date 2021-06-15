import sys

file = open(sys.argv[1], 'r')

REGS_COUNT = 8
MEM_DATA_SIZE = 100

LOAD_LATENCY = 0
STORE_LATENCY = 0
OVERHEAD = 0
THREAD_NUM = 0

# opcode enum:
CMD_NOP   = 0
CMD_ADD   = 1  # dst <- src1 + src2
CMD_SUB   = 2  # dst <- src1 - src2
CMD_ADDI  = 3  # dst <- src1 + imm
CMD_SUBI  = 4  # dst <- src1 - imm
CMD_LOAD  = 5  # dst <- Mem[src1 + src2]  (src2 may be an immediate)
CMD_STORE = 6  # Mem[dst + src2] <- src1  (src2 may be an immediate)
CMD_HALT  = 7 

instDict = {CMD_NOP:"nop", CMD_ADD:"add", CMD_SUB:"sub", CMD_ADDI:"addi", CMD_SUBI:"subi", CMD_LOAD:"load", CMD_STORE:"store", CMD_HALT:"halt"}

# TODO
# what if overhead > 1

class Thread:
	"""thread class for parsing purposes"""
	def __init__(self, num, offset):
		self.num = num
		self.offset = offset
		self.program = []
		
		self.ready = True
		self.active = True
		self.pc = 0
		self.context = None
		
		self.waitCycle = 0
	
	def updateWaitCycles(self, cycles):
		if self.waitCycle > 0:
			self.waitCycle -= 1
			if self.waitCycle == 0:
				self.ready = True
	
	def __str__(self):
		string = "T" + str(self.num) + " " + hex(self.offset) + ": " + str(self.program)
		return string
	def __repr__(self):
		return str(self)
	def printState(self):
		print("[", self.num, ":", self.waitCycle, "]")
	def printRegs(self):
		print("Register file thread id", self.num, ":")
		print(self.context)

class Instruction:
	"""Instruction class as seen in core_api.h"""
	def __init__(self):
		self.opcode = 0
		self.dst_index = 0
		self.src1_index = 0
		self.src2_index_imm = 0
		self.isSrc2Imm = False
		
class tcontext:
	""" registers array per thread as seen in core_api.h"""
	def __init__(self):
		self.regs = [0] * REGS_COUNT
	def __str__(self):
		string = '\t'
		for i, val in enumerate(self.regs):
			string += "R" + str(i) + " = " + hex(val) + "\t"
		return string
	def __repr__(self):
		return str(self)

def SIM_MemInstRead(line, tid):
	string = threadsPrograms[tid].program[line]
	dst = Instruction()
	string = string.split()
	# print(string)
	
	if string[0] == 'LOAD':
		dst.opcode = CMD_LOAD
		dst.dst_index = int(string[1][1:-1])
		dst.src1_index = int(string[2][1:-1])
		if string[3][0] == '$':
			dst.src2_index_imm = int(string[3][1:-1])
			dst.isSrc2Imm = False
		else:
			dst.src2_index_imm = int(string[3], 16)
			dst.isSrc2Imm = True
	elif string[0] == 'STORE':
		dst.opcode = CMD_STORE
		dst.dst_index = int(string[1][1:-1])
		dst.src1_index = int(string[2][1:-1])
		if string[3][0] == '$':
			dst.src2_index_imm = int(string[3][1:-1])
			dst.isSrc2Imm = False
		else:
			dst.src2_index_imm = int(string[3], 16)
			dst.isSrc2Imm = True
	elif string[0] == 'ADD':
		dst.opcode = CMD_ADD
		dst.dst_index = int(string[1][1:-1])
		dst.src1_index = int(string[2][1:-1])
		dst.src2_index_imm = int(string[3][1:])
		dst.isSrc2Imm = False
	elif string[0] == 'ADDI':
		dst.opcode = CMD_ADDI
		dst.dst_index = int(string[1][1:-1])
		dst.src1_index = int(string[2][1:-1])
		dst.src2_index_imm = int(string[3], 16)
		dst.isSrc2Imm = True
	elif string[0] == 'SUB':
		dst.opcode = CMD_SUB
		dst.dst_index = int(string[1][1:-1])
		dst.src1_index = int(string[2][1:-1])
		dst.src2_index_imm = int(string[3][1:-1])
		dst.isSrc2Imm = False
	elif string[0] == 'SUBI':
		dst.opcode = CMD_SUBI
		dst.dst_index = int(string[1][1:-1])
		dst.src1_index = int(string[2][1:-1])
		dst.src2_index_imm = int(string[3], 16)
		dst.isSrc2Imm = True
	elif string[0] == 'HALT':
		dst.opcode = CMD_HALT
		# $0 - ???
	elif string[0] == 'NOP':
		dst.opcode = CMD_NOP
	return dst

def loadFromMem(address):
	index = address - dataOffset
	return dataArr[index]
	
def storeToMem(address, value):
	index = address - dataOffset
	dataArr[index] = value

def executeInst(inst, threadId):
	if inst.opcode == CMD_ADD:
		param1 = threadsPrograms[threadId].context.regs[inst.src1_index]
		param2 = threadsPrograms[threadId].context.regs[inst.src2_index_imm]
		threadsPrograms[threadId].context.regs[inst.dst_index] = param1 + param2
	elif inst.opcode == CMD_ADDI:
		param1 = threadsPrograms[threadId].context.regs[inst.src1_index]
		param2 = inst.src2_index_imm
		threadsPrograms[threadId].context.regs[inst.dst_index] = param1 + param2
	elif inst.opcode == CMD_SUB:
		param1 = threadsPrograms[threadId].context.regs[inst.src1_index]
		param2 = threadsPrograms[threadId].context.regs[inst.src2_index_imm]
		threadsPrograms[threadId].context.regs[inst.dst_index] = param1 - param2
	elif inst.opcode == CMD_SUBI:
		param1 = threadsPrograms[threadId].context.regs[inst.src1_index]
		param2 = inst.src2_index_imm
		threadsPrograms[threadId].context.regs[inst.dst_index] = param1 - param2
	elif inst.opcode == CMD_LOAD:
		base = threadsPrograms[threadId].context.regs[inst.src1_index]
		offset = inst.src2_index_imm
		loaded = loadFromMem(base + offset)
		threadsPrograms[threadId].context.regs[inst.dst_index] = loaded
	elif inst.opcode == CMD_STORE:
		base = threadsPrograms[threadId].context.regs[inst.dst_index]
		offset = inst.src2_index_imm
		value = threadsPrograms[threadId].context.regs[inst.src1_index]
		storeToMem(base + offset, value)

S_COMMAND = 0
S_EVENT = 1
S_OVERHEAD = 2
S_IDLE = 3

def CORE_BlockedMT():
	currentThreadId = 0
	totalCycles = 0
	overheadingCounter = 0
	state = S_COMMAND
	totalInst = 0
	
	while True:
		# if totalCycles > 30:
			# break
		
		# check if any active
		if not any([i.active for i in threadsPrograms]):
			break
			
		# check if current is ready
		if not threadsPrograms[currentThreadId].ready:
			state = S_IDLE
		
		if state == S_COMMAND:
			# fetch instruction:
			currentPc = threadsPrograms[currentThreadId].pc
			currentInst = SIM_MemInstRead(currentPc, currentThreadId)
			executeInst(currentInst, currentThreadId)
			threadsPrograms[currentThreadId].pc += 1
			print(totalCycles, ":", currentThreadId, ":", instDict[currentInst.opcode])
			
			if currentInst.opcode in [CMD_STORE, CMD_LOAD, CMD_HALT]:
				state = S_EVENT
			
			totalCycles += 1
			totalInst += 1
			[i.updateWaitCycles(OVERHEAD) for i in threadsPrograms]
		
		elif state == S_EVENT:
			threadsPrograms[currentThreadId].ready = False
			# thread wait
			if currentInst.opcode == CMD_STORE:
				threadsPrograms[currentThreadId].waitCycle = STORE_LATENCY
			if currentInst.opcode == CMD_LOAD:
				threadsPrograms[currentThreadId].waitCycle = LOAD_LATENCY
			if currentInst.opcode == CMD_HALT:
				threadsPrograms[currentThreadId].active = False
				
			# try to context switch
			readyThreads = [i.ready for i in threadsPrograms]
			if any(readyThreads):
				for i in range(THREAD_NUM):
					candidateThread = (currentThreadId + i + 1) % THREAD_NUM
					if threadsPrograms[candidateThread].ready:
						currentThreadId = candidateThread
						state = S_OVERHEAD
						overheadingCounter = OVERHEAD
						# print("--- context switch on", totalCycles, "to:", currentThreadId, "---")
						break
			# else:
				# print("---no ready threads, continuing with current---")

		elif state == S_OVERHEAD:
			print(totalCycles, ":", " ", ":", "overhead")
			totalCycles += 1
			[i.updateWaitCycles(OVERHEAD) for i in threadsPrograms]
			overheadingCounter -= 1
			if overheadingCounter == 0:
				state = S_COMMAND
		
		elif state == S_IDLE:
			print(totalCycles, ":", " ", ":", "idle")
			totalCycles += 1
			[i.updateWaitCycles(OVERHEAD) for i in threadsPrograms]
			
			if threadsPrograms[currentThreadId].ready:
				state = S_COMMAND
				continue
			
			readyThreads = [i.ready for i in threadsPrograms]
			if any(readyThreads):
				for i in range(THREAD_NUM):
					candidateThread = (currentThreadId + i + 1) % THREAD_NUM
					if threadsPrograms[candidateThread].ready:
						if currentThreadId == candidateThread:
							state = S_COMMAND
						else:
							currentThreadId = candidateThread
						state = S_OVERHEAD
						overheadingCounter = OVERHEAD
						# totalCycles += OVERHEAD; print("cycle after overhead:", totalCycles)
						# print("--- context switch on", totalCycles, "to:", currentThreadId, "---")
						break
			# else:
				# print("---no ready threads, continuing with current---")
			
	
	print("---- Blocked MT Simulation ----\n")
	for i in range(THREAD_NUM):
		threadsPrograms[i].printRegs()
	print("Blocked MT CPI for this program", totalCycles / totalInst)
			
################################################################################## MAIN


# parse input file
threadsPrograms = []
thread = -1
segment = "start"
dataArr = [0] * MEM_DATA_SIZE
dataOffset = 0

for line in file:
	if '#' in line:
		line = line[:line.index('#')]
	line = line.replace("\n", "")
	line = line.strip()
	if line == "":
		continue
	
	if segment == "start":
		if line[0] == 'L':
			LOAD_LATENCY = int(line[1:])
		if line[0] == 'S':
			STORE_LATENCY = int(line[1:])
		if line[0] == 'O':
			OVERHEAD = int(line[1:])
		if line[0] == 'N':
			THREAD_NUM = int(line[1:])
	
	if 'T' in line and segment == "start":
		segment = "threads"
		continue
		
	if segment == "threads":
		if '@' in line:
			thread += 1
			if thread == THREAD_NUM:
				segment = "data"
				dataIndex = 0
				dataOffset = int(line[line.index('@')+1:], 16)
				continue
			t = Thread(thread, int(line[line.index('@')+1:], 16))
			t.context = tcontext()
			threadsPrograms.append(t)
			continue
		threadsPrograms[thread].program.append(line)
	
	if segment == "data":
		if '0x' in line:
			dataArr[dataIndex] = int(line, 16)
			dataIndex += 1
		else:
			dataArr[dataIndex] = int(line)
			dataIndex += 1

# print parsed info
if False:
	print(LOAD_LATENCY, STORE_LATENCY, OVERHEAD, THREAD_NUM)
	print(threadsPrograms)
	print(hex(dataOffset))
	print(dataArr)

CORE_BlockedMT()










