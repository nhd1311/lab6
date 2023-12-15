#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_LINE 80 /* The maximum length command */

int main(void) {
    char inputBuffer[MAX_LINE];
    int should_run = 1; /* flag to determine when to exit program */

    while (should_run) {
        printf("it007sh> ");
        fflush(stdout);

        fgets(inputBuffer, sizeof(inputBuffer), stdin); // Đọc lệnh từ người dùng
        inputBuffer[strcspn(inputBuffer, "\n")] = '\0'; // Xóa kí tự newline

        // Tách các lệnh theo dấu "|"
        char *commands[MAX_LINE / 2 + 1];
        int num_commands = 0;

        char *token = strtok(inputBuffer, "|");
        while (token != NULL) {
            commands[num_commands++] = token;
            token = strtok(NULL, "|");
        }
        commands[num_commands] = NULL; // Kết thúc danh sách lệnh bằng NULL

        int prev_pipe[2]; // Pipe cho lệnh trước
        int current_pipe[2]; // Pipe cho lệnh hiện tại
        pid_t pid;

        for (int i = 0; i < num_commands; i++) {
            if (i > 0) { // Nếu không phải lệnh đầu tiên, sử dụng pipe của lệnh trước
                prev_pipe[0] = current_pipe[0];
                prev_pipe[1] = current_pipe[1];
            }

            if (i < num_commands - 1) { // Nếu không phải lệnh cuối cùng, tạo pipe mới
                if (pipe(current_pipe) < 0) {
                    perror("Pipe error");
                    return EXIT_FAILURE;
                }
            }

            pid = fork(); // Tạo tiến trình con

            if (pid < 0) {
                perror("Fork error");
                return EXIT_FAILURE;
            } else if (pid == 0) { // Tiến trình con
                if (i > 0) { // Đối với các lệnh không phải lệnh đầu tiên
                    close(STDIN_FILENO); // Đóng đầu vào chuẩn
                    dup2(prev_pipe[0], STDIN_FILENO); // Sử dụng đầu vào từ pipe trước đó
                    close(prev_pipe[0]); // Đóng đối tượng file descriptor không cần thiết
                    close(prev_pipe[1]);
                }

                if (i < num_commands - 1) { // Đối với các lệnh không phải lệnh cuối cùng
                    close(STDOUT_FILENO); // Đóng đầu ra chuẩn
                    dup2(current_pipe[1], STDOUT_FILENO); // Sử dụng đầu ra đến pipe hiện tại
                    close(current_pipe[0]); // Đóng đối tượng file descriptor không cần thiết
                    close(current_pipe[1]);
                }

                // Tách các đối số của lệnh và thực thi lệnh
                char *args[MAX_LINE / 2 + 1];
                char *token = strtok(commands[i], " ");
                int arg_index = 0;

                while (token != NULL) {
                    args[arg_index++] = token;
                    token = strtok(NULL, " ");
                }
                args[arg_index] = NULL; // Kết thúc danh sách đối số bằng NULL

                execvp(args[0], args); // Thực thi lệnh

                perror("Execution error");
                return EXIT_FAILURE;
            }
        }

        close(prev_pipe[0]); // Đóng pipe trong tiến trình cha
        close(prev_pipe[1]);

        // Chờ tất cả các tiến trình con kết thúc
        for (int i = 0; i < num_commands; i++) {
            wait(NULL);
        }
    }

    return 0;
}
