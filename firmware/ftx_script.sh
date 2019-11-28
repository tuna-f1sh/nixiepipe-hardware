#!/bin/bash

echo 'JBR Nixie Pipe FTDI Programmer'
# ftx_unload
ftx_prog --manufacturer "JBR Engineering"
ftx_prog --product "Nixie Pipe Master"
# ftx_prog --new-serial-number JBRNP04HGFB
echo $(ftx_prog --dump) >> devices
