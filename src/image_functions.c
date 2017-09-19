#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sass.h>

#define GET_ERROR -1;

struct IMAGE_HANDLER {
	const char *type;
	size_t length;
	uint8_t *signature;
	int (*handler)(int, uint32_t*, uint32_t*);
};

uint8_t png_signature[] = { '\211', 'P', 'N', 'G', '\r', '\n', '\032', '\n' };
uint8_t gif87a_signature[] = { 'G', 'I', 'F', '8', '7', 'a' };
uint8_t gif89a_signature[] = { 'G', 'I', 'F', '8', '9', 'a' };

static int get_gif_dimensions(int fd, uint32_t* width, uint32_t* height)
{
	uint16_t w, h;
	if (-1 == lseek(fd, 6, SEEK_SET)) {
		return GET_ERROR;
	}
	if (2 != read(fd, &w, 2) || 2 != read(fd, &h, 2)) {
		return GET_ERROR;
	}
	*width = w;
	*height = h;

	return 0;
}

static int get_png_dimensions(int fd, uint32_t* width, uint32_t* height)
{
	if (-1 == lseek(fd, 16, SEEK_SET)) {
		return GET_ERROR;
	}
	int32_t w, h;
	if (4 != read(fd, &w, 4) || 4 != read(fd, &h, 4)) {
		return GET_ERROR;
	}
	*width = ntohl(w);
	*height = ntohl(h);

	return 0;
}

struct IMAGE_HANDLER TABLE[] = {
	{ "png", sizeof(png_signature), png_signature, get_png_dimensions },
	{ "gif", sizeof(gif87a_signature), gif87a_signature, get_gif_dimensions },
	{ "gif", sizeof(gif89a_signature), gif89a_signature, get_gif_dimensions },
	{ NULL, 0, NULL, NULL }
};

static int get_dimensions(const char* path, uint32_t *width, uint32_t *height)
{
	int return_val = GET_ERROR;
	int i = 0;
	size_t MAX_LENGTH = 8;
	uint8_t buf[MAX_LENGTH];
	int fd = open(path, O_RDONLY);

	if (fd < 0) {
		return GET_ERROR;
	}
	if (MAX_LENGTH != read(fd, buf, MAX_LENGTH)) {
		return GET_ERROR;
	}
	for (i = 0; TABLE[i].type != NULL; i++) {
		if (0 == memcmp(buf, TABLE[i].signature, TABLE[i].length)) {
			if (TABLE[i].handler) {
				return_val = (*TABLE[i].handler)(fd, width, height);
			}
			break;
		}
	}
	close(fd);

	return return_val;
}

union Sass_Value* sass_image_width(const union Sass_Value* s_args, Sass_Function_Entry cb, struct Sass_Compiler* comp)
{
	if (!sass_value_is_list(s_args)) {
		return sass_make_error("Invalid arguments");
	}
	if (sass_list_get_length(s_args) != 1) {
		return sass_make_error("Exactly one arguments expected");
	}
	union Sass_Value* path_val = sass_list_get_value(s_args, 0);
	uint32_t width = 0, height = 0;
	if (0 != get_dimensions(sass_string_get_value(path_val), &width, &height)) {
		return sass_make_error("Cannot get image dimensions");
	}
	return sass_make_number(width, "px");
}

union Sass_Value* sass_image_height(const union Sass_Value* s_args, Sass_Function_Entry cb, struct Sass_Compiler* comp)
{
	if (!sass_value_is_list(s_args)) {
		return sass_make_error("Invalid arguments");
	}
	if (sass_list_get_length(s_args) != 1) {
		return sass_make_error("Exactly one arguments expected");
	}
	union Sass_Value* path_val = sass_list_get_value(s_args, 0);
	uint32_t width = 0, height = 0;
	if (0 != get_dimensions(sass_string_get_value(path_val), &width, &height)) {
		return sass_make_error("Cannot get image dimensions");
	}
	return sass_make_number(height, "px");
}

const char* ADDCALL libsass_get_version()
{
	return libsass_version();
}

Sass_Function_List ADDCALL libsass_load_functions()
{
	Sass_Function_List fn_list = sass_make_function_list(2);

	sass_function_set_list_entry(fn_list, 0, sass_make_function("image_width($value)", sass_image_width, NULL));
	sass_function_set_list_entry(fn_list, 1, sass_make_function("image_height($value)", sass_image_height, NULL));

	return fn_list;
}
