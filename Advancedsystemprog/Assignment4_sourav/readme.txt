Name: Sourav parmar

**********************************************************************************************************************************
Running Instructions:
Shell go to src directory /Assignment4_sourav
Kindly use the edited userapp
Sample test case at end of readme
•	Compile  : make all
•	Userapp  : make app
•	Run code: 
		• Insert Module :Default sudo insmod char_driver.ko
	        • Insert Module :Variable devices  sudo insmod char_driver.ko noOfDevices= <>
	        • Userapp : sudo ./userapp <drv_no>
	        • Remove Module : sudo rmmod char_driver
•	clean existing:  make clean 
• key 'r'- read , 'w' - write, 'c' - changedirection, 'o'- change current data access offset, 'e' -exit and close device. (keys are case sensetive)
**********************************************************************************************************************************
Source files:
• char_driver.c
• userapp.c 

Other files:
• makefile
• readme.txt  

**********************************************************************************************************************************
Implementation Details:
•	char_driver : Implements character driver
  	• Functions:
   	• mycdrv_read: Reads from driver and copies to user space buffer, driver can be read in both forward or reverse direction.
   	  reading in forward direction changes current file offset to end, so subsequent reading from currentpos will fail, however
   	  reading from begin will work as expected.
   	  To read from current offset change it to a valid one i.e within buffer boundary using key 'o' to set current offset
   	• mycdrv_write: Reads from user space bufer and copies to driver, driver can be witten in both forward or reverse direction
   	• mycdrv_ioctl: Implements handling for change of direction.
   	• mycdrv_llseek: set offset for driver based on user input at read.
   	• mycdrv_open:	Driver open handling.
   	• mycdrv_close: Driver close handling .
•	userapp: Implements user application that communicates with character driver

************************************************************************************************************************************
•	Concurrency Mechanism: Every device is associated with semaphore to prevent race condition during write,lseek and ioctl.
* 	syncronisation has been achieved using counting semaphore initialsed to one.
 
************************************* Sample Test Cases  *****************************************************************
 Write:

Write will happen from current file offset

'w' hello write from  currentpos 0 , file offset changed to 5    driverdata:hello
'w' there write from  currentpos 5 , file offset changed to 10 driverdata:hellothere

reverse write
'c' 1 :direction reverse
'w'  sou, write from current pos 10 ,file offset changed to 7 driverdata:hellotheuos


Read:
'c' 0: direction normal
'r' 1 offset 0 : read from current pos i.e 7 euos, offset is now at end i.e 11
'r' 0 offset 0 : read from beginning o/p hellotheuos
'r' 0 offset 1: read from beginning+offset o/p ellotheuos ,offset is at end
'r' 1: reading failed: since offset is at end nothing to read

reverse read
'c' 1
'r' 0: offset 0 h
'r' 0: offset 2 leh
'r' 0: offset 10 souehtolleh


