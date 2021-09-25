## Arguments reqiurements:
### 1. com port
### 2. receiver option: 
The client has predesigned configuration for ublox M8P and ZED-F9P. 

Value 0 for others receivers, 1 for M8P, 2 for ZED-F9P. 

If you use orther receivers, you can customize the configuration commands in src/utility.h. Or you may setup the receiver by manufacture software. 

The recommanded configuration are following: Set elevation cutoff for 15 degree; 

The following configurations are required: Enable GPS/GALILEO/BeiDou; Enable RTCM input througn USB; Enable NMEA protocol $GxGNS message. 

### 3. Server host IP, server port

### 4. System identifier code
This configuration is to send request to server for receiving different observation types.
Note that the availability of observation type is determined by the SSR stream selected on the VN_DGNSS server.
The code is formated by GPS+GALILEO+BeiDou, the default is "010101".

<img width="293" alt="obs_type" src="https://user-images.githubusercontent.com/45580484/134788618-5198a648-8774-4540-a4a9-6e87ce5e7a79.png">

## Client for Linux
The executable file for Linux exists in the 'bin' folder
Example command:
```
sudo ./VN_DGNSS_Client /dev/ttyACM0 1 192.168.1.107 3636 010101
```

## Client for Windows
The executable file for Windows exists in the 'app' folder.
You may have to input arguments for com port, option, Server host, port, and System identifier code in Run_Client.bat file.
Then double Run_Client.bat to run VN_DGNSS_Client.exe.

The source code build with CMake. If you want to compile the source code in Windows, for instance, in VS 2019, please open 'VN_DGNSS_Client
' by CMake
