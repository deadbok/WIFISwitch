/**
 * @file itoa.c
 *
 * @brief Integer to string conversion.
 * 
 * Nicked from http://www.jb.man.ac.uk/~slowe/cpp/itoa.html.
 * Simplified for my use case, and did some cosmetic changes to make it
 * easier to understand.
 */
#include "user_config.h" 
#include "c_types.h"
#include "debug.h"

char *itoa(long value, char *result, const unsigned char base)
{
	const char *chr_map = "0123456789abcdef";
	char *ptr = result;
	char *ptr1 = result;
	char tmp_char;
	long tmp_value;
	bool sign = false;
	
	if ((base == 0) || (base > 16))
	{
		error("Wrong base %d, when converting integer to string.\n", base);
		return(NULL);
	}
	
	if (!result)
	{
		error("No where to put the result.\n");
		return(NULL);
	}
	
	//Negative number handling.
	if ((value < 0) && (base == 10))
	{
		value = -value;
		sign = true;
	}

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
	//Add sign.
	if (sign)
	{
		*ptr++ = '-';
	}
	//Add zero byte, and go back to the last real character.
	*ptr-- = '\0';
	
	//Reverse the string
	while (ptr1 < ptr) {
		//Save character to the right
		tmp_char = *ptr;
		//Replace the right character with the left
		*ptr-- = *ptr1;
		//Replace left character with the right
		*ptr1++ = tmp_char;
	}

	return(result);
}
