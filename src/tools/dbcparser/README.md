
# 🔥 **Ember DBC Parser**

## Overview

The DBC parser is primarily a code generation tool that allows for the transformation of XML definition files into C++ and SQL queries that can be used by the core to work with DBC data in an intuitive fashion.

## Capability Overview

* Validate DBC structures
* Print information on a DBC structure
* Generate custom DBC files that can be opened using DBC editors
* Generate C++ code for reading DBCs
* Generate SQL tables and population queries

## DBC Structure

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

## Usage examples

For a full list of options, specify `-h`.

### Code generation

To generate code, the tool must be provided with an input path, an output path and the `--disk` switch to request that it generates code capable of loading DBCs from disk. The input path may reference one or more DBC definitions and/or directories. When providing a directory, all XML files will be used as input.

An example invokation of the tool that reads all DBC definitions from two directory paths and outputs code to the current directory:

```dbc-parser -o . -d dbcs/definitions/server dbcs/definition/client --disk```

### Print DBC overview

Prints out all an overview of all loaded DBC definitions.

```dbc-parser -d dbcs/definitions/client --print-dbcs```


Output example:

```
+-----------------------------------------------------------------------------+
|                  DBC Name|   #|                                      Comment|
+-----------------------------------------------------------------------------+
|                 AddonData|   8|                                             |
|             CharStartBase|   4|                                             |
|           CharStartSkills|   4|                                             |
|           CharStartSpells|   4|                                             |
|            CharStartZones|   4|                                             |
|           ItemDefinitions|   0|                                             |
|       StartItemQuantities|   2|                                             |
+-----------------------------------------------------------------------------+
```

## Print DBC fields

Prints out the structure for every loaded DBC definition.

```dbc-parser -d dbcs/definitions/server --print-dbcs```

Output example:

```
AreaTable
+-----------------------------------------------------------------------------+
|                           Field|              Type| Key|             Comment|
+-----------------------------------------------------------------------------+
|                              id|            uint32|   p|                    |
|                             map|            uint32|   f|                    |
|               parent_area_table|            uint32|   f|                    |
|                        area_bit|             int32|    |                    |
|                           flags|         AreaFlags|    |                    |
|               sound_preferences|            uint32|   f|                    |
|    sound_preferences_underwater|            uint32|   f|                    |
|                  sound_ambience|            uint32|   f|                    |
|                      zone_music|            uint32|   f|                    |
|                zone_music_intro|            uint32|   f|                    |
|               exploration_level|             int32|    |                    |
|                       area_name|    string_ref_loc|    |                    |
|                   faction_group|            uint32|   f|                    |
|                     liquid_type|            uint32|   f|                    |
|                   min_elevation|             int32|    |                    |
|              ambient_multiplier|             float|    |                    |
|                           light|            uint32|   f|                    |
+-----------------------------------------------------------------------------+
```

## Generate SQL tables & queries

To generate SQL tables *only*:

```dbc-parser -d dbcs/definitions/server --sql-schema```

Note: Foreign keys are not currently generated

Output example:

```sql
CREATE TABLE `area_table` (
  `id` int unsigned NOT NULL, 
  PRIMARY KEY(`id`),
  `map` int unsigned NOT NULL COMMENT 'References Map',
  `parent_area_table` int unsigned NOT NULL COMMENT 'References AreaTable',
  `area_bit` int NOT NULL,
  `flags` int NOT NULL,
  `sound_preferences` int unsigned NOT NULL COMMENT 'References SoundProviderPreferences',
  `sound_preferences_underwater` int unsigned NOT NULL COMMENT 'References SoundProviderPreferences',
  `sound_ambience` int unsigned NOT NULL COMMENT 'References SoundAmbience',
  `zone_music` int unsigned NOT NULL COMMENT 'References ZoneMusic',
  `zone_music_intro` int unsigned NOT NULL COMMENT 'References ZoneIntroMusicTable',
  `exploration_level` int NOT NULL,
  `area_name_en_gb` mediumtext NOT NULL,
  `area_name_ko_kr` mediumtext NOT NULL,
  `area_name_fr_fr` mediumtext NOT NULL,
  `area_name_de_de` mediumtext NOT NULL,
  `area_name_en_cn` mediumtext NOT NULL,
  `area_name_en_tw` mediumtext NOT NULL,
  `area_name_es_es` mediumtext NOT NULL,
  `area_name_es_mx` mediumtext NOT NULL,
  `area_name_flags` int NOT NULL,
  `faction_group` int unsigned NOT NULL COMMENT 'References FactionGroup',
  `liquid_type` int unsigned NOT NULL COMMENT 'References LiquidType',
  `min_elevation` int NOT NULL,
  `ambient_multiplier` float NOT NULL,
  `light` int unsigned NOT NULL COMMENT 'References Light'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

To generate insertion queries:

````dbc-parser -d dbcs/definitions/server --sql-queries````

Note: The tool will chunk insertions into a maximum of 100 rows per statement to avoid potential issues with large queries timing out or being rejected by an SQL server's configuration. That is to say, a DBC with 1000 rows will generate 10 insertion queries with 100 rows each.

## Generate blank DBC

To generate an 'empty' DBC file from a definitions directory:

````dbc-parser -d dbcs/definitions/server --dbc-gen````

Note: Files will be generated with one dummy row inserted. This is due to some tested DBC editors incorrectly handling empty DBC files.