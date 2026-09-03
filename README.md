# Operating System Simulator

A small educational operating-system simulator written in C. The project models process creation, scheduling, contiguous memory allocation, swapping, system calls, and synchronization with binary semaphores. Programs are loaded from text files, translated into internal opcodes, and executed one instruction per simulated clock step.

The current executable is console-based. It prints scheduler decisions, process state changes, memory activity, semaphore operations, and system-call results while the simulation runs.

## Features

- Process control blocks with process states, program counters, arrival times, and wait-time tracking.
- Three scheduling algorithms:
  - Round Robin (RR), with a fixed quantum of two instructions.
  - Highest Response Ratio Next (HRRN).
  - Multi-Level Feedback Queue (MLFQ), with four priority levels.
- A 40-word simulated main memory with contiguous allocation.
- Process swapping to `src/disk/pid_<pid>.swap` when memory cannot hold all active processes.
- Three binary semaphores representing user input, user output, and file access.
- Ready and blocked process queues.
- File, console input, console output, and simulated-memory system calls.
- A safety limit and deadlock check to prevent an unfinished simulation from running indefinitely.

## Repository Layout

```text
.
├── Makefile                 # GCC build configuration
├── architecture.txt         # Original architecture notes
├── build/                   # Generated executable and object files
└── src/
    ├── main.c               # Simulation entry point and clock loop
    ├── disk/                # Runtime files and swap files
    ├── interpreter/         # Program parser and instruction executor
    ├── memory/              # Simulated memory and swapping
    ├── os/                  # OS lifecycle, clock, and system calls
    ├── process/             # Process and PCB definitions
    ├── programs/            # Input programs for the simulator
    ├── scheduler/           # Queues and scheduling algorithms
    └── synchronization/     # Semaphores and blocked queues
```

## Requirements

The current `Makefile` is configured for Windows with an MSYS2 UCRT64 GCC installation:

- GCC with C11 support
- GNU Make
- MSYS2 tools providing `find`, `mkdir`, and `rm`
- Raylib installed under `C:/msys64/ucrt64`

The Makefile links Raylib and Windows graphics/system libraries. The current source tree does not use a Raylib GUI, but the dependency remains part of the configured build.

## Build

Run the build from the repository root:

```sh
make
```

The executable is written to:

```text
build/os_gui.exe
```

To remove generated object files and the executable:

```sh
make clean
```

On PowerShell, the same commands can be run from an MSYS2-enabled terminal. The project uses relative paths such as `src/programs` and `src/disk`, so the working directory matters.

## Run

From the repository root:

```sh
build/os_gui.exe
```

At startup, choose a scheduler:

```text
Choose scheduler (1=RR, 2=HRRN, 3=MLFQ) [default=2]:
```

The default is HRRN. Any choice other than `1` or `3`, including end-of-file, selects HRRN.

The simulation loads the built-in programs at these simulated clock times:

| Program         | Arrival time | Purpose                                                           |
| --------------- | -----------: | ----------------------------------------------------------------- |
| `Program_1.txt` |            0 | Reads two numbers and prints the inclusive range between them     |
| `Program_2.txt` |            1 | Reads a filename and content, then writes the content to the file |
| `Program_3.txt` |            4 | Reads a filename, reads the file, and prints its content          |

The main loop stops when all programs finish, when a deadlock is detected, or after 1000 simulation cycles.

## Program Language

Programs are plain text files stored in `src/programs`. One instruction is written per line. Arguments are separated by spaces or tabs.

### Supported Instructions

| Instruction                      | Description                                                                        |
| -------------------------------- | ---------------------------------------------------------------------------------- |
| `semWait <resource>`             | Acquires a binary semaphore or blocks the process if it is busy                    |
| `semSignal <resource>`           | Releases a semaphore and wakes a waiting process                                   |
| `assign <variable> <value>`      | Stores a value in one of the process's variables; `input` can be used as the value |
| `print <variable>`               | Prints the value stored in a process variable                                      |
| `printFromTo <from> <to>`        | Prints an inclusive numeric range using two process variables                      |
| `writeFile <filename> <content>` | Writes a process-variable value to a file under `src/disk`                         |
| `readFile <filename>`            | Reads a file under `src/disk`; the result can be assigned to a variable            |
| `input`                          | Reads a line from standard input                                                   |

The supported semaphore names are:

| Name         | Numeric value | Resource         |
| ------------ | ------------: | ---------------- |
| `userInput`  |             0 | Console input    |
| `userOutput` |             1 | Console output   |
| `file`       |             2 | Disk/file access |

The uppercase names `USER_INPUT`, `USER_OUTPUT`, and `FILE_RESOURCE` are also accepted. Numeric resource values `0`, `1`, and `2` are accepted as well.

