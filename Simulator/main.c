#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "main.h"

#define REG_INSTRUCTION_SIZE 1
#define IMM_INSTRUCTION_SIZE 3


#define IMEM_LINE_SIZE 12
#define DMEM_LINE_SIZE 8
#define DISK_LINE_SIZE 8

#define NUM_OF_INST_FIELDS 7
#define NUM_OF_REGISTERS 16
#define NUM_OF_IO_REGISTERS 23


#define MEM_SIZE 4096
#define DISK_SIZE 128 * 128
#define MONITOR_BUFF_SIZE 256

#define PC_SIZE 12
#define WORD 32


// malloc wrapper with added check
void* malloc_and_check(size_t bytes) {

	void* ptr = malloc(bytes);

	if (ptr == NULL) {
		printf("Memory assignment error encountered\nTerminating program...");
		exit(-1);
	}

	return (ptr);
}

// read data from file, returns malloc`d data
void* readfile(char* p_fname, int line_size, int data_size) {

	FILE* fptr;
	int i = 0;

	char* p_line_buffer = malloc_and_check(line_size);
	char* data = malloc_and_check(data_size * (line_size + 1));

	fopen_s(&fptr, p_fname, "r");

	if (fptr == NULL) {
		printf("IO error encountered\nTerminating program...");
		exit(-1);
	}

	while (fgets(p_line_buffer, line_size + 2, fptr) != NULL) {

		if (i % MEM_SIZE == 0 && i != 0) {

			data_size += MEM_SIZE;
			char* temp_buffer = realloc(data, data_size * (line_size + 1));					// Increment instructions buffer 

			if (temp_buffer == NULL) {
				printf("Memory assignment error encountered\nTerminating program...");
				exit(-1);
			}

			data = temp_buffer;
		}

		p_line_buffer[strcspn(p_line_buffer, "\n")] = 0;
		memcpy(data + i * line_size, p_line_buffer, line_size + 2);

		/*int j;
		printf("\n[%d] ", i);
		for (j = 0; j < IMEM_LINE_SIZE; j++) {
			printf("%c", data[i * (IMEM_LINE_SIZE) + j] );
		}*/

		i++;
	}
	fclose(fptr);

	return data;
}

// Converts 'hex_size' bytes of hex to decimal. starts from hex_index. increments hex_index by number of bytes read, for the next conversion.
// input NULL for hex_index if there is no need for index icrementing.
int hex_to_dec(char *hex_data, int hex_size, int *hex_index) {

	char* hex = malloc_and_check(hex_size + 1);
	memcpy(hex, hex_data, hex_size);
	hex[hex_size] = '\0';

	int decimal = strtol(hex, NULL, 16); //TODO - signed hex strtol if it is needed.

	free(hex);

	if (hex_index != NULL) {
		*hex_index += hex_size;
	}

	return decimal;
}

// decimal to Hex
void dec_to_hex(char* hex, int dec, int size) {

	int temp;
	int q = dec;
	int j;

	for (j = 0; j < size; j++) {

		temp = q % 16;

		if (temp < 10)
			temp = temp + 48; 
		else
			temp = temp + 55;

		hex[size - 1 - j] = temp;
		q = q / 16;

	}
	
	
}

// converts dec to binary, storing the resultant signed binary string in indicated array.
void set_register(char* binary, int decimal, int len) {

	int i, j;

	for (i = len - 1; i >= 0; i--) {

		j = decimal >> i;

		if (j & 1) {
			binary[len - 1 - i] = '1';
		}
		else
			binary[len - 1 - i] = '0';
	}

	//binary[len] = '\0';   // for debug prints

	return;
}

// implementation of strtol for signed binary
int signed_binary_strtol(char *str) {
	
	if (str[0] == '1') {   // adjusted conversion for negative numbers.

		char flip_str[WORD + 1];
		int i;

		for (i = 0; i < WORD; i++) {      // flip all bits of the signed representation

			if (str[i] == '0') {
				flip_str[i] = '1';
			}							 
			else {                       
				flip_str[i] = '0';
			}

		}

		int num = -strtol(flip_str, NULL, 2) - 1 ;   // strtol of flipped bits, minus one, gives us the correct conversion for signed negative numbers.
		return num;
	}


	int num = strtol(str, NULL, 2);
	return num;

}

