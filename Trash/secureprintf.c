#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <readline/readline.h>

// On VSCode on settings add the path /opt/homebrew/opt/readline/include/

// Thanks to:
// https://stackoverflow.com/questions/5068846/how-to-output-messages-to-stdout-on-another-thread-while-reading-from-stdin-with
// Used to not mess up the terminal with stdio/stdout asynchronous.


#define printf(...) my_rl_printf(__VA_ARGS__)
void my_rl_printf(char *fmt, ...) {
    int need_hack = (rl_readline_state & RL_STATE_READCMD) > 0;
    char *saved_line;
    int saved_point;
    if (need_hack)
    {
        saved_point = rl_point;
        saved_line = rl_copy_text(0, rl_end);
        rl_save_prompt();
        rl_replace_line("", 0);
        rl_redisplay();
    }

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    if (need_hack)
    {
        rl_restore_prompt();
        rl_replace_line(saved_line, 0);
        rl_point = saved_point;
        rl_redisplay();
        free(saved_line);
    }
}
     
int main(void){
    
    printf("Hello!\n");

    return 0;
    
} 
    
