# SO1
Primeiro projeto da cadeira de sistemas operativos - 2º ano de faculdade

IST Key Value Store (IST-KVS)

Description

IST-KVS is a key-value storage system that enables the creation, reading, and writing of key-value pairs. The system stores data in a hash table and supports concurrent access, synchronization, and non-blocking backups.

This project was developed as part of the Operating Systems course for the 2024-25 academic year.

Features

Write: Store or update key-value pairs.

Read: Retrieve values associated with given keys.

Delete: Remove specific key-value pairs.

Show: Display all stored key-value pairs in alphabetical order.

Wait: Introduce delays for testing under load conditions.

Backup: Create non-blocking backups using separate processes.

Help: Display available commands and usage information.

Input Format

IST-KVS processes commands either interactively via standard input or in batch mode using .job files. Commands follow the format:

WRITE [(key1,value1) (key2,value2) ...]

READ [key1, key2, ...]

DELETE [key1, key2, ...]

SHOW

WAIT <delay_ms>

BACKUP

HELP

Comments can be added using # at the beginning of a line.

Implementation Details

Uses POSIX file descriptors for file operations.

Implements synchronization mechanisms for concurrent access.

Employs multi-threading to process multiple .job files in parallel.

Utilizes fork() to create non-blocking backups.

Compilation & Execution

Compile with:

make

Run with:

./ist-kvs <directory> <max_backups> <max_threads>

<directory>: Path to the directory containing .job files.

<max_backups>: Maximum number of concurrent backups.

<max_threads>: Number of threads for processing .job files.

Example

Input (jobs/test.job):

# Read a non-existent key
READ [key1]

# Write key-value pairs
WRITE [(key1,value1) (key2,value2)]

# Read an existing key
READ [key2]

# Backup data
BACKUP

Execution:

./ist-kvs jobs 2 4

Output (jobs/test.out):

[(key1,KVSERROR)]
[(key1,value1)(key2,value2)]
[(key2,value2)]
Backup created: jobs/test-1.bck

Grade

Score: 18.86/20
This project was graded 18.86/20, demonstrating a strong implementation of the IST-KVS system.