// manual srl 
int right_logical_shift(char* binary, int shamt) {

	if (shamt == 0) {
		return signed_binary_strtol(binary);
	}

	int i,j;
	char shifted_binary[WORD + 1];

	for (i = 0; i < shamt; i++) {

		for (j = WORD - 1; j > 0; j--) {

			shifted_binary[j] = binary[j - 1];

		}

		shifted_binary[0] = '0';
	}

	return signed_binary_strtol(shifted_binary);
}

// increment binary string representation by one.
void increment_binary(char *binary, int size) {

	int bin_int = strtol(binary, NULL, 2);

	bin_int += 1;
	set_register(binary, bin_int, size);

}

// set PC to indicated binary string, taking the lower 12 bits.
void set_PC(char* PC, char* binary) {

	int i;
	
	for (i = 0; i < PC_SIZE; i++) {

		PC[i] = binary[WORD - PC_SIZE + i] ;
	}
}

// refreshes decimal value IO registers
int fetch_IO(int *IO[], char IO_R[NUM_OF_IO_REGISTERS][WORD + 1]) {

	int i;

	for (i = 0; i < 23; i++) {
		*IO[i] = strtol(IO_R[i], NULL, 2);
	}


}

// decodes the given instruction from hex to dec, storing the results in the given array of instruction fields.
void decode_instruction(char* hex_data, int* field_array) {

	int hex_index = 0;

	int opcode = hex_to_dec(hex_data + hex_index, 2, &hex_index);
	int rd = hex_to_dec(hex_data + hex_index, REG_INSTRUCTION_SIZE, &hex_index);
	int rs = hex_to_dec(hex_data + hex_index, REG_INSTRUCTION_SIZE, &hex_index);  // hex_index incremented inside func
	int rt = hex_to_dec(hex_data + hex_index, REG_INSTRUCTION_SIZE, &hex_index);
	int rm = hex_to_dec(hex_data + hex_index, REG_INSTRUCTION_SIZE, &hex_index);
	int imm1 = hex_to_dec(hex_data + hex_index, IMM_INSTRUCTION_SIZE, &hex_index);
	int imm2 = hex_to_dec(hex_data + hex_index, IMM_INSTRUCTION_SIZE, &hex_index);

	*field_array++ = opcode; *field_array++ = rd; *field_array++ = rs; *field_array++ = rt; *field_array++ = rm; *field_array++ = imm1; *field_array++ = imm2;

	//printf(" %d | %d | %d | %d | %d | %d | %d |\n", opcode, rd, rs, rt, rm, imm1, imm2);
}

