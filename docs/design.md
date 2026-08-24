# Design

---

## Specifications
(Organized from the [technical reference](CHIP‐8-Technical-Reference.md))

### Memory
* **Size**: 4kB RAM.
* **Implementation**: `std::array`.
* **Details**: ROM loads at `0x200` and cuts off at `0xE8F`.

### Display
* **Size**: 64 x 32 pixels monochrome.
* **Refresh Rate**: 60 Hz.
* **Implementation**: Currently uses `TerminalDisplay` (ANSI escape codes). Future implementation will use a graphics library (SDL) for Phase 3.

### Program Counter (PC)
* **Purpose**: Points to the current instruction in memory.
* **Implementation**: Integer index.

### Index Register (I)
* **Purpose**: 16-bit register (uses 12 bits) to point to locations in memory.
* **Implementation**: `std::uint16_t`.

### Stack
* **Purpose**: Stores at least 12 16-bit memory addresses to call and return from subroutines.
* **Implementation**: `std::stack` for LIFO operations.

### Delay Timer
* **Purpose**: 8-bit timer that decrements at 60 Hz.
* **Implementation**: `std::uint8_t` decremented inside a loop.

### Sound Timer
* **Purpose**: 8-bit timer that decrements at 60 Hz and makes a beep when value is `0x02` or higher.
* **Implementation**: `std::uint8_t` decremented inside a loop. Audio generation to be implemented later.

### Variable Registers
* **Purpose**: 16 8-bit registers numbered `V0` through `VF`. `VF` acts as a flag register for instruction status.
* **Implementation**: Array of 16 `std::uint8_t` elements (unsigned to handle unsigned overflow, maybe overflow should be implemented by me?).

### Fonts
* **Purpose**: 4x5 pixel sprite data representing hex characters 0 through F.
* **Implementation**: Stored in the main memory array with locations defined by constants.

### Keypad
* **Purpose**: 16-key hex keypad (`0`-`F`), read by `EX9E`/`EXA1` (skip on key state) and `FX0A` (blocking wait for a keypress).
* **Layout**: The physical COSMAC VIP layout is remapped to a modern keyboard as follows:

  |     |     |     |     |
  |-----|-----|-----|-----|
  | `1` | `2` | `3` | `C` |
  | `4` | `5` | `6` | `D` |
  | `7` | `8` | `9` | `E` |
  | `A` | `0` | `B` | `F` |

  mapped to:

  |     |     |     |     |
  |-----|-----|-----|-----|
  | `1` | `2` | `3` | `4` |
  | `Q` | `W` | `E` | `R` |
  | `A` | `S` | `D` | `F` |
  | `Z` | `X` | `C` | `V` |
* **Implementation**: `Keypad` holds a 16-entry boolean state array exposed through `press()`, `release()`, and `is_key_down()`. An input backend updates that state by calling `press()`/`release()` from its own event loop (SDL).

---

## Fetch/Decode/Execute Loop

The emulator execution follows this sequence during each cycle:
1. **Fetch**: Read the instruction from memory at the current PC and advance the PC.
2. **Execute**: Decode the raw instruction bits and perform the operation.

*Note: Target clock speed ranges from 500 Hz to 1000 Hz for modern implementations. Clock speed and ROM path are handled via command-line arguments.*

### Fetch
1. **Combine Bytes**: Instructions are two bytes wide (big-endian). The `fetch()` method reads the byte at the current `m_pc` and the next byte, then combines them into a single 16-bit opcode.
2. **Advance Pointer**: Increment the `m_pc` by 2.

### Execute

#### Decode
Opcodes are decoded on-the-fly using bitwise masks and shifts. The following helper functions are used to extract specific parts of the 16-bit instruction:
* `id`: `(opcode & 0xF000) >> 12` (The first nibble, used for the main instruction switch).
* `x`: `(opcode & 0x0F00) >> 8` (The second nibble, usually a register index).
* `y`: `(opcode & 0x00F0) >> 4` (The third nibble, usually a register index).
* `n`: `opcode & 0x000F` (The fourth nibble, a 4-bit constant).
* `nn`: `opcode & 0x00FF` (The last 8 bits, an 8-bit constant).
* `nnn`: `opcode & 0x0FFF` (The last 12 bits, a memory address).

#### Execution Workflow
1. **Fetch**: The `cycle()` method calls `fetch()` to get the next opcode.
2. **Dispatch**: The `execute()` method receives the raw opcode and uses a `switch` statement on the `id` (first nibble).
3. **Sub-dispatch**: For instruction families with the same prefix (like `0x0`, `0x8`, `0xE`, or `0xF`), a nested `switch` on `nn` or `n` is used to identify the specific operation.
4. **Operation**: The registers, memory, and display are modified according to the instruction logic.

### Architecture
The execution logic lives directly inside the `CPU` class.

Most of the executor is a single, large switch statement that evaluates the `id` field of the instruction object.

#### PC Management
By default, the PC is incremented by 2 during the Fetch stage. The execution stage handles exceptions to this directly within the switch cases:
* **Jumps**: The case overwrites the PC with the target address.
* **Subroutines**: The case pushes the current PC onto the stack before overwriting the PC with the subroutine address.
* **Skips**: If a conditional check passes (such as comparing two registers), the case adds 2 to the PC to skip the next instruction.

#### Helper Functions and Modularity
To maintain code readability, complex operations or instructions of similar functional types will be extracted from the switch statement into helper functions.