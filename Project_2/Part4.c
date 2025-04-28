/*
Part 4
After each time slice, MCP must act like a mini top, reading /proc, 
picking out meaningful info, and printing it cleanly.
You are adding live system monitoring to your scheduler

1	After each scheduler cycle (each time slice), gather information from 
    /proc for every workload process.
2	Pick useful information like: execution time, memory usage, I/O stats.
3	Analyze and nicely format the output (not just raw dump).
4	Display an updated table of stats every cycle (kind of like Linux top).
5	Continue gathering and showing stats until all processes finish.
*/