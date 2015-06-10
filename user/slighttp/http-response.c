/** @file http-response.c
 *
 * @brief Dealing with responses for the HTTP server.
 * 
 * @copyright
 * Copyright 2015 Martin Bo Kristensen Grønholdt <oblivion@@ace2>
 * 
 * @license
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
#include "tools/missing_dec.h"
#include "c_types.h"
#include "osapi.h"
#include "user_config.h"
#include "tools/strxtra.h"
#include "net/tcp.h"
#include "fs/fs.h"
#include "http-common.h"
#include "http-mime.h"
#include "http.h"
#include "http-response.h"

/**
 * @brief Number of headers that #http_generate_response response will create.
 */
#define HTTP_N_RESPONSE_HEADERS 4

/**
 * @brief 200 status-line.
 */
char *http_status_line_200 = HTTP_STATUS_200;
/**
 * @brief 400 status-line.
 */
char *http_status_line_400 = HTTP_STATUS_400;
/**
 * @brief 404 status-line.
 */
char *http_status_line_404 = HTTP_STATUS_404;
/**
 * @brief 501 status-line.
 */
char *http_status_line_501 = HTTP_STATUS_501;

/**
 * @brief Server header.
 */
struct http_header http_hdr_server = HTTP_HDR_SERVER;
/**
 * @brief Connection header.
 */
struct http_header http_hdr_connection_close = HTTP_HDR_CONNECTION_CLOSE;

/**
 * @brief Send a HTTP response.
 * 
 * Send a HTTP response, and close the connection, if the headers says
 * so.
 * 
 * @param connection Connection to use when sending.
 * @param response Pointer to a structure with the response data.
 * @param send_content If `false` only send headers.
  */
void ICACHE_FLASH_ATTR http_send_response(struct http_response *response, 
                                     bool send_content)
{
	struct tcp_connection *connection = response->connection;
	char status_code[4] = "000\0";
	char *s_status_code;
	unsigned short i;
	bool close = false;
	size_t msg_size = os_strlen(response->message);
	
	//Probably would be better to put the whole thing in a buffer, and
	//send it at once.
	
	debug("Sending HTTP response %p.\n", response);
	debug(" Status-code: %d.\n", response->status_code);
	debug(" Status-line: %s.\n", response->status_line);
	debug(" Headers: %p.\n", response->headers);
	debug(" Number of headers: %d.\n", response->n_headers);
	debug(" Message (%d bytes): %p.\n", msg_size, response->message);
	
    //Send the status-line.
    tcp_send(connection, response->status_line, os_strlen(response->status_line));
    tcp_send(connection, "\r\n", 2);
    //Send headers.
    for (i = 0; i < response->n_headers; i++)
    {
		if (response->headers[i].name && response->headers[i].value)
		{
			debug(" Header pointers: %p, %p.\n", response->headers[i].name, response->headers[i].value);
			tcp_send(connection, response->headers[i].name, os_strlen(response->headers[i].name));
			tcp_send(connection, ": ", 2);
			tcp_send(connection, response->headers[i].value, os_strlen(response->headers[i].value));
			//Check to see if we can close the connection.
			if (os_strncmp(response->headers[i].name, "Connection", 10) == 0)
			{
				if (os_strncmp(response->headers[i].value, "close", 5) == 0)
				{
					debug("Closing connection when done.\n");
					close = true;
				}
			}
		}
		tcp_send(connection, "\r\n", 2);
	}
    tcp_send(connection, "\r\n", 2);
    //Send content.
    if (send_content)
    {
		if (response->message)
		{
			msg_size = os_strlen(response->message);
			tcp_send(connection, response->message, msg_size);
			
		}
    }
    //Find status code and skip spaces.
    for (s_status_code = os_strstr(response->status_line, " "); *s_status_code == ' '; s_status_code++);
    if (s_status_code)
    {
        os_memcpy(status_code, s_status_code, 3);
        print_clf_status(connection, status_code, msg_size);
    }
    else
    {
        print_clf_status(connection, "-", msg_size);
    }
    if (close)
    {
        tcp_disconnect(connection);
    }
}

