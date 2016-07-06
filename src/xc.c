#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

int token;              // current token
char *src, *old_src;    // pointer to source code string
int poolsize;           // default size of text/data/stack
int line;               // line number

int *text;          // text segment
int *old_text;      // for dump text segment
int *stack;         // stack
char *data;         // data segment

/*
 * virtual machine registers:
 *
 * PC 程序计数器，它存放的是一个内存地址，该地址中存放着 下一条 要执行的计算机指令。
 * SP 指针寄存器，永远指向当前的栈顶。注意的是由于栈是位于高地址并向低地址增长的，所以入栈时 SP 的值减小。
 * BP 基址指针。也是用于指向栈的某些位置，在调用函数时会使用到它。
 * AX 通用寄存器，我们的虚拟机中，它用于存放一条指令执行后的结果。
 * */
int *pc, *bp, *sp, ax, cycle;

// instructions
enum { LEA, IMM,    JMP,    CALL,   JZ,     JNZ,    ENT,    ADJ,    LEV,    LI,     LC,     SI,     SC,     PUSH,
        OR, XOR,    AND,    EQ,     NE,     LT,     GT,     LE,     GE,     SHL,    SHR,    ADD,    SUB,    MUL,    DIV,    MOD,
        OPEN,READ,  CLOS,   PRTF,   MALC,   MSET,   MCMP,   EXIT};

void next()
{
    token = *src++;
    return;
}

void program()
{
    next(); // get next token
    while (token > 0) {
        printf("token is: %c\n", token);
        next();
    }
}

void expression(int level)
{

}

