package main

import "fmt"
import "os/exec"

func setup_network() {
    fmt.Println("Setting up the network...")
    out, err := exec.Command("bash","../mgmt.sh").Output()
    fmt.Printf("Output:\n%s\nErrors:\n%s\n", out, err)
    fmt.Println("done.")
}

func main() {
    setup_network()
}