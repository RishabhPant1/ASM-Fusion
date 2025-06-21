; Simple 8085 Assembly Program
; Add two numbers and store the result

    ORG 2000H       ; Start program at address 2000H

START:
    MVI A, 15H      ; Load first number (15H) into accumulator
    MVI B, 25H      ; Load second number (25H) into register B
    
    ADD B           ; Add B to A (15H + 25H = 3AH)
    STA 3000H       ; Store result at memory location 3000H
    
    MVI C, 10H      ; Load 10H into register C
    SUB C           ; Subtract C from A (3AH - 10H = 2AH)
    STA 3001H       ; Store new result at 3001H
    
    INR A           ; Increment A (2AH + 1 = 2BH)
    STA 3002H       ; Store incremented value
    
    HLT             ; Halt the program
    
    END             ; End of program