/**
 * @file main.c
 * @brief Checks files
 * @date 2021-11-06
 * @author Raoul Neves 2191203
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

#include "debug.h"
#include "memory.h"
#include "args.h"

#define MATCHED 999
#define MISMATCHED 998
#define NOT_SUPPORTED 997

int fileTester(char *name_or_path);

int main(int argc, char *argv[]) {

    /* Disable warnings */
    (void)argc;
    (void)argv;

    FILE *file;

    int counter_total = 0,counter_match = 0, counter_nomatch = 0, counter_nots = 0;    

    // gengetopt required struct
    struct gengetopt_args_info args;

    // gengetopt parser
    if(cmdline_parser(argc, argv, &args))
        ERROR(1, "Error: Execution failed of cmdline_parser.\n");


    if(args.file_given){
    
        char file_name_copy[strlen(args.file_arg)+1];

        // Make a copy of filename
        strcpy(file_name_copy, args.file_arg);

        fileTester(file_name_copy);

    }
    
    if(args.batch_given){

        // Verify if file exists
        if ( (file = fopen(args.batch_arg, "r")) ) 
        {

            // Arbitrary size for line buffer
            char line[1000];

            printf("[INFO] analyzing files listed in ‘%s’.\n", args.batch_arg);
            while (fgets(line, sizeof(line), file)) {

                // Temporary fix for strings not ending properly
                if(line[strlen(line) - 1] == '\n'){
                    line[strlen(line) - 1] = '\0';
                }else{
                    line[strlen(line)] = '\0';
                }


                // Switch to keep track of outcomes
                switch(fileTester(line)){
                    case MATCHED:
                        counter_match++;
                        break;
                    case MISMATCHED:
                        counter_nomatch++;
                        break;
                    case NOT_SUPPORTED:
                        counter_nots++;
                        break;
                }

                counter_total++;
            }
            printf("[SUMMARY] Files Analyzed: %d; Files OK: %d; Files MISMATCH: %d; Errors: %d\n", counter_total, counter_match, counter_nomatch, counter_nots);
            fclose(file);
        }
        else
        {
            fprintf(stderr, "ERROR: cannot open file <%s>  --  %s\n", args.file_arg, strerror(errno));
            exit(1);
        }

    }

    if(args.dir_given){
    
            pid_t pid;
            int second_link[2];
            // Arbitrary sized buffer
            char second_readbuffer[5000];

            // Pipe creation verification
            if(pipe(second_link) == -1){
                fprintf(stderr, "ERROR: cannot create a pipe --  %s\n", strerror(errno));
                exit(1);
            }


            pid = fork();
            if (pid == 0) {  
                // Send stdout to the pipe
                dup2 (second_link[1], STDOUT_FILENO);

                // Close reading side of pipe for child
                close(second_link[0]);
                // No longer needed
                close(second_link[1]);

                // Runs the find command
                if(execlp("find", "find", args.dir_arg, "-type", "f", (char *) NULL) == -1){
                    fprintf(stderr, "ERROR: execlp() failed  --  %s\n", strerror(errno));
                }

                // Should not reach here
                exit(1);
            } else if (pid > 0) {
                // Not used
            } else            
                ERROR(2, "Erro na execucao do fork()");

            // Closes writting end of the pipe
            close(second_link[1]);
            // Waits for child output
            wait(NULL);


            // Retrieves the output
            int nbytes = read(second_link[0], second_readbuffer, sizeof(second_readbuffer));

            if(nbytes == 0){
                fprintf(stderr, "ERROR: pipe() reading failed.\n");
                exit(1);
            }


            // Reading of lines inside the buffer that are separated by \n done with help from:
            // https://stackoverflow.com/questions/17983005/c-how-to-read-a-string-line-by-line
            
            char * curLine = second_readbuffer;

            printf("\n\n[INFO] analyzing files of directory '%s'\n", args.dir_arg);

            while(curLine)
            {
                char * nextLine = strchr(curLine, '\n');
                if (nextLine) *nextLine = '\0';  // temporarily terminate the current line


                switch(fileTester(curLine)){
                    case MATCHED:
                        counter_match++;
                        break;
                    case MISMATCHED:
                        counter_nomatch++;
                        break;
                    case NOT_SUPPORTED:
                        counter_nots++;
                        break;
                }

                counter_total++;

                if (nextLine) *nextLine = '\n';   
                curLine = nextLine ? (nextLine+1) : NULL;
            }
            printf("[SUMMARY] Files Analyzed: %d; Files OK: %d; Files MISMATCH: %d; Errors: %d\n", counter_total, counter_match, counter_nomatch, counter_nots);


    }

    // gengetopt release
    cmdline_parser_free(&args);

    return 0;
}



