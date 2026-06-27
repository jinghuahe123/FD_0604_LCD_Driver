#include "char_helper.h"
#include <string.h>
#include <ctype.h> // for isspace()

void trim(char* str) {
    if (!str || !*str) return;  // empty string check
    
    // trim leading spaces
    char* start = str;
    while (isspace((unsigned char)*start)) start++;
    
    // trim trailing spaces
    char* end = str + strlen(str) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    
    // shift the trimmed string to the beginning
    if (start != str) {
        memmove(str, start, end - start + 1);
    }
    str[end - start + 1] = '\0';  // null terminate
}
