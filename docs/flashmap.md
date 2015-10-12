Flash layout.
=============

No boot loader and no OTA 512 KB flash.
---------------------------------------

	|----Start-----| = 0x0000000 (Mapped to 0x40200000)
	|              |
	|   Resident   |
	|   firmware   |
	|              |
	|~~~~~~~~~~~~~~| < 0x003c000
	|              |
	| File system  |
	|    image     |
	|              | 
	|--------------| = 0x003c000
	|              |
	|   Firmware   |
	|    config    |
	|     data     |
	|              |
	|--------------| = 0x0040000
	|              |
	| Non-resident |
	|   SDK and    |
	|   firmware   |
	|              |
	|--------------| = 0x007c000
	|              |
	|  SDK config  |
	|     data     |
	|              |
	|-----End------| = 0x0080000
