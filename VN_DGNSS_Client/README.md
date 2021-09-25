The executable file for Linux exists in the 'bin' folder
Example command:
```
sudo ./WADGNSS_Client_Linux /dev/ttyACM0 1 192.168.1.107 3636 010101
```
The executable file for Windows exists in the 'app' folder.
You would have to input arguments for com port, option, Server host, port in Run_Client.bat file.
Then double Run_Client.bat to run VN_DGNSS_Client.exe.

The source code build with CMake. If you want to compile the source code in Windows, for instance, in VS 2019, please open 'VN_DGNSS_Client
' by CMake
