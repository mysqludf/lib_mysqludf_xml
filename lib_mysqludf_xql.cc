/*
 * XML libary for MySQL.
 * A set of MySQL user defined functions (udf) to create XML output directly from MySQL using an SQL query
 *
 * Copyright (C) 2006-2009 Arnold Daniels <arnold@adaniels.nl>
 * web: http://www.mysqludf.com/lib_mysqludf_xql
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "mysqludf.h"

#include <libxml/tree.h>
#include <libxml/xmlwriter.h>
#include <libxml/uri.h>

#ifndef __WIN__
#include "config.h"
#endif

/* These must be right or mysqld will not find the symbol! */

#ifdef	__cplusplus
extern "C" {
#endif
	DLLEXP my_bool lib_mysqludf_xql_info_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
	DLLEXP void lib_mysqludf_xql_info_deinit(UDF_INIT *initid);
	DLLEXP char *lib_mysqludf_xql_info(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error);

	DLLEXP my_bool xql_test_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
	DLLEXP void xql_test_deinit(UDF_INIT *initid);
	DLLEXP char *xql_test(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error);

	DLLEXP my_bool xql_textdecl_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
	DLLEXP void xql_textdecl_deinit(UDF_INIT *initid);
	DLLEXP char *xql_textdecl(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error);

	DLLEXP my_bool xql_pi_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
	DLLEXP void xql_pi_deinit(UDF_INIT *initid);
	DLLEXP char *xql_pi(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error);

	DLLEXP my_bool xql_dtd_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
	DLLEXP void xql_dtd_deinit(UDF_INIT *initid);
	DLLEXP char *xql_dtd(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error);


	DLLEXP my_bool xql_element_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
	DLLEXP void xql_element_deinit(UDF_INIT *initid);
	DLLEXP char *xql_element(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error);

	DLLEXP my_bool xql_forest_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
	DLLEXP void xql_forest_deinit(UDF_INIT *initid);
	DLLEXP char *xql_forest(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error);

	DLLEXP my_bool xql_concat_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
	DLLEXP void xql_concat_deinit(UDF_INIT *initid);
	DLLEXP char *xql_concat(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error);

	DLLEXP my_bool xql_agg_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
	DLLEXP void xql_agg_deinit(UDF_INIT *initid);
	DLLEXP void xql_agg_reset(UDF_INIT* initid, UDF_ARGS* args, char* is_null, char* message);
	DLLEXP void xql_agg_clear(UDF_INIT* initid, char* is_null, char* message);
	DLLEXP void xql_agg_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
	DLLEXP char *xql_agg(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error);


	DLLEXP my_bool xql_comment_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
	DLLEXP void xql_comment_deinit(UDF_INIT *initid);
	DLLEXP char *xql_comment(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error);

	DLLEXP my_bool xql_cdata_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
	DLLEXP void xql_cdata_deinit(UDF_INIT *initid);
	DLLEXP char *xql_cdata(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error);

	DLLEXP my_bool xql_encode_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
	DLLEXP void xql_encode_deinit(UDF_INIT *initid);
	DLLEXP char *xql_encode(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error);
#ifdef	__cplusplus
}
#endif

#define CONTENT_ENCODE	0		// XML encode content (normal behaviour)
#define CONTENT_RAW 	1		// Do not encode content
#define CONTENT_CDATA	2		// Place content in a CDATA section


/* === Helper function === */

/**
 * Structure to hold XMLWriter pointers and additional info gathered in the %_init function
 */ 
typedef struct
{
	xmlTextWriterPtr writer;
	xmlBufferPtr buffer;
	char add_root;
	char add_child;
	char content_type;
	char **arg_names;
} xmlWriterBuffer;

/**
 * Create an XMLWriter + buffer and init the whole var
 */
xmlWriterBuffer *newXmlWriterBuffer(char *message)
{
	xmlWriterBuffer* data = new xmlWriterBuffer;

	data->buffer = xmlBufferCreate();
	if (data->buffer == NULL) {
		strcpy(message, "Init xql_element failed; Unable to create output buffer");
		return NULL;
	}

	data->writer = xmlNewTextWriterMemory(data->buffer, 0);
	if (data->writer == NULL) {
		strcpy(message, "Init xql_element failed; Unable to create xml text writer");
		xmlBufferFree(data->buffer);
		return NULL;
	}

	data->add_root = 0; 
	data->add_child = 0;
	data->content_type = CONTENT_ENCODE;
	data->arg_names = NULL;

	return data;
}

