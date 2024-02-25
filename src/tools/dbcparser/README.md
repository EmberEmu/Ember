
# 🔥 **Ember DBC Parser**

## Overview

The DBC parser is primarily a code generation tool that allows for the transformation of XML definition files into C++ code that can be used by the Ember core to work with DBCs.

## Capability Overview

* Validate DBC structures
* Print information on a DBC structure
* Generate custom DBC files that can be opened using DBC editors
* Generate C++ code for reading DBCs

## DBC file structure

A collection of DBC files can be thought of as a basic, read-only, relational database where each DBC file defines a table. DBC files can contain primary keys as well as fields that act as foreign keys by referencing the primary keys within other DBC files.

All DBC files contain a header of the same format, which is followed by a given number of records. Records are essentially just structures dumped to disk.

```cpp
struct DBCHeader {
    std::uint32_t magic;
    std::uint32_t records;
    std::uint32_t fields;
    std::uint32_t record_size;
    std::uint32_t string_block_len;
};
```

## Defining a DBC

In order for Ember to interact with a DBC file, the DBC parser must be provided with a definition of a DBC in XML. Using the XML, the parser will generate code capable of loading DBC files from disk, deserialising them and resolving any references to other DBC files.

An example of two XML DBC definitions where one references to the other:

```xml
<dbc>
    <name>AreaMusic</name>

    <field>
        <type>uint32</type>
        <name>area_id</name>
    </field>

    <field>
        <name>song</name>
        <type>uint32</type>
        <key>
            <type>foreign</type>
            <parent>Music</parent>
        </key>
    </field>
</dbc>

<dbc>
    <name>Music</name>

    <field>
        <type>uint32</type>
        <name>track_id</name>
        <key>
            <type>primary</type>
        </key>
    </field>

    <field>
        <name>name</name>
        <type>string</type>
    </field>

    <enum>
        <type>int8</type>
        <name>Format</name>
        <options>
            <option name="ogg"  value="0x00" />
            <option name="mp3"  value="0x01" />
            <option name="flac" value="0x02" />
        </options>
    </enum>

    <field>
        <type>Format</type>
        <name>audio_format</name>
    </field>
</dbc>
```

In the above example, DBC tool would produce code that linked AreaMusic's 'song' field to the 'Music' DBC definition.

The generated DBC interfaces are straightforward to use:

```cpp
// loop over every record in AreaMusic
for(auto& i : dbcs.area_music) {
    std::cout << "Song name: " << i.song->name    // accesses 'Music' DBC record
              << " plays in area #" << i.area_id;
}
```

Unfortunately, DBC foreign keys are not required to point to valid records in other DBC files, so it's prudent to [do a `nullptr` check before dereferencing](https://www.youtube.com/watch?v=bLHL75H_VEM).

### Supported types

The parser supports the following types:

* `dbc`
* `struct`
* `enum`
* `field`, must be of underlying type...
      * int8, uint8, int16, uint16, int32, uint32, bool, bool32, string_ref, string_ref_loc, float, double or a user-defined struct

Additionally, DBCs may be given an alias with `<alias>example_alias</alias>`.

## Usage

To generate code, the tool must be provided with an input path, an output path and the `--disk` switch to request that it generates code capable of loading DBCs from disk. The input path may reference one or more DBC definitions and/or directories. When providing a directory, all XML files will be used as input.

An example invokation of the tool that reads all DBC definitions from two directory paths and outputs code to the current directory:

```dbc-parser -o . -d dbcs/definitions/server dbcs/definition/client --disk```

For a full list of options, specify `-h`.
