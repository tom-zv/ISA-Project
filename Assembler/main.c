#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "tokenizer.h"
#include "hash_table.h"

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#define MAX_LINE_LENGTH 500

int isDecimal(char *label) {
    char *pos;
    pos = label;
    while (*pos != '\0') {
        if ((isdigit(*pos))||(*pos =='-')) {
            pos++;
        }else {
            return 0;
        }
    }
    return 1;
}


int isHexdecimal(const char *label) {
    if (label[0] == '0' && label[1] == 'x') {
        return 1;
    }
    return 0;
}

/*
* The structs below map a character to an integer.
* They are used in order to map a specific instruciton/register to its binary format in ASCII
*/

// Struct that stores registers and their respective binary reference
struct {
    const char *name;
    char *address;
} registerMap[] = {
        {"zero", "0"},
        {"imm1", "1"},
        {"imm2", "2"},
        {"v0",   "3"},
        {"a0",   "4"},
        {"a1",   "5"},
        {"a2",   "6"},
        {"t0",   "7"},
        {"t1",   "8"},
        {"t2",   "9"},
        {"s0",   "a"},
        {"s1",   "b"},
        {"s2",   "c"},
        {"gp",   "d"},
        {"sp",   "e"},
        {"ra",   "f"},
        {NULL,   0}};

// Struct for R-Type instructions mapping for the 'function' field in the instruction
struct {
    const char *name;
    char *address;
} rMap[] = {
        {"add",  "00"},
        {"sub",  "01"},
        {"mac",  "02"},
        {"and",  "03"},
        {"or",   "04"},
        {"xor",  "05"},
        {"sll",  "06"},
        {"sra",  "07"},
        {"srl",  "08"},
        {"beq",  "09"},
        {"bne",  "0a"},
        {"blt",  "0b"},
        {"bgt",  "0c"},
        {"ble",  "0d"},
        {"bge",  "0e"},
        {"jal",  "0f"},
        {"lw",   "10"},
        {"sw",   "11"},
        {"reti", "12"},
        {"in",   "13"},
        {"out",  "14"},
        {"halt", "15"},
        {NULL,   0}};

char *return_label(char *imm1, hash_table_t *table);


// Array that holds the supported instructions
char *instructions[] = {
        "add",    // 0
        "sub",    // 1
        "mac",    // 2
        "and",    // 3
        "or",    // 4
        "xor",    // 5
        "sll",    // 6
        "sra",    // 7
        "srl",    // 8
        "beq",    // 9
        "bne",    // 10
        "blt",    // 11
        "bgt",    // 12
        "ble",    // 13
        "bge",    // 14
        "jal",    // 15
        "lw",    // 16
        "sw",    // 17
        "reti",    // 18
        "in",   //19
        "out",  //20
        "halt"  //21
};

// Size of array
int inst_len = sizeof(instructions) / sizeof(char *);

int search(char *instruction) {

    for (int i = 0; i < inst_len; i++) {

        if (strcmp(instruction, instructions[i]) == 0) {
            return i;
        }
    }
    return -1;
}

// Quick Sort String Comparison Function
int string_comp(const void *a, const void *b) {
    return strcmp(*(char **) a, *(char **) b);
}

// Return the  hex representation of registers
char *register_address(char *registerName) {

    size_t i;
    for (i = 0; registerMap[i].name != NULL; i++) {
        if (strcmp(registerName, registerMap[i].name) == 0) {
            return registerMap[i].address;
        }
    }

    return NULL;
}


int convert_to_decimal(char *hex_value) {
    char hex[10];
    long long decimal = 0, base = 1;
    int i, length = 10;
    /* Get hexadecimal value from user */
    memcpy(hex, hex_value, 10 * sizeof(char));
    for (i = length--; i >= 0; i--) {
        if (hex[i] >= '0' && hex[i] <= '9') {
            decimal += (hex[i] - 48) * base;
            base *= 16;
        } else if (hex[i] >= 'A' && hex[i] <= 'F') {
            decimal += (hex[i] - 55) * base;
            base *= 16;
        } else if (hex[i] >= 'a' && hex[i] <= 'f') {
            decimal += (hex[i] - 87) * base;
            base *= 16;
        }
    }
    return (int)(decimal);
}