/**
 * Delete XMLWriter + buffer and other things used
 */
void freeXmlWriterBuffer(xmlWriterBuffer *data)
{
	xmlBufferFree(data->buffer);
	xmlFreeTextWriter(data->writer);
	if (data->arg_names) ptr_free((void **)data->arg_names);
	
	free(data);
}


/* === XQL UDF functions === */

/**
 * Output the library version.
 * lib_mysqludf_xql_info_init()
 */

my_bool lib_mysqludf_xql_info_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
	return 0;
}

void lib_mysqludf_xql_info_deinit(UDF_INIT *initid)
{
}

char* lib_mysqludf_xql_info(UDF_INIT *initid, UDF_ARGS *args, char* result, unsigned long* length,	char *is_null, char *error)
{
	strcpy(result, PACKAGE_STRING);
	*length = strlen(PACKAGE_STRING);
	return result;
}

/**
 * A simple xmlwriter test function.
 * xql_test()
 */

my_bool xql_test_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
	xmlWriterBuffer* data;

	data = newXmlWriterBuffer(message);
	if (data == NULL) return 1;

	initid->ptr = (char*)data;
	return 0;
}

void xql_test_deinit(UDF_INIT *initid)
{
	freeXmlWriterBuffer((xmlWriterBuffer*)initid->ptr);
}

char *xql_test(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error)
{
	xmlTextWriterPtr writer;
	xmlBufferPtr buffer;

	writer = ((xmlWriterBuffer*)initid->ptr)->writer;
	buffer = ((xmlWriterBuffer*)initid->ptr)->buffer;
	xmlBufferEmpty(buffer);

	xmlTextWriterStartDocument(writer, "1.0", "ISO-8859-1", NULL);
	xmlTextWriterWriteElement(writer, (xmlChar*)"Hello", (xmlChar*)"World");
	xmlTextWriterEndDocument(writer);
	xmlTextWriterFlush(writer);

	*length = (unsigned long)buffer->use;
	return (char*)buffer->content;
}


/**
 * Write a text declarion; <?xml version="1.0" encoding="iso-8859-1" standalone="yes"?>.
 * xql_textdecl(string version, string encoding [, string standalone])
 */

my_bool xql_textdecl_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
	xmlWriterBuffer* data;

	if (args->arg_count < 1) RETURN_ERR("xql_textdecl() requires at least 1 argument (version)");
	if (args->arg_count > 3) RETURN_ERR("xql_textdecl() takes at most 3 arguments (version, encoding, standalone)");

	data = newXmlWriterBuffer(message);
	if (data == NULL) return 1;

	args->arg_type[0] = STRING_RESULT;
	args->arg_type[1] = STRING_RESULT;
	if (args->arg_count >= 3) args->arg_type[2] = STRING_RESULT;

	initid->ptr = (char*)data;
	return 0;
}

void xql_textdecl_deinit(UDF_INIT *initid)
{
	freeXmlWriterBuffer((xmlWriterBuffer*)initid->ptr);
}

char *xql_textdecl(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error)
{
	xmlTextWriterPtr writer;
	xmlBufferPtr buffer;

	if (!args->args[0] || !args->lengths[0]) {
		*is_null=1;
		return 0;
	}

	writer = ((xmlWriterBuffer*)initid->ptr)->writer;
	buffer = ((xmlWriterBuffer*)initid->ptr)->buffer;
	xmlBufferEmpty(buffer);

	char **ntargs = (char**) malloc(args->arg_count * sizeof(char*));
	for (int i=0; i<args->arg_count; i++) {
		ntargs[i] = strncpy_alloc(args->args[i], args->lengths[i]);
	}

	xmlTextWriterStartDocument(writer, ntargs[0], args->arg_count >= 2 && ntargs[1] && args->lengths[1] > 0 ? ntargs[1] : NULL, args->arg_count >= 3 && ntargs[2] && args->lengths[2] > 0 ? ntargs[2] : NULL);
	xmlTextWriterFlush(writer);

	for (int i=0; i<args->arg_count; i++) {
		if (ntargs[i]!=NULL) free(ntargs[i]);
	}
	free(ntargs);

	*length = (unsigned long)buffer->use;
	return (char*)buffer->content;
}


