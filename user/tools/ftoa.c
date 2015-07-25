/**
 * @file ftoa.c
 *
 * @brief Float to string conversion.
 * 
 * Nicked from http://www.jb.man.ac.uk/~slowe/cpp/itoa.html.
 * Simplified for my use case, and did some cosmetic changes to make it easier to understand.
 */
#include "user_config.h" 
#include "c_types.h"

/**
 * @brief Convert a base 10 float to a string.
 * 
 * @param value The value to be converted.
 * @param result A pointer to where the string is written.
 * @param fractional_digits Digits after the point.
 * @return A pointer to #result, or NULL on error.
 */ 
char ICACHE_FLASH_ATTR *ftoa(float value, char *result, unsigned char fractional_digits)
{
	const char *chr_map = "0123456789";
	unsigned long int_part;
	float frac_part;
	char *ptr = result;
	char *ptr1 = result;
	char tmp_char;
	long tmp_value;
	bool sign = false;
	
	//Negative number handling.
	if ((value < 0))
	{
		value = -value;
		sign = true;
	}

	int_part = value;
	frac_part = value - int_part;
	
	//Convert float part digits
	while (fractional_digits > 0)
	{
		//Get the digit.
		frac_part = frac_part * 10;
		tmp_value = frac_part;
		//Look up the number.
        *ptr++ = chr_map[tmp_value];
        //Remove integer.
        frac_part = frac_part - tmp_value;
        //We're done
        fractional_digits--;	
	}
	
	//Convert integer part.
	//Run until conversion is done
	do {
		//Save old value
		tmp_value = int_part;
		//Eat the least significant digit
		int_part /= 10;
		//Remainder used to look up the character in the map
		//Blame HP for the way I explicitly tell it to multiply first
        //using parentheses.
        *ptr++ = chr_map[tmp_value - (int_part * 10)];
	} while (int_part);
	
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

	return result;
}
