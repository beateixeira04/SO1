# SO1
Primeiro projeto da cadeira de sistemas operativos - 2º ano de faculdade

IST-KVS Project
Overview

The IST Key-Value Store (IST-KVS) is a system that stores data as key-value pairs, implemented as part of the Sistemas Operativos course (2024-25). It supports basic operations like WRITE, READ, DELETE, SHOW, WAIT, and BACKUP. The system uses a hashtable to manage data and includes optimizations for concurrent backups and parallel processing of multiple job files.
Features

    WRITE: Insert or update key-value pairs.
    READ: Retrieve values for one or more keys.
    DELETE: Remove one or more keys.
    SHOW: List all key-value pairs.
    WAIT: Introduce a delay between commands.
    BACKUP: Create a backup using a non-blocking process.

Usage

    Build the project:

make

Run the program with:

    ./ist-kvs <directory_path> <max_concurrent_backups> <max_threads>

Example:

./ist-kvs /path/to/jobs 2 4

Grading

    Grade: 18.86

License

MIT License
