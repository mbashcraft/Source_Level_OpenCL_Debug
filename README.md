
Follow instructions on the ROSE compiler github page to install ROSE. 
$ROSE_HOME  needs to be set according to the instructions for the tool https://github.com/rose-compiler/rose/wiki

Copy DefaultTranslator.c to $ROSE_HOME/src/exampleTranslators/defaultTranslator/

Run “make install” in $ROSE_HOME/build/exampleTranslators/defaultTranslator/

Add an hlsDebugConfig.txt file to the directory with the OpenCL file. This file contains kernel names, and a buffer size. 
An example would be:

KERNEL_NAME fetch
KERNEL_NAME fft1d
BUFFER_SIZE 512

If you need multiple buffer sizes you can manually change the uses in the new OpenCL file 
Note the buffer size needs to be a power of two for the calculation offsets into the buffer in each thread.

Run “python3 hlstool.py” with the OpenCL file as an argument. Note the python portion of the tool only supports a single 
OpenCL file at the moment.
  The tool collects all of the assignment variables in the kernels, records them and their unique identifiers to a file, and 
  calls a the python gui. In the gui the user can select variables to record, and select the Confirm button in the top right 
  corner. 
    Note the user should only select variables in the top level kernel, as support for recording variables in other called
    functions has been disabled due to excessive overhead in the generated RTL design. If the user  wants to record variables 
    in called functions they should be inlined first.
  After the user confirms their selection, the selection is recorded to a file. ROSE then continues to execute, reads the
  selection from the file, and inserts the recording instrucitons
  
Note the default type of the trace buffer is long, but it can be manually changed to whatever the user desires. 

The host file needs to be modified to retrieve the trace buffer. An extra argument needs to be added to the kernel, which will contain the trace buffer. The size of the trace buffer is the size of each threads trace buffer times the number of threads. Each device gets its own thread. The type is dependent upon the type of the trace buffer in the kernel.

After retrieving the trace buffer kernel argument, the buffer needs to be dumped to a file. 
The layout is as follows, all data separated by a space:

NUM_DEVICES NUM_THREADS TB_SIZE DEVICE_ID TRACE_BUFFER_FOR_CURRENT_DEVICE DEVICE_ID  TRACE_BUFFER_FOR_CURRENT_DEVICE ...

For example if there was 2 device, 1 thread per device, a trace buffer size of 8, the DEVICE_IDs were 101 and 102, and the values in the buffers were 1-8, then the layout would be as follows:

2 1 8 101 1 2 3 4 5 6 7 8 102 1 2 3 4 5 6 7 8

Note the device IDs may not be less than (2^28) + 1 or 268435457

The parsing and generation of the OpenCL Debug Trace viewer is done by TODO
