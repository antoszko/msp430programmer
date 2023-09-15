/*
 * jtag_programmer.h
 *
 *  Created on: Sep 10, 2023
 *      Author: stefan
 */

#ifndef INC_JTAG_PROGRAMMER_H_
#define INC_JTAG_PROGRAMMER_H_

#include "stdint.h" // for uint32_t
#include "stdio.h" // for printf

typedef struct node node_t;	// allows referencing self inside definition

typedef struct node
{
	uint16_t address;
	uint8_t length;
	uint16_t* data;
	node_t* next;
} node_t;

/**
 * @brief Returns 1 if the parameter c is between '0'-'9' or 'A'-'F' or 'a'-'f'.
 */
uint8_t is_valid_hex_char(uint8_t c);

void free_linkedlist(node_t* linkedlist);

/**
 * @brief reads an ascii string and returns the byte represented there in ASCII
 * @param string is the ascii string which is read. Only 2 characters are read. The string must be at least 2 characters long.
 * Assumes no error can occur. String must contain ascii 'a'-'f', 'A'-'F', or '0'-'9'.
 *
 * Eg. string '4e11' -> returns 0x4e = 78. ignores other characters.
 */
uint8_t ascii_string_to_byte(const uint8_t* const string);

/**
 * @brief reads an ascii string and returns the address represented there in ASCII
 * @param string is the ascii string. Only 4 bytes are read. The string must be at least 4 bytes long.
 * String is in big endian. First byte is most significant then second byte is least significant.
 * Assumes no error can occur. String must contain ascii 'a'-'f', 'A'-'F', or '0'-'9'.
 */
uint16_t ascii_string_to_address(uint8_t* const string);

/**
 * @brief same as ascii_string_to_address but its little endian
 */
uint16_t ascii_string_to_word(uint8_t* const string);

/**
 * @brief Verifies the hexfile. Outputs a program as a linked list of node_t into output parameter program
 */
uint8_t verify_hexfile_and_return_program_linklist(const uint8_t *const file, uint16_t file_size, node_t** program);

#endif /* INC_JTAG_PROGRAMMER_H_ */
