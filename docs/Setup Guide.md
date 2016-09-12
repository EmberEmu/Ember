EMBER DOCUMENTATION: INSTALLATION PROCESS
-----------------------------------------

PRE-REQUIREMENTS:

LIBRARIES
http://www.filedropper.com/botan-1108
http://www.filedropper.com/flatbuffers
http://www.filedropper.com/pcre-839
http://www.filedropper.com/zlib

MYSQL CONNECTOR/c++ LIBRARY (Windows x86, 32bit - ZIP archive)
http://dev.mysql.com/downloads/connector/cpp/

CMake (Installer)
https://cmake.org/download/

BOOST (1.61 or later)
http://www.boost.org/

VISUAL STUDIO (2015 or later)
https://www.visualstudio.com/en-us/downloads/download-visual-studio-vs.aspx

WINMPQ
http://sfsrealm.hopto.org/downloads/WinMPQ.html

EMBER DB
http://www.filedropper.com/dbc

-------------------------------

STEP 1: GETTING READY

Download everything.

Unpack the contents of all the library archives into a dedicated folder and install CMake as well as boost.

-------------------------------

STEP 2: COMPILING VS SOLUTION

Launch CMake GUI

Press 'Browse Source...' and select your dedicated Ember source folder (root)

Press 'Browse Build...' and select your dedicated build folder (default is Ember/build)

Press 'Add Entry' and enter the following;
Name: CMAKE_PREFIX_PATH
Type: PATH
Value: C:\Libs\pcre-8.39;C:\boost\boost_1_60_0;C:\Libs\Botan-1.10.8;C:\Libs\flatbuffers;C:\Libs\zlib
(adjust the folders to fit your setup)

Press 'Add entry' once more and enter the follow;
Name: BOTANROOT
Type: PATH
Value: C:/Libs/Botan-1.10.8/botan/build/

Press 'Configure' and select Visual Studio 2015 and use default native compilers. Press Finish.

[Fill in here, Chaos]

--------------------------------

STEP 3: COMPILING EMBER

Launch the VS solution from the dedicated build folder from before

Build the solution (only release available currently)

--------------------------------

STEP 4: COPYING OVER DEPENDENCIES AND CONFIG FILES

Copy in library files pcre.dll from the pcre-8.39\lib folder, mysqlcppconn.dll from the mysql-connector-c++-noinstall-1.1.7-win32\lib folder, 
and zlib.dll from the zlib\lib folder (all located in your dedicated library folder) and place them all in Ember/build/bin

Copy in config files from the Ember/configs folder and place them in Ember/build/bin

--------------------------------

STEP 5: EXTRACTING CLIENT DBC FILES

Install WinMPQ

Locate your game client and extract all .dbc files from the mpq files mentioned below (located in World of Warcraft/Data, and place them all in Ember/build/bin/dbcs:
dbc.MPQ
patch.MPQ
patch-2.MPQ

--------------------------------

STEP 6: SETTING UP EMBER DB

Run create.sql from the Ember/sql/mysql folder (if you merge my PR :3) 

Run full.sql from the Ember/sql/mysql folder

--------------------------------

STEP 7: CREATING AN ACCOUNT

[Fill in here, Chaos]

....

also fill in with aditional steps..