/**
 * Write a processing instruction.
 * xql_pi(string content AS target)
 */

my_bool xql_pi_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
	xmlWriterBuffer* data;

	if (args->arg_count != 1) RETURN_ERR("xql_pi() requires 1 argument (content AS target)");

	data = newXmlWriterBuffer(message);
	if (data == NULL) return 1;

	args->arg_type[0] = STRING_RESULT;
	
	initid->ptr = (char*)data;
	return 0;
}

void xql_pi_deinit(UDF_INIT *initid)
{
	freeXmlWriterBuffer((xmlWriterBuffer*)initid->ptr);
}

char *xql_pi(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error)
{
	xmlTextWriterPtr writer;
	xmlBufferPtr buffer;
	char *arg, *att;

	if (!args->args[0]) {
		*error=1;
		return 0;
	}

	writer = ((xmlWriterBuffer*)initid->ptr)->writer;
	buffer = ((xmlWriterBuffer*)initid->ptr)->buffer;
	xmlBufferEmpty(buffer);

	arg = strncpy_alloc(args->args[0], args->lengths[0]);
	att = strncpy_alloc(args->attributes[0], args->attribute_lengths[0]);

	xmlTextWriterWritePI(writer, (xmlChar*)att, (xmlChar*)arg);
	xmlTextWriterFlush(writer);

	free(arg);
	free(att);

	*length = (unsigned long)buffer->use;
	return (char*)buffer->content;
}


/**
 * Write a DTD tag.
 * xql_dtd(string name [, string pubid, string sysid [, string subset]]])
 */

my_bool xql_dtd_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
	xmlWriterBuffer* data;

	if (args->arg_count < 1) RETURN_ERR("xql_dtd() requires at least 1 argument (name)");
	if (args->arg_count == 2) RETURN_ERR("xql_dtd() argument pubid given without argument sysid");
	if (args->arg_count > 4) RETURN_ERR("xql_dtd() takes at most 4 arguments");

	data = newXmlWriterBuffer(message);
	if (data == NULL) return 1;

	for (int i=0; i<args->arg_count; i++) {
		args->arg_type[i] = STRING_RESULT;
	}

	initid->ptr = (char*)data;
	return 0;
}

void xql_dtd_deinit(UDF_INIT *initid)
{
	freeXmlWriterBuffer((xmlWriterBuffer*)initid->ptr);
}

char *xql_dtd(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error)
{
	xmlTextWriterPtr writer;
	xmlBufferPtr buffer;

	if (!args->args[0] || args->lengths[0] == 0 || (args->arg_count > 1 && ((!args->args[1] || args->lengths[1] == 0) ^ (!args->args[2] || args->lengths[2] == 0)))) {
		*is_null=1;
		return 0;
	}

	writer = ((xmlWriterBuffer*)initid->ptr)->writer;
	buffer = ((xmlWriterBuffer*)initid->ptr)->buffer;
	xmlBufferEmpty(buffer);
	
	char **ntargs = (char**) malloc(args->arg_count * sizeof(char*));
	for (int i=0; i<args->arg_count; i++) {
		ntargs[i] = strncpy_alloc(args->args[i], args->lengths[i]);
	}

	xmlTextWriterWriteDTD(writer, (xmlChar*)ntargs[0], (xmlChar*)(args->arg_count >= 2 ? ntargs[1] : NULL), (xmlChar*)(args->arg_count >= 3 ? ntargs[2] : NULL), (xmlChar*)(args->arg_count >= 4 ? ntargs[3] : NULL));
	xmlTextWriterFlush(writer);
	
	for (int i=0; i<args->arg_count; i++) {
		if (ntargs[i]!=NULL) free(ntargs[i]);
	}
	free(ntargs);

	*length = (unsigned long)buffer->use;
	return (char*)buffer->content;
}


/**
 * Write element node with attributes.
 * xql_element(string name, [string content | string XML AS `xql:raw` | string CDATA AS `xql:cdata` [, string att_val AS `att_name`, ...]])
 */

