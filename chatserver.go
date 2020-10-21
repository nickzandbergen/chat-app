package main

import (
	"fmt"
	"net"
	"os"
	"sync"
)

const exitFailure = 1

type serverData struct {
	addr uint32
	port uint16
	wait bool
}

var clients = make(map[string]serverData)
var m = new(sync.RWMutex)

// func err(value, err) {
// 	if err != nil {
// 		fmt.Printf("Err: %v\n", err)
// 		os.Exit(exitFailure)
// 	}
// 	return value
// }

// todo use To16(IP) byte and ParseIP(string0

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

	fmt.Printf("\"%v\"\n", len)

	buffer := make([]byte, len)
	c.Read(buffer)
	id := string(buffer)

	return id
}

func handleConnection(c net.Conn) {
	fmt.Println("Got connection!")

	b := make([]byte, 3)

	// for {
	// 	rv, err := c.Read(buffer)
	// 	fmt.Printf("%v\n", rv)
	// 	if err != nil {
	// 		fmt.Printf("Err: %v\n", err)
	// 		os.Exit(exitFailure)
	// 	}

	// 	if rv == 0 {
	// 		break
	// 	}

	// 	fmt.Printf("\"%v\"\n", string(buffer))
	// }

	_, err := c.Read(b[0:1])
	if err != nil {
		fmt.Printf("Err: %v\n", err)
	}
	fmt.Printf("\"%v\"\n", string(b[0]))

	id := readLenPrefacedBuf(c)

	switch b[0] {
	case 'a': // availability
		checkAvailability(c)
	case 'q': //query ID for connection details
		queryClient(id, c)
	case 'r': // remove

	case 'l': //list

	case 'u', 'w': // unwait
		setWaitClient(id, c, b[0] == 'u')

	case '0': // N/A

	default:
		fmt.Printf("Couldn't recognize %d", rune(b[0]))
	}
	return
}

func checkAvailability(id string, c net.Conn) {
	m.Lock()

	res, found := clients[id]

	if found {
		// send 'y' back
		addr := c.RemoteAddr()
		host, port, _ := net.SplitHostPort(addr.String()
		ip := net.ParseIP(addr.String())
	} else {
		// send a 'n' back
	}

	m.Unlock()
	return
}

func listClients(c net.Conn) {
	m.RLock()

	m.RUnlock()
	return
}

func setWaitClient(id string, c net.Conn, waitStatus bool) {
	m.Lock()

	m.Unlock()
	return
}

func removeClient(id string, c net.Conn) {
	m.Lock()

	m.Unlock()
	return
}

func addClient(id string, c net.Conn) {
	m.Lock()

	m.Unlock()
	return
}

func queryClient(id string, c net.Conn) {
	m.Lock()
	res, found := clients[id]
	if found == false || !res.wait {
		// 'n'
	} else {
		// 'y' | addr | port
	}
	m.Unlock()
}
