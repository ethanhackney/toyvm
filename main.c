#include <stdint.h>
#include <stdio.h>

/* to get gcc to stop complaining about unused variables */
#define UNUSED(id) ((void)(id))

/* command of our virtual machine */
enum {
        /* halt virtual machine */
        CMD_HALT,
        /* increment register value */
        CMD_INC,
        /* move between registers or memory */
        CMD_MOV,
        /* compare register to value */
        CMD_CMP,
        /* jump if not equal */
        CMD_JMP_NE,
        NR_CMDS,
};

/* names for debugging */
static const char *cmdnames[NR_CMDS] = {
        "CMD_HALT",
        "CMD_INC",
        "CMD_MOV",
        "CMD_CMP",
        "CMD_JMP_NE",
};

/* get command of instruction */
#define CMD_OF(_ins) \
        (((_ins) & CMD_MASK))

/* our registers */
enum {
        REG_0,
        REG_1,
        REG_2,
        REG_3,
        REG_4,
        REG_5,
        REG_6,
        NR_REGS,
};

/* names for debugging */
static const char *regnames[NR_REGS] = {
        "REG_0",
        "REG_1",
        "REG_2",
        "REG_3",
        "REG_4",
        "REG_5",
        "REG_6",
};

/* transfer types */
enum {
        /* if this is transfer type, then the next
         * nibble of the next byte of memory after the instruction
         * contains the first register and the next nibble contains
         * the destination register
         *
         * registers[src] = registers[dest]
         */
        TX_REG_2_REG,

        /* if this is transfer type, the next byte of
         * memory after the instruction contains the
         * register and the next 4 bytes contains the address
         *
         * registers[src] = memory[0xff]
         */
        TX_REG_2_MEM,

        /* same format as TX_REG_2_MEM
         *
         * memory[0xff] = registers[src]
         */
        TX_MEM_2_REG,

        /* if this is transfer type, the 4 bytes of
         * memory after the instruction contains the
         * destination address and the next 4 bytes after
         * that contain the source address
         *
         * memory[0xff] = memory[0xf]
         */
        TX_MEM_2_MEM,

        NR_TX_TYPES,
};

/* get transfer type */
#define TX_TYPE_OF(_ins) \
        (((_ins) & TX_TYPE_MASK) >> 4)

/* names for debugging */
static const char *txnames[NR_TX_TYPES] = {
        "TX_REG_2_REG",
        "TX_REG_2_MEM",
        "TX_MEM_2_REG",
        "TX_MEM_2_MEM",
};

/* masks for parsing commands */
enum {
        CMD_MASK        = 0xf,
        TX_TYPE_MASK    = 0xf0,
};

/* print the bits of an integers */
static void printbits(uint64_t n);

/* misc constants */
enum {
        MAIN_MEM_SIZE   = 0xffff,
};

/* main memory, memory is stored in little endian format */
static char memory[MAIN_MEM_SIZE] = {
        /* registers[REG_0] = 0 */
        CMD_MOV | (TX_MEM_2_REG << 4),
        REG_0,
        19, 0x0, 0x0, 0x0,

        /* registers[REG_0]++ */
        CMD_INC,
        REG_0,

        /* registers[REG_0] == 10 */
        CMD_CMP,
        REG_0,
        10, 0x0, 0x0, 0x0,

        /* jump to MOV command */
        CMD_JMP_NE,
        0x6, 0x00, 0x00, 0x00,

        CMD_HALT,

        [19] = 0,
};

/* registers */
static int registers[NR_REGS];

/* program counter */
static unsigned int pc;

/* value of previous comparison */
static int cmp;

int
main(void)
{
        UNUSED(cmdnames);
        UNUSED(regnames);
        UNUSED(txnames);

        registers[REG_0] = 0;
        while (memory[pc] != CMD_HALT) {
                unsigned int ins;
                unsigned int dest;
                unsigned int value;

                ins = memory[pc];

                switch (CMD_OF(ins)) {
                case CMD_INC:
                        dest = (memory[++pc]);
                        ++registers[dest];
                        ++pc;
                        break;
                case CMD_CMP:
                        dest = (memory[++pc]);
                        value = memory[++pc];
                        value |= (memory[++pc] << 8);
                        value |= (memory[++pc] << 16);
                        value |= (memory[++pc] << 24);
                        if (registers[dest] == value)
                                cmp = 1;
                        ++pc;
                        break;
                case CMD_MOV:
                        if (TX_TYPE_OF(ins) == TX_REG_2_REG) {
                                unsigned int regs;
                                unsigned int dest;
                                unsigned int src;

                                regs = memory[++pc];
                                dest = (regs & 0xf0) >> 4;
                                src = (regs & 0xf);
                                registers[src] = registers[dest];
                                ++pc;
                        } else if (TX_TYPE_OF(ins) == TX_REG_2_MEM) {
                                unsigned int reg;
                                unsigned int addr;

                                reg = memory[++pc];
                                addr = memory[++pc];
                                addr |= (memory[++pc] << 8);
                                addr |= (memory[++pc] << 16);
                                addr |= (memory[++pc] << 24);
                                printbits(addr);

                                registers[reg] = memory[addr];
                                ++pc;
                        } else if (TX_TYPE_OF(ins) == TX_MEM_2_REG) {
                                unsigned int reg;
                                unsigned int addr;

                                reg = memory[++pc];
                                addr = memory[++pc];
                                addr |= (memory[++pc] << 8);
                                addr |= (memory[++pc] << 16);
                                addr |= (memory[++pc] << 24);

                                memory[addr] = registers[reg];
                                ++pc;
                        } else if (TX_TYPE_OF(ins) == TX_MEM_2_MEM) {
                                unsigned int dest;
                                unsigned int src;

                                dest = memory[++pc];
                                dest |= (memory[++pc] << 8);
                                dest |= (memory[++pc] << 16);
                                dest |= (memory[++pc] << 24);

                                src = memory[++pc];
                                src |= (memory[++pc] << 8);
                                src |= (memory[++pc] << 16);
                                src |= (memory[++pc] << 24);

                                memory[dest] = memory[src];
                                ++pc;
                        }
                        break;
                case CMD_JMP_NE:
                        if (cmp == 0) {
                                dest = memory[++pc];
                                dest |= (memory[++pc] << 8);
                                dest |= (memory[++pc] << 16);
                                dest |= (memory[++pc] << 24);
                                pc = dest;
                        } else {
                                pc += 5;
                        }
                        break;
                }
        }
        printf("%d\n", registers[REG_0]);
}

static void
printbits(uint64_t n)
{
        uint64_t bit;

        for (bit = 31; bit != (uint64_t)-1; --bit)
                putchar((n & ((uint64_t)1 << bit)) ? '1' : '0');

        putchar('\n');
}
