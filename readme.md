
# pumpkin-cpu

pumpkin-cpu is small general purpose, scalable, 16-bit, 16 instruction soft CPU core written in VHDL. It connects to two separate memories referred to as program and IO memory. The program memory is 16 bits wide, and has a maximum size of 4K words, it is used for program code and data storage and should be implemented, at least partially, in RAM. The IO memory is also 16 bits wide, and has a maximum size of 64K words, it is used for peripherals and data storage.

![System block diagram](pictures/block1.png)

## File List
A description of the files included in this repo.  
**readme.md**      - This file  
**pumpkin.vhd**    - pumpkin-cpu VHDL source code  
**pasm.c**         - PASM assembler for pumpkin-cpu, C source code  
**led_flash.vhd**  - Example top level LED flash example using hand assembled machine code  
  
**led_example/led.asm**      - LED flash example program  
**led_example/led.vhd**      - Output file from assembler, initialized RAM model  
**led_example/led_top.vhd**  - Top level led flash example module  
  
**hello_example/hello_world.asm**      - Hello world example program  
**hello_example/hello_world.vhd**      - Output file from assembler, initialized RAM model  
**hello_example/hello_world_top.vhd**  - Top level hello world example module  
##  Architecture

**Arithmetic Logic Unit**  
The 16-bit ALU supports add, subtract, or, and, xor, shift right through carry and a high/low byte swap. Except for ROR and SWAP, the ALU operates on the accumulator ('A' register) and a value fetched from program memory, the result is stored in the accumulator. The instruction word contains the address of the value to be operated on. There are no immediate instructions, instead any constant values must be placed elsewhere in program memory and referenced by the instruction. This is not quite as limiting as it may sound, values only needed defining in one place and can be used by many instructions. ROR and SWAP read the program memory and store the result in the accumulator. LOAD reads the program memory and stores the value in the accumulator. STORE writes the accumulator to program memory. Both use the same addressing scheme as the ALU instructions. STORE is the only instruction that can change the contents of the program memory.

**Carry Flag**  
The carry flag is updated by three instructions, it stores the 17th bit of the result from ADD and SUB instructions, ROR shifts right one bit right through carry. It should be noted that ADD and SUB instructions do not use the carry flag as an input.

**Program Flow**  
After reset the CPU will fetch and execute the instruction at program address 0 and continue executing instructions in sequence until a branch or call instruction is encountered. There are three branch instructions. The unconditional branch (BR) will always branch to the location referenced in the instruction. BNZ will branch if the contents of the accumulator register is not zero, BNC will branch if the carry flag is clear. CALL will call the referenced subroutine and push the return address onto the call stack. RETURN pops the return address from the stack and continues execution from there.

**Input / Output**  
IO memory is accessed by the IN and OUT instructions, IN reads a 16-bit word from IO memory, storing the value in the accumulator, OUT writes the contents of the accumulator to the IO memory. The IN and OUT instructions use indirect addressing, the memory location referenced in the instruction contains the IO address, this way 64K words can be addressed.


![CPU block diagram](pictures/block2.png)

## Instruction Set

The 16-bit instruction is divided into a 4-bit opcode and 12-bit operand, the opcode occupying the most significant bits. The the operand references a location in program memory for all instructions except RETURN, which ignores these bits. The table below shows the 16 instructions, along with a description and the number of clock cycles each instruction takes to execute.

```

   15    14    13    12    11    10    9     8     7     6     5     4     3     2     1     0
+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
|        Op-Code        |                        Program memory address                         |
+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+

 A   = Accumulator 
 C   = Carry flag 
 AC  = Carry flag with accumulator 
 X   = 12-bit memory address from the instruction word 
 M   = Program memory referenced by X 
 CM  = Carry flag with program memory 
 IO  = IO memory 
 ( ) = Memory Subscript 

+-------+-----------+------+----------------------------+
|Op-code|Instruction|Cycles|Description                 |
+-------+-----------+------+----------------------------+
| 0x0   | LOAD      | 2    | A = M(X)                   |
| 0x1   | STORE     | 2    | M(X) = A                   |
| 0x2   | ADD       | 2    | AC = M(X) + A              |
| 0x3   | SUB       | 2    | AC = M(X) - A              |
| 0x4   | OR        | 2    | A = M(X) OR A              |
| 0x5   | AND       | 2    | A = M(X) AND A             |
| 0x6   | XOR       | 2    | A = M(X) XOR A             |
| 0x7   | ROR       | 2    | AC = CM(X) >> 1            |
| 0x8   | SWAP      | 2    | A = M(X) [byte-swapped]    | 
| 0x9   | IN        | 2    | A = IO(M(X))               |
| 0xA   | OUT       | 2    | IO(M(X)) = A               |
| 0xB   | BR        | 1    | Branch unconditional       |
| 0xC   | BNC       | 1    | Branch if carry flag is 0  |
| 0xD   | BNZ       | 1    | Branch if A is not 0       |
| 0xE   | CALL      | 1    | CALL subroutine            |
| 0xF   | RETURN    | 1    | RETURN from subroutine     |
+-------+-----------+------+----------------------------+

```

