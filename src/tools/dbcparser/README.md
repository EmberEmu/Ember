# 🔥 **Ember DBC Tool**
---

# Overview

The DBC tool is a basic code generation utility that allows for the transformation of XML definition files into C++ code that can be used by Ember to parse binary DBC files.

# DBC file structure

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

# Defining a DBC

In order for Ember to interact with a DBC file, the DBC tool must be used to produce code. The produced code will be responsible for loading DBC files from disk, deserialising them and linking any 'foreign keys' up to their referenced primary keys.

An example of DBC definitions in XML:
```xml
<struct>
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
</struct>

<struct>
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
</struct>
```

In the above example, DBC tool would produce code that linked AreaMusic's 'song' field to the 'Music' DBC definition. 

The generated DBC interfaces, intended for use by developers, are straightforward to use:

```cpp
// loop over every record in AreaMusic
for(auto& i : dbcs.area_music) {
    std::cout << "Song name: " << i.song->name    // accesses 'Music' DBC record
              << " plays in area #" << i.area_id;
}
```

Unfortunately, DBC foreign keys are not required to point to valid records in other DBC files, so it's prudent to do a nullptr check before attempting to dereference them.

# Generating code
todo
