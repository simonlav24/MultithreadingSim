import sys

file = open(sys.argv[1], 'r')

LOAD_LATENCY = 0
STORE_LATENCY = 0
OVERHEAD = 0
THREAD_NUM = 0

class Thread:
	"""thread class for parsing purposes"""
	def __init__(self, num, offset):
		self.num = num
		self.offset = offset
		self.program = []
	def __str__(self):
		string = "T" + str(self.num) + " " + hex(self.offset) + ": " + str(self.program)
		return string
	def __repr__(self):
		return str(self)

# parse input file
dataOffset = 0
threadsPrograms = []
thread = -1
segment = "start"
dataArr = []

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
				dataOffset = int(line[line.index('@')+1:], 16)
				continue
			t = Thread(thread, int(line[line.index('@')+1:], 16))
			threadsPrograms.append(t)
			continue
		threadsPrograms[thread].program.append(line)
	
	if segment == "data":
		dataArr.append(line)

# print parsed info
print(LOAD_LATENCY, STORE_LATENCY, OVERHEAD, THREAD_NUM)
print(threadsPrograms)
print(hex(dataOffset))
print(dataArr)

