
;
; LED Flash example
;

LED_PORT    DW 0  ; IO address of LED port register
COUNTER     DW 0  ; Counter variable

            LOAD #1         ; Load A with 1
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