my_bool xql_element_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
	xmlWriterBuffer* data;
	char *att;

	if (args->arg_count < 1) RETURN_ERR("xql_element() requires at least 1 argument (name)");

	data = newXmlWriterBuffer(message);
	if (data == NULL) return 1;

	if (args->arg_count > 2) data->arg_names = (char **)ptr_calloc(args->arg_count, sizeof(char *));

	args->arg_type[0] = STRING_RESULT;

	if (args->arg_count > 1) {
		args->arg_type[1] = STRING_RESULT;

		if (ATTRIBUTE_COMPARE(1, "xql:raw", 7) || args->attribute_lengths[1] > 5 && strncmp_caseins(args->attributes[1], "xql_", 4) == 0 && charinstr(args->attributes[1], '(', args->attribute_lengths[1]) > 4) {
			data->content_type = CONTENT_RAW;
		} else if (ATTRIBUTE_COMPARE(1, "xql:cdata", 9)) {
			data->content_type = CONTENT_CDATA;
		} else if (args->attribute_lengths[1] >= 4 && strncmp_caseins(args->attributes[1], "xql:", 4) == 0) {
			att =copy_argname(args->attributes[1], args->attribute_lengths[1]);
			sprintf(message, "Illigal XQL directive '%s'", att);
			free(att);
			return 1;
		}
	}
	
	for (int i=2; i < args->arg_count; i++) {
		args->arg_type[i] = STRING_RESULT;
		data->arg_names[i] =copy_argname(args->attributes[i], args->attribute_lengths[i]);
	}
		
	initid->ptr = (char*)data;
	return 0;
}

void xql_element_deinit(UDF_INIT *initid)
{
	freeXmlWriterBuffer((xmlWriterBuffer*)initid->ptr);
}

char *xql_element(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error)
{
	xmlWriterBuffer* data;
	xmlTextWriterPtr writer;
	xmlBufferPtr buffer;

	if (!args->args[0] || args->lengths[0] == 0) {
		*is_null=1;
		return 0;
	}

	data = (xmlWriterBuffer*)initid->ptr;
	writer = data->writer;
	buffer = data->buffer;
	xmlBufferEmpty(buffer);

	char **ntargs = (char**) malloc(args->arg_count * sizeof(char*));
	for (int i=0; i<args->arg_count; i++) {
		ntargs[i] = strncpy_alloc(args->args[i], args->lengths[i]);
	}

	xmlTextWriterStartElement(writer, (xmlChar*)ntargs[0]);
	for (int i=2; i<args->arg_count; i++) {
		if (ntargs[i]) xmlTextWriterWriteAttribute(writer, (xmlChar*)(data->arg_names[i]), (xmlChar*)ntargs[i]);
	}
	
	if (args->arg_count > 1 && ntargs[1]) {
		switch (((xmlWriterBuffer*)initid->ptr)->content_type) {
			case CONTENT_ENCODE:	xmlTextWriterWriteString(writer, (xmlChar*)ntargs[1]); break;
			case CONTENT_RAW:		xmlTextWriterWriteRaw(writer, (xmlChar*)ntargs[1]); break;
			case CONTENT_CDATA: 	xmlTextWriterWriteCDATA(writer, (xmlChar*)ntargs[1]); break;
		}
	}
	xmlTextWriterEndElement(writer);
	xmlTextWriterFlush(writer);

	for (int i=0; i<args->arg_count; i++) {
		if (ntargs[i]!=NULL) free(ntargs[i]);
	}
	free(ntargs);

	*length = (unsigned long)buffer->use;
	return (char*)buffer->content;
}


/**
 * Write an element for each name/content pair.
 * Arguments named `xql:raw` and output of xql functions are inserted as raw XML.
 * Arguments starting with `xql:cdata:` are inserted as CDATA sections.
 * 
 * xql_forest([string name_root AS `xql:root`,] string content AS name | string XML AS `xql:raw` | string CDATA AS `xql:cdata:%`, ...)
 */

