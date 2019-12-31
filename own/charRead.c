#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
#include<time.h>

#define BUFFER_LENGTH 12 * 1024               ///< The buffer length (crude but fine)
static char receive[BUFFER_LENGTH] = {0};     ///< The receive buffer from the LKM

void forkzer() {
   fork();
   printf("%s", "Hello world\n");
}

int main(){
   int ret, fd;
   char stringToSend[BUFFER_LENGTH] = "Hello world!";

   printf("Starting device test code example...\n");
   fd = open("/dev/opsysmem", O_RDWR);             // Open the device with read/write access
   
   if (fd < 0){
      printf("%s","Failed to open the device...");
      return errno;
   }
   
   for(int i=0; i<100000; i++) {
      
      ret = write(fd, stringToSend, strlen(stringToSend)); // Send the string to the LKM

      if (ret < 0) {
         perror("Failed to write the message to the device.");
         break;
      }
   }

   for(int i=0; i<10; i++) {
      ret = read(fd, receive, BUFFER_LENGTH); // Read the response from the LKM
      if (ret < 0)
      {
         perror("Failed to read the message from the device.");
         break;
      }
      printf("The received message is: [%s]\n", receive);
   }

   return 0;
}