// executes an instruction.
void execute_instruction(int *instruction_fields_array, char R[NUM_OF_REGISTERS][WORD + 1], char IO_R[NUM_OF_IO_REGISTERS][WORD + 1], char *dmemin, char *PC) {

	int opcode = instruction_fields_array[0];
	int rd = instruction_fields_array[1];
	int rs = instruction_fields_array[2];		//
	int rt = instruction_fields_array[3];		// extract the instruction fields from array to variables, for readability.
	int rm = instruction_fields_array[4];		//
	int imm1 = instruction_fields_array[5];
	int imm2 = instruction_fields_array[6];

	set_register(R[1], imm1, WORD);		//  store imm1 and imm2 in their respective registers.
	set_register(R[2], imm2, WORD);    //

	int result;
	int index;
	

	// TODO : writing to zero/imm1/imm2 doesnt change them

	switch (opcode) {

	case 0:  //add

		//printf("opcode %d, instruction: add\n", opcode);

		result = signed_binary_strtol(R[rs]) + signed_binary_strtol(R[rt]) + signed_binary_strtol(R[rm]);
		set_register(R[rd], result, WORD);

		//R[rd][WORD] = '\0';
		//printf("result %s\n", R[rd]);

		break;

	case 1: //sub


		printf("opcode %d, instruction: sub\n", opcode);

		result = signed_binary_strtol(R[rs]) - signed_binary_strtol(R[rt]) - signed_binary_strtol(R[rm]);
		set_register(R[rd], result, WORD);

		//R[rd][WORD] = '\0';
		//printf("result %s\n", R[rd]);
		break;

	case 2: //mac

		printf("opcode %d, instruction: mac\n", opcode);

		result = signed_binary_strtol(R[rs]) * signed_binary_strtol(R[rt]) - signed_binary_strtol(R[rm]);
		set_register(R[rd], result, WORD);

		//R[rd][WORD] = '\0';
		//printf("result %s\n", R[rd]);
		break;

	case 3: //and

		printf("opcode %d, instruction: and\n", opcode);
		
		result = signed_binary_strtol(R[rs]) & signed_binary_strtol(R[rt]) & signed_binary_strtol(R[rm]);
		set_register(R[rd], result, WORD);

		//R[rd][WORD] = '\0';
		//printf("result %s\n", R[rd]);
		break;

	case 4: //or

		printf("opcode %d, instruction: or\n", opcode);
		
		result = signed_binary_strtol(R[rs]) | signed_binary_strtol(R[rt]) | signed_binary_strtol(R[rm]);
		set_register(R[rd], result, WORD);

		//R[rd][WORD] = '\0';
		//printf("result %s\n", R[rd]);
		break;

	case 5: //xor

		printf("opcode %d, instruction: xor\n", opcode);

		result = signed_binary_strtol(R[rs]) ^ signed_binary_strtol(R[rt]) ^ signed_binary_strtol(R[rm]);
		set_register(R[rd], result, WORD);

		//R[rd][WORD] = '\0';
		//printf("result %s\n", R[rd]);
		break;

	case 6: //sll

		printf("opcode %d, instruction: sll\n", opcode);
		
		result = signed_binary_strtol(R[rs]) << signed_binary_strtol(R[rt]);                                    // sll - shifting is cyclical, 32 is a zero shift.
		set_register(R[rd], result, WORD);

		//R[rd][WORD] = '\0';
		//printf("result %s\n", R[rd]);

		break;

	case 7: //sra

		printf("opcode %d, instruction: sra\n", opcode);

		result = signed_binary_strtol(R[rs]) >> signed_binary_strtol(R[rt]);                       // sra - for positive nums, reaches 0 (000...) and stays there for increased shift amounts.
		set_register(R[rd], result, WORD);																			   //       for negative nums, reaches -1 (111...) and stays there,
		break;

	case 8: //srl

		printf("opcode %d, instruction: srl\n", opcode); 

		result = right_logical_shift(R[rs], signed_binary_strtol(R[rt]));
		set_register(R[rd], result, WORD);

		break; 
		
		

	case 9: //beq

		printf("opcode %d, instruction: beq\n", opcode);

		if (signed_binary_strtol(R[rs]) == signed_binary_strtol(R[rt])) {
			
			set_PC(PC, R[rm]);
		}

		break;

	case 10: //bne

		printf("opcode %d, instruction: bne\n", opcode);

		if (signed_binary_strtol(R[rs]) != signed_binary_strtol(R[rt])) {

			set_PC(PC, R[rm]);
		}

		break;

	case 11: //blt

		printf("opcode %d, instruction: blt\n", opcode);

		if (signed_binary_strtol(R[rs]) < signed_binary_strtol(R[rt])) {

			set_PC(PC, R[rm]);
		}

		break;

	case 12: //bgt

		printf("opcode %d, instruction: bgt\n", opcode);

		if (signed_binary_strtol(R[rs]) > signed_binary_strtol(R[rt])) {

			set_PC(PC, R[rm]);
		}

		break;

	case 13: //ble

		printf("opcode %d, instruction: ble\n", opcode);

		if (signed_binary_strtol(R[rs]) <= signed_binary_strtol(R[rt])) {

			set_PC(PC, R[rm]);
		}

		break;

	case 14: //bge

		printf("opcode %d, instruction: bge\n", opcode);

		if (signed_binary_strtol(R[rs]) >= signed_binary_strtol(R[rt])) {

			set_PC(PC, R[rm]);
		}

		break;

	case 15: //jal

		printf("opcode %d, instruction: jal\n", opcode);

		result = strtol(PC, NULL, 2) + 1;
		set_register(R[rd], result, WORD);
		
		set_PC(PC, R[rm]);

		break;
		

	case 16: //lw

		printf("opcode %d, instruction: lw\n", opcode);

		index = signed_binary_strtol(R[rs]) + signed_binary_strtol(R[rt]);
		result = hex_to_dec(dmemin + index * DMEM_LINE_SIZE, DMEM_LINE_SIZE, NULL) + signed_binary_strtol(R[rm]) ;
		set_register(R[rd], result, WORD);

		break;

	case 17: //sw

		printf("opcode %d, instruction: sw\n", opcode);

		index = signed_binary_strtol(R[rs]) + signed_binary_strtol(R[rt]);
		result = signed_binary_strtol(R[rm]) + signed_binary_strtol(R[rd]);
		
		char hex[DMEM_LINE_SIZE];
		dec_to_hex(hex, result, DMEM_LINE_SIZE);
		memcpy(dmemin + DMEM_LINE_SIZE * index, hex, DMEM_LINE_SIZE);


		break;
		

	case 18: //reti

		printf("opcode %d, instruction: reti\n", opcode);

		set_PC(PC, IO_R[7]);                // 

		break;

	case 19: //in

		printf("opcode %d, instruction: in\n", opcode);

		result = IO_R[signed_binary_strtol(R[rs]) + signed_binary_strtol(R[rt])];
		set_register(R[rd], result, WORD);

		break;

	case 20: //out

		printf("opcode %d, instruction: out\n", opcode);

		index = signed_binary_strtol(R[rs]) + signed_binary_strtol(R[rt]);
		result = signed_binary_strtol(R[rm]);
		set_register(IO_R[index], result, WORD);

		break;
	
	case 21:
		printf("HALT;");
		exit(1);

	default:  //halt; change to case 21?
		printf("HALT;");
		//exit(1);
	}




}


