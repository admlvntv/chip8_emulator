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
* **Refresh Rate**: 60 Hz (or updated only when a draw `DXYN` instruction executes).
* **Implementation**: Graphics library (SDL?).

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

---

## Fetch/Decode/Execute Loop

The emulator execution follows this sequence during each cycle:
1. **Fetch**: Read the instruction from memory at the current PC.
2. **Decode**: Translate the raw instruction bits into an instruction object.
3. **Execute**: Perform the operation specified by the object.
4. **Delay**: Wait for the next loop cycle.

*Note: Target clock speed ranges from 1 MHz to 4 MHz based on the OG hardware. Clock speed and ROM path will be handled via user input.*

### Fetch
1. **Combine Bytes**: Instructions are two bytes wide. Read the byte at the current PC and the next byte, then combine them into a single 16-bit instruction.
2. **Advance Pointer**: Increment the PC by 2.

### Decode

#### Instruction Representation
Every instruction is decoded into a container object which holds the components of the instruction to be read by the executor. Unused variables default to `0xFF`

The instruction object contains the following fields:
* `id`: An integer representing the unique operation identifier.
* `reg_x`: An integer representing the first register index (0–15), or `0xFF` if unused.
* `reg_y`: An integer representing the second register index (0–15), or `0xFF` if unused.
* `input`: A variable-width integer (4, 8, or 12 bits) holding raw numeric data or addresses.

#### Instruction Types
The decoder handles 5 categories of instructions:
* **No inputs**: e.g., `00E0`
* **Address input**: e.g., `1NNN`
* **Register input**: e.g., `5XY0`
* **Register and value input**: e.g., `6XYN`
* **Split opcode input**: Instructions with identifiers at both the start and end, e.g., `8XY1`, `8XY2`.

#### Decoding Workflow
The decoding process executes in the following sequence:
1. **Initialization**: An instruction object is created. `reg_x`, `reg_y`, and `input` are initialized to `0xFF`.
2. **Identification**: The decoder reads the first nibble of the 16-bit instruction. If the first nibble corresponds to an instruction family that uses trailing sub-opcodes (such as the `8` or `F` series), the decoder reads the final 1 or 2 nibbles and appends them to `id`
3. **Extraction**: The decoder uses a switch statement with 5 cases based on the instruction type category. Inside each case, only the specific variables needed for that instruction type are extracted from the remaining bits and written to the object
4. **Execution**: The completed object is passed to the execution stage
