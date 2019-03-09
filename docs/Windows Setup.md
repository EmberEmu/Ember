EMBER DOCUMENTATION: WINDOWS INSTALLATION PROCESS
-------------------------------------------------

PRE-REQUIREMENTS:

TOOLCHAIN
CMake - https://cmake.org/download/ (3.10.3 or later)

Visual Studio - https://visualstudio.microsoft.com/downloads/ (2017 update 6 or later)
When installing Visual Studio make sure to select 'Desktop Development with C++'
Also make sure that you install a version of python fitting the architecture you are aiming to build ember with
You can find it under 'Individual Components' and then 'Compilers, build tools, and runtimes'
Also install support for python language under 'Development activities'
You can also download and install Python manually through https://www.python.org/

MySQL Server - https://dev.mysql.com/downloads/mysql/ (Get any version you want, really)
You need MySQL, both for compiling mysqlccpp but also for compiling and running ember.

WinMPQ - http://sfsrealm.hopto.org/downloads/WinMPQ.html - Get all of the required files;
WinMPQ or WinMPQ_VB6, 
Visual Basic 4 runtime files or Visual Basic 6 runtime library, as well as
Runtime Files Pack 3

(!OPTIONAL) - The following is only needed if you want to compile mysqlccpp yourself, for ember debug
Perl - http://strawberryperl.com/ OR https://www.activestate.com/products/activeperl/downloads/
Strawberry perl comes with both openssl and zlib pre-installed but if you wanna run ember in debug
you will need to compile zlib yourself. OpenSSL is needed for compiling MySQLConnector-C++
Alternatively you can also set up and install your own version of OpenSSL.


LIBRARIES
boost - https://sourceforge.net/projects/boost/files/boost-binaries
1.69 or later, make sure it fits your version of Visual Studio

botan - https://github.com/randombit/botan/releases
2.4.0 or later

flatbuffers - https://github.com/google/flatbuffers/releases
If you are not interested in building ember in debug you can choose to download both the source and the flatc.exe executable
That way you won't have to build flatbuffers, but instead you'll have to move the flatc.exe file into a folder called 'bin' inside flatbuffers
If you want to build ember in debug just get the source

pcre - ftp://ftp.csx.cam.ac.uk/pub/software/programming/pcre
8.39 or later - NOT pcre2!

zlib - https://github.com/madler/zlib/releases
1.2.8 or later

mysqlcppconn (Pre-compiled) - https://dev.mysql.com/downloads/connector/cpp/1.1.html (This guide presumes you got the no-install, but you can get either)
We need the pre-compiled release even if we are building the source ourselves since it has the correct file staging for include files, which we need.
Pick the source as well if you want to be able to run ember in debug
mysqlcppconn (Source) - https://github.com/mysql/mysql-connector-cpp/releases (Make sure you grab the 1.x.x and not the 8.x.x version)

(refer with CMakeList.txt for up to date toolchain and library requirements)

-------------------------------

STEP 1: GETTING READY

Download everything.

Unpack the contents of all the library archives into a dedicated folder 
(For this guide we are going to assume C:/Libs is used)

Install CMake, Boost, Python, Visual Studio and WinMPQ.
Optionally also install Perl.

With boost you need to manually add a new system environment variable with the name BOOST_ROOT pointing to your boost install location.

You can skip this if you installed Python through Visual Studio, 
otherwise Python will ask to do it for you during installation, which if you had it do that, you can skip this as well.
You can also manually add it as an entry to the PATH system environment variable pointing to your python root installation directory.

(!OPTIONALS) - The following is only needed if you want to compile mysqlccpp yourself, for debugging
With perl the system environment path variable should be set automatically during installation of Strawberry Perl.

--------------------------------

STEP 2: COMPILING AND INSTALLING BOTAN

To compile botan you must have python and visual studio installed.

Open x86/x64 Native Tools Command Prompt for VS 2017 (just search for it) with administrative rights,
and change directory to your Botan path (hint: Libs/botan)

Enter the following commands;

$ .\configure.py --debug
$ nmake
$ botan-test.exe
$ nmake install

You should now have the debug version of botan installed locally (C:/Botan)

Rename the folder lib inside C:/Botan to libd

