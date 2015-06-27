package main

import (
    "bufio"
    "fmt"
    "encoding/json"
    "log"
    "net"
    "os"
    "os/exec"
    "reflect"
    "regexp"
    "sort"
    "strings"
    "strconv"
    "sync"
    "time"
)

/* content_type */
const (
    CONTENT_TYPE_JSON = iota
    CONTENT_TYPE_OTHER = iota
)

/* better safe than sorry */
const CHAN_BUF_SIZE = 500

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

func check_str(s string, e error) {
    if e != nil {
        fmt.Println("OMG EVERYBODY PANIC")
        fmt.Println("Offending string: ", s)
        panic(e)
    }
}

/* Figure out the type of the content of a string */
func get_content_type(str string) int {
    json_template := make(map[string]interface{})
    err := json.Unmarshal([]byte(str), &json_template)

    if err == nil {
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

/* Create the log directory for experiment experiment_name and return the path towards it */
func setup_logdir_path(experiment_name string) string {
    t := time.Now()
    logdir_path := fmt.Sprintf("./logs/%s_%s", t.Format("02:Jan:06_15:04_MST"), experiment_name)
    err := os.Mkdir(logdir_path, 0776)
    check(err)

    return logdir_path
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
func (s stream_channels) sort_stream(conn *net.Conn, logger *log.Logger) {
    reader := bufio.NewReader(*conn)

    for {
        str, err := reader.ReadString('\n')
        check_str(str, err)

        if len(str)>0 {
            if strings.HasPrefix(str, ">") {
                /* end of a shell command execution, create clean newline */
                s.rcv_other <- ">\n"
                /* remove > from str*/
                str = strings.TrimPrefix(str, "> ")
            }

            /* If there's something left, log, check line content and sort */
            if (len(str) > 0) {
                logger.Print(str)
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

/* Send a command to the RIOT behind stream_channels. */
func (s stream_channels) send (command string) {
    s.snd <- command
}

/* Look for string matching exp in the channels */
func (s stream_channels) expect_JSON (expected_str string) {
    expected := make(map[string]interface{})
    received := make(map[string]interface{})

    err := json.Unmarshal([]byte(expected_str), &expected)
    check(err)

    for {
        received_str := <- s.rcv_json

        err := json.Unmarshal([]byte(received_str), &received)
        //fmt.Printf("expected: %s\nreceived: %s\n", expected_str, received_str)
        check(err)

        if reflect.DeepEqual(expected, received) {
            return
        }
    }
}

/* Look for string matching exp in the channels (TODO: use regex) */
func (s stream_channels) expect_other (exp string) {
    for {
        content := <- s.rcv_other
        if content == exp {
            fmt.Println(exp)
            return
        }
    }
}

/* Goroutine at place index in the line which takes care of the RIOT behind port */
func control_riot(index int, port int, wg *sync.WaitGroup, logdir_path string) {
    logfile_path := fmt.Sprintf("%s/riot_%d_port_%d.log", logdir_path, index, port)
    logfile, err := os.Create(logfile_path)
    check(err)
    defer logfile.Close()

    logger := log.New(logfile, "", log.Lshortfile)

    conn, err := net.Dial("tcp", fmt.Sprint("localhost:",port))
    check(err)
    defer conn.Close()

    /* create channels and add them to the info about this thread stored in riot_line*/
    send_chan  := make(chan string, CHAN_BUF_SIZE) /* messages from the main routine */
    json_chan  := make(chan string, CHAN_BUF_SIZE) /* JSON messages from the RIOT */
    other_chan := make(chan string, CHAN_BUF_SIZE) /* other messages from the RIOT */
    channels   := stream_channels{rcv_json: json_chan, rcv_other: other_chan, snd: send_chan}
    riot_line[index].channels = channels

    /*sort that stuff out*/
    go channels.sort_stream(&conn, logger)

    _, err = conn.Write([]byte("ifconfig\n"))
    check(err)

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
            // make sure command ends with a newline
            message = fmt.Sprint(message, "\n")
        }

        logger.Print(message)
        _, err := conn.Write([]byte(message))
        check(err)
    }
}

/* Set up connections to all RIOTs  */
func connect_to_RIOTs(logdir_path string) {
    riot_line = load_position_port_info_line(desvirt_path)

    var wg sync.WaitGroup
    wg.Add(len(riot_line))

    for index, elem := range riot_line {
        go control_riot(index, elem.port, &wg, logdir_path)
    }

    /* wait for all goroutines to finish setup before we go on */
    wg.Wait()
}

/* create a clean slate: restart all RIOTs and set up logging identifiable by
 * experiment_id. */
func create_clean_setup(experiment_id string) {
    fmt.Println("Setting up clean RIOTs for ", experiment_id)
    setup_network()
    logdir_path := setup_logdir_path(experiment_id)
    connect_to_RIOTs(logdir_path)
    fmt.Println("Setup done.")
}

func test_route_creation_0_to_3() {
    /* route states */
    const
    (
        ROUTE_STATE_ACTIVE = iota
        ROUTE_STATE_IDLE = iota
        ROUTE_STATE_INVALID = iota
        ROUTE_STATE_TIMED = iota
    )
    const test_string = "xoxotesttest"
    const json_template_sent_rreq = "{\"log_type\": \"sent_rreq\", \"log_data\": {\"orig_addr\": \"%s\", \"targ_addr\": \"%s\", \"seqnum\": %d, \"metric\": %d}}"
    const json_template_received_rreq = "{\"log_type\": \"received_rreq\", \"log_data\":{\"last_hop\": \"%s\", \"orig_addr\": \"%s\", \"targ_addr\": \"%s\", \"orig_addr_seqnum\": %d, \"metric\": %d}}"
    const json_template_sent_rrep = "{\"log_type\": \"sent_rrep\", \"log_data\": {\"next_hop\": \"%s\",\"orig_addr\": \"%s\", \"orig_addr_seqnum\": %d, \"targ_addr\": \"%s\"}}"
    const json_template_received_rrep = "{\"log_type\": \"received_rrep\", \"log_data\":{\"last_hop\": \"%s\", \"orig_addr\": \"%s\", \"orig_addr_seqnum\": %d, \"targ_addr\": \"%s\"}}"
    const json_template_added_rt_entry = "{\"log_type\": \"added_rt_entry\", \"log_data\": {\"addr\": \"%s\", \"next_hop\": \"%s\", \"seqnum\": %d, \"metric\": %d, \"state\": %d}}"

    create_clean_setup("testtest")

    fmt.Println("Starting test...")

    beginning := riot_line[0]
    end := riot_line[len(riot_line)-1]

    beginning.channels.send(fmt.Sprintf("send %s %s\n", end.ip, test_string))
    fmt.Print(".")

    /* Discover route...  */
    expected_json := fmt.Sprintf(json_template_sent_rreq, beginning.ip, end.ip, 1, 0)
    fmt.Print(".")
    beginning.channels.expect_JSON(expected_json)
    fmt.Print(".")

    expected_json = fmt.Sprintf(json_template_received_rreq, beginning.ip, beginning.ip, end.ip, 1, 0)
    riot_line[1].channels.expect_JSON(expected_json)
    fmt.Print(".")
    expected_json = fmt.Sprintf(json_template_added_rt_entry, beginning.ip, beginning.ip, 1, 1, ROUTE_STATE_ACTIVE)
    riot_line[1].channels.expect_JSON(expected_json)
    fmt.Print(".")
    expected_json = fmt.Sprintf(json_template_sent_rreq, beginning.ip, end.ip, 1, 1)
    riot_line[1].channels.expect_JSON(expected_json)
    fmt.Print(".")

    expected_json = fmt.Sprintf(json_template_received_rreq, riot_line[1].ip, beginning.ip, end.ip, 1, 1)
    riot_line[2].channels.expect_JSON(expected_json)
    fmt.Print(".")
    expected_json = fmt.Sprintf(json_template_added_rt_entry, beginning.ip, riot_line[1].ip, 1, 2, ROUTE_STATE_ACTIVE)
    riot_line[2].channels.expect_JSON(expected_json)
    fmt.Print(".")
    expected_json = fmt.Sprintf(json_template_sent_rreq, beginning.ip, end.ip, 1, 2)
    riot_line[2].channels.expect_JSON(expected_json)
    fmt.Print(".")

    expected_json = fmt.Sprintf(json_template_received_rreq, riot_line[2].ip, beginning.ip, end.ip, 1, 2)
    end.channels.expect_JSON(expected_json)
    fmt.Print(".")
    expected_json = fmt.Sprintf(json_template_added_rt_entry, beginning.ip, riot_line[2].ip, 1, 3, ROUTE_STATE_ACTIVE)
    end.channels.expect_JSON(expected_json)
    fmt.Print(".")
    /* And send a RREP back */
    /* NOTE: added_rt_entry isn't checked on the was back yet because apparently
     * weird RREQs are sent out before the experiment, screwing up the targaddr seqnum
     * and I haven't figured out why yet. TODO FIXME */
    expected_json = fmt.Sprintf(json_template_sent_rrep, riot_line[2].ip, beginning.ip, 1, end.ip)
    end.channels.expect_JSON(expected_json)
    fmt.Print(".")

    expected_json= fmt.Sprintf(json_template_received_rrep, end.ip, beginning.ip, 1, end.ip)
    riot_line[2].channels.expect_JSON(expected_json)
    fmt.Print(".")
    expected_json= fmt.Sprintf(json_template_sent_rrep, riot_line[1].ip, beginning.ip, 1, end.ip)
    riot_line[2].channels.expect_JSON(expected_json)
    fmt.Print(".")

    expected_json= fmt.Sprintf(json_template_received_rrep, riot_line[2].ip, beginning.ip, 1, end.ip)
    riot_line[1].channels.expect_JSON(expected_json)
    fmt.Print(".")
    expected_json= fmt.Sprintf(json_template_sent_rrep, beginning.ip, beginning.ip, 1, end.ip)
    riot_line[1].channels.expect_JSON(expected_json)
    fmt.Print(".")

    expected_json= fmt.Sprintf(json_template_received_rrep, riot_line[1].ip, beginning.ip, 1, end.ip)
    beginning.channels.expect_JSON(expected_json)
    fmt.Print(".")

    //TODO: defer dump channels
    fmt.Println("\nDone.")
}

func start_experiments() {
    /* TODO: move this to dedicated test file */
    test_route_creation_0_to_3()
}

func main() {
    //TODO: build fresh RIOT image
    start_experiments()
}