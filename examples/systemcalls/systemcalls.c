#include "systemcalls.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

 // --- 實作開始 ---
    int status = system(cmd);
    
    // 如果 system() 執行失敗會回傳 -1
    if (status == -1) {
        return false;
    }

    return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

 fflush(stdout); // 清空輸出緩衝區，避免 printf 重複印出
    
    pid_t pid = fork();
    
    if (pid == -1) {
        // fork 失敗
        return false;
    } else if (pid == 0) {
        // 子行程
        execv(command[0], command);
        // 如果 execv 成功，程式會被替換，下面這行永遠不會執行
        // 會走到這行代表 execv 失敗 (例如找不到指令)
        exit(EXIT_FAILURE); 
    } else {
        // 父行程
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            return false;
        }
        // 檢查子行程是否正常結束 (WIFEXITED) 且回傳值為 0 (WEXITSTATUS)
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            return true;
        } else {
            return false;
        }
    }
    // --- 你的實作到這裡結束 ---

    va_end(args);

    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


  // --- 你的實作從這裡開始 ---
    
    // 打開或建立檔案，設定權限為 0644
    int fd = open(outputfile, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if (fd < 0) {
        return false;
    }

    fflush(stdout); 
    
    pid_t pid = fork();
    
    if (pid == -1) {
        close(fd);
        return false;
    } else if (pid == 0) {
        // 子行程：將 stdout(檔案描述符為1) 重導向到 fd
        if (dup2(fd, 1) < 0) {
            exit(EXIT_FAILURE);
        }
        close(fd); 
        
        execv(command[0], command);
        exit(EXIT_FAILURE); 
    } else {
        // 父行程：不需要寫入檔案，直接關閉
        close(fd); 
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            return false;
        }
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            return true;
        } else {
            return false;
        }
    }
    // --- 你的實作到這裡結束 ---

    va_end(args);

    return true;
}
