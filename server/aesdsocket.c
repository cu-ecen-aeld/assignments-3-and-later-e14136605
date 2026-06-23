
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>

#define PORT "9000"
#define DATA_FILE "/var/tmp/aesdsocketdata"
#define BUFFER_SIZE 1024

// 全域變數，用來標記是否收到結束訊號 (Ctrl+C 或 kill)
volatile sig_atomic_t caught_sig = 0;

// 訊號處理函數
void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        caught_sig = 1;
    }
}

int main(int argc, char *argv[]) {
    bool is_daemon = false;
    int sockfd = -1, client_fd = -1;
    struct addrinfo hints, *servinfo;
    struct sigaction sa;

    // 1. 檢查是否以 Daemon 模式啟動 (-d)
    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        is_daemon = true;
    }

    // 2. 初始化 syslog
    openlog("aesdsocket", LOG_PID, LOG_USER);

    // 3. 註冊訊號處理器 (捕捉 SIGINT 和 SIGTERM)
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // 4. 設定 Socket 參數
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // 使用本機 IP

    if (getaddrinfo(NULL, PORT, &hints, &servinfo) != 0) {
        syslog(LOG_ERR, "getaddrinfo failed");
        return -1;
    }

    // 5. 建立 Socket
    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (sockfd == -1) {
        syslog(LOG_ERR, "socket creation failed");
        freeaddrinfo(servinfo);
        return -1;
    }

    // 設定 SO_REUSEADDR，避免重啟時卡 "Address already in use"
    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        syslog(LOG_ERR, "setsockopt failed");
        freeaddrinfo(servinfo);
        close(sockfd);
        return -1;
    }

    // 6. 綁定 Port 9000
    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        syslog(LOG_ERR, "bind failed");
        freeaddrinfo(servinfo);
        close(sockfd);
        return -1;
    }
    freeaddrinfo(servinfo); // 綁定完就可以釋放了

    // 7. 如果有 -d 參數，此時才進入 Daemon 模式
    if (is_daemon) {
        syslog(LOG_INFO, "Entering daemon mode");
        if (daemon(0, 0) == -1) {
            syslog(LOG_ERR, "daemon creation failed");
            close(sockfd);
            return -1;
        }
    }

    // 8. 開始監聽
    if (listen(sockfd, 10) == -1) {
        syslog(LOG_ERR, "listen failed");
        close(sockfd);
        return -1;
    }

    // 9. 伺服器主迴圈 (直到收到中斷訊號才停止)
    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);

    while (!caught_sig) {
        // 接受連線
        client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_size);
        if (client_fd == -1) {
            if (caught_sig) break; // 如果是因為收到訊號而中斷 accept，則跳出迴圈
            continue;
        }

        // 紀錄客戶端 IP
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
        syslog(LOG_DEBUG, "Accepted connection from %s", ip_str);

        // --- 接收資料邏輯 ---
        size_t current_buf_size = BUFFER_SIZE;
        char *rx_buffer = malloc(current_buf_size);
        if (rx_buffer == NULL) {
            syslog(LOG_ERR, "malloc failed");
            close(client_fd);
            continue;
        }

        size_t total_received = 0;
        ssize_t bytes_recv;
        bool packet_complete = false;

        // 不斷接收直到看到換行符號 \n
        while (!packet_complete && (bytes_recv = recv(client_fd, rx_buffer + total_received, current_buf_size - total_received - 1, 0)) > 0) {
            total_received += bytes_recv;
            rx_buffer[total_received] = '\0'; // 確保字串結尾

            if (strchr(rx_buffer, '\n') != NULL) {
                packet_complete = true;
            } else {
                // 如果緩衝區快滿了，動態擴充記憶體
                if (total_received >= current_buf_size - 1) {
                    current_buf_size *= 2;
                    char *new_buffer = realloc(rx_buffer, current_buf_size);
                    if (new_buffer == NULL) {
                        syslog(LOG_ERR, "realloc failed");
                        free(rx_buffer);
                        break; // 記憶體不足，放棄這個封包
                    }
                    rx_buffer = new_buffer;
                }
            }
        }

        // 將收到的完整封包寫入檔案
        if (packet_complete) {
            FILE *fp = fopen(DATA_FILE, "a+");
            if (fp != NULL) {
                fputs(rx_buffer, fp);
                
                // --- 回傳資料邏輯 ---
                // 將檔案指標移回開頭，把所有內容傳回給客戶端
                fseek(fp, 0, SEEK_SET);
                char tx_buffer[BUFFER_SIZE];
                size_t bytes_read;
                while ((bytes_read = fread(tx_buffer, 1, sizeof(tx_buffer), fp)) > 0) {
                    send(client_fd, tx_buffer, bytes_read, 0);
                }
                fclose(fp);
            }
        }
        
        free(rx_buffer); // 釋放動態記憶體，避免 Memory Leak

        // 關閉客戶端連線
        close(client_fd);
        syslog(LOG_DEBUG, "Closed connection from %s", ip_str);
    }

    // 10. 程式優雅結束 (Cleanup)
    syslog(LOG_DEBUG, "Caught signal, exiting");
    if (sockfd != -1) close(sockfd);
    remove(DATA_FILE); // 刪除暫存檔
    closelog();

    return 0;
}