## Building

The pumpkin-cpu core is defined in a single VHDL file (pumpkin.vhd), it has been tested with Lattice Diamond and Intel Quartus tool chains and should work with many others. The two generics determine the depth of the call stack and the size of the program memory in address bits. The maximum size of the program memory is 4096 words, or 12 bits. Connection to the memories is straight forward. The control signals **program_wr**, **io_rd** and **io_rd** stay active for a single clock period. The CPU reads the IO data while **io_rd** is active, this may present a problem for synchronous peripherals and memories which require one or more clock cycles for the data to be available. In this case, with suitable external logic, the **clock_enable** input can be used to pause the CPU, the same **clock_enable** must also connect to the program memory to prevent the next instruction being read too soon. 

```vhdl
entity pumpkin is
    generic(
        stack_depth     : integer := 4;
        program_size    : integer := 12);
    port (
        clock           : in std_logic;
        clock_enable    : in std_logic;
        reset           : in std_logic;
        program_data_in : in std_logic_vector(15 downto 0);
        data_out        : out std_logic_vector(15 downto 0);
        program_address : out std_logic_vector(program_size-1 downto 0);
        program_wr      : out std_logic;
        io_data_in      : in std_logic_vector(15 downto 0);
        io_address      : out std_logic_vector(15 downto 0);
        io_rd           : out std_logic;
        io_wr           : out std_logic);
end entity;
```

## Flashing LED Example

The obligatory flashing-led example program is shown below in pumpkin machine code. The program assumes the LED is connected to a register located at bit 0 of IO address 0, with read and write access. The bulk of the program consists of delay consisting of an inner and outer loop. Outside of the loop, the LED status is read, inverted, and written back. The inner loop uses the accumulator as a down counter, the outer loop uses a memory variable to keep track of the count.

```
  0x00: 0x000D    ; LOAD  [0x0D]  'Load A with 1'
  0x01: 0xA00E    ; OUT   [0x0E]  'Write A to LED port, IO address 0'
  0x02: 0x000F    ; LOAD  [0x0F]  'Load A with 30, number of outer delay loop iterations'
  0x03: 0x1011    ; STORE [0x11]  'Store A in the outer counter loop variable'
  0x04: 0x0010    ; LOAD  [0x10]  'Load A with 65535'
  0x05: 0x300D    ; SUB   [0x0D]  'Subtract 1 from A'
  0x06: 0xD005    ; BNZ   [0x05]  'Branch to 0x05 if A is not zero'
  0x07: 0x0011    ; LOAD  [0x11]  'Load A from the outer counter variable'
  0x08: 0x300D    ; SUB   [0x0D]  'Subtract 1 from A'
  0x09: 0xD003    ; BNZ   [0x03]  'Branch to 0x03 if A is not zero'
  0x0A: 0x900E    ; IN    [0x0E]  'Load A (bit 0) with the current state of the LED'
  0x0B: 0x600D    ; XOR   [0x0D]  'XOR A with 1, inverting bit 0'
  0x0C: 0xB001    ; BR    [0x01]  'Branch to 0x01'
  0x0D: 0x0001    ; 'Constant 1' 
  0x0E: 0x0000    ; 'Constant 0'
  0x0F: 0x001E    ; 'Constant 30'
  0x10: 0xFFFF    ; 'Constant 65535'
  0x11: 0x0000    ; 'Outer loop counter variable'
```

