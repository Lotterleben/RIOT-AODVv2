package main

import (
    "bufio"
    "fmt"
    "os"
    "os/exec"
    "sort"
    "strings"
    "strconv"
)

type riot_info struct {
    port int
    ip   string
}

const MAX_LINE_LEN = 10

const desvirt_path = "../../riot/desvirt_mehlis/ports.list"
var riot_line []riot_info /* the key is the position */

func check(e error) {
    if e != nil {
        fmt.Println("OMG EVERYBODY PANIC")
        panic(e)
    }
}

/* Set up the network. This will be switched to our own abstraction (hopefully soon). */
func setup_network() {
    fmt.Println("Setting up the network (this may take some seconds)...")
    out, err := exec.Command("bash","../mgmt.sh").Output()
    fmt.Printf("Output:\n%s\nErrors:\n%s\n", out, err)
    fmt.Println("done.")
}

func load_position_port_info_line(path string) (info []riot_info) {
    var keys []int
    /* temporarily store info under wonky position numbers such as -47, -48, ... */
    tmp_map := make(map[int]riot_info)

    file, err := os.Open(path)
    check(err)
    defer file.Close()

    scanner := bufio.NewScanner(file)
    for scanner.Scan() {
        pieces := strings.Split(scanner.Text(), ",")
        if len(pieces) != 2 {
            panic(fmt.Sprintf("Problematic line in ports.list: %s\n", scanner.Text()))
        }

        index_, err := strconv.Atoi(pieces[0])
        check(err)
        port_, err := strconv.Atoi(pieces[1])
        check(err)

        tmp_map[index_] = riot_info{port: port_}
    }

     /* Make sure everything went fine */
    if err := scanner.Err(); err != nil {
        check(err)
    }

    /* Since ports.list entries aren't sorted or with sensible position
     * values, we'll have to rearrange and tweak them a bit */

    /* first, get and sort the indices (i.e. all the index_ values) */
    for key := range tmp_map {
        keys = append(keys, key)
    }
    sort.Ints(keys)

    /* Then, according to the order we just created, add the value associated with
     * each key to riot_info */
    for i := range keys {
        port_ := tmp_map[keys[i]]
        info = append(info, port_)
    }

    return info
}

func connect_to_RIOTs() {
    riot_line = load_position_port_info_line(desvirt_path)
}

func main() {
    //setup_network()
    connect_to_RIOTs()
}