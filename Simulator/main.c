#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define REG_INSTRUCTION_SIZE 1
#define IMM_INSTRUCTION_SIZE 3

#define TRACE_LINE_SIZE 160
#define MONITOR_LINE_SIZE 2
#define IMEM_LINE_SIZE 12
#define DMEM_LINE_SIZE 8
#define DISK_LINE_SIZE 8

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

	//data[(i-1) * line_size ] =   'F';    /// init with calloc for zeroed arrays. manually add '\0' when accessing file 
	//data[(i-1) * line_size +1] = 'N';
	fclose(fptr);

	return data;
}

/*
  description:
	 read irq2 file.

*/
int readirq2(char* p_fname, int *irq2_up_clocks, int irq2_up_clocks_buffer_size) {

	FILE* fptr;

	int irq2_clock;
	int char_read = 0;
	int line_buff_size = 128;
	
	
	irq2_up_clocks_buffer_size*=  sizeof(int);   

	if (irq2_up_clocks_buffer_size <= 8) {
		printf("irq2 buff too small!\n");   //check for suppressing a warning.
		exit(1);
	}

	int number_of_clocks = 0;   // number of clocks irq2 is up that were saved.
	char chunk[5];
	int chunk_size = sizeof(chunk);

	char* line = calloc_and_check(line_buff_size, sizeof(char));

	fopen_s(&fptr, p_fname, "r");

	if (fptr == NULL) {
		printf("IO error encountered\nTerminating program...");
		exit(-1);
	}

	while ( fgets(chunk, chunk_size, fptr) != NULL) {  // read by chunks of size 8

		
		if (line_buff_size <= char_read) {   // realloc line if too small.

			line_buff_size *= 2;
		
			char* temp_buffer = realloc(line, line_buff_size);			

			if (temp_buffer == NULL) {
				printf("Memory assignment error encountered\nTerminating program...");
				exit(-1);
			}

			line = temp_buffer;
		}

		strncpy_s(line + char_read, chunk_size, chunk, chunk_size);
		char_read += strlen(chunk);

		if (irq2_up_clocks_buffer_size <= number_of_clocks * sizeof(int)) {   // realloc line if too small.

			irq2_up_clocks_buffer_size *= 2;

			char* temp_buffer = realloc(irq2_up_clocks, irq2_up_clocks_buffer_size);

			if (temp_buffer == NULL) {
				printf("Memory assignment error encountered\nTerminating program...");
				exit(-1);
			}

			irq2_up_clocks = temp_buffer;
		}


		if (line[char_read - 1] == '\n') {

			irq2_clock = strtol(line, NULL, 10);
			irq2_up_clocks[number_of_clocks++] = irq2_clock;
			
			memset(line, 0, line_buff_size);
			char_read = 0;
		}

	}

	irq2_clock = strtol(line, NULL, 10);     // last line, no newline char to catch.
	irq2_up_clocks[number_of_clocks++] = irq2_clock;

	fclose(fptr);
	free(line);

	return number_of_clocks;
}

