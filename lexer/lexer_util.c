#include "./lexer_util.h"
#include "./token_codes.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void get_file_info(char *file_info_str, int length, FileInfo *file_info) {
  free(file_info->file_name);
  file_info->file_name = malloc(sizeof(char) * length);
  sscanf(file_info_str, "# %d \"%s\"%*s", &(file_info->file_line_start),
         file_info->file_name);
  file_info->file_line_start--;
}

YYLVALTYPE convert_to_str(char *character, int len) {
  int has_prefix = character[0] != '"';
  int prefix_count = has_prefix + has_prefix * (character[1] == '8');
  int t_len = len - 2 - prefix_count + 1; // len - 2("") - prefix + 1(\0)
  char *str = (char *)malloc(sizeof(char) * t_len); // TODO free this after user
  strncpy(str, character + prefix_count + 1, t_len);
  str[t_len - 1] = 0;
  // gets the true str length
  int i;
  int true_str_len = 0;
  for (i = 0; i < t_len - 1; ++i) {
    if (str[i] == '\\') {
      if (str[i + 1] == '\\') {
        ++true_str_len;
        ++i;
      } else if (str[i + 1] == 'x') {
        i += 2;
        if (i + 1 < t_len - 1 && isxdigit(str[i + 1])) {
          ++i;
        }
        ++true_str_len;
      } else if (isdigit(str[i + 1])) {
        ++i;
        int j;
        // check the next two
        // TODO CODE IS UGLY AND BAD
        for (j = 0; j < 2; ++j) {
          if (i + 1 < t_len - 1 && isdigit(str[i + 1])) {
            ++i;
          }
        }
        ++true_str_len;
      }
      continue;
    }
    ++true_str_len;
  }
  YYNVal num;
  num.str = str;
  YYLVALTYPE r_val;
  r_val.value = num;
  r_val.type = Tu8;
  r_val.str_len = true_str_len;
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
  if (character[has_prefix + 1] != '\\')
    val = character[has_prefix + 1];
  else {
    switch (character[has_prefix + 2]) {
    case '\'':
      val = '\'';
      break;
    case '"':
      val = '\"';
      break;
    case '?':
      val = '\?';
      break;
    case '\\':
      val = '\\';
      break;
    case 'a':
      val = '\a';
      break;
    case 'b':
      val = '\b';
      break;
    case 'f':
      val = '\f';
      break;
    case 'n':
      val = '\n';
      break;
    case 'r':
      val = '\r';
      break;
    case 't':
      val = '\t';
      break;
    case 'v':
      val = '\v';
      break;
    case 'x': {
      int c_code;
      sscanf(character + has_prefix, "'\\x%x'", &c_code);
      val = c_code;
      break;
    }
    default: {
      int c_code;
      sscanf(character + has_prefix, "'\\%o'", &c_code);
      val = c_code;
    }
    }
  }

  YYNVal num;
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
  YYNVal num;
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
  YYNVal num;
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