int eval()
{
    int op, *tmp;
    while (1) {
        switch (op) {
            case IMM:
                // IMM <num> 将 <num> 放入寄存器 ax 中。
                ax = *pc++;
                break;
            case LC:
                // LC 将对应地址中的 char 载入 ax 中，要求 ax 中存放地址。
                ax = *(char *)ax;
                break;
            case LI:
                // LI 将对应地址中的 int 载入 ax 中，要求 ax 中存放地址。
                ax = *(int *)ax;
                break;
            case SC:
                // SC 将 ax 中的数据作为 char 存放入地址中，要求栈顶存放地址。
                *(char *)*sp++ = ax;
                break;
            case SI:
                // SI 将 ax 中的数据作为 int 存放入地址中，要求栈顶存放地址。
                *(int *)*sp++ = ax;
                break;
            case PUSH:
                // 入栈
                *--sp = ax;
                break;

            case JMP:
                // pc 寄存器指向的是 下一条 指令。所以此时它存放的是 JMP 指令的参数，即 <addr> 的值。
                pc = (int *)*pc;
                break;
            case JZ:
                pc = ax ? pc + 1 : (int *)*pc;
                break;
            case JNZ:
                pc = ax ? (int *)*pc : pc + 1;
                break;

                // Function Call
            case CALL:
            {
                *--sp = (int)(pc + 1);
                pc = (int *)*pc;
                break;
            }
            case ENT:
            {
                /*
                 * ENT <size> 指的是 enter，用于实现 ‘make new call frame’ 的功能，即保存当前的栈指针，同时在栈上保留一定的空间，用以存放局部变量。对应的汇编代码为：
                 * ; make new call frame
                 * push    ebp
                 * mov     ebp, esp
                 * sub     1, esp       ; save stack for variable: i
                 */
                *--sp = (int)bp;
                bp = sp;
                sp = sp - *pc++;
                break;
            }
            case ADJ:
            {
                /*
                 * ADJ <size> 用于实现 ‘remove arguments from frame’。在将调用子函数时压入栈中的数据清除，本质上是因为我们的 ADD 指令功能有限。对应的汇编代码为：
                 * ; remove arguments from frame
                 * add     esp, 12
                 */
                sp = sp + *pc++;
            }
            case LEV:
            {
                /* 本质上这个指令并不是必需的，只是我们的指令集中并没有 POP 指令。并且三条指令写来比较麻烦且浪费空间，所以用一个指令代替。对应的汇编指令为：
                 * ; restore old call frame
                 * mov     esp, ebp
                 * pop     ebp
                 * ; return
                 * ret
                 */
                sp = bp;
                bp = (int *)*sp++;
                pc = (int *)*sp++;
            }
            case LEA:
            {
                /*
sub_function(arg1, arg2, arg3);

|    ....       | high address
+---------------+
| arg: 1        |    new_bp + 4
+---------------+
| arg: 2        |    new_bp + 3
+---------------+
| arg: 3        |    new_bp + 2
+---------------+
|return address |    new_bp + 1
+---------------+
| old BP        | <- new BP
+---------------+
| local var 1   |    new_bp - 1
+---------------+
| local var 2   |    new_bp - 2
+---------------+
|    ....       |  low address

                 */
                ax = (int)(bp + *pc++);
            }

            // operation instructions
            case OR: ax = *sp++ | ax; break;
            case XOR: ax = *sp++ ^ ax; break;
            case AND: ax = *sp++ & ax; break;
            case EQ: ax = *sp++ == ax; break;
            case NE: ax = *sp++ != ax; break;
            case LT: ax = *sp++ < ax; break;
            case LE: ax = *sp++ <= ax; break;
            case GT: ax = *sp++ > ax; break;
            case GE: ax = *sp++ >= ax; break;
            case SHL: ax = *sp++ << ax; break;
            case SHR: ax = *sp++ >> ax; break;
            case ADD: ax = *sp++ + ax; break;
            case SUB: ax = *sp++ - ax; break;
            case MUL: ax = *sp++ * ax; break;
            case DIV: ax = *sp++ / ax; break;
            case MOD: ax = *sp++ % ax; break;

            case EXIT:
                printf("exit(%d)", *sp);
                return *sp;
                break;
            case OPEN:
                ax = open((char *)sp[1], sp[0]);
                break;
            case CLOS:
                ax = close(*sp);
                break;
            case READ:
                ax = read(sp[2], (char *)sp[1], *sp);
                break;
            case PRTF:
            {
                tmp = sp + pc[1];
                ax = printf((char *)tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5], tmp[-6]);
                break;
            }
            case MALC:
                ax = (int)malloc(*sp);
                break;
            case MSET:
                ax = (int)memset((char *)sp[2], sp[1], *sp);
                break;
            case MCMP:
                ax = memcmp((char *)sp[2], (char *)sp[1], *sp);
                break;

            default:
                printf("unknown instruction:%d\n", op);
                break;
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    int i, fd;
//    argc--;
//    argv++;
//
    poolsize = 256 * 1024;
    line = 1;
//
//    if ((fd = open(*argv, 0)) < 0) {
//        printf("could not open(%s)\n", *argv);
//        return -1;
//    }
//
    if (!(src = old_src = malloc(poolsize)))
    {
        printf("could not malloc(%d) for source area\n", poolsize);
        return -1;
    }
//
//    // read the source file
//    if ((i = read(fd, src, poolsize - 1)) <= 0)
//    {
//        printf("read() returned %d\n", i);
//        return -1;
//    }
//
    src[i] = 0; //add EOF character.
//    close(fd);
//


    // allocate memory for virtual machine
    if (!(text = old_text = malloc(poolsize))) {
        printf("could not malloc(%d) for text area\n", poolsize);
        return -1;
    }
    if (!(data = malloc(poolsize))) {
        printf("could not malloc(%d) for data area\n", poolsize);
        return -1;
    }
    if (!(stack = malloc(poolsize))) {
        printf("could not malloc(%d) for stack area\n", poolsize);
        return -1;
    }

    memset(text, 0, poolsize);
    memset(data, 0, poolsize);
    memset(stack, 0, poolsize);

    // init virtual machine register.
    bp = sp = (int *)((int)stack + poolsize);
    ax = 0;
//
//    program();
//    return eval();

    ax = 0;

    // 10 + 20
    i = 0;
    text[i++] = IMM;
    text[i++] = 10;
    text[i++] = PUSH;
    text[i++] = IMM;
    text[i++] = 20;
    text[i++] = ADD;
    text[i++] = PUSH;
    text[i++] = EXIT;

    pc = text;

//    program();

    return eval();
}

