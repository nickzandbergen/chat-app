package main

import (
	"fmt"
	"net"
	"os"
)

const exitFailure = 1

func main() {
	var args []string = os.Args

	if len(args) < 2 {
		fmt.Println("usage: go run chatserver.go <port number>")
		os.Exit(exitFailure) // error
	}

	// change from local address sometime?
	ln, err := net.Listen("tcp", "127.0.0.1:"+args[1])
	if err != nil {
		// handle error
		fmt.Printf("Err: %v\n", err)
		os.Exit(exitFailure)
	}
	fmt.Printf("Running on %v\n", args[1])

	for {
		conn, err := ln.Accept()
		if err != nil {
			// handle error
		}
		go handleConnection(conn)
	}

}

func handleConnection(c net.Conn) {
	fmt.Println("Got connection!")

	// buffer := make([]byte, 128)
	b := make([]byte, 1)

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

	rv, err := c.Read(b)
	if err != nil {
		fmt.Printf("Err: %v\n", err)
	}
	fmt.Printf("\"%v\"\n", string(b))

	return
}
