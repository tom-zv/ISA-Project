#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "main.h"

#define REG_INSTRUCTION_SIZE 1
#define IMM_INSTRUCTION_SIZE 3


#define IMEM_LINE_SIZE 12
#define DMEM_LINE_SIZE 8
#define DISK_LINE_SIZE 8
#define TRACE_LINE_SIZE 160

#define NUM_OF_INST_FIELDS 7
#define NUM_OF_REGISTERS 16
#define NUM_OF_IO_REGISTERS 23


#define MEM_SIZE 4096
#define DISK_SIZE 128 * 128
#define MONITOR_BUFF_SIZE 256
#define MAX_LINES 16385

#define PC_SIZE 12
#define WORD 32


/*
  description: 
	malloc wrapper with added check
 
*/
void* calloc_and_check(size_t count, size_t elem_size) {

	void* ptr = calloc(count, elem_size);

	if (ptr == NULL) {
		printf("Memory assignment error encountered\nTerminating program...");
		exit(-1);
	}

	return (ptr);
}


/*
  description:
	 read data from file, returns malloc`d data

*/
void* readfile(char* p_fname, int line_size, int data_size) {

	FILE* fptr;
	int i = 0;

	char* p_line_buffer = calloc_and_check(line_size, sizeof(char));
	char* data = calloc_and_check(data_size * (line_size + 1), sizeof(char));

	memset(data, '0', data_size * (line_size));

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

		//p_line_buffer[strcspn(p_line_buffer, "\n")] = 0;

		memcpy(data + i * line_size, p_line_buffer, line_size);

		//printf("data = %s  line = %s\n", data,p_line_buffer);

		/*int j;
		printf("\n[%d] ", i);
		for (j = 0; j < IMEM_LINE_SIZE; j++) {
			printf("%c", data[i * (IMEM_LINE_SIZE) + j] );
		}*/

		i++;
	}

	//data[(i-1) * line_size ] = 'F';    /// init with calloc for zeroed arrays. manually add '\0' when accessing file 
	//data[(i-1) * line_size +1] = 'N';
	fclose(fptr);

	return data;
}

/*
  description:
	 write data to output file.

*/
void* writefile(char* p_fname, int line_size, char* data_buffer) {

	FILE* fptr;
	int buffer_index = 0;
	int line_num = 0;
	char* line = calloc_and_check(line_size + 1, sizeof(char));


	fopen_s(&fptr, p_fname, "w");
	if (fptr == NULL) {
		printf("IO error encountered\nTerminating program...");
		exit(-1);
	}


	while (line_num < MAX_LINES) {      // max size to prevent infinite loop if null character cant be found.

		memcpy(line, data_buffer + buffer_index, line_size);
		buffer_index += line_size;

		line[line_size] = '\0';

		if (data_buffer[buffer_index] != '\0') {     // check if next line is the last, if it is, dont print a newline

			fprintf(fptr, "%s\n", line);
		}
		else {
			fprintf(fptr, "%s", line);
			break;
		}
		
		line_num++;
		
	}
	

	fclose(fptr);
	free(line);
}

/*
  description:
	Converts 'hex_size' digits of hex to decimal.

	params:
		in hex_data - buffer containing the hex representation.
		in hex_size - number of digits to convert.
		in,out hex_index - used to increment index when converting a stream of hex numbers. input NULL when no need for increment. 
*/
int hex_to_dec(char *hex_data, int hex_size, int *hex_index) {

	char* hex = calloc_and_check(hex_size + 1, sizeof(char));
	memcpy(hex, hex_data, hex_size);
	hex[hex_size] = '\0';

	int decimal = strtol(hex, NULL, 16); 

	if (hex_size > 1 ) {     // signed hex

		printf("\nyippe?\n");


	}



	free(hex);

	if (hex_index != NULL) {
		*hex_index += hex_size;
	}

	return decimal;
}


