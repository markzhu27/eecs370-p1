    lw 0 1 num1
    jalr 1 2
    lw 0 2 num2
    add 1 2 3
    noop
    halt
done1    halt
    lw 0 3 num2
    jalr 3 3
done2    halt

num1 .fill done1
num2 .fill done2