Now open up the Native Tools Command Prompt for VS 2017 again at the same location.

Enter the following commands;

$ .\configure.py
$ nmake
$ botan-test.exe
$ name install

You should now have the release version of botan installed locally (C:/Botan)

For some reason Botan has a very strange include folder structure, so we have to do a bit of file staging
copy/move the contents of Botan/include/botan-2 and place them in Botan/include

-------------------------------

STEP 3 (OPTIONAL): COMPILING MYSQLCCPP
(Skip this part if you did not download the source files)

To compile MySQL-Connector-C++ you must have an SSL solution (OpenSSL or WolfSSL)
If you got Strawberry perl you already have OpenSSL. If you got Activeperl, maybe you don't.
In that case you'll need to compile and install OpenSSL (You'll need nasm for that)

Launch CMake GUI

Press 'Browse Source...' and select your dedicated MySQL-Connector-C++ source folder (default is Libs/mysql-connector-cpp)
Press 'Brose Build...' and select your dedicated build folder (default is Libs/mysql-connector-cpp/build)

Press 'Configure' and select Visual Studio 2017 and use default native compilers.
Press 'Generate' and then 'Open Project'

Select Debug in Visual Studio and build the solution.

move all the contents of the folder Libs/mysql-connector-cpp/Build/driver/Debug 
 and place it all in Libs/mysql-connector-c++-noinstall/libd

-------------------------------