/*
  description:
	decimal to digit hex, stored in indicated array.

	params:
		in hex - array that stores the hex representation. 
		in dec - decimal number to convert.
		in size - number of digits in hex representation - Padded with zeros
*/
void dec_to_hex(char* hex, int dec, int size) {  //signed hex needed.

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



/*
  description:
	implementation of strtol for signed binary.

*/
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

/*
  description:
	implementation of logical right shift.

*/
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



/*
  description:
	copy source register to destination register, taking the lower 'dst_size' bits.
	'registers' are char arrays containing binary or hex representations of integers.

*/
void copy_register(char *dst_register, char *src_register, int dst_size) {

	int i;
	
	for (i = 0; i < dst_size; i++) {

		dst_register[i] = src_register[WORD - dst_size + i] ;
	}
}


/*
  description:
	converts decimal to binary, storing the resultant signed binary string in indicated register array.

*/
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

/*
  description:
	increment binary string representation by one.

*/
void increment_binary(char* binary, int size) {

	int bin_int = strtol(binary, NULL, 2);

	bin_int += 1;
	set_register(binary, bin_int, size);

}

/*
  description:
	constructs the trace for the current clock.

*/
void build_trace(char* trace, char* PC, char* instruction, char R[NUM_OF_REGISTERS][WORD + 1]) {

	int PC_int = strtol(PC, NULL, 2);    //
	char PC_hex[3];						 // convert PC from binary to 3 digit hex representation.
	dec_to_hex(PC_hex, PC_int, 3);	 	 //

	int trace_index = 0;
	char register_hex[8];
	int i;


	memcpy(trace + trace_index, PC_hex, 3);
	trace_index += 3;
	memset(trace + trace_index, ' ', 1);
	trace_index += 1;

	memcpy(trace + trace_index, instruction, IMEM_LINE_SIZE);
	trace_index += IMEM_LINE_SIZE;
	memset(trace + trace_index, ' ', 1);
	trace_index += 1;

	for (i = 0; i < NUM_OF_REGISTERS; i++) {

		int register_int = signed_binary_strtol(R[i]);       // convert registers from binary to 8 digit hex representation.
		dec_to_hex(register_hex, register_int, 8);	   //

		memcpy(trace + trace_index, register_hex, DMEM_LINE_SIZE);
		trace_index += DMEM_LINE_SIZE;

		if (i != NUM_OF_REGISTERS - 1) {          // if not on last register, add whitespace.
			memset(trace + trace_index, ' ', 1);
			trace_index += 1;
		}


	}

}
/*
  description:
	refreshes decimal value IO registers from their binary representation array.

*/
void fetch_IO(int *IO[], char IO_R[NUM_OF_IO_REGISTERS][WORD + 1]) {

	int i;

	for (i = 0; i < 23; i++) {
		*IO[i] = strtol(IO_R[i], NULL, 2);
	}


}


/*
  description:
	decodes the given instruction from hex to dec, storing the results in the given array of instruction fields.

*/
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

/*
  description:

	executes a single assembly instruction.

*/
void execute_instruction(int *instruction_fields_array, char R[NUM_OF_REGISTERS][WORD + 1], char IO_R[NUM_OF_IO_REGISTERS][WORD + 1], char * dmem, char *PC) {

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
			
			copy_register(PC, R[rm], PC_SIZE);
			set_register(IO_R[18], 1, WORD); // reserved1 = 1, to signal PC doesnt need to be incremented

		}

		break;

	case 10: //bne

		printf("opcode %d, instruction: bne\n", opcode);

		if (signed_binary_strtol(R[rs]) != signed_binary_strtol(R[rt])) {

			copy_register(PC, R[rm], PC_SIZE);
			set_register(IO_R[18], 1, WORD); // reserved1 = 1, to signal PC doesnt need to be incremented
		}

		break;

	case 11: //blt

		printf("opcode %d, instruction: blt\n", opcode);

		if (signed_binary_strtol(R[rs]) < signed_binary_strtol(R[rt])) {

			copy_register(PC, R[rm], PC_SIZE);
			set_register(IO_R[18], 1, WORD); 
		}

		break;

	case 12: //bgt

		printf("opcode %d, instruction: bgt\n", opcode);

		if (signed_binary_strtol(R[rs]) > signed_binary_strtol(R[rt])) {

			copy_register(PC, R[rm], PC_SIZE);
			set_register(IO_R[18], 1, WORD); 
		}

		break;

	case 13: //ble

		printf("opcode %d, instruction: ble\n", opcode);

		if (signed_binary_strtol(R[rs]) <= signed_binary_strtol(R[rt])) {

			copy_register(PC, R[rm], PC_SIZE);
			set_register(IO_R[18], 1, WORD); 
		}

		break;

	case 14: //bge

		printf("opcode %d, instruction: bge\n", opcode);

		if (signed_binary_strtol(R[rs]) >= signed_binary_strtol(R[rt])) {

			copy_register(PC, R[rm], PC_SIZE);
			set_register(IO_R[18], 1, WORD); 
		}

		break;

	case 15: //jal

		printf("opcode %d, instruction: jal\n", opcode);

		result = strtol(PC, NULL, 2) + 1;
		set_register(R[rd], result, WORD);
		
		copy_register(PC, R[rm], PC_SIZE);
		set_register(IO_R[18], 1, WORD); 

		break;
		

	case 16: //lw

		printf("opcode %d, instruction: lw\n", opcode);
	
		index = signed_binary_strtol(R[rs]) + signed_binary_strtol(R[rt]);
		if (index < 0) {
			printf("\n *Assembly instructions bug: attempting write to negative index; skipping instruction!*\n\n");
			break;
		}
		result = hex_to_dec(dmem + index * DMEM_LINE_SIZE, DMEM_LINE_SIZE, NULL) + signed_binary_strtol(R[rm]) ;
		set_register(R[rd], result, WORD);

		break;

	case 17: //sw

		printf("opcode %d, instruction: sw\n", opcode);



		index = signed_binary_strtol(R[rs]) + signed_binary_strtol(R[rt]);

		if (index < 0) {
			printf("\n *Assembly instructions bug: attempting write to negative index; skipping instruction!*\n\n");
			break;
		}
		result = signed_binary_strtol(R[rm]) + signed_binary_strtol(R[rd]);
		
		char hex[DMEM_LINE_SIZE];
		dec_to_hex(hex, result, DMEM_LINE_SIZE);
		memcpy(dmem + DMEM_LINE_SIZE * index, hex, DMEM_LINE_SIZE);


		break;
		

	case 18: //reti

		printf("opcode %d, instruction: reti\n", opcode);

		copy_register(PC, IO_R[7], PC_SIZE);
		set_register(IO_R[18], 1, WORD); 

		break;

	case 19: //in

		printf("opcode %d, instruction: in\n", opcode);

		index = signed_binary_strtol(R[rs]) + signed_binary_strtol(R[rt]);
		if (index < 0) {
			printf("\n *Assembly instructions bug: attempting write to negative index; skipping instruction!*\n\n");
			break;
		}

		result = IO_R[index];
		set_register(R[rd], result, WORD);

		break;

	case 20: //out

		printf("opcode %d, instruction: out\n", opcode);

		index = signed_binary_strtol(R[rs]) + signed_binary_strtol(R[rt]);
		if (index < 0) {
			printf("\n *Assembly instructions bug: attempting write to negative index; skipping instruction!*\n\n");
			break;
		}

		result = signed_binary_strtol(R[rm]);
		set_register(IO_R[index], result, WORD);

		break;
	
	case 21:
		printf("HALT;\n");
		exit(1);            // TODO replace exit with halt flag, so that only the while loop ends.

	default:  //halt; change to case 21?
		printf("HALT;");
		//exit(1);
	}




}


