package main

import (
	"fmt"
	"net"
	"os"
	"sync"
)

const exitFailure = 1

type clientData struct {
	addr []byte // 4 byte
	port []byte // 2 byte
	wait bool
}

var clients = make(map[string]clientData)
var m = new(sync.RWMutex)

// echo -n -e y\\x00\\x03foo | nc 127.0.0.1 5

func main() {
	var args []string = os.Args

	if len(args) < 2 {
		fmt.Println("usage: go run chatserver.go <port number>")
		os.Exit(exitFailure) // error
	}

	// change from local address sometime?
	ln, err := net.Listen("tcp", "127.0.0.1:"+args[1])
	if err != nil {
		fmt.Printf("Err: %v\n", err)
		os.Exit(exitFailure)
	}
	fmt.Printf("Running on %v\n", args[1])

	for {
		conn, err := ln.Accept()
		if err != nil {
			fmt.Printf("Err: %v\n", err)
		}
		go handleConnection(conn)
	}

}

func readLenPrefacedBuf(c net.Conn) string {
	b := make([]byte, 2)
	rv, err := c.Read(b)
	if err != nil || rv < 2 {
		fmt.Printf("Err: %v\n", err)
	}

	var len uint16 = uint16(b[0])
	len <<= 8
	len |= uint16(b[1])

	buffer := make([]byte, len)
	c.Read(buffer)
	id := string(buffer)

	fmt.Printf("%v:%v\n", id, len)

	return id
}

func getAddrData(addr net.Addr) (ip []byte, port []byte) {
	host, portString, _ := net.SplitHostPort(addr.String())
	ip = net.ParseIP(host).To4()
	port = make([]byte, 2)

	var res uint16 = 0
	for _, b := range []byte(portString) {
		res *= 10
		res += uint16(b - 48) // '0'
	}

	port[0] = byte(res >> 8)
	port[1] = byte(res & 0xFF)
	return
}

func handleConnection(c net.Conn) {
	b := make([]byte, 1)

	_, err := c.Read(b[0:1])
	if err != nil {
		fmt.Printf("Err: %v\n", err)
	}
	fmt.Printf("\"%v\"\n", string(b[0]))

	id := readLenPrefacedBuf(c)

	switch b[0] {
	case 'a': // availability
		checkUsernameAvailability(id, c)
	case 'q': //query ID for connection details
		getConnectionInfo(id, c)
	case 'r': // remove
		removeClient(id, c)
	case 'l': //list
		listClients(c)
	case 'u', 'w': // unwait
		setWaitClient(id, c, b[0] == 'u')

	case '0': // N/A

	default:
		fmt.Printf("Couldn't recognize %d\n", rune(b[0]))
	}
	c.Close()
	return
}

func checkUsernameAvailability(id string, c net.Conn) {
	m.Lock()

	_, found := clients[id]
	var b []byte = make([]byte, 1)

	if !found { //id free, can use
		// send 'y' back
		b[0] = 'y'
		ip, port := getAddrData(c.RemoteAddr())
		clients[id] = clientData{ip, port, false}
	} else {
		// send a 'n' back
		b[0] = 'n'
	}
	c.Write(b)

	m.Unlock()
	return
}

func listClients(c net.Conn) {
	m.RLock()

	i := 1
	for k := range clients {
		fmt.Fprintf(c, "%v) %v\n", i, k)
		fmt.Printf("%v) %v\n", i, k)
		i++
	}

	m.RUnlock()
	return
}

func setWaitClient(id string, c net.Conn, waitStatus bool) {
	m.Lock()

	res, found := clients[id]
	if found {
		res.wait = waitStatus
	}

	m.Unlock()
	return
}

func removeClient(id string, c net.Conn) {
	m.Lock()
	delete(clients, id)
	m.Unlock()
	return
}
func getConnectionInfo(id string, c net.Conn) {
	m.Lock()
	res, found := clients[id]
	var b []byte = make([]byte, 1)

	if found && res.wait {
		// 'y' | addr | port
		b[0] = 'y'
		c.Write(b)
	} else {
		b[0] = 'n'
		c.Write(b)
	}
	m.Unlock()

	return
}
