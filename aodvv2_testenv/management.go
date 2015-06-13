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
    "sync"
)

/* content_type */
const (
    CONTENT_TYPE_JSON = iota
    CONTENT_TYPE_OTHER = iota
)

/* All channels for communication of a RIOT node */
type stream_channels struct {
    snd       chan string /* Send commands to the node */
    rcv_json  chan string /* Receive JSONs */
    rcv_other chan string /* Receive other stuff */
}

type riot_info struct {
    port int
    ip   string
    channels stream_channels
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

/* Figure out the type of the content of a string */
func get_content_type(str string) int {
    if strings.HasPrefix(str, "{") {
        return CONTENT_TYPE_JSON
    }
    return CONTENT_TYPE_OTHER
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
    for {
        str, err := reader.ReadString('\n')
        check(err)

        if len(str)>0 {
            if strings.HasPrefix(str, ">") {
                /* end of a shell command execution, create clean newline */
                s.rcv_other <- ">\n"
                /* remove > from str*/
                str = str[1:]
            }

            /* If there's something left, check line content and sort */
            if (len(str) > 0) {
                switch (get_content_type(str)){
                case CONTENT_TYPE_JSON:
                    /* this line contains a JSON */
                    s.rcv_json <- str
                case CONTENT_TYPE_OTHER:
                    /* this line contains something else */
                    s.rcv_other <- str
                }
            }
        }
    }
}

/* Goroutine at place index in the line which takes care of the RIOT behind port */
func crank_this_mofo_up(index int, port int, wg *sync.WaitGroup) {
    conn, err := net.Dial("tcp", fmt.Sprint("localhost:",port))
    check(err)

    /* create channels and add them to the info about this thread stored in riot_line*/
    send_chan  := make(chan string) /* messages from the main routine */
    json_chan  := make(chan string) /* JSON messages from the RIOT */
    other_chan := make(chan string) /* other messages from the RIOT */
    channels   := stream_channels{rcv_json: json_chan, rcv_other: other_chan, snd: send_chan}
    riot_line[index].channels = channels

    /*sort that stuff out*/
    connbuf := bufio.NewReader(conn)
    go channels.sort_stream(connbuf)

    conn.Write([]byte("ifconfig\n"))

    fmt.Println(port,"/",index,": getting my IP...")
    /* find my IP address in the output */
    for {
        str := <- other_chan
        r, _ := regexp.Compile("inet6 addr: (fe80(.*))/.*scope: local")
        match := r.FindAllStringSubmatch(str, -1)

        if len(match) >0 {
            riot_line[index].ip = match[0][1]
            fmt.Println(port,"/",index,": my IP is", match[0][1])
            break
        }
    }

    /* Signal to main that we're ready to go */
    (*wg).Done()

    /* Read and handle any input from the outside (i.e. main)
     * (just assume every message from main is a command for now) */
    for {
        message := <- send_chan
        if !strings.HasSuffix(message, "\n") {
            /* make sure command ends with a newline */
            fmt.Sprint(message, "\n")
        }

        conn.Write([]byte(message))
    }
}

/* Send a command to the RIOT behind stream_channels. */
func (s stream_channels) send (command string) {
    s.snd <- command
}

/* Look for string matching exp in the channels (TODO: actually use JSON) */
func (s stream_channels) expect_JSON (exp string) {
    for {
        content := <- s.rcv_json
        /* TODO: use JSON parser. */
        if content == exp {
            fmt.Println(exp)
            return
        }
    }
}

/* Look for string matching exp in the channels (TODO: use regex) */
func (s stream_channels) expect_other (exp string) {
    for {
        content := <- s.rcv_other
        /* TODO: use JSON parser. */
        if content == exp {
            fmt.Println(exp)
            return
        }
    }
}

func connect_to_RIOTs() {
    riot_line = load_position_port_info_line(desvirt_path)

    var wg sync.WaitGroup
    wg.Add(len(riot_line))

    for index, elem := range riot_line {
        go crank_this_mofo_up(index, elem.port, &wg)
    }

    /* wait for all goroutines to finish setup before we go on */
    wg.Wait()
}

func start_experiments() {
    fmt.Println("starting experiments...")
    beginning := riot_line[0]
    beginning.channels.send("hlp\n")
    beginning.channels.expect_other("hlp\n")
}

func main() {
    //setup_network()
    connect_to_RIOTs()
    start_experiments()
}