This example is in the file 'led_flash.vhd'. The top-lvel VHDL module contains an initialised RAM image and the LED register, it needs to be built alongside the CPU core 'pumpkin.vhd'. The LED will flash approximately once per second with a 12MHz clock. For faster or slower clock speeds the initial value of the outer loop counter can be adjusted.
# Assembler
PASM is an assembler for the pumpkin-cpu. The source code comprises of a single file (pasm.c) and can be built with GCC. An example of what an PASM source code file looks like is shown below.
```
;
; Hello World Example
;

;
; This program will transmit a serial message, 8-n-1 at 115200 assuming the CPU is clocked at 12MHz
;
; Other baud rates and clock speeds can be used by adjusting value of BIT_TIME.
; To calculate this value first divide the clock frequency by the baud rate, subtract 16, then divde by 3.
; For example:
;                 12000000 / 115200 = 104
;                 104 - 16 = 88
;                 88 / 3 = 29
;
; The TX line is at bit 0 of IO address 0.
;

                  BR START    ; Branch to start of program

HELLO             DB "pumpkin-cpu demo",13,10,"Hello World!",13,10,0
BIT_TIME          DW 29
BYTE_PTR          DW 0
TEMP              DW 0
TX_PIN            DW 0
BIT_COUNTER       DW 0

; Main program

START             LOAD #1                 ; Load A with 1
                  OUT TX_PIN              ; Set TX pin high
                  LOAD #500
LP1               SUB #1                  ; Loop 500 times (1500 clock cycles)
                  BNZ LP1
                  LOAD @HELLO             ; Load A with the address of the string
                  CALL PRINT_STRING       ; Call the print string routine
LP2               BR LP2                  ; Wait forever

; Routine to print string addressed by A

PRINT_STRING      STORE BYTE_PTR          ; 'A' contains the 'word' address of the string
                  ADD BYTE_PTR            ; Double to get the byte address
PSTR1             STORE BYTE_PTR
                  CALL READ_BYTE          ; Read the addressed byte from program memory
                  BNZ PSTR2               ; Reading 0 indicates the end of the string
                  RETURN                  ; Return if byte is 0
PSTR2             CALL TX_BYTE            ; Transmit 
                  LOAD BYTE_PTR           ; Load the byte pointer
                  ADD #1                  ; Increment
                  BR PSTR1                ; Loop to read the next byte

; Routine to read a byte from program memory A is the byte address

READ_BYTE         STORE TEMP              
                  ADD #0                  ; Clear carry flag 
                  ROR TEMP                ; Divide by 2 to get the word address and carry flag for high/low byte selection
                  STORE RB1               ; Store the byte address for execution as a LOAD instruction
RB1               NOP                     ; Load the word containing the addressed byte
                  BNC RB2                 ; IF carry is 0 high byte is addressed so SWAP the read word
                  BR RB3                  ; ELSE low byte is addressed so don't SWAP
RB2               STORE TEMP
                  SWAP TEMP
RB3               AND #0xFF               ; Clear the upper byte and return
                  RETURN   

; Serial transmission of byte stored in 'A'

TX_BYTE           STORE TEMP              ; Shift left appending start bit (0)
                  ADD TEMP
                  OR #0x200               ; Append stop bit (1)
                  STORE TEMP              ; TEMP used as a 'shift register'
                  LOAD #10                ; 10 Bits to transmit 
TXB1              STORE BIT_COUNTER    
                  LOAD TEMP               ; Load 'shift register'
                  OUT TX_PIN              ; Set TX pin to bit 0 of A, no other pins exist are used on this IO port
                  ROR TEMP                ; Shift 'shift register' 1 bit right
                  STORE TEMP
                  LOAD BIT_TIME           ; Load bit delay time
TXB2              SUB #1
                  BNZ TXB2                ; Bit delay loop
                  LOAD BIT_COUNTER        
                  SUB #1                  ; Decrement bit counter
                  BNZ TXB1                ; Loop until all bits are sent
                  RETURN

```

## Comments
Whenever PASM encounters a semicolon the rest of the line is ignored, empty lines are also ignored.
## Labels
A label is used to identify a location within the source file. A label must start with an alpha character and is only allowed to contain alphanumeric characters and underscore. Labels must be defined starting from the first column in the source file and can either be followed by statements or have a line to itself. None of the reserved words can be used as a label, the reserved words are the 16 instructions, 4 directives and the pseudo instruction 'NOP'. 

```

; Some examples of label usage

COUNTER      DW 0           ; Label referances storage location

MAIN_LOOP                   ; Label has line to itself
             LOAD #12
LOOP1        STORE COUNTER  ; Label referances branch destination
             SUB #1
             BNZ LOOP1
             BR MAIN_LOOP

```

### NOP
Most CPU instruction sets include a NOP or 'no-operation' instruction. Genally used for timing or memory alignment. The pumpkin-cpu does not have a specific NOP instruction, instead the assembler supports NOP as an pseudo instruction. Whenever PASM encounters a NOP it creates a BR (branch unconditional) with the address set to the current address plus one. This takes one CPU clock cycle to execute.

