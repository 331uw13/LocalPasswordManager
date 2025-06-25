# LocalPasswordManager
Very simple local password/info manager with AES-256 encryption and Key derivation

```
lpwmanager set <entry> <file> : Write new encrypted entry to a file.
lpwmanager get <entry> <file> : Read and decrypt existing entry.
"set action will create a new file with permissions 600 if it doesnt exist.
```
