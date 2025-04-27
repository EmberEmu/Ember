# 🔥 **SRP6 Credentials Generator**
---

This is a simple tool for generating SRP6 credentials for storing in the database. Simply provide a username and password on the command line and enter the provided details into the accounts table.

For example:

```
srpgen -u MyUser -p MyPass -s
```

### Flags:

`-s` (`--sbin`) flag will place the salt in a binary file in addition to printing out the hexadecimal representation. 

`-j` (`--json`) flag will output the result as JSON.