my_bool xql_forest_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
	xmlWriterBuffer* data;
	char *att;

	if (args->arg_count < 1) RETURN_ERR("xql_forest() requires at least 1 argument");
	
	data = newXmlWriterBuffer(message);
	if (data == NULL) return 1;

	data->arg_names = (char **)ptr_calloc(args->arg_count, sizeof(char *));
	if (!data->arg_names) RETURN_ERR("Could not allocate mereallocmory to store argument names.");

	if (args->arg_count > 0 && ATTRIBUTE_COMPARE(0, "xql:root", 8)) { 
		data->add_root = 1;
		args->arg_type[0] = STRING_RESULT;
	}

	for (int i=data->add_root ? 1 : 0; i < args->arg_count; i++) {
		args->arg_type[i] = STRING_RESULT;

		if (ATTRIBUTE_COMPARE(i, "xql:raw", 7) || args->attribute_lengths[i] > 5 && strncmp_caseins(args->attributes[i], "xql_", 4) == 0 && charinstr(args->attributes[i], '(', args->attribute_lengths[i]) > 4) {
			// Leave nameless
		} else if (args->attribute_lengths[i] >= 9 && strncmp_caseins(args->attributes[i], "xql:cdata", 9) == 0 && (args->attribute_lengths[i] == 9 || args->attributes[i][9] == ':')) {
			data->arg_names[i] = args->attribute_lengths[i] == 9 ? (char *)calloc(2, sizeof(char)) :copy_argname(args->attributes[i]+9, args->attribute_lengths[i]-9);
			if (!data->arg_names[i]) RETURN_ERR("Could not allocate memory to save argument name.");
			data->arg_names[i][0] = 1; // Directive to add CDATA section
		} else if (args->attribute_lengths[i] < 4 || strncmp_caseins(args->attributes[i], "xql:", 4) != 0) {
			data->arg_names[i] =copy_argname(args->attributes[i], args->attribute_lengths[i]);
			if (!data->arg_names[i]) RETURN_ERR("Could not allocate memory to save argument name.");
		} else {
			att =copy_argname(args->attributes[i], args->attribute_lengths[i]);
			sprintf(message, "Illigal XQL directive '%s'", att);
			free(att);
			return 1;
		}
	}

	initid->ptr = (char*)data;
	return 0;
}

void xql_forest_deinit(UDF_INIT *initid)
{
	freeXmlWriterBuffer((xmlWriterBuffer*)initid->ptr);
}

char *xql_forest(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error)
{
	xmlWriterBuffer* data;
	xmlTextWriterPtr writer;
	xmlBufferPtr buffer;
	char has_root=0;

	data = (xmlWriterBuffer*)initid->ptr;
	writer = data->writer;
	buffer = data->buffer;
	xmlBufferEmpty(buffer);

	*is_null=1;

	char **ntargs = (char**) malloc(args->arg_count * sizeof(char*));
	for (int i=0; i<args->arg_count; i++) {
		ntargs[i] = strncpy_alloc(args->args[i], args->lengths[i]);
	}

	if (data->add_root && args->args[0] && args->lengths[0] > 0) {	
		xmlTextWriterStartElement(writer, (xmlChar*)args->args[0]);
		*is_null=0;
	}

	for (int i=(int)(data->add_root); i < args->arg_count; i++) {
		if (ntargs[i]) {
			if (!data->arg_names[i]) {
				xmlTextWriterWriteRaw(writer, (xmlChar*)ntargs[i]);
			} else if (strlen(data->arg_names[i]) == 0) {
				xmlTextWriterStartElement(writer, (xmlChar*)ntargs[i]);
				xmlTextWriterEndElement(writer);
			} else if (data->arg_names[i][0] == (char)1) {
				xmlTextWriterStartElement(writer, (xmlChar*)(data->arg_names[i]+1));
				xmlTextWriterWriteCDATA(writer, (xmlChar*)ntargs[i]);
				xmlTextWriterEndElement(writer);
			} else {
				xmlTextWriterWriteElement(writer, (xmlChar*)data->arg_names[i], (xmlChar*)ntargs[i]);
			}
			
			*is_null=0;
		}
	}

	if (data->add_root && args->args[0] && args->lengths[0] > 0) {
		xmlTextWriterEndElement(writer);
	}

	xmlTextWriterFlush(writer);

	for (int i=0; i<args->arg_count; i++) {
		if (ntargs[i]!=NULL) free(ntargs[i]);
	}
	free(ntargs);

	if (*is_null) return 0;

	*length = (unsigned long)buffer->use;
	return (char*)buffer->content;
}