```
; Example using NOP

            IN SWITCH
            NOP
            OUT LED

; Equivalent to code generated above 

            IN SWITCH
            BR NEXT_LINE
NEXT_LINE   OUT LED

```

## Directives
PASM currently supports 3 directives. The directives are not translated directly into opcodes. Instead, they are used to adjust the location of the program in memory and initialize memory.

### DB - Define bytes in program memory
The directive DB defines bytes in program memory. Normally DB will be preceded by a label. Data can be expressed as integers in hexadecimal, octal or decimal format or as text enclosed in double-quotes, a combination of text and integers can be defined on a single line. Because the program memory is 16-bit, the DB directive packs two bytes into each location with the high byte stored first. If there is an odd number of bytes, the low byte of the last word is set to 0.

```

LABEL1     DB 0x12,0x34     ; Two bytes expressed in hexadecimal
LABEL1     DB 012,034       ; ... Octal
LABEL2     DB 12,13         ; ... and decimal
LABEL3     DB 12            ; A single byte occupies the upper byte of program memory with 0 stored in the lower byte

MIXTURE    DB “Hello World!”,0x13 ; Text and integers can be combined

; Multi-line DB statements

MENU       DB “This multiline message”,0x13
           DB “only needs a single label”,0x13
           DB “to reference the start address”,0

```

If multi-line DB statements are used, the assembler will finish each line by adding a padding 0 to the low byte of the last word if an odd number of bytes are defined, starting the new line with the next program memory address.

```
; Here we define 8 consecutive program memory words with the low byte of each set to 0

SPRITE1    DB 0x38
           DB 0x28
           DB 0x38
           DB 0x10
           DB 0x7C
           DB 0x10
           DB 0x28
           DB 0x44

; In the two examples below, we define 4 consecutive program memory words bytes packed
; with the high bytes first

SPRITE2    DB 0x38,0x28,0x38,0x10,0x7C,0x10,0x28,0x44

SPRITE3    DB 0x38,0x28
           DB 0x38,0x10
           DB 0x7C,0x10
           DB 0x28,0x44

```

Single bytes can be duplicated with DUP to define blocks of memory.

```

; Here we define 100 bytes (50 words) of program memory initialized to 0

TEXT_BUFFER   DB 0 DUP 100 

; ...followed by 10 bytes initialized to 0xff

               DB 0xFF DUP 10

```

It is only possible to duplicate a single byte, text and multiple values are not allowed. The example above is the only valid syntax for DUP. The label is optional, and the integer can be expressed in hexadecimal, octal or decimal.
### DW - Define words in program memory
The directive DW defines words in program memory in much the same way as DB defines bytes. Normally DW will be preceded by a label. Data can be expressed as integers in hexadecimal, octal or decimal format or as a LABEL. DW will accept a mixture of literal and label definitions on the same line separated by a comma. The DUP statement can also be used to define multiple words with the same value.

