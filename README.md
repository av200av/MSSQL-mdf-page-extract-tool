# MSSQL-mdf-page-extract-tool
Free open‑source MSSQL MDF page extractor. Scan raw file/disk image, extract SQL Server data blocks by page‑header signature rules.
# MSSQL MDF Page Extract Tool

Free open‑source C++ utility for extracting valid SQL Server data pages from MDF files, raw disk partitions or disk images.

This tool iterates over input in fixed‑size blocks (default 8192‑byte SQL Server page size).
Each block is validated against user‑configurable byte‑signature rules.
Blocks matching all check rules are written sequentially to output file.

> This is a lightweight forensic helper tool, not a full database recovery solution.

## Features
- Scan MDF file, raw disk dump or disk image
- Configurable block size (default 8192 for SQL Server)
- Custom signature check rules `offset=hexbytes`
- Default rules for standard SQL Server data‑page header
- Output only blocks that pass all signature checks
- Pure read‑only scan; input file will not be modified
- Compile‑from‑source C++ single‑file implementation

## Source file
- [`mdf_extract.cpp`](mdf_extract.cpp)

## Build
### Linux / MinGW
```bash
g++ mdf_extract.cpp -o mdf_extract -std=c++11