/**
 * Concatenate xml.
 * xql_concat([string name_root AS `xql:root`,] string xml, ...)
 */

my_bool xql_concat_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
	xmlWriterBuffer* data;

	if (args->arg_count < 1) RETURN_ERR("xql_concat() requires at least 1 argument");

	data = newXmlWriterBuffer(message);
	if (data == NULL) return 1;

	for (int i=0; i< args->arg_count; i++) {
		args->arg_type[i] = STRING_RESULT;
	}

	if (args->arg_count > 0) data->add_root = ATTRIBUTE_COMPARE(0, "xql:root", 8);

	initid->ptr = (char*)data;
	return 0;
}

void xql_concat_deinit(UDF_INIT *initid)
{
	freeXmlWriterBuffer((xmlWriterBuffer*)initid->ptr);
}

char *xql_concat(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error)
{
	xmlWriterBuffer* data;
	xmlTextWriterPtr writer;
	xmlBufferPtr buffer;

	data = (xmlWriterBuffer*)initid->ptr;
	writer = data->writer;
	buffer = data->buffer;
	xmlBufferEmpty(buffer);

	*is_null = !((xmlWriterBuffer*)initid->ptr)->add_root;

	char **ntargs = (char**) malloc(args->arg_count * sizeof(char*));
	for (int i=0; i<args->arg_count; i++) {
		ntargs[i] = strncpy_alloc(args->args[i], args->lengths[i]);
	}

	if (data->add_root) xmlTextWriterStartElement(writer, (xmlChar*)ntargs[0]);

	for (int i=data->add_root ? 1 : 0; i < args->arg_count; i++) {
		if (ntargs[i]) {
			xmlTextWriterWriteRaw(writer, (xmlChar*)ntargs[i]);
			*is_null = 0;
		}
	}

	if (data->add_root) xmlTextWriterEndElement(writer);

	xmlTextWriterFlush(writer);

	for (int i=0; i<args->arg_count; i++) {
		if (ntargs[i]!=NULL) free(ntargs[i]);
	}
	free(ntargs);

	if (*is_null) return 0;

	*length = (unsigned long)buffer->use;
	return (char*)buffer->content;
}


/**
 * Group concatenate XML for each row.
 * Normal arguments are inserted as XML nodes.
 * Arguments named `xql:raw` and output of xql functions are inserted as raw XML.
 * Arguments starting with `xql:cdata:` as name are inserted as CDATA sections.
 * 
 * xql_agg([string name_root AS `xql:root`,] [string name_child AS `xql:child`,] string content AS name | string xml, ...)
 */
 
my_bool xql_agg_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
	xmlWriterBuffer* data;
	char *att;
	if (args->arg_count < 1) RETURN_ERR("xql_agg() requires at least 1 argument");

	data = newXmlWriterBuffer(message);
	if (data == NULL) return 1;
	data->arg_names = (char **)ptr_calloc(args->arg_count, sizeof(char *));
	if (!data->arg_names) RETURN_ERR("Could not allocate memory to store argument names.");

	if (args->arg_count > 0 && ATTRIBUTE_COMPARE(0, "xql:root", 8)) {
		args->arg_type[0] = STRING_RESULT;
		data->add_root = 1;
	}
	
	if (args->arg_count > (data->add_root ? 1 : 0) && ATTRIBUTE_COMPARE((data->add_root ? 1 : 0), "xql:child", 9)) {	
		args->arg_type[(data->add_root ? 1 : 0)] = STRING_RESULT;
		data->add_child = 1;
	}
	
	for (int i = (data->add_root ? 1 : 0) + (data->add_child ? 1 : 0); i < args->arg_count; i++) {
		args->arg_type[i] = STRING_RESULT;

		if (ATTRIBUTE_COMPARE(i, "xql:raw", 7) || args->attribute_lengths[i] > 5 && strncmp_caseins(args->attributes[i], "xql_", 4) == 0 && charinstr(args->attributes[i], '(', args->attribute_lengths[i]) > 4) {
			// Leave nameless
		} else if (args->attribute_lengths[i] >= 9 && strncmp_caseins(args->attributes[i], "xql:cdata", 9) == 0 && (args->attribute_lengths[i] == 9 || args->attributes[i][9] == ':')) {
			data->arg_names[i] = args->attribute_lengths[i] == 9 ? (char *)calloc(2, sizeof(char)) :copy_argname(args->attributes[i]+9, args->attribute_lengths[i]-9);
			if (!data->arg_names[i]) RETURN_ERR("Could not allocate memory to save argument name.");
			data->arg_names[i][0] = (char)1; // Directive to add CDATA section
		} else if (args->attribute_lengths[i] < 4 || strncmp_caseins(args->attributes[i], "xql:", 4) != 0) {
			data->arg_names[i] =copy_argname(args->attributes[i], args->attribute_lengths[i]);
			if (!data->arg_names[i]) RETURN_ERR("Could not allocate memory to save argument name.");
		} else {
			att =copy_argname(args->attributes[i], args->attribute_lengths[i]);
			sprintf(message, "Illigal XQL directive '%s'", att);
			free(att);
			return 1;
		}
	}
	initid->ptr = (char*)data;
	return 0;
}