### Example

```text
semWait userInput
assign x input
assign y input
semSignal userInput
semWait userOutput
printFromTo x y
semSignal userOutput
```

The parser translates the command names to internal opcodes before storing them in the process's code area:

| Opcode | Source instruction |
| ------ | ------------------ |
| `000`  | `semWait`          |
| `001`  | `semSignal`        |
| `010`  | `assign`           |
| `011`  | `print`            |
| `100`  | `printFromTo`      |
| `101`  | `writeFile`        |
| `110`  | `readFile`         |

The `input` token is handled as an execution-time input operation rather than one of the numbered instruction opcodes.

## Runtime Architecture

### Main loop and clock

`src/main.c` initializes the selected scheduler and the OS, loads programs when their arrival times are reached, then advances the simulation with `os_tick()`. Each OS step advances the simulated clock and asks the selected scheduler for work. A status line reports the clock, running PID, ready-queue size, and blocked-queue size.

### Processes

Each process owns a PCB, its parsed code lines, up to three process variables, and scheduling metadata. Process states are:

```text
READY -> RUNNING -> FINISHED
                  \-> BLOCKED -> READY
```

A process can become blocked while waiting for a semaphore. It becomes ready again when another process signals the corresponding resource.

### Scheduling

#### Round Robin

RR executes a process for at most two instructions per time slice. An unfinished process whose quantum expires is returned to the end of the ready queue. A process that blocks or finishes leaves the CPU immediately.

#### HRRN

HRRN selects the ready process with the greatest response ratio:

```text
response ratio = (waiting time + burst time) / burst time
```

Here, burst time is based on the process's number of code lines. The selected process executes one instruction per scheduler step and remains on the CPU until it finishes or blocks.

#### MLFQ

MLFQ maintains four queues. Processes start in queue 0, and the time quantum for queue `i` is `2^i`:

| Queue |        Quantum |
| ----: | -------------: |
|     0 |  1 instruction |
|     1 | 2 instructions |
|     2 | 4 instructions |
|     3 | 8 instructions |

After using a full quantum, an unfinished process moves down one level. Processes already in queue 3 remain there. A process released from a semaphore is placed into MLFQ queue 0.

### Memory and swapping

The simulator models 40 memory words. A process needs space for its PCB fields, three variable slots, and its code lines. Allocation is contiguous. When a process cannot fit in an available block, the memory subsystem can write its contents to a swap file in `src/disk` and free its memory range. The process is restored with `swap_in()` when it is selected to run.

The three supplied programs require more total space than the 40-word memory capacity, so running the complete workload exercises the swapping path. Swap files and files created by `writeFile` are runtime artifacts and should not be treated as program source.

### Synchronization

The simulator starts all three semaphores in the available state. A failed `semWait` places the process in both the resource-specific blocked queue and the general blocked queue. `semSignal` wakes the next process waiting for that resource and returns it to the appropriate ready queue.

## Adding a Program

1. Create a text file in `src/programs`.
2. Use one supported instruction per line.
3. Add the filename and arrival time to the arrays in `src/main.c` if it should be loaded automatically.
4. Rebuild with `make`.
5. Run the executable from the repository root so relative program and disk paths resolve correctly.

Programs can also use filenames or paths that contain `/` or `\\`; otherwise file operations are resolved relative to `src/disk`.

## Troubleshooting

### `make` cannot find GCC, Raylib, or Unix tools

Confirm that MSYS2 UCRT64 is installed, that `C:/msys64/ucrt64/bin` is available to the build environment, and that Raylib headers and libraries exist under `C:/msys64/ucrt64/include` and `C:/msys64/ucrt64/lib`.

### Programs or disk files cannot be opened

Run the executable from the repository root. The loader expects programs at `src/programs/<filename>`, and file system calls expect relative files under `src/disk`.

### The simulation stops with a deadlock message

This means there is no running or ready process, but at least one process remains blocked. Review the matching `semWait` and `semSignal` instructions in the programs and ensure every acquired resource is eventually released.

### The simulation reaches the safety limit

The workload did not become idle within 1000 cycles. Inspect the trace for a process that is repeatedly blocked, incorrectly rescheduled, or unable to finish.

## Implementation Notes

- The canonical build entry point is the root `Makefile`.
- `architecture.txt` contains earlier design notes and names some modules differently from the current source tree. For the current implementation, use the actual files under `src` and the Makefile.
- The configured output is named `os_gui.exe`, although the current implementation uses console output and does not contain an active Raylib GUI module.
- Runtime swap files and files generated by sample programs are created under `src/disk`.

## License

No license file is currently included in the repository.
