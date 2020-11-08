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
