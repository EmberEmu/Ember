#🔥 **SQL & DBC Data Storage**

This document serves to outline Ember's usage of SQL and DBC for data storage.

## DBC

DBC, ***d***ata***b***ase ***c***lient, files are used as a storage format by the game client. 

These files contain static data that is often shared between the client and the server and would be automatically generated from a relational database. It is likely that the code used by the game to read the files was also automatically generated.

Ember uses the DBC format for storing its static data and defines many DBCs that are exclusive to the server. Definitions of for these DBCs can be found in *'src/tools/dbcparser/definitions'* and the matching DBC files can be found in the *'dbc/'* directory.

#### Code generation

To allow for rapid iteration and development, Ember uses its own DBC parser to convert XML-based DBC definitions into the C++ required to load and work with the data from the DBCs. 

The DBC parser tool is also capable of generating DBC files based on a definition. That is, when given a DBC definition, it will produce a matching DBC file that can be then opened by the server and various DBC editing tools.

For example:
```
./dbctool path/to/definitions --template CharStartBase
```

This command will take "CharStartBase.xml" from the path and output a matching CharStartBase.dbc.

For further usage examples, see the dedicated DBC parser document.

## SQL

Relational databases are used to store all of Ember's dynamic data. This includes data such as world/character state and character definitions.

## DBC to SQL migration

The aim is to move the static DBC data into a database while maintaining the code generation capabilities that allow for rapid iteration.

This will require further tooling work but the eventual switch-over should be entirely transparent to the code that makes use of the static data.

A move to SQL will allow the project to take advantage of the better tooling available for working with SQL as well as referential integrity between the dynamic and static data.