STEP 4 (OPTIONAL): COMPILING FLATBUFFERS
(Skip this part if you downloaded flatc.exe and aren't building ember in debug)

Launch CMake GUI

Press 'Brose Source...' and select your dedicated FlatBuffers dep folder (default is Libs/flatbuffers)
Press 'Brose Build...' and select your dedicated build folder (default is Libs/flatbuffers/bin)

Press 'Configure' and select Visual Studio 2017 and use default native compilers.
Press 'Generate' and then 'Open Project'

Select release or debug in Visual Studio and build the solution.

Move the file flatc.exe from Libs/flatbuffers/bin/Release or Libs/flatbuffers/bin/Debug to Libs/flatbuffers/bin

-------------------------------

STEP 5: COMPILING ZLIB

Press 'Brose Source...' and select your dedicated zlib dep folder (default is Libs/zlib)
Press 'Brose Build...' and select your dedicated build folder (default is Libs/zlib/build)

Press 'Configure' and select Visual Studio 2017 and use default native compilers.

Set the path to LIBRARY_OUTPUT_PATH to Libs/zlib/lib

Press 'Generate' and then 'Open Project'

Select release in Visual Studio and build the solution.

Copy/move the files zconf.h from the Libs/zlib/build folder and place in Libs/zlib/include
Copy the file zlib.h from the Libs/zlib folder and place in Libs/zlib/include
Copy/move the files from the folder Libs/zlib/lib/Release and place them in Libs/zlib/lib

To build ember in debug open up the solution in Visual studio again and build the solution in debug mode this time
then copy/move the files from the folder Libs/zlib/lib/Debug and place them in Libs/zlib/lib

-------------------------------

STEP 6: COMPILING PCRE

Launch CMake GUI

Press 'Brose Source...' and select your dedicated pcre dep folder (default is Libs/pcre)
Press 'Brose Build...' and select your dedicated build folder (default is Libs/pcre/build)

Press 'Configure' and select Visual Studio 2017 and use default native compilers.
Select UTF and SUPPORT_JIT on and leave the rest at default
Press 'Configure' again, then press 'Generate' and then 'Open Project'

Select release in Visual Studio and build the solution.

Copy/move the file 'pcre.h' from the Libs/pcre/build folder and place it in Libs/pcre/include
Copy/move the files from the folder Libs/pcre/build/Release and place them in Libs/pcre/lib

To build ember in debug open up the solution in Visual studio again and build the solution in debug mode this time
then copy/move the files from the folder Libs/pcre/build/Debug and place them in Libs/pcre/lib

-------------------------------

STEP 7: GENERATING EMBER VS SOLUTION

Launch CMake GUI

Press 'Browse Source...' and select your dedicated Ember source folder (hint: Ember)
Press 'Browse Build...' and select your dedicated build folder (hint: Ember/build)

Press 'Add entry' and enter all of the following entries BEFORE configuring;

Name                      Type      Value
BOTAN_ROOT_DIR            PATH      C:/Botan
MYSQLCCPP_ROOT_DIR        PATH      C:/Libs/mysql-connector-cpp-noinstall/
FLATBUFFERS_ROOT_DIR      PATH      C:/Libs/flatbuffers
ZLIB_ROOT_DIR             PATH      C:/Libs/zlib
PCRE_ROOT_DIR             PATH      C:/Libs/pcre

OBS! Adjust the paths to fit your setup!

Press 'Configure', select your version of Visual Studio 
and the architecture you built the dependencies with,
and your prefered compilers, then press 'Generate'.

--------------------------------

STEP 8: COPYING OVER DEPENDENCIES AND CONFIG FILES

Copy in the following library files and place them in Ember/build/bin:

FOR RELEASE:
botan.dll from the Botan/lib folder
zlib.dll from the Libs/zlib/lib folder
mysqlcppconn.dll from the Libs/mysql-connector-c++/lib folder

FOR DEBUG:
bontan.dll from the Botan/libd folder
zlibd.dll from the the Libs/zlib/lib folder
mysqlccppconn.dll from the Libs/mysql-connector-c++/libd folder

FOR BOTH:
libmysql.dll from C:/Program Files/MySQL/MySQL Server 8.0/lib

Copy in config files from the Ember/build/configs folder and place them in Ember/build/bin
Remove the .dist extension so they all end in .conf

(adjust folders to fit your setup)

--------------------------------

STEP 9: COMPILING EMBER

Launch the VS solution from the dedicated build folder from before

Choose your solution (debug/release) and build.

If you are having trouble with any of the included header files when building,
then you can add them manually by right clicking on the project module in the solution explorer 
and selecting 'Properties', then 'Configuration Properties' and then 'VC++ Directories'.
If you are building both debug and release you should choose 'All Configurations' under 'Configuration',
otherwise you'll have to set them both for release and debug.
Add the path to the include folder with the header file that isn't linking
to the 'Include Directories' by selecting the dropdown menu and choosing 'Edit'.

You will have to add each include folder that doesn't link properly

You will also have to do this for each project module which uses the troublesome headers

And until they've fixed the issue with boost on visual studio you will also have to link the libraries
by selecting 'Properties', then 'Linker' and then 'Input'.
Add the library 'bcrypt.lib' by adding it to the 'Additional Dependencies', selecting the dropdown menu and choosing 'Edit'
and then insterting it as a new line at the bottom.
You will have to do this for the following sub-modules;
'account', 'character', 'gateway', 'login' & 'social'.

--------------------------------

STEP 10: EXTRACTING CLIENT DBC FILES

Locate your game client and extract all .dbc files (normally located in World of Warcraft/Data) 
from the mpq files (They'll all be in a subfolder in the .mpq called DBFilesClient) mentioned below using WinMPQ

dbc.MPQ
patch.MPQ
patch-2.MPQ

and place them all in Ember/build/bin/dbcs

Now copy all .dbc files from Ember/ebcs and place them in Ember/build/bin/dbcs

--------------------------------

STEP 11: SETTING UP EMBER DB

Run create.sql from the Ember/sql/mysql folder

Run full.sql from the Ember/sql/mysql folder on the database 'ember'
Run full.sql from the Ember/sql/mysql/login folder on the database 'emberlogin'
Run full.sql from the Ember/sql/mysql/realm folder on the database 'emberrealm' (empty atm)

--------------------------------

STEP 12: CREATING AN ACCOUNT

Run this command against the `ember` database for an account named 'Admin' with the password 'admin';
INSERT INTO `ember`.`users` (`id`, `username`, `s`, `v`, `subscriber`, `survey_request`, `pin_method`, `pin`) VALUES ('0', 'ADMIN', 'E2F18CE18E7FEAD7322CB6AD5055B62D5892545452BA2EF2961FEC66B25DD4D1', '2D494E0B39AB678B5203C7E0CEB8E0B431D45C4D4B214ADE189F214554B433CC', true, false);