/**
 * @brief Generate a HTTP response.
 * 
 * @param connection Pointer to the connection used.
 * @param status_code Status code of the response.
 * @return A pointer to a #http_response struct.
 */
struct http_response ICACHE_FLASH_ATTR *http_generate_response(
									struct tcp_connection *connection,
									unsigned short status_code)
{
    struct http_request *request = connection->free;
	struct http_response *response;
    unsigned short  i;
    FS_FILE_H file;
    long data_size = 0;
    char *message = NULL;
    char *buffer = NULL;
    char *uri = NULL;
    char *fs_uri = NULL;
    size_t uri_size, doc_root_size, ext_size;
    bool found = false;
    
    //Get memory for response.
    response = db_malloc(sizeof(struct http_response), "response http_generate_response");
    debug("Generate HTTP response %p.\n", response);
    //Set initial values for the response.
    response->connection = connection;
    response->status_code = status_code;
    response->status_line = NULL;
    response->headers = db_malloc(sizeof(struct http_header) * HTTP_N_RESPONSE_HEADERS, "response->headers  http_generate_response");
    response->headers[0].name = "Content-Type";
    response->headers[0].value = http_mime_types[1].type;
    response->headers[1] = http_hdr_connection_close;
    response->headers[2] = http_hdr_server;
    response->headers[3].name = "Content-Length";
    response->headers[3].value = NULL;
    response->n_headers = HTTP_N_RESPONSE_HEADERS;
    response->message = NULL;
    
    //If there is an error, respond with an error page.
    if (status_code != 200)
    {
		uri = db_malloc(sizeof(char) * (digits(status_code) + 7), "uri  http_generate_response");
		os_sprintf(uri, "/%d.html", status_code);
		fs_uri = uri;
		debug(" HTTP error response.\n");
	}
	else
	{
		//Change an '/' uri into '/index.html'.
		//Find the last '/'.
		for (i = 0; request->uri[i] != '\0'; i++)
		{
			if (request->uri[i] == '/')
			{
				uri = &request->uri[i];
			}
		}
		//Check if it is at the end of the uri.
		uri++;
		uri_size = os_strlen(request->uri);
		if (*uri == '\0')
		{
			debug(" Adding index.html.\n");
			//The uri ends on '/', add 'index.html'.
			uri = db_malloc(uri_size + 11, "uri http_generate_response");
			os_memcpy(uri, request->uri, uri_size);
			os_memcpy(uri + uri_size, "index.html\0", 11);
		}
		else
		{
			uri = request->uri;
		}
		fs_uri = uri;
		//Add document root to file system path.
		//Check if we have a document root path.
		if (http_fs_doc_root)
		{
			debug(" Adding root %s.\n", http_fs_doc_root);
			//Document root is non emtpy, find its length.
			doc_root_size = os_strlen(http_fs_doc_root);
			uri_size = os_strlen(uri);
			//Get mem.
			fs_uri = db_malloc(uri_size + doc_root_size, 
							"fs_uri http_generate_response");
			os_memcpy(fs_uri, http_fs_doc_root, doc_root_size);
			os_memcpy(fs_uri + doc_root_size, uri, uri_size);
			fs_uri[uri_size + doc_root_size] = '\0';
		}
	}

	debug(" Final static request URI: %s.\n", uri);
	debug(" Final file system request URI: %s.\n", fs_uri);
    
    //Try to open the URI as a file.
    file = fs_open(fs_uri);
    //Check if it went okay.
    if (file > FS_EOF)
    {
		found = true;
        debug(" Found file: %s.\n", fs_uri);
        
        //Get the size of the message.
        data_size = fs_size(file);
        //Get memory for the message.
        buffer = db_malloc((int)data_size + 1, "buffer http_generate_response");
        //Read the message from the file to the buffer.
        fs_read(buffer, data_size, sizeof(char), file);
        //Zero terminate, to make sure.
        buffer[data_size] = '\0';
        fs_close(file);
        
        //Find the MIME-type
        for (i = 0; i < HTTP_N_MIME_TYPES; i++)
        {
			ext_size = os_strlen(http_mime_types[i].ext);
			if (os_strncmp(http_mime_types[i].ext, fs_uri + os_strlen(fs_uri) - ext_size, ext_size) == 0)
			{
				response->headers[0].value = http_mime_types[i].type;
				break;
			}
		}
		if (!response->headers[0].value)
		{
			response->headers[0].value = http_mime_types[1].type;
		}
    }
    
    if (!found)
    {
		//Check in static uris, go through and stop if URIs match.
		for (i = 0; 
			 ((i < n_static_uris) && (!static_uris[i].test_uri(uri))); 
			 i++);
		
		//Send response if URI is found
		if (i < n_static_uris)
		{
			found = true;
			debug(" Found static page.\n");
			//Get a pointer to the message.
			message = static_uris[i].handler(uri, request, response);
			if (message)
			{
				//Get size of the message.
				data_size = os_strlen(message);
				//Allocate memory. The message is copied to a buffer, so it can
				//always be freed.
				buffer = db_malloc((int)data_size, "buffer http_generate_response");
				//Copy the message into the buffer.
				os_memcpy(buffer, message, data_size);
			}
			//Clean up call back.
			if (static_uris[i].destroy)
			{
				static_uris[i].destroy(message);
			}
		}
	}

	//Dealloc uri mem, if it has been malloced.
	if (fs_uri != uri)
	{
		db_free(fs_uri);
	}
	if (uri != request->uri)
	{
		db_free(uri);
	}
	//If there is a response.
	if (found)
	{
		switch (status_code)
		{
			case 200: response->status_line = http_status_line_200;
					  break;
			case 400: response->status_line = http_status_line_400;
					  break;
			case 404: response->status_line = http_status_line_404;
					  break;
			case 501: response->status_line = http_status_line_501;
					  break;
			default:  warn("Unknown response code: %d.\n", status_code);
					  response->status_line = http_status_line_501;
					  break;
		}
		//debug(" Message size: %ld.\n", os_strlen(buffer));
		//Fill in the rest of the response.
        response->headers[3].value = db_malloc(sizeof(char) * digits(data_size) + 1, "response->headers[3].value http_generate_response");
        os_sprintf(response->headers[3].value, "%ld", data_size);
        //Commented out the following line, it seems to introduce a bug, where
        //the message is somehow shortened.
        //response->headers[3].value[data_size] = '\0';
        response->message = buffer;
        
        //debug(" Buffer size: %ld.\n", os_strlen(buffer));
        //debug(" Message size: %ld.\n", os_strlen(response->message));
		//debug(" Message: %s\n", response->message);
        
        return(response);
    }
	//Last escape 404 page.
	response->status_line = http_status_line_404;
    response->status_code = 404;
    //Could just do "response->headers[3].value = "#HTTP_404_HTML_LENGTH";"
    //but this makes sure we can always free it.
    response->headers[3].value = db_malloc(sizeof(char) * digits(HTTP_404_HTML_LENGTH), "response->headers[3].value http_generate_response");
    os_sprintf(response->headers[3].value, "%d", HTTP_404_HTML_LENGTH);
	
	response->message = HTTP_404_HTML;
		
    debug("Didn't find a usable page.\n");
	return(response);
}

/**
 * @brief Free data allocated by a response.
 * 
 * @param response Pointer to the response to free.
 */
void ICACHE_FLASH_ATTR http_free_response(struct http_response *response)
{
	debug("Freeing response data at %p.\n", response);
	//db_free(response->headers[3].value);
	db_free(response->headers);
	if (response->message)
	{
		db_free(response->message);
	}
	db_free(response);
}
