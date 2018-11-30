#include <errno.h>
#include <limits.h>
#include <math.h>
#include <memory.h>
#include <stdlib.h>
#include "cte_internal.h"


static bool is_hex_character(int ch)
{
    if(ch >= '0' && ch <= '9')
    {
        return true;
    }
    return ch >= 'a' && ch <= 'f';
}

static bool is_octal_character(int ch)
{
    return ch >= '0' && ch <= '7';
}

// Unquotes and unescapes a string in-place (modifies the original string)
// Returns NULL if successful, or else a pointer to the offending escape sequence.
static char* string_unquote_unescape(char* str)
{
    char* read_pos = str;
    char* read_end = read_pos + strlen(read_pos);
    char* write_pos = str;

    if(*read_pos != '"')
    {
        return read_pos;
    }
    read_pos++;

    for(; read_pos < read_end; read_pos++)
    {
        switch(*read_pos)
        {
            case '"':
                *write_pos = 0;
                return NULL;
            case '\\':
            {
                char* escape_pos = read_pos;
                int remaining_bytes = read_end - read_pos;
                if(remaining_bytes < 1)
                {
                    return escape_pos;
                }

                read_pos++;
                switch(*read_pos)
                {
                    case 'r': *write_pos++ = '\r'; break;
                    case 'n': *write_pos++ = '\n'; break;
                    case 't': *write_pos++ = '\t'; break;
                    case '\\': *write_pos++ = '\\'; break;
                    case '\"': *write_pos++ = '\"'; break;
                    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
                    {
                        uint8_t number_buffer[4] = {*read_pos, 0, 0, 0};
                        read_pos++;
                        if(is_octal_character(*read_pos))
                        {
                            number_buffer[1] = *read_pos;
                            read_pos++;
                        }
                        if(is_octal_character(*read_pos))
                        {
                            number_buffer[2] = *read_pos;
                            read_pos++;
                        }
                        *write_pos++ = (uint8_t)strtoul((char*)number_buffer, NULL, 8);
                        read_pos--;
                        break;
                    }
                    case 'x':
                    {
                        if(remaining_bytes < 2)
                        {
                            return escape_pos;
                        }
                        read_pos++;
                        if(!is_hex_character(*read_pos))
                        {
                            return escape_pos;
                        }
                        uint8_t number_buffer[3] = {*read_pos, 0, 0};
                        read_pos += 1;
                        if(is_hex_character(*read_pos))
                        {
                            number_buffer[1] = *read_pos;
                            read_pos += 1;
                        }
                        *write_pos++ = (uint8_t)strtoul((char*)number_buffer, NULL, 16);
                        read_pos--;
                        break;
                    }
                    case 'u':
                    {
                        if(remaining_bytes < 5)
                        {
                            return escape_pos;
                        }
                        if(!is_hex_character(read_pos[1])
                        || !is_hex_character(read_pos[2])
                        || !is_hex_character(read_pos[3])
                        || !is_hex_character(read_pos[4]))
                        {
                            return escape_pos;
                        }
                        uint8_t number_buffer[5] = {read_pos[1], read_pos[2], read_pos[3], read_pos[4], 0};
                        read_pos += 4;
                        unsigned int codepoint = strtoul((char*)number_buffer, NULL, 16);
                        if(codepoint <= 0x7f)
                        {
                            *write_pos++ = (uint8_t)codepoint;
                        }
                        else if(codepoint <= 0x7ff)
                        {
                            *write_pos++ = (uint8_t)((codepoint >> 6) | 0xc0);
                            *write_pos++ = (uint8_t)((codepoint & 0x3f) | 0x80);
                        }
                        else
                        {
                            *write_pos++ = (uint8_t)((codepoint >> 12) | 0xe0);
                            *write_pos++ = (uint8_t)(((codepoint >> 6) & 0x3f) | 0x80);
                            *write_pos++ = (uint8_t)((codepoint & 0x3f) | 0x80);
                        }
                        break;
                    }
                    default:
                        return escape_pos;
                }
                break;
            }
            default:
                *write_pos++ = *read_pos;
                break;
        }
    }
    *write_pos = 0;
    return NULL;
}


static void on_error(cte_real_decode_process* process, const char* message)
{
	process->callbacks->on_error((cte_decode_process*)process, message);
}

