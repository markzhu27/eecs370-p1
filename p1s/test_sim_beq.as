        lw 0 1 three
        lw 0 2 neg
loop    add 1 2 1
        beq 1 0 done
        beq 0 0 loop
done    halt
three   .fill 3
neg     .fill -1