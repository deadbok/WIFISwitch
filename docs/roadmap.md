Roadmap.
========

|File | Scope | Item |
|:--- | :---: | :--- |
|*na* | All | Port to esp-open-rtos. |
|*na* | Network | Add NTP. |
|*na* | Network | Add OTA update. (*)|
|*na* | Network | Add client and stand-alone mode (client/AP). |
|*na* | Network, slighttp | Add WebSockets. |
|*na* | Network, slighttp | There is probably to much buffering. net layer has a send buffer. task is buffered when sending. SligHTTP does some buffering as well. |
|*na* | All | Check out the SDK/Crosstools-NG integration patches from https://github.com/cesanta/esp-open-sdk. (Malloc from SDK and such) |

(*) Probably not possible on 512Kb flash.