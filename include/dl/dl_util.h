/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_UTIL_H_INCLUDED
#define DL_DL_UTIL_H_INCLUDED

/*
	File: dl_reflect.h
		Utility functions for DL. Mostly for making tools writing easier.
*/

#include <dl/dl.h>

/*
	Enum: dl_util_file_type_t
		Enumeration of possible file types that can be read by util-functions.

	DL_UTIL_FILE_TYPE_BINARY - File must be an binary instance.
	DL_UTIL_FILE_TYPE_TEXT   - File must be an text instance.
	DL_UTIL_FILE_TYPE_AUTO   - Let the function decide if binary or text.
*/
enum dl_util_file_type_t
{
	DL_UTIL_FILE_TYPE_BINARY = DL_BIT(0),
	DL_UTIL_FILE_TYPE_TEXT   = DL_BIT(1),
	DL_UTIL_FILE_TYPE_AUTO   = DL_UTIL_FILE_TYPE_BINARY  | DL_UTIL_FILE_TYPE_TEXT
};

/*
	Function: dl_util_load_from_file
		Utility function that loads an dl-instance from file.

	Note:
		This function allocates memory internally by use of malloc/free and should therefore
		be used accordingly.
		The pointer returned in _ppInstance need to be freed with free() when not needed any
		more.

	Parameters:
		dl_ctx       - Context to use for operations.
		type         - Type expected to be found in file.
		filename     - Path to file to load from.
		filetype     - Type of file to read, see EDLUtilFileType.
		out_instance - Pointer to fill with read instance.

	Returns:
		DL_ERROR_OK on success.
*/
dl_error_t dl_util_load_from_file( dl_ctx_t dl_ctx, dl_typeid_t type, const char* filename, dl_util_file_type_t filetype, void** out_instance );

/*
	Function: dl_util_load_from_file_inplace
		Utility function that loads an dl-instance from file to a specified memory-area.

	Note:
		This function allocates memory internally by use of malloc/free and should therefore
		be used accordingly.

	Parameters:
		dl_ctx            - Context to use for operations.
		type              - Type expected to be found in file.
		filename          - Path to file to load from.
		filetype          - Type of file to read, see EDLUtilFileType.
		out_instance      - Pointer to area to load instance to.
		out_instance_size - Size of buffer pointed to by _ppInstance

	Returns:
		DL_ERROR_OK on success.
*/
dl_error_t dl_util_load_from_file_inplace( dl_ctx_t dl_ctx, dl_typeid_t type, const char* filename, dl_util_file_type_t filetype, void* out_instance, unsigned int out_instance_size );

/*
	Function: dl_util_store_to_file
		Utility function that writes an instance to file.

	Note:
		This function allocates memory internally by use of malloc/free and should therefore
		be used accordingly.

	Parameters:
		dl_ctx       - Context to use for operations.
		type         - Type expected to be found in instance.
		filename     - Path to file to store to.
		filetype     - Type of file to write, see EDLUtilFileType.
		out_endian   - Endian of stored instance if binary.
		out_ptr_size - Pointer size of stored instance if binary.
		out_instance - Pointer to instance to write

	Returns:
		DL_ERROR_OK on success.
*/
dl_error_t dl_util_store_to_file( dl_ctx_t dl_ctx, dl_typeid_t type, const char* filename, dl_util_file_type_t filetype, dl_endian_t out_endian, unsigned int out_ptr_size, void* out_instance );

#endif // DL_DL_UTIL_H_INCLUDED
