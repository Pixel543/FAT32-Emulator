# FAT32 file system emulator

A simple C emulator program that manages the FAT32 file system in an image file. The project was completed as part of a class assignment.

## Features

* `format`: formats a disk image to FAT32.
* `ls [path]`: shows the contents of a directory.
* `mkdir <name>`: creates a directory.
* `touch <name>`: creates an empty file.
* `cd <path>`: changes the current directory.

## Build and Run

1. **Build:**
To build the project, run the `make` command:
```bash
make
```
or
```bash
gcc .\fat32_emulator.c -o fat32_emulator
```

This will create the `f32disk` executable.

2. **Launch:**
Launch the emulator by passing it the name of the image file:
```bash
./f32disk mydisk.disk
```

## Testing and Limitations

A detailed description of the testing process can be found in the file [TESTING.md](TESTING.md).
Known limitations of the project are described in [LIMITATIONS.md](LIMITATIONS.md).