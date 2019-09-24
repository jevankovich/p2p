#include <stdio.h>

#include <netinet/in.h>

#include "net.h"

int main(int argc, char **argv)
{
    int ctrl = open_control_socket(0);
    if (ctrl == -1) {
        perror("Failed opening control socket");
        return -1;
    }
}