/*
  description:
	malloc wrapper with added check

*/
void *calloc_and_check(size_t count, size_t elem_size) {

    void *ptr = calloc(count, elem_size);

    if (ptr == NULL) {
        printf("Memory assignment error encountered\nTerminating program...");
        exit(-1);
    }

    return (ptr);
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

void write_mem_file(FILE *Mem, char *data, int line_num) {

    FILE *fptr = Mem;
    char *mem_line= calloc_and_check(9,sizeof(char));
    //memset(mem_line, 0, sizeof mem_line);
    int replaced = 0;
    int count = 0;
    while ((fgets(mem_line, 9, fptr)) != NULL) {
        count++;

        /* If current mem_line is mem_line to replace */
        if (count == line_num) {
            replaced = 1;
            fputs(data, fptr);
            fflush(Mem);
        } else {
            fputs(mem_line, fptr);
            fflush(Mem);
        }
    }
    if (!replaced) {
        mem_line="00000000";
        while (count < line_num) {
            fprintf(fptr, "%s\n", mem_line);
            fflush(Mem);
            count++;
        }
        fprintf(fptr, "%s", data);
        fflush(Mem);
    }
    //free(fptr);
}

int islabel(char *label) {
    return (!isHexdecimal(label)) && (!isDecimal(label));
}

/*
  description:
	decimal to digit hex, stored in indicated array.

	params:
		in hex - array that stores the hex representation.
		in dec - decimal number to convert.
		in size - number of digits in hex representation - Padded with zeros
*/
void dec_to_hex(char *hex, int dec, int size) {

    int temp;
    long long q = dec;
    int j;

    if (q < 0) {              // Signed representation for negative numbers.

        q = pow(2, ((double) size * 4)) +q;              // 2^bits + negative number ran through an unsigned conversion equals the signed conversion.

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
    for (int i = 0; hex[i]; i++) {
        hex[i] = tolower(hex[i]);
    }

}


char *check_imm_and_return(char *imm, hash_table_t *hash_table) {
    char *imm1hex;
    imm1hex = calloc_and_check(12, sizeof(char));
    if (islabel(imm)) {
        imm1hex = return_label(imm, hash_table);
    } else if (isHexdecimal(imm)) {
        imm1hex = imm + 2;
    } else {
        dec_to_hex(imm1hex, atoi(imm), 3);
    }
    return imm1hex;
}

void rtype_instruction(char *instruction, char *rd, char *rs, char *rt, char *rm, char *imm1, char *imm2, FILE *Out,
                       hash_table_t *hash_table) {
    int count = 0;
    char *opcode = NULL;
    // Set the instruction bits
    for (int i = 0; rMap[i].name != NULL; i++) {
        if (strcmp(instruction, rMap[i].name) == 0) {
            opcode = rMap[i].address;
            continue;
        }
    }

    char *rdhex = "0";
    if (strcmp(rd, "0") != 0)
        rdhex = register_address(rd);

    char *rshex = "0";
    if (strcmp(rs, "0") != 0)
        rshex = register_address(rs);

    char *rthex = "0";
    if (strcmp(rt, "0") != 0)
        rthex = register_address(rt);

    char *rmhex = "0";
    if (strcmp(rm, "0") != 0)
        rmhex = register_address(rm);

    char *imm1hex = check_imm_and_return(imm1, hash_table);

    char *imm2hex = check_imm_and_return(imm2, hash_table);


    // Print out the instruction to the file

    fprintf(Out, "%s%s%s%s%s%s%s\n", opcode, rdhex, rshex, rthex, rmhex, imm1hex, imm2hex);
    fflush(Out);
    //free(opcode);
    //free(rdhex);
    //free(rshex);
    //free(rthex);
    //free(rmhex);
    //free(imm1hex);
    //free(imm2hex);
}


char *return_label(char *imm1, hash_table_t *hash_table) {

    if (strcmp(imm1, "0") != 0) {
        if (isDecimal(imm1)) {
            int value = atoi(imm1);
            dec_to_hex(imm1,value,3);
            return imm1;
        }
        else if (islabel(imm1)) {
            // Find hash address for a label and put in an immediate

            char *address = hash_find(hash_table, imm1, strlen(imm1) + 1);
            int check = *address;

            char  *temp_address;        
            temp_address = calloc_and_check(12, sizeof(char));

            dec_to_hex(temp_address, check, 3);
            return temp_address;

        } else {
            imm1 += 2;
            return imm1;
        }
    }
    return "0";
}


void word_instruction(char *address, char *value, FILE *Mem) {
    char *value_hex;
    value_hex = calloc_and_check(9,sizeof(char));
    if (isDecimal(value)) {
        dec_to_hex(value_hex,atoi(value),8);
    } else {
        value_hex += 2;
    }
    int value_address;
    if (isHexdecimal(address)) {
        value_address = convert_to_decimal(address);
    } else {
        value_address = atoi(address);
    }
    write_mem_file(Mem, value_hex, value_address);
}

void parser(FILE *fptr, int pass, hash_table_t *hash_table, FILE *Out, FILE *Mem) {
    char *token_pointer, *token = NULL;
    int instruction_count = 1;
    char line[MAX_LINE_LENGTH + 1];
    int pass_checked = 0;
    while (1) {//line
        if (fgets(line, MAX_LINE_LENGTH, fptr) == NULL)
            break;
        //to lowercase any input
        for (int i = 0; line[i]; i++) {
            line[i] = tolower(line[i]);
        }
        // 0 in the end of the line
        line[MAX_LINE_LENGTH] = 0;
        token_pointer = line;

        /* parse the tokens within the line */
        while (1) {//words
            token = parse_token(token_pointer, " \n\t$,", &token_pointer, NULL);
            /* blank line or comment begins here. go to the next line */
            if (token == NULL || *token == '#') {
                instruction_count++;
                free(token);
                break;
            }
            //first pass
            if (pass == 1) {
                // if token has ':', then it is a label so add it to hash table
                if (strstr(token, ":")) {
                    instruction_count--;

                    printf("Label\n");

                    // Strip out ':'
                    //printf("Label: %s at %d with address %d: \n", token, line_num, instruction_count);
                    int token_len = strlen(token);
                    token[token_len - 1] = '\0';

                    // Insert variable to hash table
                    uint32_t *inst_count;
                    inst_count = (uint32_t *) malloc(sizeof(uint32_t));
                    *inst_count = instruction_count;
                    int32_t insert = hash_insert(hash_table, token, strlen(token) + 1, inst_count);

                    if (insert != 1) {
                        printf( "Error inserting into hash table\n");
                        exit(1);
                    }
                }
            } else if (pass == 2) {

                if (pass_checked == 0) {
                    printf("############    Pass 2   ##############\n");
                    pass_checked = 1;
                }

                // start interpreting here
                int instruction_supported = search(token);
                if (instruction_supported != -1) {
                    // Parse the instruction - get rd, rs, rt,rm,imm1,imm2 registers
                    char *inst_ptr = token_pointer;
                    char *reg = NULL;
                    // Create an array of char* that stores registers respectively
                    char **reg_store;
                    reg_store = malloc(6 * sizeof(char *));
                    if (reg_store == NULL) {
                        printf( "Out of memory\n");
                        exit(1);
                    }
                    for (int i = 0; i < 6; i++) {
                        if (i < 4) {
                            reg_store[i] = malloc(4 * sizeof(char));
                            if (reg_store[i] == NULL) {
                                printf( "Out of memory\n");
                                exit(1);
                            }
                        } else {
                            reg_store[i] = malloc(50 * sizeof(char));//imm can be 50 chr long
                            if (reg_store[i] == NULL) {
                                printf( "Out of memory\n");
                                exit(1);
                            }
                        }
                    }
                    // Keeps a reference to which register has been parsed for storage
                    int count = 0;
                    while (1) {

                        reg = parse_token(inst_ptr, " $,\n\t", &inst_ptr, NULL);

                        if (reg == NULL || *reg == '#') {
                            break;
                        }

                        strcpy(reg_store[count], reg);
                        count++;
                        free(reg);
                    }

                    // Send reg_store for output
                    // rd is in position 0, rs is in position 1 and rt is in position 2 and rm is in position 3 and imm1 in position 4, and imm2 in position 5
                    rtype_instruction(token, reg_store[0], reg_store[1], reg_store[2], reg_store[3], reg_store[4],
                                      reg_store[5], Out, hash_table);

                    // Dealloc reg_store
                    //for (int i = 0; i < 6; i++) {
                    //free(reg_store[i]);
                    //}
                    //free(reg_store);
                } else if (strstr(token, ".word")) {
                    // Parse the instruction - get address, data
                    char *inst_ptr = token_pointer;
                    char *reg = NULL;
                    // Create an array of char* that stores address, data respectively
                    char **reg_store;
                    reg_store = malloc(2 * sizeof(char *));
                    if (reg_store == NULL) {
                        printf("Out of memory\n");
                        exit(1);
                    }
                    reg_store[0] = malloc(5 * sizeof(char));
                    if (reg_store[0] == NULL) {
                        printf( "Out of memory\n");
                        exit(1);
                    }
                    reg_store[1] = malloc(10 * sizeof(char));
                    if (reg_store[1] == NULL) {
                        printf("Out of memory\n");
                        exit(1);
                    }
                    // Keeps a reference to which register has been parsed for storage
                    int count = 0;
                    while (1) {

                        reg = parse_token(inst_ptr, " $,\n\t", &inst_ptr, NULL);

                        if (reg == NULL || *reg == '#') {
                            break;
                        }

                        strcpy(reg_store[count], reg);
                        count++;
                        free(reg);
                    }

                    // Send reg_store for mem
                    // address is in position 0, value is in position 1
                    word_instruction(reg_store[0], reg_store[1], Mem);

                    // Dealloc reg_store
                    for (int i = 0; i < 2; i++) {
                        free(reg_store[i]);
                    }
                    free(reg_store);

                }
                else
                {
                    printf("%s", line);
                    continue;
                }

            }
            instruction_count++;
            printf("%s", line);
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    // Make sure correct number of arguments input
    if (argc != 4) {
        printf("Incorrect number of arguments");
    } else {
        // Open I/O files
        // Check that files opened properly
        FILE *In=NULL;
        In = fopen(argv[1], "r+");
        if (In == NULL) {
            printf("Input file could not be opened.");
            exit(1);
        }

        FILE *Out=NULL;
        Out = fopen(argv[2], "w");
        if (Out == NULL) {
            printf("Output file could not opened.");
            exit(1);
        }

        FILE *Mem=NULL;
        Mem = fopen(argv[3], "w");
        if (Mem == NULL) {
            printf("Memory file could not opened.");
            exit(1);
        }

        // Sort the array using qsort for faster search
        qsort(instructions, inst_len, sizeof(char *), string_comp);

        // Create a hash table of size 127
        hash_table_t *hash_table = create_hash_table(127);

        // Parse in passes
        int passNumber = 1;
        parser(In, passNumber, hash_table, Out, Mem);

        // Rewind input file & start pass 2
        rewind(In);
        passNumber = 2;
        parser(In, passNumber, hash_table, Out, Mem);

        // Close files
        fclose(In);
        fclose(Out);
        fclose(Mem);

        exit(0);

    }
}

