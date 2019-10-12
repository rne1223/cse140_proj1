#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include "computer.h"
#undef mips			/* gcc already has a def for mips */


unsigned int endianSwap(unsigned int);

// Personal functions
void getOpName(char *, DecodedInstr*);
void helpMessage();
// Function Definitions
void PrintInfo (int changedReg, int changedMem);
// Five stages
unsigned int Fetch (int);
void Decode (unsigned int, DecodedInstr*, RegVals*);
int Execute (DecodedInstr*, RegVals*);
int Mem(DecodedInstr*, int, int *);
void RegWrite(DecodedInstr*, int, int *);
void UpdatePC(DecodedInstr*, int);
void PrintInstruction (DecodedInstr*);

/*Globally accessible Computer variable*/
Computer mips;
RegVals rVals;

/** 
  * Trying to make interactive mode user friendly 
  * Why you may ask?  "\_(ʘ_ʘ)_/" I DON'T KNOW...it helps calm down the nervers I guess
  *
  * Thank you for reading this pointless comment...it really means alot to me. 
  * No very professional I know, but fun :)
  *
  * A few hours laters....Okay I need to stop adding arguments to this thing
  * 
  * A few hours laters....maybe I should make it autocomplete lol
  *
*/
void helpMessage(){
    printf("Type 'm' to print memory\n");
    printf("     'r' to print registers  \n");
    printf("     'h' to print help\n");
    printf("     'p' to print info\n");
    printf("     'v' to print the register and their values\n");
    printf("     'b' to pop instruction from statcks\n");
    printf("     'q' to quit\n\n");
    printf("Press 'Enter' for next instruction\n");
}

/*
 *  Return an initialized computer with the stack pointer set to the
 *  address of the end of data memory, the remaining registers initialized
 *  to zero, and the instructions read from the given file.
 *  The other arguments govern how the program interacts with the user.
 */
void InitComputer (FILE* filein, int printingRegisters, int printingMemory,
        int debugging, int interactive) {
    int k;
    unsigned int instr;

    /* Initialize registers and memory */

    for (k=0; k<32; k++) {
        mips.registers[k] = 0;
    }

    /* stack pointer - Initialize to highest address of data segment */
    mips.registers[29] = 0x00400000 + (MAXNUMINSTRS+MAXNUMDATA)*4;

    for (k=0; k<MAXNUMINSTRS+MAXNUMDATA; k++) {
        mips.memory[k] = 0;
    }

    k = 0;
    while (fread(&instr, 4, 1, filein)) {
        /*swap to big endian, convert to host byte order. Ignore this.*/
        mips.memory[k] = ntohl(endianSwap(instr));
        k++;
        if (k>MAXNUMINSTRS) {
            fprintf (stderr, "Program too big.\n");
            exit (1);
        }
    }

    mips.printingRegisters = printingRegisters;
    mips.printingMemory = printingMemory;
    mips.interactive = interactive;
    mips.debugging = debugging;
}

unsigned int endianSwap(unsigned int i) {
    return (i>>24)|(i>>8&0x0000ff00)|(i<<8&0x00ff0000)|(i<<24);
}

/*
 *  Run the simulation.
 */
