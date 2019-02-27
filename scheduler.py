import random
import math
from enum import Enum
import statistics

PROCESS_COUNT = 50

class State(Enum):
	PREREADY = 0
	PAUSED = 1
	BLOCKED = 2
	FINISHED = 3
	TERMINATED = 4

class Process:
	def __init__(self, index, priority=None, max_time=None):
		self.index = index

		if (priority != None and max_time != None):
			self.priority = priority
			self.time_left = max_time
		else:
			self.priority = random.randint(0, 3)
			self.time_left = random.randint(400, 40000) # in ms

		self.instructions = []
		for i in range(random.randint(1000, 100000)):
			self.instructions.append(random.randint(1, 20)) # in us

		# self.position = 0
		# self.memory = random.randint(10, 200)

		self.state = State.PREREADY

		self.init_time = -1 # first time execute was called
		self.last_start = 0 # last time
		self.end_time = 0 # when execute finished
		self.unblock_time = 0 # clock time at which to unblock
		self.max_interval = 0 # max time between calls to execute

	def __str__(self):
		return f'Process #{self.index} ({self.state})'
		# return 'Process #' + str(self.index) + str(self.state)

	def execute(self, clock):
		time = 0

		if self.init_time == -1:
			self.init_time = clock
		self.last_start = clock

		self.max_interval = max(self.max_interval, self.last_start - self.end_time)

		while (self.instructions):
			instruction = self.instructions.pop(0) / 1000
			self.time_left -= instruction
			time += instruction

			if self.time_left <= 0: # kill process if time limit exceeded
				self.state = State.TERMINATED
				break

			if time > 10: # max runtime (quantum) for one execution
				self.state = State.PAUSED
				break

			if (random.randint(1, 100) <= 2): # 2% chance for block
				self.state = State.BLOCKED
				delay = 0
				if (random.randint(1, 100) <= 90):  # 90% chance for 4-100 ms
					delay = random.randint(math.sqrt(4), math.sqrt(100))
				else: #10 % chance for 100, 100000
					delay = random.randint(math.sqrt(100), int(math.sqrt(100000)))
				self.unblock_time = clock + time + delay * delay # exponential delay
				break

		if not self.instructions:
			self.state = State.FINISHED

		# clock += time
		self.end_time = clock + time
		return time

if __name__ == "__main__":
	clock = 0
	next_process_at = 0

	priority_queues = [[], [], [], []] # queues for the four priorities
	blocked_queue = [] # processes that have been blocked
	process_count = 0

	# re: stats
	runtimes = [[], [], [], []]
	max_intervals = [[], [], [], []]
	terminated = [0, 0, 0, 0]
	success = [0, 0, 0, 0]

	# every priority is given twice the runtime as the next one
	order = [0, 1, 0, 2, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0] # 0 = highest
	current_priority = -1

	# run scheduler until all processes are done
	while any(q for q in priority_queues) \
	or blocked_queue or process_count < PROCESS_COUNT:
		# cycle through the priority order
		current_priority = (current_priority + 1) % len(order)

		# check if a blocked process is ready to run
		if blocked_queue:
			process = blocked_queue[0]
			if clock >= process.unblock_time or not all(q for q in priority_queues):
				clock = max(clock, process.unblock_time)
				blocked_queue.pop(0)
				process.state = State.PAUSED
				priority_queues[process.priority].append(process)

		# select the queue for current priority
		processes = priority_queues[order[current_priority]]
		if processes:
			process = processes[0]
			if process.state in [State.PREREADY, State.PAUSED]:
				# dequeue from ready queue, might enqueue again later
				processes.pop(0)

				# run the process and advance the clock
				clock += process.execute(clock)

				# process was killed for taking too long
				if process.state == State.TERMINATED:
					terminated[process.priority] += 1

				# I/O block; add to the correct spot in blocked_queue
				if process.state == State.BLOCKED:
					blocked_queue.append(process)
					blocked_queue.sort(key=lambda x: x.unblock_time)
					
				# exceeded quantum (10 ms), enqueue in the back
				if process.state == State.PAUSED:
					processes.append(process)

				# finished successfully, calculate stats
				if process.state == State.FINISHED:
					runtimes[process.priority].append(process.end_time - process.init_time)
					max_intervals[process.priority].append(process.max_interval)
					success[process.priority] += 1

		# new process every 20-10000 ms
		if process_count < PROCESS_COUNT and not processes and not blocked_queue:
			clock += next_process_at
		if clock >= next_process_at and process_count < PROCESS_COUNT:
			p = Process(process_count)
			processes.append(p)
			process_count += 1
			next_process_at = clock + random.randint(20, 10000)

	# final stats
	for i in range(4):
		if len(runtimes[i]) == 1:
			runtimes[i].append(runtimes[i][0])
			max_intervals[i].append(max_intervals[i][0])
		print(f'Priority {i}:')
		print('-----------')
		print(f'Average runtime: {round(statistics.mean(runtimes[i]), 2)} ms')
		print(f'Standard deviation: {round(statistics.stdev(runtimes[i]), 2)} ms')
		print(f'Average max interval: {round(statistics.mean(max_intervals[i]), 2)} ms')
		print(f'Standard deviation: {round(statistics.stdev(max_intervals[i]), 2)} ms')
		print(f'Processes finished successfully: {success[i]}')
		print(f'Processes terminated: {terminated[i]}')
		print()
