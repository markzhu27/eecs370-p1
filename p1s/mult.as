        lw 0 1 mcand
        lw 0 2 mplier
        add 0 0 3
        lw 0 4 one
        lw 0 5 limit
loop    beq 4 5 done
        nor  2 2 6
        nor 4 4 7
        nor 6 7 6
        beq 6 0 skip
        add 1 3 3
skip    add 1 1 1
        add 4 4 4
        beq 0 0 loop
done    halt
mcand   .fill 6203
mplier  .fill 1429
one     .fill 1
limit   .fill 32768