```
; Example of literal integers defined by DW

REGISTER_A    DW 0x0100     ; Single word initialized to hex 100
REGISTER_B    DW 012        ; Single word initialized to octal 12
REGISTER_C    DW 66         ; Single word initialized to decimal 66

REGISTERS     DW REGISTER_A,REGISTER_B,REGISTER_C ; Example of using labels to define words

WORD_BUFFER   DW 0xFFFF DUP 16  ; Example of using DUP

```
### ORG
The ORG (origin) directive sets the assemblers location counter to the value specified; subsequent statements are assigned memory locations starting with the new location counter value. The value specified can be in hex, octal or decimal. The assembler checks for overlaps caused ORG, and unused gaps in the program memory are filled with 0.
```
; Example of using ORG to specify the start location of a section of code

             ORG 0x200
JUMP_TABLE   BR OPTION_1
             BR OPTION_2
             BR OPTION_3
             BR OPTION_4
             BR OPTION_5
             BR OPTION_6
             BR OPTION_7
             BR OPTION_8

```
### Instruction Operands
All instructions except **RETURN** require an operand referencing a program memory location. The assembler supports three different ways to express this. Firstly, a label can be used, either to reference a storage location or the destination of branch or call instruction.
```
; In this example the LOAD instruction loads the accumulator with 24

HOURS     DW 24

          LOAD HOURS

; Example of branching to a label

MAIN_LOOP
          

          BR MAIN_LOOP

```
Secondly an immediate value can be specified preceded by '#'. This value can be expressed in hex, octal or decimal. The pumpkin-cpu does not support immediate addressing, instead when the assembler sees an immediate value it defines a word initialized to the value then references the instruction to it. If the assembler has previously defined the same value earlier in the assembly process a new word will not be defined, and the assembler will reference the previous definition. All the extra word definitions are located at the end of the user program.
```
; Example of using immediate values

COUNTER   DW 0

          LOAD #6           ; Load the value 6 into the accumulator 
LP1       STORE COUNTER     ; Store the accumulator in memory
 

          LOAD COUNTER      ; Load accumulator from memory
          SUB #1            ; Subtract 1
          BNE LP1           ; Branch if accumulator is not 0


```
Thirdly an address referenced by a label can be specified by using the label name preceded by '@'. Here the reference is used instead of the value referenced. As with immediate, an additional word is defined in the background by the assembler for the instruction to reference, with the same mechanism to prevent duplicate values being defined multiple times.
```
; Example of using '@' to get the value of a memory reference rather than the value itself 

HELLO     DB "Hello World!",0

          LOAD @HELLO
          CALL PRINT_STRING

; This is equivalent to code above but without using @

HELLO     DB "Hello World!",0
HELLO_REF DW HELLO

          LOAD HELLO_REF
          CALL PRINT_STRING

```
## Command Line
PASM is a console application and can be run from the command line.
```
  pasm source.asm [S] output.(vhd|mem|mif)
```
The source file and output file names must be specified. The optional argument 'S' refers to the size of the program output image specified as a base 2 number. Valid program sizes are 32,64,128 etc. The maximum size is 4096 and if no value is specified the default value of 2048 is assumed. PASM supports three different output formats determined by the output filename extension.
```
  .vhd  creates a VHLD initialized RAM model
  .mif  creates a Intel/Altera MIF File
  .mem  creates a Lattice Semiconductors MEM File
```
As an example if we assemble 'led.asm':
```
;
; LED Flash example
;
            BR START ; Branch to start of program

LED_PORT    DW 0  ; IO address of LED port register
COUNTER     DW 0  ; Counter variable

START       LOAD #1         ; Load A with 1
LP1         OUT LED_PORT    ; Write A to LED port
            LOAD #30        ; Load A with outer loop start count value, adjust this value for differnt clock speeds
LP2         STORE COUNTER   ; Store A in counter variable
            LOAD #0xFFFF    ; Load A with 65536
LP3         SUB #1          ; Subtract 1 from A
            BNZ LP3         ; Loop if A is not 0
            LOAD COUNTER    ; Load A with counter variable
            SUB #1          ; Subtract 1 from A
            BNZ LP2         ; Loop if A is not 0
            IN LED_PORT     ; Read current LED state
            XOR #1          ; Invert LED state
            BR LP1          ; Main loop

```
With this command...
```
C:\pumpkin>pasm led.asm 32 led.vhd
Pass 1
Pass 2
Assembly successful 19 memory words used
VHDL file 'led.vhd' created.
C:\pumpkin>
```
We get a VHD file we can use within our FPGA IDE called 'led.vhd'.
```vhdl
---------------------------------------------------------------------
--
-- Built with PASM version 1.3
-- File name: led.vhd
-- 8-11-2020 15:11:14
-- 
---------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity led is
port (
    clock        : in std_logic;
    clock_enable : in std_logic;
    address      : in std_logic_vector(4 downto 0);
    data_out     : out std_logic_vector(15 downto 0);
    data_in      : in std_logic_vector(15 downto 0);
    write_enable : in std_logic);
end entity;

architecture rtl of led is

    type ram_type is array (0 to 31) of std_logic_vector(15 downto 0);
    signal ram : ram_type := (
      X"B003",X"0000",X"0000",X"0010",X"A001",X"0011",X"1002",X"0012",
      X"3010",X"D008",X"0002",X"3010",X"D006",X"9001",X"6010",X"B004",
      X"0001",X"001E",X"FFFF",X"0000",X"0000",X"0000",X"0000",X"0000",
      X"0000",X"0000",X"0000",X"0000",X"0000",X"0000",X"0000",X"0000");
begin

    process(clock)
    begin
        if rising_edge(clock) then
            if clock_enable = '1' then
                if write_enable = '1' then
                    ram(to_integer(unsigned(address))) <= data_in;
                else
                    data_out <= ram(to_integer(unsigned(address)));
                end if;
            end if;
        end if;
    end process;

end rtl;

--- End of file ---
```
The output file formats specific to Lattice and Intel can be used with the memory IP generation tools for their respective tool chains. 
## TODO

* Allow spaces between commas in DB and DW statements
* Correct spelling of ‘successful’ in assembler 
* Add listing output to assembler
* Programming examples and detailed usage of instructions

