#include <stdio.h>
#include <string.h>
#include <stdlib.h>


char* concat(const char *s1, const char *s2){
  char *result = malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
  // in real code you would check for errors in malloc here
  strcpy(result, s1);
  strcat(result, s2);
  return result;
}


int run_command(char* cmd, char** output){
 
  int BUFSIZE=1024;
    // char buf[BUFSIZE];
  FILE* fp;
  if ((fp = popen(cmd, "r")) == NULL) {
    printf("Error opening pipe!\n");
    return -1;
  }

  while (fgets(*output, BUFSIZE, fp) != NULL) {
    // Do whatever you want here...
    printf("%s", *output);
  }

  if (pclose(fp)) {
    printf("Command not found or exited with error status\n");
    return -1;
  }
  return 0;
}

void foo(char* int_name){
  char* command_tmp = "ethtool -T ";
  char* command = concat(command_tmp, int_name);
  // char* command = "pwd";
  printf("%s\n",command); 
  char ** output;
  *output = (char *)malloc(1024+1);
  return; 
  int res = run_command(command, output);
  printf("%s\n", *output);
  printf("%s\n", command);
}


int main(){
  char*  int_name="eno1";
  foo(int_name); 
  return 0;
}