void Simulate () {
    char s[40];  /* used for handling interactive input */
    unsigned int instr;

    int changedReg=-1, changedMem=-1, val;
    DecodedInstr d;

    /* Initialize the PC to the start of the code section */
    mips.pc = 0x00400000;

    /** 
     * tmps use for interactive mode
     * and easier debugging 
     */
    int tmp_prMemory= mips.printingMemory;
    int tmp_prRegisters = mips.printingRegisters;

    /*** 
      * We don't want to display the help message 
      * if interactive mode is off
      */
    if (mips.interactive)
        helpMessage();

    while (1) {
        /**
         * "Look at it professor...look at my interactive mode"
         *                      - by a desperate student  extra credit
         */
        if (mips.interactive) {
            printf ("> ");
            fgets (s,sizeof(s),stdin);

            if (s[0] == 'q') {
                printf("BYE BYE\n");
                return; // bye bye interactive mode...it was fun while it lasted :(
            } else if(s[0] == 'm'){
                mips.printingMemory = 1;
                mips.pc-=4; // update PC to stay with the current instruction
            } else if(s[0] == 'b'){
                if(mips.pc < 0x00400008){
                    printf("Not enough instructions to pop\n");
                    continue;
                }
                printf("Poping an instruction\n");
                mips.pc-=8; // update PC to two instructions back
            } else if(s[0] == 'r'){
                mips.printingRegisters = 1;
                mips.pc-=4; // update PC to stay with the current instruction
            } else if(s[0] == 'p'){
                if(mips.pc == 0x00400000){
                    printf("No instructions have been executed\n");
                    continue;
                }
                printf ("Previous Executed instruction at %8.8x: %8.8x\n", mips.pc, instr);
                PrintInstruction(&d);
                PrintInfo (changedReg, changedMem); 
                continue;

            } else if(s[0] == 'v'){ 

                printf("rs:$%d=%d\n",d.regs.r.rs, rVals.R_rs);
                printf("rt:$%d=%d\n",d.regs.r.rt, rVals.R_rt);

                if(d.type == 2) // print address instead
                    printf("address:%#010x\n", rVals.R_rd);
                else
                    printf("rd:$%d=%d\n",d.regs.r.rd, rVals.R_rd);

                printf("Val=%d\n", val);
                continue;
            } else if(s[0] == 'h'){ 
                helpMessage();
                continue;
            }                      
        }
        /* Fetch instr at mips.pc, returning it in instr */
        instr = Fetch (mips.pc);                                                                               

        printf ("Executing instruction at %8.8x: %8.8x\n", mips.pc, instr);

        // If no more instructions to fetch get out
        if(instr == 0x0){
            break;
        }


        /* 
         * Decode instr, putting decoded instr in d
         * Note that we reuse the d struct for each instruction.
         */
        Decode (instr, &d, &rVals);

        /*Print decoded instruction*/
        PrintInstruction(&d);

        /* 
         * Perform computation needed to execute d, 
         * returning computed value in val 
         */
        val = Execute(&d, &rVals);

        UpdatePC(&d,val);

        /* 
         * Perform memory load or store. Place the
         * address of any updated memory in *changedMem, 
         * otherwise put -1 in *changedMem. 
         * Return any memory value that is read, otherwise return -1.
         */
        val = Mem(&d, val, &changedMem);

        /* 
         * Write back to register. If the instruction modified a register--
         * (including jal, which modifies $ra) --
         * put the index of the modified register in *changedReg,
         * otherwise put -1 in *changedReg.
         */
        RegWrite(&d, val, &changedReg);

        PrintInfo (changedReg, changedMem);
        printf("\n"); // printing new line to separate the instructions

        // For Debugging Purposes :p
        mips.printingMemory = tmp_prMemory;
        mips.printingRegisters = tmp_prRegisters;
    }

    printf("PROGRAM DONE!!!!\n");
}

/*
 *  Print relevant information about the state of the computer.
 *  changedReg is the index of the register changed by the instruction
 *  being simulated, otherwise -1.
 *  changedMem is the address of the memory location changed by the
 *  simulated instruction, otherwise -1.
 *  Previously initialized flags indicate whether to print all the
 *  registers or just the one that changed, and whether to print
 *  all the nonzero memory or just the memory location that changed.
 */
void PrintInfo ( int changedReg, int changedMem) {
    printf ("New pc = %8.8x\n", mips.pc);

    int k, addr;
    if (!mips.printingRegisters && changedReg == -1) {
        printf ("No register was updated.\n");
    } else if (!mips.printingRegisters) {
        printf ("Updated r%2.2d to %8.8x\n",
                changedReg, mips.registers[changedReg]);
    } else {
        for (k=0; k<32; k++) {
            printf ("r%2.2d: %8.8x  ", k, mips.registers[k]);
            if ((k+1)%4 == 0) {
                printf ("\n");
            }
        }
    }
    if (!mips.printingMemory && changedMem == -1) {
        printf ("No memory location was updated.\n");
    } else if (!mips.printingMemory) {
        printf ("Updated memory at address %8.8x to %8.8x\n",
                changedMem, Fetch (changedMem));
    } else {
        printf ("Nonzero memory\n");
        printf ("ADDR	  CONTENTS\n");
        for (addr = 0x00400000+4*MAXNUMINSTRS;
                addr < 0x00400000+4*(MAXNUMINSTRS+MAXNUMDATA);
                addr = addr+4) {
            if (Fetch (addr) != 0) {
                printf ("%8.8x  %8.8x\n", addr, Fetch (addr));
            }
        }
    }
}

