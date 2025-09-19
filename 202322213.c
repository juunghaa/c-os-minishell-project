#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
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

    }


    return 0;
}