/*
  description:
	 write data to output file.

	parameters:
	in p_fname
	in line_size - length of one line.
	in data_buffer - data is formatted without seperetors or newlines, i.e for data_buffer = "0000" and line_size = 2,  00  is the output	
																													    00
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
	 return number of digits in n

*/
int num_of_digits(int n) {

	int digit_num = 0;
	int temp_n = n;
	int i;

	while (temp_n != 0) {    // calculating number of digits the decimal clock value is for index incrementation.

		temp_n /= 10;
		digit_num++;

	}

	return digit_num;
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
	Converts 'hex_size' digits of hex to decimal.

	params:
		in hex_data - buffer containing the hex representation.
		in hex_size - number of digits to convert.
		in,out hex_index - used to increment index when converting a stream of hex numbers. input NULL when no need for increment. 
		in sign_flag - when flag is 1, return signed value of hex.
*/
int hex_to_dec(char *hex_data, int hex_size, int *hex_index, int signed_flag) {

	char* hex = calloc_and_check(hex_size + 1, sizeof(char));
	char* binary = calloc_and_check(hex_size * 4 + 1, sizeof(char));
	
	memcpy(hex, hex_data, hex_size);
	hex[hex_size] = '\0';

	int decimal = strtol(hex, NULL, 16); 

	if (signed_flag) {     // signed hex

		set_register(binary, decimal, hex_size * 4);   // convert hex to binary representation.

		decimal = signed_binary_strtol(binary, hex_size * 4);
		binary[ 4 * hex_size] = '\0';

		//printf(" HEX : '%s' | BINARY : %s | signed value : %d \n", hex, binary,decimal);
	}

	free(binary);
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
void dec_to_hex(char* hex, int dec, int size) { 

	int temp;
	long long q = dec;
	int j;

	if (q < 0) {              // Signed representation for negative numbers. 

		q = pow(2, ((double)size * 4)) + q;              // 2^bits + negative number ran through an unsigned conversion equals the signed conversion.

	}

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
int signed_binary_strtol(char *str, int size) {
	
	if (str[0] == '1') {   // adjusted conversion for negative numbers.

		char *flip_str = calloc_and_check(size + 1,sizeof(char));
		int i;

		for (i = 0; i < size; i++) {      // flip all bits of the signed representation

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
		return signed_binary_strtol(binary, WORD);
	}

	int i,j;
	char shifted_binary[WORD + 1];

	for (i = 0; i < shamt; i++) {

		for (j = WORD - 1; j > 0; j--) {

			shifted_binary[j] = binary[j - 1];

		}

		shifted_binary[0] = '0';
	}

	return signed_binary_strtol(shifted_binary, WORD);
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
void build_trace(char* trace, char* PC, char* instruction, char R[NUM_OF_REGISTERS][WORD + 1], int *trace_size, int clock) {

	int PC_int = strtol(PC, NULL, 2);    //
	char PC_hex[3];						 // convert PC from binary to 3 digit hex representation.
	dec_to_hex(PC_hex, PC_int, 3);	 	 //

	int trace_index = TRACE_LINE_SIZE * (clock - 1);
	char register_hex[8];
	int i;

	if (*trace_size == clock * TRACE_LINE_SIZE * sizeof(char)) {   // trace buffer realloc, doubling the old buffer size.

		// printf(" Reallocating 'trace' buffer.\nCurrent trace : %s || size = %d\n", trace, *trace_size);


		*trace_size *= 2;
		char* temp_buffer = realloc(trace, *trace_size);


		if (temp_buffer == NULL) {
			printf("Memory assignment error encountered\nTerminating program...");
			exit(-1);
		}

		trace = temp_buffer;

	}


	memcpy(trace + trace_index, PC_hex, 3);
	trace_index += 3;
	memset(trace + trace_index, ' ', 1);
	trace_index += 1;

	memcpy(trace + trace_index, instruction, IMEM_LINE_SIZE);
	trace_index += IMEM_LINE_SIZE;
	memset(trace + trace_index, ' ', 1);
	trace_index += 1;

	for (i = 0; i < NUM_OF_REGISTERS; i++) {

		int register_int = signed_binary_strtol(R[i], WORD);       // convert registers from binary to 8 digit hex representation.
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
	writes hwregtrace every time the registry is changed.

*/
void write_hwregtrace(char *fname, int *hw_info, int clock, int *first_operation) {


	if (hw_info[0] == 0) {   // no read/write this clock cycle.

		return;     

	}

	int digit_num = num_of_digits(clock);
	
	char register_names[23][21] = { "irq0enable", "irq1enable", "irq2enable", "irq0status", "irq1status", "irq2status", "irq2statusirqhandler", "irqreturn",
				  "clks", "leds", "display7seg", "timerenable", "timercurrent", "timermax", "diskcmd", "disksector", "diskbuffer", "diskstatus",
				  "res1", "res2", "monitoraddr", "monitordata", "monitorcmd" };


	char DATA[9];
	DATA[8] = '\0';
	dec_to_hex(DATA, hw_info[2], 8);

	int register_name_length = strlen(register_names[hw_info[0]]);

	FILE* fptr;
	if (*first_operation == 1) {
		fopen_s(&fptr, fname, "w");
	}
	else {
		fopen_s(&fptr, fname, "a");
	}

	if (fptr == NULL) {
		printf("IO error encountered\nTerminating program...");
		exit(-1);
	}


	

	if (hw_info[1] == 1) {   // READ operation.
		
		if (*first_operation == 1) {

			fprintf_s(fptr, "%d READ %s %s", clock, register_names[hw_info[0]], DATA);
			*first_operation = 0;

		}
		else {
			fprintf_s(fptr, "\n%d READ %s %s", clock, register_names[hw_info[0]], DATA);
		}
	}

	if (hw_info[1] == 2) {   // WRITE operation.

		if (*first_operation == 1) {

			fprintf_s(fptr, "%d WRITE %s %s", clock, register_names[hw_info[0]], DATA);
			*first_operation = 0;
			
		}
		else {
			fprintf_s(fptr, "\n%d WRITE %s %s", clock, register_names[hw_info[0]], DATA);
		}
	}

	fclose(fptr);
	memset(hw_info, 0, 3 * sizeof(int));
	
	return;
}

/*
  description:

	writes both leds and display7seg as their format is the same. 

*/
void write_output(char* fname, int clock, int led_data, int* first_operation) {

	int digit_num = num_of_digits(clock);

	FILE* fptr;

	if (*first_operation == 1) {
		fopen_s(&fptr, fname, "w");
	}
	else {
		fopen_s(&fptr, fname, "a");
	}

	if (fptr == NULL) {
		printf("IO error encountered\nTerminating program...");
		exit(-1);
	}

	char DATA[9];
	DATA[8] = '\0';
	dec_to_hex(DATA, led_data, 8);

	if (*first_operation == 1) {

		fprintf_s(fptr, "%d %s", clock, DATA);
		*first_operation = 0;

	}
	else {
		fprintf_s(fptr, "\n%d %s", clock, DATA);
	}

	#define write_leds write_output
	#define write_display7seg write_output

	fclose(fptr);
	return;
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

	int opcode = hex_to_dec(hex_data + hex_index, 2, &hex_index, 0);
	int rd = hex_to_dec(hex_data + hex_index, REG_INSTRUCTION_SIZE, &hex_index, 0);
	int rs = hex_to_dec(hex_data + hex_index, REG_INSTRUCTION_SIZE, &hex_index, 0);  // hex_index incremented inside func
	int rt = hex_to_dec(hex_data + hex_index, REG_INSTRUCTION_SIZE, &hex_index, 0);
	int rm = hex_to_dec(hex_data + hex_index, REG_INSTRUCTION_SIZE, &hex_index, 0);
	int imm1 = hex_to_dec(hex_data + hex_index, IMM_INSTRUCTION_SIZE, &hex_index, 1);
	int imm2 = hex_to_dec(hex_data + hex_index, IMM_INSTRUCTION_SIZE, &hex_index, 1);

	*field_array++ = opcode; *field_array++ = rd; *field_array++ = rs; *field_array++ = rt; *field_array++ = rm; *field_array++ = imm1; *field_array++ = imm2;

	//printf(" %d | %d | %d | %d | %d | %d | %d |\n", opcode, rd, rs, rt, rm, imm1, imm2);
}

/*
  description:

	executes a single assembly instruction.

*/
void execute_instruction(int *instruction_fields_array, char R[NUM_OF_REGISTERS][WORD + 1], char IO_R[NUM_OF_IO_REGISTERS][WORD + 1], char *dmem, char *PC,
	                     int *hw_info, int *PC_set_flag, int *halt_flag, int* irq_subroutine_flag) {
						 

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

		printf("opcode %d, instruction: add\n", opcode);

		result = signed_binary_strtol(R[rs], WORD) + signed_binary_strtol(R[rt], WORD) + signed_binary_strtol(R[rm], WORD);
		set_register(R[rd], result, WORD);

		//R[rd][WORD] = '\0';
		//printf("result %s\n", R[rd]);

		break;

	case 1: //sub


		printf("opcode %d, instruction: sub\n", opcode);

		result = signed_binary_strtol(R[rs], WORD) - signed_binary_strtol(R[rt], WORD) - signed_binary_strtol(R[rm], WORD);
		set_register(R[rd], result, WORD);

		//R[rd][WORD] = '\0';
		//printf("result %s\n", R[rd]);
		break;

	case 2: //mac

		printf("opcode %d, instruction: mac\n", opcode);

		result = signed_binary_strtol(R[rs], WORD) * signed_binary_strtol(R[rt], WORD) - signed_binary_strtol(R[rm], WORD);
		set_register(R[rd], result, WORD);

		//R[rd][WORD] = '\0';
		//printf("result %s\n", R[rd]);
		break;

	case 3: //and

		printf("opcode %d, instruction: and\n", opcode);
		
		result = signed_binary_strtol(R[rs], WORD) & signed_binary_strtol(R[rt], WORD) & signed_binary_strtol(R[rm], WORD);
		set_register(R[rd], result, WORD);

		//R[rd][WORD] = '\0';
		//printf("result %s\n", R[rd]);
		break;

	case 4: //or

		printf("opcode %d, instruction: or\n", opcode);
		
		result = signed_binary_strtol(R[rs], WORD) | signed_binary_strtol(R[rt], WORD) | signed_binary_strtol(R[rm], WORD);
		set_register(R[rd], result, WORD);

		//R[rd][WORD] = '\0';
		//printf("result %s\n", R[rd]);
		break;

	case 5: //xor

		printf("opcode %d, instruction: xor\n", opcode);

		result = signed_binary_strtol(R[rs], WORD) ^ signed_binary_strtol(R[rt], WORD) ^ signed_binary_strtol(R[rm], WORD);
		set_register(R[rd], result, WORD);

		//R[rd][WORD] = '\0';
		//printf("result %s\n", R[rd]);
		break;

	case 6: //sll

		printf("opcode %d, instruction: sll\n", opcode);
		
		result = signed_binary_strtol(R[rs], WORD) << signed_binary_strtol(R[rt], WORD);                                    // sll - shifting is cyclical, 32 is a zero shift.
		set_register(R[rd], result, WORD);

		//R[rd][WORD] = '\0';
		//printf("result %s\n", R[rd]);

		break;

	case 7: //sra

		printf("opcode %d, instruction: sra\n", opcode);

		result = signed_binary_strtol(R[rs], WORD) >> signed_binary_strtol(R[rt], WORD);                       // sra - for positive nums, reaches 0 (000...) and stays there for increased shift amounts.
		set_register(R[rd], result, WORD);																			   //       for negative nums, reaches -1 (111...) and stays there,
		break;

	case 8: //srl

		printf("opcode %d, instruction: srl\n", opcode); 

		result = right_logical_shift(R[rs], signed_binary_strtol(R[rt], WORD));
		set_register(R[rd], result, WORD);

		break; 
		
		

	case 9: //beq

		printf("opcode %d, instruction: beq\n", opcode);

		if (signed_binary_strtol(R[rs], WORD) == signed_binary_strtol(R[rt], WORD)) {
			
			copy_register(PC, R[rm], PC_SIZE);
			*PC_set_flag = 1; 

		}

		break;

	case 10: //bne

		printf("opcode %d, instruction: bne\n", opcode);

		if (signed_binary_strtol(R[rs], WORD) != signed_binary_strtol(R[rt], WORD)) {

			copy_register(PC, R[rm], PC_SIZE);
			*PC_set_flag = 1; // 
		}

		break;

	case 11: //blt

		printf("opcode %d, instruction: blt\n", opcode);

		if (signed_binary_strtol(R[rs], WORD) < signed_binary_strtol(R[rt], WORD)) {

			copy_register(PC, R[rm], PC_SIZE);
			*PC_set_flag = 1;
		}

		break;

	case 12: //bgt

		printf("opcode %d, instruction: bgt\n", opcode);

		if (signed_binary_strtol(R[rs], WORD) > signed_binary_strtol(R[rt], WORD)) {

			copy_register(PC, R[rm], PC_SIZE);
			*PC_set_flag = 1;
		}

		break;

	case 13: //ble

		printf("opcode %d, instruction: ble\n", opcode);

		if (signed_binary_strtol(R[rs], WORD) <= signed_binary_strtol(R[rt], WORD)) {

			copy_register(PC, R[rm], PC_SIZE);
			*PC_set_flag = 1;
		}

		break;

	case 14: //bge

		printf("opcode %d, instruction: bge\n", opcode);

		if (signed_binary_strtol(R[rs], WORD) >= signed_binary_strtol(R[rt], WORD)) {

			copy_register(PC, R[rm], PC_SIZE);
			*PC_set_flag = 1;
		}

		break;

	case 15: //jal

		printf("opcode %d, instruction: jal\n", opcode);

		result = strtol(PC, NULL, 2) + 1;
		set_register(R[rd], result, WORD);
		
		copy_register(PC, R[rm], PC_SIZE);
		*PC_set_flag = 1;

		break;
		

	case 16: //lw

		printf("opcode %d, instruction: lw\n", opcode);
	
		index = signed_binary_strtol(R[rs], WORD) + signed_binary_strtol(R[rt], WORD);
		if (index < 0) {
			printf("\n *Assembly instructions bug: attempting write to negative index; skipping instruction!*\n\n");
			break;
		}
		result = hex_to_dec(dmem + index * DMEM_LINE_SIZE, DMEM_LINE_SIZE, NULL, 1) + signed_binary_strtol(R[rm], WORD) ;
		set_register(R[rd], result, WORD);

		break;

	case 17: //sw

		printf("opcode %d, instruction: sw\n", opcode);



		index = signed_binary_strtol(R[rs], WORD) + signed_binary_strtol(R[rt], WORD);

		if (index < 0) {
			printf("\n *Assembly instructions bug: attempting write to negative index; skipping instruction!*\n\n");
			break;
		}
		result = signed_binary_strtol(R[rm], WORD) + signed_binary_strtol(R[rd], WORD);
		
		char hex[DMEM_LINE_SIZE];
		dec_to_hex(hex, result, DMEM_LINE_SIZE);
		memcpy(dmem + DMEM_LINE_SIZE * index, hex, DMEM_LINE_SIZE);


		break;
		

	case 18: //reti

		printf("opcode %d, instruction: reti\n", opcode);

		copy_register(PC, IO_R[7], PC_SIZE);
		*irq_subroutine_flag = 0;
		*PC_set_flag = 1;

		break;

	case 19: //in

		printf("opcode %d, instruction: in\n", opcode);

		index = signed_binary_strtol(R[rs], WORD) + signed_binary_strtol(R[rt], WORD);
		if (index < 0) {
			printf("\n *Assembly instructions bug: attempting write to negative index; skipping instruction!*\n\n");
			break;
		}

		result = IO_R[index];
		set_register(R[rd], result, WORD);

		hw_info[0] = index;  // which register was changed.
		hw_info[1] = 1;      // 1 indicating hw READ operation.
		hw_info[2] = result;
			
		break;

	case 20: //out

		printf("opcode %d, instruction: out\n", opcode);

		index = signed_binary_strtol(R[rs], WORD) + signed_binary_strtol(R[rt], WORD);
		if (index < 0) {
			printf("\n *Assembly instructions bug: attempting write to negative index; skipping instruction!*\n\n");
			break;
		}

		result = signed_binary_strtol(R[rm], WORD);
		set_register(IO_R[index], result, WORD);

		hw_info[0] = index;  // which register was changed.
		hw_info[1] = 2;      // 2 indicating hw WRITE operation.
		hw_info[2] = result;

		break;
	
	case 21:
		printf("HALT;\n");
		*halt_flag = 1;

		break;

	default:  
		printf("OPCODE out of range;");
		//exit(1);
	}




}


void main(int argc, char* argv[]) {

	char *imem, *dmem, *disk, *trace, *monitor;
	

	imem = readfile(argv[1], IMEM_LINE_SIZE, MEM_SIZE);  // contains data in sequence, imemin[ i * data_size] for the i+1 line start address.
	dmem = readfile(argv[2], DMEM_LINE_SIZE, MEM_SIZE);  
	disk = readfile(argv[3], DISK_LINE_SIZE, DISK_SIZE);

	trace = calloc_and_check(TRACE_LINE_SIZE * 1024, sizeof(char));
	monitor = calloc_and_check(MONITOR_BUFF_SIZE * MONITOR_BUFF_SIZE * MONITOR_LINE_SIZE, sizeof(char));
	memset(monitor, '0', MONITOR_BUFF_SIZE * MONITOR_BUFF_SIZE * MONITOR_LINE_SIZE);

	int irq2_up_clocks_buffer_size = 4;
	int* irq2_up_clocks = calloc_and_check(irq2_up_clocks_buffer_size, sizeof(int));

	int num_irq2_up_clocks = readirq2(argv[4], irq2_up_clocks, irq2_up_clocks_buffer_size);

	char PC[PC_SIZE + 1];
	set_register(PC, 0, PC_SIZE);

	char registers[NUM_OF_REGISTERS][WORD+1];           // +1 for null character used in debugging prints. TODO
	char IO_registers[NUM_OF_IO_REGISTERS][WORD + 1];
	int instruction_fields_array[NUM_OF_INST_FIELDS];

	//-------------  initialize decimal value IO registers by name for readability 

	int irq0enable = 0, irq1enable = 0, irq2enable = 0, irq0status = 0, irq1status = 0, irq2status = 0;	 
	int irq2statusirqhandler = 0, irqreturn = 0, clks = 0, leds = 0, display7seg = 0;
	int res1 = 0, res2 = 0, monitoraddr = 0, monitordata = 0, monitorcmd = 0;                                 
	int diskcmd = 0, disksector = 0, diskbuffer = 0, diskstatus = 0;
	int timerenable = 0, timercurrent = 0, timermax = 0;
	
	int *IO[] = { &irq0enable, &irq1enable, &irq2enable, &irq0status, &irq1status, &irq2status, &irq2statusirqhandler, &irqreturn,            // array holding pointers to IO decimal value variables,
				  &clks, &leds, &display7seg, &timerenable, &timercurrent, &timermax, &diskcmd, &disksector, &diskbuffer, &diskstatus,        // passed to a function refreshing them from IO_registers
				  &res1, &res2, &monitoraddr, &monitordata, &monitorcmd };																	

	//-------------

	int irq;
	int irq_subroutine_flag = 0;
	int halt_flag = 0;
	int PC_set_flag = 0;
	int clock = 1;
	int disk_read_end = 0;
	int irq2_up_clocks_index = 0;
	int hw_info[3] = {0};
	int hw_firstwrite = 1;
	int leds_firstwrite = 1;
	int dis7seg_firstwrite = 1;
	int trace_size = 1024 * TRACE_LINE_SIZE;     


	//-------------------------------------------------------------------------------------       MANUAL ASSIGNMENT 

	//int dec = 1;
	//set_register(registers[7], dec, WORD);    // rs
	//dec = 0;
	//set_register(registers[8], dec, WORD);    // rt 
	//dec = 0;
	//set_register(registers[9], dec, WORD);    // rm

	//set_register(IO_registers[11], 1, WORD);   // timerenable = 1 

	//set_register(IO_registers[14], 1, WORD);   // diskcmd = 0 

	//set_register(IO_registers[16], 128, WORD); // diskbuffer = 0

	//set_register(IO_registers[15], 0, WORD);   // disksector = 0;
	
	
	//--------------------------------------------------------------------------------------


	
	//-----------------------------------------------------------------------------------------------------------------\\
	//-----------------------------------------------------------------------------------------------------------------\\	            MAIN LOOP

	int tmp_cond = 0; // while loop exit condition, temporary for controlling how many instructions are ran.
	
	while (tmp_cond != 140) {

		fetch_IO(IO, IO_registers);

		if (timercurrent == timermax) {

			irq0status = 1;
			set_register(IO_registers[3], 1, WORD);

			timercurrent = 0;
			set_register(IO_registers[12], 0, WORD);  
		}
		

		if (irq2_up_clocks[irq2_up_clocks_index] == clock) {

			set_register(IO_registers[5], 1, WORD);
			irq2status = 1;

			if (irq2_up_clocks_index < num_irq2_up_clocks) {
				irq2_up_clocks_index++;
			}
		}

		irq = (irq0enable & irq0status) | (irq1enable & irq1status) | (irq2enable & irq2status);        
		
		if (irq ) {

			if (irq_subroutine_flag == 0) {

				copy_register(IO_registers[7], PC, WORD);	  // save current PC in irqreturn
				copy_register(PC, IO_registers[6], PC_SIZE); // set PC to irqhandler address
				irq_subroutine_flag = 1;

			}

		}

		build_trace(trace, &PC, imem + strtol(PC, NULL, 2) * IMEM_LINE_SIZE, &registers, &trace_size, clock);

		decode_instruction(imem + strtol(PC, NULL, 2) * IMEM_LINE_SIZE, instruction_fields_array);

		printf(" \nPC : %d ||\n", strtol(PC, NULL, 2));
		registers[7][WORD] = '\0';
		printf("reg 7 = %d\n", signed_binary_strtol(registers[7], WORD));
		execute_instruction(&instruction_fields_array, &registers, &IO_registers, dmem, &PC, hw_info, &PC_set_flag, &halt_flag, &irq_subroutine_flag);
		
		if (leds != signed_binary_strtol(IO_registers[9], WORD)) {      // if leds register has been changed, write it to leds.txt

			write_leds(argv[10], clock, signed_binary_strtol(IO_registers[9],WORD), &leds_firstwrite);

		}

		if (display7seg != signed_binary_strtol(IO_registers[10], WORD)) {      // if display7seg register has been changed, write it to display7seg.txt

			write_leds(argv[11], clock, signed_binary_strtol(IO_registers[10], WORD), &dis7seg_firstwrite);

		}

		fetch_IO(IO, IO_registers);

		write_hwregtrace(argv[8], hw_info, clock, &hw_firstwrite);

		//PC[PC_SIZE] = '\0';
		//printf("PC = %s | current time = %d\n", PC, timercurrent);

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

			dec_to_hex(monitor + (monitoraddr * 2), monitordata, 2);

		}

		if (timerenable) {   

			increment_binary(IO_registers[12], WORD);  

		}

		if (PC_set_flag == 0) {

			increment_binary(PC, PC_SIZE);  // increment if PC wasnt already set, using reserve IO to avoid passing another variable to execute_instruction.

		}

		if (halt_flag) {

			break;

		}
		
		
		increment_binary(IO_registers[8], WORD); // clks++
		set_register(IO_registers[5], 0, WORD);  // irq2status = 0
		PC_set_flag = 0;
		clock++;   // software clock
		tmp_cond++;
	}

	//-----------------------------------------------------------------------------------------------------------------\\
	//-----------------------------------------------------------------------------------------------------------------\\



	//registers[3][WORD] = '\0';              // print result in v0, for debugging.
	//printf("\nresult\nsigned binary: %s  |  dec: %d\n", registers[3], signed_binary_strtol(registers[3], WORD));

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

	char* regout = calloc_and_check(DMEM_LINE_SIZE * (NUM_OF_REGISTERS - 3) + 1, sizeof(char));
	memset(regout + DMEM_LINE_SIZE * (NUM_OF_REGISTERS - 3), '\0', 1);

	for (k = 3; k < NUM_OF_REGISTERS; k++) {

		val = signed_binary_strtol(registers[k], WORD);
		dec_to_hex(regout + index, val, DMEM_LINE_SIZE);
		index += DMEM_LINE_SIZE;

	}

	trace[(clock - 1) * TRACE_LINE_SIZE] = '\0';  


	int digit_num = num_of_digits(clock);
	char* clock_output = calloc_and_check(digit_num + 1, sizeof(char));
	sprintf_s(clock_output, digit_num + 1, "%d", clock);
	

	writefile(argv[5], DMEM_LINE_SIZE, dmem);
	writefile(argv[6], DMEM_LINE_SIZE, regout);
	writefile(argv[7], TRACE_LINE_SIZE, trace);
	writefile(argv[9], digit_num, clock_output);
	writefile(argv[12], DISK_LINE_SIZE, disk);
	writefile(argv[13], MONITOR_LINE_SIZE, monitor);

	free(imem);
	free(dmem);
	free(regout);
	free(trace);
	free(clock_output);
	free(disk);
	free(monitor);
}