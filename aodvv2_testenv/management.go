package main

import (
    "bufio"
    "fmt"
    "net"
    "os"
    "os/exec"
    "regexp"
    "sort"
    "strings"
    "strconv"
)

/* All channels onto which the output of a RIOT node is sorted */
type stream_channels struct {
    json  chan string
    other chan string
}

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

/* load port numbers, sorted by position, from the ports.info file behind path into info. */
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

/* Sort every line which arrives through reader onto one of the channels from
 * s, depending on the line's content. */
func (s stream_channels) sort_stream(reader *bufio.Reader) {
    fmt.Println("sort_stream started...")

    for {
        str, err := reader.ReadString('\n')
        check(err)

        if len(str)>0 {
            if strings.HasPrefix(str, ">") {
                /* end of a shell command execution, create clean newline */
                s.other <- ">\n"
                /* remove > from str*/
                str = str[1:]
            }

            /* If there's something left, check line content and sort */
            if (len(str) > 0) {
                if strings.HasPrefix(str, "{") {
                    /* this line contains a JSON */
                    s.json <- str
                } else {
                    /* this line contains something else */
                    s.other <- str
                }
            }
        }
    }
}

func crank_this_mofo_up(index int, port int) {
    fmt.Printf("HELLO THIS IS %d SPEAKING\n", index)

    conn, err := net.Dial("tcp", fmt.Sprint("localhost:",port))
    check(err)

    /*create channels*/
    json_chan  := make(chan string)
    other_chan := make(chan string)
    channels   := stream_channels{json: json_chan, other: other_chan}

    /*sort that stuff out*/
    connbuf := bufio.NewReader(conn)
    go channels.sort_stream(connbuf)

    conn.Write([]byte("ifconfig\n"))

    /* find my IP address in the output */
    for {
        str := <- other_chan
        r, _ := regexp.Compile("inet6 addr: (fe80(.*))/.*scope: local")
        match := r.FindAllStringSubmatch(str, -1)

        if len(match) >0 {
            riot_line[index].ip = match[0][1]
            break
        }
    }
}

func connect_to_RIOTs() {
    done := make(chan bool, 1)
    riot_line = load_position_port_info_line(desvirt_path)

    for index, elem := range riot_line {
        go crank_this_mofo_up(index, elem.port)
    }

    <-done
}

func main() {
    //setup_network()
    foo:="asdf"
    fmt.Println(foo[1:])
    connect_to_RIOTs()
}