void xql_agg_deinit(UDF_INIT *initid)
{
	freeXmlWriterBuffer((xmlWriterBuffer*)initid->ptr);
}

/* This is only for MySQL 4.0 compability */
void axql_agg_reset(UDF_INIT* initid, UDF_ARGS* args, char* is_null, char* message)
{
	xql_agg_clear(initid, is_null, message);
	xql_agg_add(initid, args, is_null, message);
}

void xql_agg_clear(UDF_INIT* initid, char* is_null, char* message)
{
	xmlBufferEmpty(((xmlWriterBuffer*)initid->ptr)->buffer);
	if (((xmlWriterBuffer*)initid->ptr)->add_root != 0) ((xmlWriterBuffer*)initid->ptr)->add_root = 1;
}

void xql_agg_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
	int i = 0;
	char add_is_null = 1;
	char added_child = 0;

	xmlWriterBuffer* data;
	xmlTextWriterPtr writer;

	data = (xmlWriterBuffer*)initid->ptr;
	writer = data->writer;

	for (i=data->add_root >= 0 ? 0 : 1; i < args->arg_count; i++) {
		if (args->args[i]) add_is_null = 0;
	}
	if (add_is_null) return;
	
	char **ntargs = (char**) malloc(args->arg_count * sizeof(char*));
	for (i=0; i<args->arg_count; i++) {
		ntargs[i] = strncpy_alloc(args->args[i], args->lengths[i]);
	}

	i = 0;

	if (data->add_root == 1) {
		if (ntargs[i]) {
			xmlTextWriterStartElement(writer, (xmlChar*)ntargs[i]);
			data->add_root = -2;
		} else {
			data->add_root = -1;
		}
	}
	
	if (data->add_root != 0) i++;
	
	if (data->add_child) {
		if (ntargs[i]) {
			added_child = 1;
			xmlTextWriterStartElement(writer, (xmlChar*)ntargs[i]);
		}
		i++;
	}

	for (; i<args->arg_count; i++) {
		if (args->args[i]) {
			if (!data->arg_names[i]) {
				xmlTextWriterWriteRaw(writer, (xmlChar*)ntargs[i]);
			} else if (strlen(data->arg_names[i]) == 0) {
				xmlTextWriterStartElement(writer, (xmlChar*)ntargs[i]);
				xmlTextWriterEndElement(writer);
			} else if (data->arg_names[i][0] == (char)1) {
				xmlTextWriterStartElement(writer, (xmlChar*)(data->arg_names[i]+1));
				xmlTextWriterWriteCDATA(writer, (xmlChar*)ntargs[i]);
				xmlTextWriterEndElement(writer);
			} else {
				xmlTextWriterWriteElement(writer, (xmlChar*)data->arg_names[i], (xmlChar*)ntargs[i]);
			}
		}
	}

	if (added_child) xmlTextWriterEndElement(writer);
	
	for (int i=0; i<args->arg_count; i++) {
		if (ntargs[i]!=NULL) free(ntargs[i]);
	}
	free(ntargs);
}

