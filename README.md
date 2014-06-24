SerialBasic-Library
===================

The following files should be included with this readme:

	-SerialBasicDocumentation.pdfA 	PDF file that contains the documentation for the SerialBasic class
	-SerialBasic.h                  The header file that contains both the template declaration and sources of the SerialBasic class
  	-COPYING.txt                    Licensing information required by boost
  
The SerialBasic class is developed to write data over a computer's serial port, specifically for a computer running a Windows operating
system (OS). Though, the source code can be adapted to support any OS for which boost libraries are available. 

SerialBasic was originally written for the Autonomous Lawnmower Project (ALP), for which a wireless channel between a laptop computer and 
a robotic vehicle was necessary. The devices utilized for creating the wireless channel required serial input. Thus, SerialBasic was 
developed such that structure types can be transmitted over the wireless channel, as opposed to only unsigned characters. More 
information can be in SerialBasicDocumentation.pdf. More information on ALP is found on the Project website.

Name:		    	Andrew Powell
Contact Email:          andrew.powell@temple.edu
Project Website:        www.powellsshowcase.bugs3.com.
