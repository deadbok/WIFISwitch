HTTP flow.
==========

Handlers work like filters, they can pass the data on to the next
handler in the list, as long as no reply data has been sent.

Parse request (receive callback).
---------------------------------

Parsing the request happens in the receive callback.

* Parse the request type.
* Copy headers.
* Copy the message


Find a handler (receive callback).
----------------------------------

The first handler is found in the receive callback.

* Search the list of handlers from the top and use the first available.
* Loop.
  * Call handler.
  * If return is RESPONSE_DONE_FINAL, exit.
  * If return is RESPONSE_DONE_CONTINUE, find next handler.
  * If return is positive, data has been sent, exit and leave it to the
    next sent callback.


Create response (receive callback -> handler).
----------------------------------------------

* Check URI.
* Check method.
* Parse the headers.
* Send status.
* Send headers.
* Send message.
   * If there is data
     * Fill buffer.
     * Return data size.
   * If no there is no data and this was the final response.
     * Return RESPONSE_DONE_FINAL.
   * If no there is no data and another handler is allowed to take over.
     * Return RESPONSE_DONE_CONTINUE.


Sent callback.
--------------

When the data has been sent we get here.

* Call handler.
* If return is RESPONSE_DONE_FINAL, exit.
* If return is RESPONSE_DONE_CONTINUE.
  * Find next handler.
  * Call handler.
* If return is positive, data has been sent, exit and leave it to the
  next sent callback.
