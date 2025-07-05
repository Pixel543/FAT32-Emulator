# FAT32 Emulator Test Report

## Test Environment

* **Operating System:** Windows 10
* **Compiler:** GCC (MinGW-w64) 13.2.0 (specify your version)

## Test Scenario

The testing was performed step by step to verify all the functional requirements specified in the task. Each step includes the executed command, the expected result, and the actual result.

---

### **Test 1: Checking an unformatted disk**
* **Action:** Run `ls` immediately after creating the image file.
* **Command:** `ls`
* **Expected Result:** Error about unknown disk format.
* **Actual Result:** ✅ Matches the expected.

### **Test 2: Formatting**
* **Action:** Execute `format` command.
* **Command:** `format`
* **Expected result:** Message `Ok`.
* **Actual result:** ✅ Same as expected.

### **Test 3: Checking root directory**
* **Action:** View contents after formatting.
* **Command:** `ls`
* **Expected result:** Empty output.
* **Actual result:** ✅ Same as expected.

### **Test 4: Creating directory (`mkdir`)**
* **Action:** Create directory `testdir`.
* **Command:** `mkdir testdir`
* **Expected result:** `Ok` message and the directory appears in the `ls` output.
* **Actual result:** ✅ Matches expected.

### **Test 5: Create a file (`touch`)**
* **Action:** Create an empty file `myfile.txt`.
* **Command:** `touch myfile.txt`
* **Expected result:** `Ok` message and the file appears in the `ls` output.
* **Actual result:** ✅ Matches expected.

### **Test 6: Change directory (`cd`)**
* **Action:** Change to the created directory `/testdir`.
* **Command:** `cd /testdir`
* **Expected Result:** Changes the command line prompt to `/testdir>`.
* **Actual Result:** ✅ Same as expected.

### **Test 7: Checking the contents of the new directory**
* **Action:** Executes `ls` inside `/testdir`.
* **Expected Result:** Prints `.` and `..` entries.
* **Actual Result:** ✅ Same as expected.

### **Test 8: Exit the program**
* **Action:** Enters the `exit` command.
* **Command:** `exit`
* **Expected Result:** Correctly terminates the emulator.
* **Actual Result:** ✅ Same as expected.

---

## Conclusion

All the main functions of the emulator (`format`, `ls`, `mkdir`, `touch`, `cd`) work in accordance with the requirements of the task. The program correctly handles errors on an unformatted disk and changing directories.