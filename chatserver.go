package main

import (
	"fmt"
	"net"
	"os"
)	

func main() {
	var args []string = os.Args

	if len(args) < 2 {
		fmt.Println("Invald number of arguments")
	}

	ln, err := net.Listen("tcp", ":8080")
	if err != nil {
		// handle error
	}
	for {
		conn, err := ln.Accept()
		if err != nil {
			// handle error
		}
		go handleConnection(conn)
	}
	
}

func handleConnection(c Conn) {


}