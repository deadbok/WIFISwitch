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
  * Find protocol and save in the `user` member of the `tcp_connection` struct.
  * Set real WebSocket TCP handlers.

WebSocket frame received.
-------------------------

* Parse frame data.
* Call handler receive or control callback set in connection->user.

WebSocket close.
----------------

* Both sides may request connection close.
* Because of limitations in the ESP8266, the connection will automatically
  time out, and close by itself.
* Situations where it is safe to close the TCP connection:
  * In the close callback, after having requested the connection close.
  * In send call back, after answering a close request from the client.
