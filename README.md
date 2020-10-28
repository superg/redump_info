# redump_info
Copyright 2019-2020 Hennadiy Brych
## Overview
This is a command line utility to aid redump.org submissions as well as general CD data track information tool.
Supported platforms: PSX, PC, Audio CD

## Usage
# Submission example 1 (recursive):
`redump_info submission --dat-file "Sony - PlayStation - Datfile (10399) (2020-09-23 04-18-16).dat" E:\dumps\psx`
Recursively scan "E:\dumps\psx" location for available dumps and generate !submissionInfo_<name>.txt for each valid *.cue file using the information from provided DAT file. DAT file is not required but useful to know whether it's already in the redump DB.

# Submission example 2 (from CUE):
`redump_info submission --dat-file "Sony - PlayStation - Datfile (10399) (2020-09-23 04-18-16).dat" E:\dumps\psx\007 - Tomorrow Never Dies (USA)\007 - Tomorrow Never Dies (USA).cue`
Generate "!submissionInfo_007 - Tomorrow Never Dies (USA).txt" for a single CUE file using the information from provided DAT file.

## Contacts
E-mail: gennadiy.brich@gmail.com

Discord: superg#9200
