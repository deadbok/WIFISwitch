/*
 * itoa.c
 *
 * Unsigned integer to string conversion.
 * Nicked from http://www.jb.man.ac.uk/~slowe/cpp/itoa.html.
 * Simplified for my use case, and did some cosmetic changes to make it easier to understand.
 */
 #include "os_type.h"
 
char ICACHE_FLASH_ATTR *itoa(unsigned int value, char* result, char base)
{
	char			*chr_map = "0123456789abcdef";
	char			*ptr = result;
	char			*ptr1 = result;
	char			tmp_char;
	unsigned int 	tmp_value;

	//Run until conversion is done
	do {
		//Save old value
		tmp_value = value;
		//Eat the least significant digit
		value /= base;
		//Remainder used to look up the character in the map
		//Blame HP for the way I explicitly tell it to multiply first
        //using parentheses.
        *ptr++ = chr_map[tmp_value - (value * base)];

	} while (value);

	*ptr-- = '\0';
	//Reverse the string
	while (ptr1 < ptr) {
		//Save character the right
		tmp_char = *ptr;
		//Replace the right character with the left
		*ptr-- = *ptr1;
		//Replace left character with the right
		*ptr1++ = tmp_char;
	}

	return result;
}