void main(int argc, char* argv[]) {

	char *imem, *dmem, *disk;
	
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

	int instruction_fields_array[NUM_OF_INST_FIELDS];
	int disk_read_end = 0;
	int irq;
	int trace_size = 1024 * TRACE_LINE_SIZE;    // initial buffer for trace output will hold 1024 lines, later realloc if needed. 

	char* trace = calloc_and_check(trace_size, sizeof(char));
	int* frame_buffer = calloc_and_check(MONITOR_BUFF_SIZE * MONITOR_BUFF_SIZE, sizeof(char));
	

	//-------------------------------------------------------------------------------------       MANUAL ASSIGNMENT 

	int dec = 1;
	set_register(registers[7], dec, WORD);    // rs
	dec = 0;
	set_register(registers[8], dec, WORD);    // rt 
	dec = 1;
	set_register(registers[9], dec, WORD);    // rm

	//set_register(IO_registers[11], 1, WORD);   // timerenable = 1 

	//set_register(IO_registers[14], 1, WORD);   // diskcmd = 0 

	//set_register(IO_registers[16], 128, WORD); // diskbuffer = 0

	//set_register(IO_registers[15], 0, WORD);   // disksector = 0;
	
	
	//--------------------------------------------------------------------------------------


	
	//-----------------------------------------------------------------------------------------------------------------\\
	//-----------------------------------------------------------------------------------------------------------------\\	            MAIN LOOP

	int tmp_cond = 0; // while loop exit condition, temporary for controlling how many instructions are ran.
	
	while (tmp_cond != 3) {

		fetch_IO(IO, IO_registers);

		if (trace_size = clks * TRACE_LINE_SIZE * sizeof(char) ) {   // trace buffer realloc, doubling the old buffer size.

			trace_size *= 2;
			realloc(trace, trace_size);

			if (trace == NULL) {
				printf("Memory assignment error encountered\nTerminating program...");
				exit(-1);
			}


		}


		if (timercurrent == timermax) {

			irq0status = 1;
			set_register(IO_registers[3], 1, WORD);

			timercurrent = 0;
			set_register(IO_registers[12], 0, WORD);  
		}
		
		irq = (irq0enable & irq0status) | (irq1enable & irq1status) | (irq2enable & irq2status);        


		decode_instruction(imem + strtol(PC, NULL, 2) * IMEM_LINE_SIZE, instruction_fields_array);

		
		build_trace(trace + (TRACE_LINE_SIZE * clks), &PC, imem + strtol(PC, NULL, 2) * IMEM_LINE_SIZE, &registers);

		//PC[PC_SIZE] = '\0';
		//printf("PC = %s | current time = %d\n", PC, timercurrent);

		execute_instruction(&instruction_fields_array, &registers, &IO_registers, dmem, &PC);
		fetch_IO(IO, IO_registers);


		if (diskcmd != 0) {

			if (disk_read_end == 0) {

				if (diskcmd == 1) {      // read from disk

					memcpy(dmem + diskbuffer * DISK_LINE_SIZE , disk + (disksector * DISK_LINE_SIZE * 128), 128 * DISK_LINE_SIZE);   // read in one go.

				}
				else {                  // write to disk

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

			frame_buffer[monitoraddr] = monitordata;

		}

		if (timerenable) {   

			increment_binary(IO_registers[12], WORD);  

		}

		if (res1 == 0) {

			increment_binary(PC, PC_SIZE);  // increment if PC wasnt already set, using reserve IO to avoid passing another variable to execute_instruction.

		}

		increment_binary(IO_registers[8], WORD); // clks++
		tmp_cond++;
	}

	//-----------------------------------------------------------------------------------------------------------------\\
	//-----------------------------------------------------------------------------------------------------------------\\



	//registers[3][WORD] = '\0';              // print result in v0, for debugging.
	//printf("\nresult\nsigned binary: %s  |  dec: %d\n", registers[3], signed_binary_strtol(registers[3]));

	//disk[128 * DISK_LINE_SIZE] = '\0';
	//dmem[128 * DISK_LINE_SIZE] = '\0';

	//printf(" disk = %s\n\n", disk);
	//printf(" dmem = %s\n",   dmem);

	//dmem[128 + DISK_LINE_SIZE * 128] = '\0';
	//printf("dmem after disk =  %s", dmem);
	
	fetch_IO(IO, IO_registers);


	int k;
	int val;
	int index = 0;

	char* regout = calloc_and_check( DMEM_LINE_SIZE *( NUM_OF_REGISTERS - 3), sizeof(char));
	memset(regout + DMEM_LINE_SIZE * (NUM_OF_REGISTERS - 3), '\0', 1);

	for (k = 3; k < NUM_OF_REGISTERS; k++) {
		
		val = signed_binary_strtol(registers[k]);
		dec_to_hex(regout + index, val, DMEM_LINE_SIZE);
		index += DMEM_LINE_SIZE;
		
	}


	
	//writefile(argv[5], DMEM_LINE_SIZE, dmem);
	//writefile(argv[6], DMEM_LINE_SIZE, regout);

	trace[clks * TRACE_LINE_SIZE] = '\0';  // clks overflow counter?
	writefile(argv[7], 160, trace);


	/*int oi = -69;
	char lmao[8+1];
	lmao[8] = '\0';
	dec_to_hex(lmao, oi, 8);

	printf("negative hex = %s", lmao);*/


	registers[1][WORD] = '\0';
	printf("imm1 neg = %s", registers[1]);

	free(dmem);
	free(imem);
	free(disk);
	free(frame_buffer);
}