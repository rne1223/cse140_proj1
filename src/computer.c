#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include "computer.h"
#undef mips			/* gcc already has a def for mips */


unsigned int endianSwap(unsigned int);

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
void getOpName(char *, DecodedInstr*);

/*Globally accessible Computer variable*/
Computer mips;
RegVals rVals;

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
     * created my own tmp mips
     * for easier debugging in interactive mode
     */
    Computer tmp_mips; 
    tmp_mips = mips;

    while (1) {
        if (mips.interactive) {
            printf ("> ");
            fgets (s,sizeof(s),stdin);
            if (s[0] == 'q') {
                return;
            }else if(s[0] == 'm'){
              mips.printingMemory = 1;
              mips.pc-=4; // update PC to stay with the current instruction
            }else if(s[0] == 'r'){
              mips.printingRegisters = 1;
              mips.pc-=4; // update PC to stay with the current instruction
            }else if(s[0] == 'n'){ 
              mips.pc+=4; // update PC to stay with the current instruction
            }        
        }

        /* Fetch instr at mips.pc, returning it in instr */
        instr = Fetch (mips.pc);

        printf ("Executing instruction at %8.8x: %8.8x\n", mips.pc, instr);

        /* 
	 * Decode instr, putting decoded instr in d
	 * Note that we reuse the d struct for each instruction.
	 */
        Decode (instr, &d, &rVals);

        /*Print decoded instruction*/
        PrintInstruction(&d);

        /* 
	 * Perform computation needed to execute d, returning computed value 
	 * in val 
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

        printf("\n");

        // For Debugging Purposes :p
        mips.printingMemory = tmp_mips.printingMemory;
        mips.printingRegisters = tmp_mips.printingRegisters;
    }
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
    int k, addr;
    printf ("New pc = %8.8x\n", mips.pc);
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

  /** Retrive Regiters **/
  //If op is 0 then R-format
  if(inst_op == 0) {

    // declare the instruction to be R-format
    d->type = 0; 

    // Get R-format registers
    RRegs rregs;
    rregs.rs    = (instr >> 21) & BITS_5;
    rregs.rt    = (instr >> 16) & BITS_5;
    rregs.rd    = (instr >> 11) & BITS_5;
    rregs.shamt = (instr >> 6) & BITS_5;
    rregs.funct = instr & BITS_6;

    // put values into rVals
    rVals->R_rs = rregs.rs;
    rVals->R_rt = rregs.rt;
    rVals->R_rd = rregs.rd;
    
    // put values in d
    d->regs.r = rregs;
    
    //Else if op is 2 or 3 then J format  
  }else if(inst_op == 2 || inst_op == 3 ) {

    // declare the instruction to be J-format
    d->type = 2; 

    //get addr
    JRegs jregs;
    jregs.target = instr & BITS_26;

    // nothing to put here
    rVals->R_rs = -1;
    rVals->R_rt = -1;
    rVals->R_rd = jregs.target;

    d->regs.j = jregs;

  //Else is I format
  }else {

    // declare the instruction to be I-format
    d->type = 1;

    // Get I-format registers and immed
    IRegs iregs;
    iregs.rs = (instr >> 21) & BITS_5;
    iregs.rt = (instr >> 16) & BITS_5;
    iregs.addr_or_immed = instr & BITS_16;
    
    // put values in rVals
    rVals->R_rs = iregs.rs;
    rVals->R_rt = iregs.rt;
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

  printf("%s ", opName);


  // R-format print
  if(d->type == 0) {
    if(!strcmp(opName, "jr")){ // if jr then only print two values
      printf(" %#010x\n", d->regs.r.rs);
    }else{
      printf(" $%d, $%d, $%d\n", d->regs.r.rs,d->regs.r.rt,d->regs.r.rd);
    }
  }else if(d->type ==1){
    if(!strcmp(opName, "lui")){
      printf(" $%d, %#010x\n", d->regs.i.rt,d->regs.i.addr_or_immed);
    } else if(!strcmp(opName, "")){

    }
    else{
      printf(" $%d, $%d, %#010x\n", 
          d->regs.i.rt,d->regs.i.rs,d->regs.i.addr_or_immed);
    }
  }else{
   printf("%#010x\n", d->regs.j.target); 
  }

  // if is a J-format then print only the address
  /* if(d->type == 2){ */
  /*   printf("%#010x\n", rVals.R_rd); */
  /*   return; */
  /* } */
  /*  */
  /* // If there is an rs val then print it */
  /* if(rVals.R_rs > -1) */
  /*   printf("$%d, ", rVals.R_rs); */
  /*  */
  /* // If there is an rt val then print it */
  /* if(rVals.R_rt > -1) */
  /*   printf("$%d, ", rVals.R_rt); */
  /*  */
  /* // If there is an rd val then print it */
  /* if(rVals.R_rd > -1) */
  /*   printf("$%d\n", rVals.R_rd); */
}

/* Perform computation needed to execute d, returning computed value */
int Execute ( DecodedInstr* d, RegVals* rVals) {
    /* Your code goes here */
  return 0;
}

/* 
 * Update the program counter based on the current instruction. For
 * instructions other than branches and jumps, for example, the PC
 * increments by 4 (which we have provided).
 */
void UpdatePC ( DecodedInstr* d, int val) {
    mips.pc+=4;
    /* Your code goes here */
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
    /* Your code goes here */
  return 0;
}

/* 
 * Write back to register. If the instruction modified a register--
 * (including jal, which modifies $ra) --
 * put the index of the modified register in *changedReg,
 * otherwise put -1 in *changedReg.
 */
void RegWrite( DecodedInstr* d, int val, int *changedReg) {
    /* Your code goes here */
}
