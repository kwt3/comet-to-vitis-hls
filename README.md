# comet-to-vitis-hls
 
comet-to-vitis-hls is a project focused on converting the Comet RISC-V ISA processor simulator into a Vitis HLS-compatible design. This conversion aims to enable hardware acceleration of the Comet simulator, making it suitable for use in FPGA-based systems. The project refactors the existing Comet simulator code to make it compatible with Xilinx Vitis HLS.

## hls_component

### Implementation files
- includes: Header files to initialize classes/structs for src files.
- src: Contains main implementation of processor.

### Testing files
- basic_tests: Basic algorithm tests
- riscv-compliance: comet RISC-V compliance header model file and makefiles

## Goals
- Convert the Comet RISC-V ISA simulator codebase to be compatible with Vitis HLS.

- Optimize the simulation for hardware acceleration.

- Provide a version of the Comet simulator that can be used in FPGA-based environments.

- Improve performance and enable more efficient RISC-V processor simulations using hardware.

## Current Status

This project is a manual conversion of the Comet RISC-V simulator to work with Vitis HLS. The current state of the project includes:

- Refactoring key parts of the Comet simulator to optimize for Vitis HLS.

- Addressing compatibility issues with hardware synthesis (e.g., memory access, loops, and function calls).

- Working on the simulation framework in Vitis HLS to validate functional correctness.

## Conversion Process

The conversion involves modifying parts of the Comet codebase to make it synthesizable for Vitis HLS, with a focus on:

- Memory Optimizations: Refactor memory accesses to use directives and ensure they are supported in Vitis HLS (e.g., HLS PRAGMA for memory management).

- Loop Optimizations: Applying loop unrolling, pipelining, and other optimizations supported by Vitis HLS.

- Function Refactoring: Rewriting functions to ensure they are compatible with hardware synthesis (e.g., removing unsupported dynamic memory allocations, handling function calls in hardware, etc.).

## Testing

The Comet simulator will be tested in both software simulation mode and hardware-accelerated simulation using Vitis HLS.

We will ensure that the refactored code still performs the same RISC-V simulations as the original software-based Comet simulator.

## License
This project is licensed under the MIT License.