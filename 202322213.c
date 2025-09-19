#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

#define MAX_ALIAS 20

struct {
    char name[50];
    char command[200];
} alias_table[MAX_ALIAS];

int aliasCount = 0;

int main(void) {
    char input[1024];
    char *tokens[20];

    while(1) { // 기본 뼈대를 만들 것 - 무한 루프
        printf("$ "); // 프롬프트 출력할 것임
        fflush(stdout); // 츌력 버퍼 비우면서 내보내기 

        if(fgets(input, sizeof(input), stdin) == NULL) {
            break; // EOF나 에러 시 루프 탈출
        }

        input[strcspn(input, "\n")] = '\0'; // 끝자리를 표시하는 역할 

        if(strcmp(input, "quit") == 0) {
            break;
        }

        int tokenCount = 0;
        char *token = strtok(input, " "); // 첫 호출 구분자 자르기
        while(token!= NULL && tokenCount <20) {
            tokens[tokenCount++] = token;
            token = strtok(NULL, " "); // 이후 호출 이어서 구분자 자르기 
        }
        tokens[tokenCount] = NULL;

        // printf("잘 잘리는지 확인출력해볼게여 -> \n"); // 과제 내기 전에 삭제할 것
        // for (int i = 0; i<tokenCount; i++) {
        //     printf("%d: %s \n", i, tokens[i]);
        // }

        // cd 명령 처리 
        if(strcmp(tokens[0], "cd") == 0) { // strcmp()는 같으면 0을 리턴 
            if(tokens[1] ==NULL) {
                fprintf(stderr, "cd: missing argument\n");
            }
            else {
                if(chdir(tokens[1])!=0) { // chdir는 현재 작업 디렉토리를 변경해줌
                    perror("cd failed"); // 변경 실패 처리해줌 
                }
            }
            continue;
        }

        // 사용자가 입력한 단어가 alias이면 별칭 등록 과정이 필요함 
        if(strcmp(tokens[0], "alias") == 0) {
            if(tokens[1] ==NULL) { // 인자가 없는 경우 처리 
                for (int i = 0; i < aliasCount; i++) {
                    printf("alias %s='%s'\n", alias_table[i].name, alias_table[i].command);
                }
            }
            else { // 인자가 있는 경우 처리 
                char *eq = strchr(tokens[1], '='); // 문자열에서 = 위치 찾음 
                if(eq ==NULL) {
                    fprintf(stderr, "alias: invalid format\n");
                } else {
                    *eq = '\0'; // 문자를 분리 
                    char *name = tokens[1];
                    char *command = eq + 1;
                    if(command[0] == '\''&&command[strlen(command)-1] == '\'') {
                        command[strlen(command)-1] = '\0'; // 문자열 끝 부분을 표시해주는 것 
                        command++; // 원래는 '를 가르키고 있었으므로 건너뛸 것 
                    }
                    strcpy(alias_table[aliasCount].name, name);
                    strcpy(alias_table[aliasCount].command, command);
                    aliasCount++;
                }
            }
            continue;
        }

        for(int i = 0; i<aliasCount; i++) {
            if(strcmp(tokens[0], alias_table[i].name) == 0) {
                strcpy(input, alias_table[i].command);
                tokenCount = 0;
                token = strtok(input, " ");
                while(token != NULL && tokenCount <20) {
                    tokens[tokenCount++] = token;
                    token = strtok(NULL, " ");
                }
                tokens[tokenCount] = NULL;
                break;
            }
        }

        // 부모 프로세스가 fork로 자식 프로세스를 만들었음 
        // 부모가 자식의 종료 상태를 확인하지 않으면
        // 자식 프로세스가 커널에 남기 때문에 회수해주어야 함!
        pid_t pid = fork(); // 새로운 프로세스를 생성할 것임 
        if (pid <0) {
            perror("fork failed");
            continue;
        }
        else if (pid ==0) {
            if (execvp(tokens[0], tokens) == -1) { // 리턴 값이 -1이라는 것은 실패했다는 것
                // 명령 이름이랑 토큰 배열을 나눠 받음
                // execvp는 현재 프로세스를 새로운 프로그램으로 덮어씌움
                // execvp는 성공하면 리턴하지 않음
                perror("exec failed");
            }
            return 1;
        }
        else {
            int status;
            waitpid(pid, &status, 0); // 여기서 0은 자식이 끝날 때까지 블럭된다는 뜻
        }

        char *pipeToken = strchr(input, '|');
        if(pipeToken != NULL) {
            *pipeToken = '\0';
            char *left = input;
            char *right = pipeToken + 1;

            char *command1[20], *command2[20];
            int cmd1, cmd2 = 0;
            char *token1 = strtok(left, " ");
            while(token1 != NULL) {
                command1[cmd1++] = token1;
                token1 = strtok(NULL, " ");
            }
            command1[cmd1] = NULL;
            char *token2 = strtok(right, " ");
            while(token2 != NULL) {
                command2[cmd2++] = token2;
                token2 = strtok(NULL, " ");
            }
            command2[cmd2] = NULL;

        int pipefd[2]; // 파이프 생성 
        if(pipe(pipefd)==-1) {
            perror("pipe failed");
            continue;
        }
        pid_t pid1 = fork();
        if(pid1 == 0) {
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);
            
            execvp(command1[0], command1);
            perror("exec failed");
            exit(1);
        }
        pid_t pid2 = fork();
        if (pid2 == 0) {
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);

            execvp(command2[0], command2);
            perror("exec failed");
            exit(1);
        }
        close(pipefd[0]);
        close(pipefd[1]);
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
        continue;
    }

    return 0;
    }
}