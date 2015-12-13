WebSocket implementation.
=========================

Setup and handshake.
--------------------

* Register websocket handlers.
* Register `http_websocket_handler` with slighttp. 

* Handshake from client comes through the HTTP handler.
  * Check handshake is compatible and valid.
  * Assemble response (protocol, key).
  * Register new TCP sent handler for connection.
  * Send response.
* Sent handler called.
  * Free request data.
  * Free unused TCP data.
  * Find protocol and save in the `user` memeber of the `tcp_connection` struct.
  * Set real websocket TCP handlers.

WebSocket frame recieved.
-------------------------

* Parse frame data.
* Determine handler from protocol and opcode.
* Call handler recieve callback.