/*
 *  Return the contents of memory at the given address. Simulates
 *  instruction fetch. 
 */
unsigned int Fetch ( int addr) {
    return mips.memory[(addr-0x00400000)/4];
}

// Get the op name 
void getOpName(char * funct_name, DecodedInstr* d){

    int funct = 0;
    int op = 0;

    switch (d->type) {

        case 0: // R-format functions

            funct = d->regs.r.funct; // get the fucnt from d , less typing

            switch (funct) {

                case 0:
                    strcpy(funct_name,"sll");
                    break;
                case 2:
                    strcpy(funct_name,"srl");
                    break;
                case 8:
                    strcpy(funct_name,"jr");
                    break;
                case 32:
                    strcpy(funct_name,"add");
                    break;
                case 33:
                    strcpy(funct_name,"addu");
                    break;
                case 34:
                    strcpy(funct_name,"sub");
                    break;
                case 35:
                    strcpy(funct_name,"subu");
                    break;
                case 36:
                    strcpy(funct_name,"and");
                    break;
                case 37:
                    strcpy(funct_name,"or");
                    break;
                case 39:
                    strcpy(funct_name,"nor");
                    break;
                case 42:
                    strcpy(funct_name,"slt");
                    break;
                case 43:
                    strcpy(funct_name,"sltu");
                    break;
            }
            break;

        case 1: // I-Format functions

            op = d->op;

            switch (op) {
                case 4:
                    strcpy(funct_name,"beq");
                    break;
                case 5:
                    strcpy(funct_name,"bne");
                    break;
                case 8:
                    strcpy(funct_name,"addi");
                    break;
                case 9:
                    strcpy(funct_name,"addiu");
                    break;
                case 10:
                    strcpy(funct_name,"slti");
                    break;
                case 11:
                    strcpy(funct_name,"sltiu");
                    break;
                case 12:
                    strcpy(funct_name,"andi");
                    break;
                case 13:
                    strcpy(funct_name,"ori");
                    break;
                case 15:
                    strcpy(funct_name,"lui");
                    break;
                case 35:
                    strcpy(funct_name,"lw");
                    break;
                case 43:
                    strcpy(funct_name,"sw");
                    break;
            }

        case 2: // J-format functions
            op = d->op;

            switch (op) {
                case 2:
                    strcpy(funct_name,"j");
                    break;
                case 3:
                    strcpy(funct_name,"jal");
                    break;
            }
            break;

    }
}

/* Decode instr, returning decoded instruction. */
void Decode ( unsigned int instr, DecodedInstr* d, RegVals* rVals) {

    int inst_op = instr>>26;  // get the first 6bits
    d->op = inst_op;

    char opName[6] = " ";
    getOpName(opName, d);

    /** Retrive Regiters **/
    //If op is 0 then R-format
    if(inst_op == 0) {

        // declare the instruction to be R-format
        d->type = 0; 

        // Get R-format registers
        RRegs rregs;
        // shifting and masking to get certain values from the instruction
        rregs.rs    = (instr >> 21) & MASK_5;
        rregs.rt    = (instr >> 16) & MASK_5;
        rregs.rd    = (instr >> 11) & MASK_5;
        rregs.shamt = (instr >> 6) & MASK_5;
        rregs.funct = instr & MASK_6;

        // Get the register values
        rVals->R_rs = mips.registers[rregs.rs];
        rVals->R_rt = mips.registers[rregs.rt];
        rVals->R_rd = mips.registers[rregs.rd];

        // put values in d
        d->regs.r = rregs;

        //Else if op is 2 or 3 then J format  
    }else if(inst_op == 2 || inst_op == 3 ) {

        // declare the instruction to be J-format
        d->type = 2; 

        //get addr
        JRegs jregs;
        jregs.target = instr & MASK_26;

        // nothing to put here
        rVals->R_rs = -1;
        rVals->R_rt = -1;
        rVals->R_rd = jregs.target;

        d->regs.j = jregs;

    //Else is I format
    } else {

        // declare the instruction to be I-format
        d->type = 1;

        // Get I-format registers and immed
        IRegs iregs;
        iregs.rs = (instr >> 21) & MASK_5;
        iregs.rt = (instr >> 16) & MASK_5;
        iregs.addr_or_immed = instr & MASK_16;

        // get the register values
        rVals->R_rs = mips.registers[iregs.rs];
        rVals->R_rt = mips.registers[iregs.rt];

        if(iregs.addr_or_immed >> 15) { 
            // If it is negative then fill the upper 16 bits with 1
            iregs.addr_or_immed |= MASK_UPPER; 
        }      

        rVals->R_rd = iregs.addr_or_immed;
        // put values in d
        d->regs.i = iregs;
    }
}


