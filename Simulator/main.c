#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define REG_INSTRUCTION_SIZE 1
#define IMM_INSTRUCTION_SIZE 3

#define IMEM_LINE_SIZE 12
#define DMEM_LINE_SIZE 8

#define NUM_OF_INST_FIELDS 7
#define NUM_OF_REGISTERS 16

#define DATA_CHUNK 64
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
void* readfile(char* p_fname, int line_size) {

	FILE* fptr;
	int fdata_sz = DATA_CHUNK;
	int i = 0;

	char* p_line_buffer = malloc_and_check(line_size);
	char* data = malloc_and_check(fdata_sz * (line_size + 1));

	fopen_s(&fptr, p_fname, "r");

	if (fptr == NULL) {
		printf("IO error encountered\nTerminating program...");
		exit(-1);
	}

	while (fgets(p_line_buffer, line_size + 2, fptr) != NULL) {

		if (i % DATA_CHUNK == 0 && i != 0) {

			fdata_sz += DATA_CHUNK;
			char* temp_buffer = realloc(data, fdata_sz * (line_size + 1));					// Increment instructions buffer 

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
int hex_to_dec(char* hex_data, int hex_size, int* hex_index) {

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

// converts dec to binary, storing the resultant binary string in indicated array.
void dec_to_bin(int decimal, char* binary) {

	int i, j;

	for (i = WORD - 1; i >= 0; i--) {

		j = decimal >> i;

		if (j & 1) {
			binary[WORD - 1 - i] = '1';
		}
		else
			binary[WORD - 1 - i] = '0';
	}

	//binary[WORD] = '\0';   // for debug prints

	return;
}

//implementation of strtol for signed binary
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

		int num = - strtol(flip_str, NULL, 2) - 1 ;   // strtol of flipped bits, minus one, gives us the correct conversion for signed negative numbers.
		return num;
			
	}


	int num = strtol(str, NULL, 2);
	return num;

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
void execute_instruction(int *instruction_fields_array[NUM_OF_INST_FIELDS], char R[NUM_OF_REGISTERS][WORD + 1], int PC) {

	int opcode = instruction_fields_array[0];
	int rd = instruction_fields_array[1];
	int rs = instruction_fields_array[2];		//
	int rt = instruction_fields_array[3];		// extract the instruction fields from array to variables, for readability.
	int rm = instruction_fields_array[4];		//
	int imm1 = instruction_fields_array[5];
	int imm2 = instruction_fields_array[6];

	dec_to_bin(imm1, R[1]);		//  store imm1 and imm2 in their respective registers.
	dec_to_bin(imm2, R[2]);    //

	int result;

	// TODO : writing to zero/imm1/imm2 doesnt change them

	switch (opcode) {

	case 0:  //add

		printf("opcode %d, instruction: add\n", opcode);

		result = signed_binary_strtol(R[rs]) + signed_binary_strtol(R[rt]) + signed_binary_strtol(R[rm]);
		dec_to_bin(result, R[rd]);

		//R[rd][WORD] = '\0';
		//printf("result %s\n", R[rd]);

		break;

	case 1: //sub


		printf("opcode %d, instruction: sub\n", opcode);

		result = signed_binary_strtol(R[rs]) - signed_binary_strtol(R[rt]) - signed_binary_strtol(R[rm]);
		dec_to_bin(result, R[rd]);

		//R[rd][WORD] = '\0';
		//printf("result %s\n", R[rd]);
		break;

	case 2: //mac

		printf("opcode %d, instruction: mac\n", opcode);

		result = signed_binary_strtol(R[rs]) * signed_binary_strtol(R[rt]) - signed_binary_strtol(R[rm]);
		dec_to_bin(result, R[rd]);

		//R[rd][WORD] = '\0';
		//printf("result %s\n", R[rd]);
		break;

	case 3: //and

		printf("opcode %d, instruction: and\n", opcode);
		
		result = signed_binary_strtol(R[rs]) & signed_binary_strtol(R[rt]) & signed_binary_strtol(R[rm]);
		dec_to_bin(result, R[rd]);

		//R[rd][WORD] = '\0';
		//printf("result %s\n", R[rd]);
		break;

	case 4: //or

		printf("opcode %d, instruction: or\n", opcode);
		
		result = signed_binary_strtol(R[rs]) | signed_binary_strtol(R[rt]) | signed_binary_strtol(R[rm]);
		dec_to_bin(result, R[rd]);

		//R[rd][WORD] = '\0';
		//printf("result %s\n", R[rd]);
		break;

	case 5: //xor

		printf("opcode %d, instruction: xor\n", opcode);

		result = signed_binary_strtol(R[rs]) ^ signed_binary_strtol(R[rt]) ^ signed_binary_strtol(R[rm]);
		dec_to_bin(result, R[rd]);

		//R[rd][WORD] = '\0';
		//printf("result %s\n", R[rd]);
		break;

	case 6: //sll

		printf("opcode %d, instruction: sll\n", opcode);
		
		result = signed_binary_strtol(R[rs]) << signed_binary_strtol(R[rt]);
		dec_to_bin(result, R[rd]);

		//R[rd][WORD] = '\0';
		//printf("result %s\n", R[rd]);

		break;

	case 7: //sra

		printf("opcode %d, instruction: sra\n", opcode);

		result = strtol(R[rs], NULL, 2) << strtol(R[rt], NULL, 2);
		dec_to_bin(result, R[rd]);

		break;

	case 8: //srl

		printf("opcode %d, instruction: srl\n", opcode); 
		break;

	case 9: //beq

		printf("opcode %d, instruction: beq\n", opcode);
		break;

	case 10: //bne

		printf("opcode %d, instruction: bne\n", opcode);
		break;

	case 11: //blt

		printf("opcode %d, instruction: blt\n", opcode);
		break;

	case 12: //bgt

		printf("opcode %d, instruction: bgt\n", opcode);
		break;

	case 13: //ble

		printf("opcode %d, instruction: ble\n", opcode);
		break;

	case 14: //bge

		printf("opcode %d, instruction: bge\n", opcode);
		break;

	case 15: //jal

		printf("opcode %d, instruction: jal\n", opcode);
		break;

	case 16: //lw

		printf("opcode %d, instruction: lw\n", opcode);
		break;

	case 17: //sw

		printf("opcode %d, instruction: sw\n", opcode);
		break;

	case 18: //reti

		printf("opcode %d, instruction: reti\n", opcode);
		break;

	case 19: //in

		printf("opcode %d, instruction: in\n", opcode);
		break;

	case 20: //out

		printf("opcode %d, instruction: out\n", opcode);
		break;

	default:  //halt; change to case 21?
		printf("HALT;");
		//exit(1);
	}




}




void main(int argc, char* argv[]) {

	char* imemin, * dmemin;

	imemin = readfile(argv[1], IMEM_LINE_SIZE);  // contains data in sequence, imemin[ i * data_size] for the i+1 line start address.
	dmemin = readfile(argv[2], DMEM_LINE_SIZE);  

	int PC = 0; // change to 12 bit binary register?
	char registers[NUM_OF_REGISTERS][WORD+1]; // +1 for null character used in debugging prints. TODO
	int inst_field_array[NUM_OF_INST_FIELDS];

	//

	int dec = 127;
	dec_to_bin(dec, registers[7]);   // manual registry assignments for testing. 

	dec = 32;
	dec_to_bin(dec, registers[8]);

	
    
	
	//

	
	int tmp_cond = 1; // while loop exit condition, temporary for controlling how many instructions are ran.

	while (tmp_cond != 2) {


		decode_instruction(imemin + PC * IMEM_LINE_SIZE, inst_field_array);

	/*	int i;
		for (i = 0; i < NUM_OF_INST_FIELDS; i++) {
			printf(" %d |", inst_field_array[i]);
		}
		printf("\n");*/

		execute_instruction(&inst_field_array, &registers, PC);

		PC++;  //  TODO add logic to increment PC here OR from the instruction.
		tmp_cond++;
	}

	registers[3][WORD] = '\0';              // print result in v0, for debugging.
	printf("result %s\n", registers[3]);
}