char *xql_agg(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error)
{
	
	xmlTextWriterPtr writer;
	xmlBufferPtr buffer;
	
	writer = ((xmlWriterBuffer*)initid->ptr)->writer;
	buffer = ((xmlWriterBuffer*)initid->ptr)->buffer;

	xmlTextWriterFlush(writer);

	// Aggregated empty strings should return an empty string, but now return NULL instead
	if ((unsigned long)((xmlWriterBuffer*)initid->ptr)->buffer->use == 0) {
		*is_null = 1;
		return 0;
	}

	if (((xmlWriterBuffer*)initid->ptr)->add_root == -2) {
		xmlTextWriterEndElement(writer);
		xmlTextWriterFlush(writer);
	}

	*length = (unsigned long)buffer->use;
	return (char*)buffer->content;
}

/**
 * Write comment node.
 * xql_comment(string content)
 */

my_bool xql_comment_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
	xmlWriterBuffer* data;

	if (args->arg_count != 1) RETURN_ERR("xql_comment() requires 1 argument");

	data = newXmlWriterBuffer(message);
	if (data == NULL) return 1;

	args->arg_type[0] = STRING_RESULT;

	initid->ptr = (char*)data;
	return 0;
}

void xql_comment_deinit(UDF_INIT *initid)
{
	freeXmlWriterBuffer((xmlWriterBuffer*)initid->ptr);
}

char *xql_comment(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error)
{
	xmlTextWriterPtr writer;
	xmlBufferPtr buffer;
	char *arg;

	if (!args->args[0]) {
		*is_null=1;
		return 0;
	}

	writer = ((xmlWriterBuffer*)initid->ptr)->writer;
	buffer = ((xmlWriterBuffer*)initid->ptr)->buffer;
	xmlBufferEmpty(buffer);

	arg = strncpy_alloc(args->args[0], args->lengths[0]);

	xmlTextWriterWriteComment(writer, (xmlChar*)arg);
	xmlTextWriterFlush(writer);

	*length = (unsigned long)buffer->use;
	return (char*)buffer->content;
}


/**
 * Write CDATA section.
 * xql_cdata(string content)
 */

my_bool xql_cdata_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
	xmlWriterBuffer* data;

	if (args->arg_count != 1) RETURN_ERR("xql_cdata() requires 1 argument");

	data = newXmlWriterBuffer(message);
	if (data == NULL) return 1;

	args->arg_type[0] = STRING_RESULT;

	initid->ptr = (char*)data;
	return 0;
}

void xql_cdata_deinit(UDF_INIT *initid)
{
	freeXmlWriterBuffer((xmlWriterBuffer*)initid->ptr);
}

char *xql_cdata(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error)
{
	xmlTextWriterPtr writer;
	xmlBufferPtr buffer;
	char *arg;

	if (!args->args[0]) {
		*is_null=1;
		return 0;
	}

	writer = ((xmlWriterBuffer*)initid->ptr)->writer;
	buffer = ((xmlWriterBuffer*)initid->ptr)->buffer;
	xmlBufferEmpty(buffer);

	arg = strncpy_alloc(args->args[0], args->lengths[0]);

	xmlTextWriterWriteCDATA(writer, (xmlChar*)arg);
	xmlTextWriterFlush(writer);

	*length = (unsigned long)buffer->use;
	return (char*)buffer->content;
}


/**
 * Encode special chars
 * xql_encode(string content)
 */

my_bool xql_encode_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
	if (args->arg_count != 1) RETURN_ERR("xql_encode() requires 1 argument");

	args->arg_type[0] = STRING_RESULT;
	return 0;
}

void xql_encode_deinit(UDF_INIT *initid)
{
	if (initid->ptr) free(initid->ptr);
}

char *xql_encode(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error)
{
	xmlTextWriterPtr writer;
	xmlBufferPtr buffer;
	char *arg;

	if (!args->args[0]) {
		*is_null=1;
		return 0;
	}

	if (initid->ptr) free(initid->ptr);

	arg = strncpy_alloc(args->args[0], args->lengths[0]);

	initid->ptr = (char*)xmlEncodeSpecialChars(NULL, (xmlChar*)arg);
	if (!initid->ptr) {
		*error=1;
		return 0;
	}

	*length = strlen(initid->ptr);
	return initid->ptr;
}
