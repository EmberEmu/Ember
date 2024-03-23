# 🔥 **MPQ Extraction Test Utility**
---

This tool only exists to test Ember's MPQ handling library, intended for future ancillary services.

## Usage examples:
### Extract all contents from `file.mpq`

`mpqextract file.mpq` 

### Extract only JPG and GIF files from `file.mpq`

`mpqextract file.mpq "([a-zA-Z0-9\s_\\.\-\(\):])+(.jpg|.gif)"`
