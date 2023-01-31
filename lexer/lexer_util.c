#include "./lexer_util.h"
#include "./token_codes.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

YYLVALTYPE convert_to_str(char *character, int len) {
  int has_prefix = character[0] != '"';
  int prefix_count = has_prefix + has_prefix * (character[1] == '8');
  int t_len = len - 2 - prefix_count + 1; // len - 2("") - prefix + 1(\0)
  char *str = (char *)malloc(sizeof(char) * t_len); // TODO free this after use
  sscanf(character + prefix_count, "\"%s\"",
         str); // NOTE this is techincaly grabbing the last " but i remove it
               // anyway
  str[t_len - 1] = 0;
  NVALTYPE num;
  num.str = str;
  YYLVALTYPE r_val;
  r_val.value = num;
  r_val.type = Tu8;
  if (prefix_count == 1) {
    switch (character[0]) {
    case 'u':
      r_val.type = Tu;
      break;
    case 'U':
      r_val.type = TU;
      break;
    case 'L':
      r_val.type = TL;
      break;
    default:
      exit(1);
      fprintf(stderr, "Lexing error this should never happen");
    }
  }
  return r_val;
}

YYLVALTYPE convert_to_char(char *character, int len) {
  char prefix = character[0];
  int has_prefix = prefix != '\'';
  char val;
  sscanf(character + has_prefix, "'%c'", &val);
  NVALTYPE num;
  num.chr = val;
  YYLVALTYPE r_val;
  r_val.value = num;

  switch (prefix) {
  case 'L':
    r_val.type = TWCHAR;
    break;
  case 'U':
    r_val.type = TCHAR32;
    break;
  case 'u':
    r_val.type = TCHAR16;
    break;
  default:
    r_val.type = TUCHAR;
    break;
  }
  return r_val;
}

YYLVALTYPE convert_to_float(char *number, int len, int base) {
  char last = number[len - 1];
  char *format_string = "%LF%*s";
  if (base == 16) {
    format_string = "0x%La%*s";
  }
  long double val;
  sscanf(number, format_string, &val);
  NVALTYPE num;
  num.flt = val;
  YYLVALTYPE r_val;
  r_val.value = num;
  r_val.type = TDOUBLE + (last == 'f') + (last == 'F') +
               2 * ((last == 'L') + (last == 'l'));
  return r_val;
}

YYLVALTYPE convert_to_int(char *number, int len, int base) {
  unsigned long long val;
  char *format_string = "%llu%*s";
  // What is the base we are converting
  if (base == 16) {
    format_string = "0x%llx%*s";
  } else if (base == 8) {
    format_string = "0%llo%*s";
  } else if (base != 10) {
    fprintf(stderr, "ERROR convert_to_int called with invalid base (error "
                    "should never reach end user)");
    exit(1);
  }

  // Get Suffix info
  int end = len < 3 ? len : 3;
  char *last_three = len < 3 ? number : number + len - 3;
  int i;
  for (i = 0; i < end; ++i) {
    if (isdigit(last_three[i]))
      continue;
    break; // grabs the first suffix and puts it in i
  }
  int start_suffix = i;
  int is_unsigned = 0;
  if (i < end &&
      (last_three[i] == 'U' || last_three[i] == 'u')) { // checks if unsigned
    ++i;
    is_unsigned = 1;
  }

  for (; i < end; ++i) { // counts length
    if (!(last_three[i] == 'L' || last_three[i] == 'l'))
      break;
  }

  // stores the number
  sscanf(number, format_string, &val);
  NVALTYPE num;
  num.u_int = val;
  YYLVALTYPE r_val;
  r_val.value = num;

  // gets the inital type
  r_val.type = TINT + (is_unsigned * 3) + (i - start_suffix - is_unsigned);

  // checks to make sure it fits
  switch (r_val.type) {
  case TINT: {
    if ((int)val == val) {
      break;
    }
    r_val.type = TLONG;
  }
  case TLONG: {
    if ((long)val == val) {
      break;
    }
    r_val.type = TLONGLONG;
  }
  case TLONGLONG: {
    if ((long long)val == val) {
      break;
    }
    // Ask what to do here
    break;
  }
  case TUINT: {
    if ((unsigned int)val == val) {
      break;
    }
    r_val.type = TULONG;
  }
  case TULONG: {
    if ((unsigned long)val == val) {
      break;
    }
    r_val.type = TULONGLONG;
  }
  case TULONGLONG: {
    if ((unsigned long long)val == val) {
      break;
    }
    // Ask what to do here
    break;
  }
  default: {
    fprintf(stderr,
            "Internal Compiler Error matched integer returns non-integer type");
    exit(1);
  }
  }
  return r_val;
}
