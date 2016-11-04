def findPipePort():
    ports = list_ports.grep("Nixie Pipe")
    if len(ports) < 1:
        raise RuntimeError('No Nixie Pipe Master module detected!')
    elif len(ports) > 1:
        for x in ports:
            print(ports[x][0])
        idx = raw_input("which serial port? enter array index\n>")
        return ports[idx][0]
    else:
        return ports[0][0]