void main(int argc, char* argv[]) {

	char *imem, *dmem, *disk;
	int frame_buffer = malloc_and_check(MONITOR_BUFF_SIZE * MONITOR_BUFF_SIZE);

	imem = readfile(argv[1], IMEM_LINE_SIZE, MEM_SIZE);  // contains data in sequence, imemin[ i * data_size] for the i+1 line start address.
	dmem = readfile(argv[2], DMEM_LINE_SIZE, MEM_SIZE);  
	disk = readfile(argv[3], DISK_LINE_SIZE, DISK_SIZE);

	char PC[PC_SIZE + 1];
	set_register(PC, 0, PC_SIZE);

	char registers[NUM_OF_REGISTERS][WORD+1];           // +1 for null character used in debugging prints. TODO
	char IO_registers[NUM_OF_IO_REGISTERS][WORD + 1];
	
	int irq0enable = 0, irq1enable = 0, irq2enable = 0, irq0status = 0, irq1status = 0, irq2status = 0;	
	int irq2statusirqhandler = 0, irqreturn = 0, clks = 0, leds = 0, display7seg = 0;
	int res1 = 0, res2 = 0, monitoraddr = 0, monitordata = 0, monitorcmd = 0;                                // initialize decimal value IO registers by name for readability
	int diskcmd = 0, disksector = 0, diskbuffer = 0, diskstatus = 0;
	int timerenable = 0, timercurrent = 0, timermax = 0;
	
	int *IO[] = { &irq0enable, &irq1enable, &irq2enable, &irq0status, &irq1status, &irq2status, &irq2statusirqhandler, &irqreturn,
				  &clks, &leds, &display7seg, &timerenable, &timercurrent, &timermax, &diskcmd, &disksector, &diskbuffer, &diskstatus,
				  &res1, &res2, &monitoraddr, &monitordata, &monitorcmd };

	int inst_field_array[NUM_OF_INST_FIELDS];
	int irq;
	int disk_read_end = 0;

	//

	int dec = 1;
	set_register(registers[7], dec, WORD);  // rs
	dec = 0;
	set_register(registers[8], dec, WORD);  // rt 
	dec = 1;
	set_register(registers[9], dec, WORD);  // rm

	set_register(IO_registers[11], 1, WORD); // timerenable = 1 

	set_register(IO_registers[14], 1, WORD); // diskcmd = 1 

	set_register(IO_registers[16], 0, WORD); // diskbuffer = 0

	set_register(IO_registers[15], 0, WORD); // disksector = 0;
	//


	
	//-----------------------------------------------------------------------------------------------------------------\\
	//-----------------------------------------------------------------------------------------------------------------\\	

	int tmp_cond = 0; // while loop exit condition, temporary for controlling how many instructions are ran.
	
	while (tmp_cond != 1025) {

		fetch_IO(IO, IO_registers);

		if (timercurrent == timermax) {
			irq0status = 1;
			set_register(IO_registers[3], 1, WORD);

			timercurrent = 0;
			set_register(IO_registers[12], 0, WORD);  
		}
		
		irq = (irq0enable & irq0status) | (irq1enable & irq1status) | (irq2enable & irq2status);        //TODO delete fetch irq


		decode_instruction(imem + strtol(PC, NULL, 2) * IMEM_LINE_SIZE, inst_field_array);

		PC[PC_SIZE] = '\0';
		//printf("PC = %s | current time = %d\n", PC, timercurrent);

		execute_instruction(&inst_field_array, &registers, &IO_registers, dmem, &PC);
		fetch_IO(IO, IO_registers);


		if (diskcmd != 0) {

			if (disk_read_end == 0) {

				if (diskcmd == 1) {      // read

					memcpy(dmem + diskbuffer , disk + (disksector * DISK_LINE_SIZE * 128), 128 * DISK_LINE_SIZE);   // read in one go.

				}
				else {                  // write

					memcpy(disk + (disksector * DISK_LINE_SIZE * 128), dmem + diskbuffer, 128 * DISK_LINE_SIZE);
				}

				set_register(IO_registers[17], 1, WORD); // diskstatus = 1
				disk_read_end = clks + 1024;
			}

			if(clks == disk_read_end ) {

				set_register(IO_registers[14], 0, WORD);    // diskcmd = 0
				set_register(IO_registers[17], 0, WORD);   // diskstatus = 0
				set_register(IO_registers[4], 1, WORD);   // irq1status = 1
				disk_read_end = 0;

			}


		}

		if (monitorcmd == 1) {
			//frame_buffer[monitoraddr] = monitordata;
		}

		if (timerenable) {    
			increment_binary(IO_registers[12], WORD);  
		}

		increment_binary(PC, PC_SIZE);  //  TODO add logic to increment PC here OR from the instruction.
		increment_binary(IO_registers[8], WORD);

		tmp_cond++;
	}

	//-----------------------------------------------------------------------------------------------------------------\\
	//-----------------------------------------------------------------------------------------------------------------\\



	registers[3][WORD] = '\0';              // print result in v0, for debugging.
	//printf("\nresult\nsigned binary: %s  |  dec: %d\n", registers[3], signed_binary_strtol(registers[3]));

	disk[128 * DISK_LINE_SIZE] = '\0';
	printf(" disk = %s\n", disk);

	//dmem[128 + DISK_LINE_SIZE * 128] = '\0';
	//printf("dmem after disk =  %s", dmem);
	

	free(dmem);
	free(imem);
	free(disk);
	free(frame_buffer);
}