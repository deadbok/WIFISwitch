Task handler system.
====================

Purpose.
--------

The ESP8266 non RTOS SDK uses an event based architecture. When the
ESP8266 is not executing inside a user function or SDK call, house
keeping system code will run. The implementation has some side
effects:

 * System code needs to run at regular intervals for among others, 
   networking to keep ticking, the firmware may crash otherwise.
 * The watchdog will time out if system code is not run at regular
   intervals.
 * Some system calls can crash the system when called at the wrong
   time.
 * Some API's have callback functions to acknowledge completion, 
   invoking the system call again before properly acknowledged might
   crash the firmware.

This task handler uses the SDk task API to move the processing from
callback functions to task handlers. Callbacks queue task, that are
fired by the SDK when it believes itself to be ready.

The task handler is meant for fairly short and simple task, as an
example parsing a HTTP request and responding might look like this.

 * Parse request type.
 * Parse headers.
 * Parse any message.
 * Create response.
 * Create headers for response.
 * Send response in chunks of 1400 bytes.

All individual steps are in fact tasks, like calling functions to do the
same. The trick here is still, that by using tasks, the ESP8266 is able
to do its own thing in between each step.

Implementation.
---------------

The ESP8266 has some basic task handling functions in the non RTOS SDK:
 
 * ``bool system_os_task(os_task_t task, uint8 prio, os_event_t *queue,
   uint8 qlen)``: Assign a task handler and internal queue.
 * ``bool system_os_post(uint8 prio, os_signal_t sig, os_param_t par)``:
   Signal a task to run.
 
There are some limitations to this API, which is why this layer was
created:

 * There are currently 3 tasks available using the ``prio`` parameter
   (0/1/2).
 * The ``prio`` parameter also represents task priority, 0 being the
   lowest.
 * There is no way of knowing if the SDK uses any of these, but forgot
   to mention this.
 * The SDK UART driver in this code, uses ``prio`` 0.
 * ``par`` is an ``uint32_t`` and this is the only kind of parameter
   that we can pass to the task.

Wanted features.
----------------

 * Well defined behaviour when the task queue is full.
		* Drop new tasks.
		* Yell in the serial console.
 * Use a single SDK task, and delegate to an internal list of task
   handlers.
 * Message queue to go along with the task queue.
 * General message type to encapsulate other parameters than 
   ```uint32_t```.
   
Task mux.
---------

To accommodate more tasks than the ones provided by the SDK, the task
handler will use the ``sig`` parameters to delegate the call, to the
designated handler, much like UNIX signals.
Internally when registering a task handler, a signal ID is returned.
This integer is used to find the handler in a linked list of registered
handlers, when signalling the task later on.

Message queue.
--------------

The SDK API only support passing an unsigned 32 bit integer as
parameter to a task. Since the ESP8266 is a 32 bit architecture, this 
could be an address (pointer), to some other known type. The code uses
this fact to pass data structures to the task, but **this is not
portable**.

When a task is signalled, the parameter pointer that comes along, is 
saved in a linked list in the designated handlers structure.


