Code to convert JSON gasman configuration data to simulated gasman output. 

This code currently compiles under Linux and Windows using Visual Studio 
Linux instruction: go to CPPDLL and run the makefile
Windows: Load GasManWeb.sln, you will need to tweak the configuration to point to the correct directory


This tool creates a shared library (e.g., DLL or SO) that expects a JSON input and produces a CSV output of simulated data. Input is specified in dllmain.cpp:
jsonStr: input JSON text
len: length of jsonStr
startSecond: time to start simulation
endSecond: time to end simulation
everySeconds: how often to return the simulation output in the CSV file

Notably, the current Gasman output is in XML; however, it can be converted to JSON using the included Python script.

To test the code, there is also a Python script in the main directory for which the input file (JSON or XML) will be converted to CSV
In Windows, you also have the option of running GasManWeb in the Release directory.

Several major modifications to the code were made to reduce its footprint while still requiring libraries that can be readily included for novice programmers. This is the reason JSON was leveraged instead of XML (which would have required either libxml2 or Xerces)

TODOs:
- Create an Excel template spreadsheet and code to convert XLSX/CSV files to JSON, which can then be run through the application.
- Verify code works in the Apple/Darwin Environment
- Fix the makefile to compile on Windows

Gasman (https://www.gasmanweb.com/) is the brainchild of Dr James (Jim) H. Philip, and if you like this project, please send kudos his way. I appreciate his willingness to share his code with the world.
Special thanks to Hal Franklin for assisting in getting this project off the ground

Code released under the MIT License.

The use of included headers is governed by the MIT license, and all credit is due to their respective authors (Niels Lohmann and Brodie Thiesfield et al.).