/*
 *  Print the disassembled version of the given instruction
 *  followed by a newline.
 */
void PrintInstruction ( DecodedInstr* d) {

    char opName[6] = " ";
    getOpName(opName, d);

    // print the the type of operation
    printf("%s ", opName);

    switch (d->type) {

        case 0: // Print R-format instructions
            if(!strcmp(opName, "jr")){ // if jr then only print two values
                printf(" %#010x\n", d->regs.r.rs);
            }else{
                printf(" $%d, $%d, $%d\n", d->regs.r.rd,d->regs.r.rs,d->regs.r.rt);
            }
            break;
        case 1: // Print I-format instructions
            if(!strcmp(opName, "lui")) {

                printf(" $%d, %#010x\n", d->regs.i.rt,d->regs.i.addr_or_immed);

            } else if(!strcmp(opName, "lw") || !strcmp(opName, "sw")){
                // printing 0x0000003($s1)
                printf(" $%d, %#010x($%d)\n", d->regs.i.rt,d->regs.i.addr_or_immed, d->regs.i.rs);

            } else {
                // printing $s0, $s1, 0x00000003
                printf(" $%d, $%d, %#010x\n", d->regs.i.rt,d->regs.i.rs,d->regs.i.addr_or_immed);
            }
            break;
        case 2: // Print J-format instructions
            printf("%#010x\n", d->regs.j.target); 
            break;

    }
}

/* Perform computation needed to execute d, returning computed value */
int Execute ( DecodedInstr* d, RegVals* rVals) {

    /**********
      * TODO:
      ********* 
      * Whatever is done mark it as done
      ***

      R-format instructions to implement
      --------------------------------
      * addu Rdest, Rsrc1, Rsrc2    - done
      * subu Rdest, Rsrc1, Rsrc2    - done **Mars turns "Subu" into "lui" "ori" and then subu :/
      * and  Rdest, Rsrc1, Rsrc2    - done
      * or   Rdest, Rsrc1, Rsrc2    - done
      * slt  Rdest, Rsrc1, Rsrc2    - done
      * sll  Rdest, Rsrc, shamt     - done
      * srl  Rdest, Rsrc, shamt     - done

      I-format instructions to implement
      --------------------------    
      * ori   Rdest, Rsrc, imm      - done
      * andi  Rdest, Rsrc, imm      - done
      * addiu Rdest, Rsrc1, imm     - done
      * lui   Rdest, imm            - done
      * beq   Rsrc1, Rsrc2, raddr   - done
      * bne   Rsrc1, Rsrc2, raddr   - done
      * lw    Rdest, offset (Radd)
      * sw    Rdest, offset (Radd) 


      J-format instructions to implement
      --------------------------
      * j   address
      * jal address                 - done
      * jr  Rsrc
      */

    char opName[6] = " ";
    getOpName(opName, d);

    // R-format
    switch (d->type) {

        case 0: // R-format instructions

            // Most of the code below is wrong...we need to fix :/
            if(!strcmp(opName, "addu") || !strcmp(opName, "add")) {

                return rVals->R_rs + rVals->R_rt;        

            } else if(!strcmp(opName, "subu") || !strcmp(opName, "sub")) {

                return rVals->R_rs - rVals->R_rt;        

            } else if(!strcmp(opName, "sltu") || !strcmp(opName, "slt") ) {

                return rVals->R_rs < rVals->R_rt;        

            } else if(!strcmp(opName, "and")) {

                return rVals->R_rs & rVals->R_rt;        
                
            } else if(!strcmp(opName, "or")) {

                return rVals->R_rs | rVals->R_rt;        

            } else if(!strcmp(opName, "nor")) {
                
                return ~(rVals->R_rs | rVals->R_rt);        

            } else if(!strcmp(opName, "sll")) {

                return rVals->R_rt << d->regs.r.shamt;        

            } else if(!strcmp(opName, "srl")) {

                return rVals->R_rt >> d->regs.r.shamt;        

            } else if(!strcmp(opName, "jr")) {

                return rVals->R_rs;        
            }         

            break;

        case 1: // I-type instructions

            // this works, but we need to add
            if(!strcmp(opName, "addi") || !strcmp(opName, "addiu")){
                return rVals->R_rs + rVals->R_rd;        
            } else if(!strcmp(opName, "lui")){
                return rVals->R_rd << 16;        
            } else if(!strcmp(opName, "ori")){
                return rVals->R_rs | rVals->R_rd;        
            } else if(!strcmp(opName, "andi")){
                return rVals->R_rs & rVals->R_rd;        
            } else if(!strcmp(opName, "bne")){
                if(rVals->R_rs != rVals->R_rt){
                    return rVals->R_rd * 4; 
                }
            } else if(!strcmp(opName, "beq")){
                if(rVals->R_rs == rVals->R_rt){
                    return rVals->R_rd * 4; 
                } 
            } 
            break;
        case 2: // J-type instructions --might need some work
            if(!strcmp(opName, "jal")){
                return d->regs.j.target << 2;
            } else {// j instruction
                return (mips.pc & MASK_UPPER) | (d->regs.j.target << 2);
            } 
            
            break;
    }

    return 0;
}