int fileTester(char *name_or_path){

    int link[2];
    char readbuffer[100];
    pid_t pid;
    FILE *file;

    // Testing if file exists
    if ( (file = fopen(name_or_path, "r")) ) {
        fclose(file);
    }
    else
    {
        fprintf(stderr, "ERROR: cannot open file <%s>  --  %s\n", name_or_path, strerror(errno));
        return NOT_SUPPORTED;
    }

    // Opening pipe for child output
    if(pipe(link) == -1){
        fprintf(stderr, "ERROR: cannot create a pipe --  %s\n", strerror(errno));
        exit(1);
    }

    pid = fork();

        // Child
        if (pid == 0) { 

            // Send stdout to the pipe
            dup2 (link[1], STDOUT_FILENO);

            // Close reading side of pipe for child
            close(link[0]);
            // No longer needed
            close(link[1]);

            // Runs the file command
            if(execlp("file", "file", "-b", name_or_path, NULL) == -1){
                fprintf(stderr, "ERROR: execlp() failed  --  %s\n", strerror(errno));
            }

            exit(1);

        } else if (pid > 0) { 
            // NOT USED
        } else  {
            // < 0 - FAILED             
            fprintf(stderr, "ERROR: fork() failed  --  %s\n", strerror(errno));
            exit(1);
        }

        // FATHER PROCESS

        // Pointer used to iterate the file name string
        char * pt_extension_file;

        // Pointer to the end of the string
        pt_extension_file = name_or_path + strlen(name_or_path)-1;

        // Finds the file extension by running back from end of string until dot is found
        while(pt_extension_file != name_or_path){

            if(*pt_extension_file == '.'){
                pt_extension_file++;
                break;
            }

            pt_extension_file--;
        }

        // Checks if there is a dot
        if(pt_extension_file == name_or_path){
            fprintf(stderr, "[ERROR] no extension for file <%s>\n", name_or_path);
            return NOT_SUPPORTED;
        }

        // Closes writting end of the pipe
        close(link[1]);
        // Waits for child output
        wait(NULL);

        // Retrieves the output
        int nbytes = read(link[0], readbuffer, sizeof(readbuffer));

        if(nbytes == 0){
            fprintf(stderr, "ERROR: pipe() reading failed.\n");
            exit(1);
        }

        char retrieved_file_type[8];
        char * pt_file_type = readbuffer;


        // Find where the separation between file type and rest of text
        pt_file_type = strchr(readbuffer, ' ');

        // Find dist between two pointers
        int length_of_extension = labs(readbuffer-pt_file_type);

        // Isolates the extention
        strncpy(retrieved_file_type, readbuffer, length_of_extension);
        retrieved_file_type[length_of_extension] = '\0';

        // Make everything lowercase for both
        for(int i = 0; pt_extension_file[i]; i++){
            pt_extension_file[i] = tolower(pt_extension_file[i]);
        }
        for(int j = 0; retrieved_file_type[j]; j++){
            retrieved_file_type[j] = tolower(retrieved_file_type[j]);
        }

        int valid_type_flag = 0;
        
        if(strcmp(pt_extension_file, "pdf") == 0){
            valid_type_flag = 1;
        }else if(strcmp(pt_extension_file, "gif") == 0){
            valid_type_flag = 1;
        }else if(strcmp(pt_extension_file, "jpg") == 0){
            valid_type_flag = 1;
        }else if(strcmp(pt_extension_file, "jpeg") == 0){
            valid_type_flag = 1;
        }else if(strcmp(pt_extension_file, "png") == 0){
            valid_type_flag = 1;
        }else if(strcmp(pt_extension_file, "mp4") == 0){
            valid_type_flag = 1;
        }else if(strcmp(pt_extension_file, "zip") == 0){
            valid_type_flag = 1;
        }else if(strcmp(pt_extension_file, "html") == 0){
            valid_type_flag = 1;
        }else{
            printf("[INFO] '%s': type '%s' is not supported by checkFile.\n",name_or_path, retrieved_file_type);
            return NOT_SUPPORTED;
        }
        
        if(valid_type_flag == 1){
            // Same extention
            if(strcmp(pt_extension_file, retrieved_file_type) == 0){
                printf("[OK] '%s': extension '%s' matches file type '%s'.\n", name_or_path, pt_extension_file, retrieved_file_type);
                return MATCHED;
            }else{
                printf("[MISMATCH] '%s': extension is '%s', file type is '%s'.\n", name_or_path, pt_extension_file, retrieved_file_type);
                return MISMATCHED;
            }
        }

        // Should not reach here
        return -1;
}