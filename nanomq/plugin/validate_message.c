/*
to build this plugin:gcc -I.. -I../../nng/include -fPIC -shared validate_message.c -o validate_message.so -L/usr/local/lib -lwjelement -lwjreader -lwjwriter
*/
#include "include/plugin.h"
#include <stdio.h>
#include <stdlib.h>
#include <wjreader.h>
#include <wjelement.h>
static WJElement cachedSchema = NULL;
char *
read_schema_file(const char *schema_file_path)
{
	FILE *schema_file = fopen(schema_file_path, "r");
	if (!schema_file) {
		log_debug("Unable to open schema file");
		return NULL;
	}

	fseek(schema_file, 0, SEEK_END);
	long schema_size = ftell(schema_file);
	fseek(schema_file, 0, SEEK_SET);

	char *schema_buffer = malloc(schema_size + 1);
	if (!schema_buffer) {
		log_debug("Memory allocation failed");
		fclose(schema_file);
		return NULL;
	}

	fread(schema_buffer, 1, schema_size, schema_file);
	schema_buffer[schema_size] = '\0';
	fclose(schema_file);
	return schema_buffer;
}

// /*
//   callback: plop validation errors to stderr
// */
static void
schema_error(void *client, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}
int
validate_message(char *payload_data)
{
	WJElement schemaElement = NULL;
	char     *schemaContent = NULL; // For schema file content

	if (cachedSchema == NULL) {
		log_debug("Schema not cached. Reading schema from file.");
		schemaContent = read_schema_file("/tmp/schema.json");
		if (schemaContent == NULL) {
			log_debug("No schema file found on the given path.");
			return 1;
		}

		schemaElement =
		    __WJEFromString(schemaContent, '"', __FILE__, 0);
		free(schemaContent); // Free schema content after
		                     // parsing
		schemaContent = NULL;

		if (schemaElement == NULL) {
			log_error("Failed to parse schema file.");
			return 0;
		}
	} else {
		log_debug("Using cached schema.");
		schemaElement = cachedSchema;
	}

	WJElement payloadElement =
	    __WJEFromString(payload_data, '"', __FILE__, 0);
	if (payloadElement == NULL) {
		log_error("Failed to parse payload data.");
		if (cachedSchema == NULL && schemaElement != cachedSchema) {
			WJECloseDocument(
			    schemaElement); // Free schemaElement if
			                    // dynamically created
		}
		return 0;
	}

	if (WJESchemaValidate(schemaElement, payloadElement, schema_error,
	        NULL, NULL, NULL)) {
		log_debug("Validation success.");
		WJECloseDocument(payloadElement); // Free payloadElement
		if (cachedSchema == NULL && schemaElement != cachedSchema) {
			WJECloseDocument(
			    schemaElement); // Free schemaElement if
			                    // dynamically created
		}
		return 1;
	} else {
		log_error("Validation failed.");
		WJECloseDocument(payloadElement); // Free payloadElement
		if (cachedSchema == NULL && schemaElement != cachedSchema) {
			WJECloseDocument(
			    schemaElement); // Free schemaElement if
			                    // dynamically created
		}
		return 0;
	}
 return 0;
}
int
cb(void *data)
{
	log_debug("validating message");
	char *payload_data = (char *) data;
	int result =  validate_message(payload_data);
	if(result == 0){
		return 1;
	}
	else{
		return 2;
	}
}
int
nano_plugin_init()
{
	plugin_hook_register(HOOK_VALIDATE_MESSAGE, cb);
	return 0;
}