/* 
 * Update the program counter based on the current instruction. For
 * instructions other than branches and jumps, for example, the PC
 * increments by 4 (which we have provided).
 */
void UpdatePC ( DecodedInstr* d, int val) {
    mips.pc+=4;

    char opName[6] = " ";
    getOpName(opName, d);

    switch (d->type) {

        case 0: // R-format instructions
            // Only worry about jr
            if(!strcmp(opName, "jr")){
                /* printf("---val address:%#010x\n", val); */
                mips.pc = val;
                /* mips.pc = mips.registers[val];  */
            }
            break;

        case 1: // I-type instructions

            // There are a few I-format instructions that mess with the PC
            if(!strcmp(opName, "beq") || !strcmp(opName, "bne" )) {
                mips.pc += val;
            } 
            break;

        case 2: // J-type instructions

            mips.pc = val;
            break;

    }
}

/*
 * Perform memory load or store. Place the address of any updated memory 
 * in *changedMem, otherwise put -1 in *changedMem. Return any memory value 
 * that is read, otherwise return -1. 
 *
 * Remember that we're mapping MIPS addresses to indices in the mips.memory 
 * array. mips.memory[0] corresponds with address 0x00400000, mips.memory[1] 
 * with address 0x00400004, and so forth.
 *
 */
int Mem( DecodedInstr* d, int val, int *changedMem) {
    
    char opName[6] = " ";       
    getOpName(opName, d);

    // R-format
    switch (d->type) {

        case 0: // R-format instructions don't use memory
            return val;
            *changedMem = -1;
            break;

        case 1: // Few I-type instructions that use memory
            if (!strcmp(opName, "lw" )) {

                /* printf("pc changed lw"); */

            } else if (!strcmp(opName, "sw" )) {

                /* printf("pc changed sw"); */

            } else {
                *changedMem = -1;
                return val;
            }
           
            break;

        case 2: // J-type instructions
            *changedMem = -1;
            break;

    }
    return 0;
}

/* 
 * Write back to register. If the instruction modified a register--
 * (including jal, which modifies $ra) --
 * put the index of the modified register in *changedReg,
 * otherwise put -1 in *changedReg.
 */
void RegWrite( DecodedInstr* d, int val, int *changedReg) {
    char opName[6] = " ";
    getOpName(opName, d);

    // R-format
    switch (d->type) {

        case 0: // R-format instructions
            mips.registers[d->regs.r.rd] = val;
            *changedReg = d->regs.r.rd;
            break;

        case 1: // I-type instructions
            if(!strcmp(opName, "bne") || !strcmp(opName, "beq") ) {  
                *changedReg = -1;
            }else{

                mips.registers[d->regs.i.rt] = val;
                *changedReg = d->regs.i.rt;
            }
            break;

        case 2: // J-type instructions
            if(!strcmp(opName, "jal")) {
                mips.registers[31] = mips.pc;
                *changedReg = 31;
            }else {
                *changedReg = -1;
            }

            break;

    }
}