static int on_empty(cte_real_decode_process* process)
{
    process->callbacks->on_empty((cte_decode_process*)process);
	return 0;
}

static int on_true(cte_real_decode_process* process)
{
    process->callbacks->on_boolean((cte_decode_process*)process, true);
	return 0;
}

static int on_false(cte_real_decode_process* process)
{
    process->callbacks->on_boolean((cte_decode_process*)process, false);
	return 0;
}

static int on_int(cte_real_decode_process* process, int base, const char* string_value)
{
    int64_t value = strtoll(string_value, NULL, base);
    if((value == LLONG_MAX || value == LLONG_MIN) && errno == ERANGE)
    {
    	process->callbacks->on_error((cte_decode_process*)process, "Number out of range");
    	return -1;
    }
    process->callbacks->on_int_64((cte_decode_process*)process, value);

	return 0;
}

static int on_float(cte_real_decode_process* process, const char* string_value)
{
    double value = strtod(string_value, NULL);
    if((value == HUGE_VAL || value == -HUGE_VAL) && errno == ERANGE)
    {
    	process->callbacks->on_error((cte_decode_process*)process, "Number out of range");
    	return -1;
    }
    process->callbacks->on_float_64((cte_decode_process*)process, value);
	return 0;
}

static int on_decimal(cte_real_decode_process* process, const char* string_value)
{
	// TODO
	(void)process;
	(void)string_value;
	return 0;
}

static int on_time(cte_real_decode_process* process, const char* string_value)
{
	// TODO
	(void)process;
	(void)string_value;
	return 0;
}

static int on_list_begin(cte_real_decode_process* process)
{
    process->callbacks->on_list_begin((cte_decode_process*)process);
	return 0;
}

static int on_list_end(cte_real_decode_process* process)
{
    process->callbacks->on_list_end((cte_decode_process*)process);
	return 0;
}

static int on_map_begin(cte_real_decode_process* process)
{
    process->callbacks->on_map_begin((cte_decode_process*)process);
	return 0;
}

static int on_map_end(cte_real_decode_process* process)
{
    process->callbacks->on_map_end((cte_decode_process*)process);
	return 0;
}

static int on_string(cte_real_decode_process* process, const char* value)
{
	// TODO: Don't pass in as const
    char* bad_data_loc = string_unquote_unescape((char*)value);
    if(bad_data_loc != NULL)
    {
    	// TODO: Show where it's bad (at bad_data_log)
    	process->callbacks->on_error((cte_decode_process*)process, "Bad string");
    	return -1;
    }
    process->callbacks->on_string((cte_decode_process*)process, value);
	return 0;
}

static int on_array_begin(cte_real_decode_process* process, cte_data_type type)
{
	// TODO
	(void)process;
	(void)type;
	return 0;
}

static int on_array_end(cte_real_decode_process* process)
{
    process->callbacks->on_array_end((cte_decode_process*)process);
	return 0;
}



cte_decode_process* cte_decode_begin(cte_decode_callbacks* callbacks, void* user_context)
{
    cte_real_decode_process* process = malloc(sizeof(*process));
    memset(process, 0, sizeof(*process));
    process->callbacks = callbacks;
    process->user_context = user_context;
    process->parse_callbacks.on_error = on_error;
    process->parse_callbacks.on_empty = on_empty;
    process->parse_callbacks.on_true = on_true;
    process->parse_callbacks.on_false = on_false;
    process->parse_callbacks.on_int = on_int;
    process->parse_callbacks.on_float = on_float;
    process->parse_callbacks.on_decimal = on_decimal;
    process->parse_callbacks.on_time = on_time;
    process->parse_callbacks.on_list_begin = on_list_begin;
    process->parse_callbacks.on_list_end = on_list_end;
    process->parse_callbacks.on_map_begin = on_map_begin;
    process->parse_callbacks.on_map_end = on_map_end;
    process->parse_callbacks.on_string = on_string;
    process->parse_callbacks.on_array_begin = on_array_begin;
    process->parse_callbacks.on_array_end = on_array_end;
    return (cte_decode_process*)process;
}

void* cte_decode_get_user_context(cte_decode_process* decode_process)
{
    cte_real_decode_process* process = (cte_real_decode_process*)decode_process;
    return process->user_context;
}