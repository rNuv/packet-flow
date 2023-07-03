#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "Reliable.h"

void usage(char *name)
{
    printf("usage: %s [filename]\n", name);
    printf("    -h,                   show help message and exit\n");
    printf("    -d ip_address         IP address of remote end (default 127.0.0.1)\n");
    printf("    -r remote_port,       port for the remote end (default 7090)\n");
    printf("    -p local_port,        port for the local end (default 10000)\n");
    printf("    -n sequence_number,   initial sequence number in SYN (default at random)\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    struct option longopts[] = {
        {"ip_address", required_argument, NULL, 'd'},
        {"remote_port", required_argument, NULL, 'r'},
        {"local_port", required_argument, NULL, 'p'},
        {"sequence_number", required_argument, NULL, 'n'},
        {0, 0, 0, 0}};

    int c, local_port = 10000, remote_port = 7090;
    char *ip_address = "127.0.0.1";
    uint32_t n = 0;
    bool nflag = false;
    while ((c = getopt_long(argc, argv, "d:r:p:n:h", longopts, NULL)) != -1)
    {
        switch (c)
        {
        case 'd':
            ip_address = optarg;
            break;
        case 'p':
            local_port = atoi(optarg);
            break;
        case 'r':
            remote_port = atoi(optarg);
            break;
        case 'n':
            n = strtoul(optarg, NULL, 10);
            nflag = true;
            break;
        case '?':
        case 'h':
        default:
            usage(argv[0]);
            break;
        }
    }

    if (optind >= argc) // getopt() cannot permutate options on MacOS
        usage(argv[0]);

    Reliable *reli = reliCreate(local_port);
    if (reli == NULL)
    {
        fprintf(stderr, "Socket error\n");
        return 0;
    }
    printf("Connecting...\n");
    if (reliConnect(reli, ip_address, remote_port, nflag, n) == -1)
    {
        fprintf(stderr, "Connect error\n");
        return 0;
    }
    printf("Connected to %s:%d\n", ip_address, remote_port);

    FILE *fin = fopen(argv[optind], "r");
    if (fin == NULL)
    {
        fprintf(stderr, "Cannot find the file %s\n", argv[optind]);
        reliClose(reli);
        return 0;
    }

    while (true)
    {
        Payload *payload = payloadCreate(PAYLOAD_SIZE, false);
        payload->len = fread(payload->buf, 1, PAYLOAD_SIZE, fin);
        if (payload->len == 0)
            break;
        reliSend(reli, payload); // payload->buf will be free in reli
    }
    
    fclose(fin);
    reliClose(reli);
    return 0;
}
