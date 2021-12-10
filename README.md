# data-cache-simulator
program which simulates the behavior of a data cache

The program will read a trace of references from standard input and then produce the statistics surrounding the trace to standard output. First the configuration of the data cache is calculated through analyzing the trace.config file within the respository. This cache simulator uses an LRU replacement algorithm. The cache simulator output will first print about the cache configurations used and then information for each reference. 
