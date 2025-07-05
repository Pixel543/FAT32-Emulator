# FAT32 emulator limitations ⚠️

This document describes known limitations of the current emulator implementation. This project is educational and does not claim to be a fully functional implementation of the FAT32 file system.

## Working with files and directories

* **Support only short names (8.3):** The emulator works exclusively with file and directory names in the 8.3 format (up to 8 characters for the name and up to 3 for the extension). Long file names (LFN) are not supported and are ignored when reading directories.
* **No writing data to files:** The `touch` command only creates empty files of zero size. The functionality for writing data to files, reading them, or changing them is not implemented.
* **No deletion and renaming:** The emulator does not have commands for deleting files or directories (`rm`, `rmdir`), as well as for renaming them (`mv`).
* **Limited navigation:** The `cd` command only supports absolute paths from the root (e.g. `/dirname`). Complex relative paths (e.g. `../otherdir`) are not handled.

## File system functionality

* **Lack of fragmentation support:** The implementation does not provide for working with files that occupy more than one cluster. All created files and directories occupy no more than one cluster.
* **Hard-coded FS parameters:** Key file system parameters, such as the cluster size (4 KB), are hard-coded in the program and cannot be changed by the user during formatting.
* **Incomplete file information:** The `ls` command displays only the name and type (file/directory). Additional information, such as size, creation date or access rights, is not displayed.

## Error handling and reliability

* **Minimal error checking:** The program performs basic checks (such as the presence of a file system), but does not handle all possible error situations, such as:
* Trying to create a file with an existing name.
* Not enough free disk space (no full disk check).
* Using invalid characters in file names.
* **No recovery:** There are no mechanisms to check the integrity of the file system or to recover it if damaged.