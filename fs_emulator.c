#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h> 
#include <stdint.h> 
#include <string.h>
#include <inttypes.h>
#include <ctype.h>
#define NUM_ARGS 2
#define FS_INDEX 1
#define MAX_SIZE 1024

void *checked_malloc(size_t size) { 
    int *p; 
    int len = 100; 
    
    p = malloc(sizeof(int) * len); 
    if (p == NULL) { 
        perror("malloc"); 
        exit(1); 
    }

    return p; 
}

char *uint32_to_str (uint32_t i) {
    int length = snprintf(NULL, 0, "%lu", (unsigned long)i); 
    char *str = checked_malloc(length + 1);

    snprintf(str, length + 1, "%lu", (unsigned long)i);
    return str; 
}

int validate_args (int argc, char *argv[]) {
    // validate that 1 command-line argument was passed
    if (argc != NUM_ARGS) {
        return 0;  
    }

    // validate that the specified directory actually exists 
    DIR *dir = opendir(argv[FS_INDEX]); 
    
    if (dir) {
        closedir(dir); 
    } else {
        fprintf(stderr, "ERROR: file system not valid\n");
        exit(1);  
    } 
    
    return 1; 
}

char *remove_whitespaces (char *s) { 
    int idx = 0; 
    int len = strlen(s); 

    while (isspace(s[idx]) && s[idx] != '\0') {
        idx++; 
    } 

    while (isspace(s[len - 1])) {
        len--; 
    } 

    s[len] = '\0';   
    return &s[idx]; 
}

void change_wd (int argc, char *argv[]) {  
    if (validate_args(argc, argv)) {
         chdir(argv[FS_INDEX]); 
    } else {
        fprintf(stderr, "ERROR: no file system given\n"); 
        exit(1); 
    }
}

uint32_t search_inodes(uint32_t inode, char *string, char inodes_list[]) {
    uint32_t num[1]; 
    char name[32]; 
    int type; 
    uint32_t index; 
    int match = 0; 
    char *dup = NULL; 
    char *string2 = NULL; 
    char *inode_str = uint32_to_str(inode); 
    FILE *curr_inode = fopen(inode_str, "r");

    while (fread(num, sizeof(uint32_t), 1, curr_inode)) { 
        fread(&name, sizeof(char), 32, curr_inode);
        dup = strdup(name); 
        string2 = strsep(&dup, "\n");
        if (!strcmp(string2, remove_whitespaces(string))) {
             match = 1; 
             index = num[0];  
             type = inodes_list[index]; 

             if (type == 100) { 
                 inode = index; 
             } 

             break; 
        }
    }
 
    if (!match || type != 100) {
        fprintf(stderr, "ERROR: not a valid directory \n"); 
    }
 
    free(dup); 
    free(string2); 
    free(inode_str); 
    fclose(curr_inode);  
    return inode; 
}

void ls_command(int inode) {
    uint32_t num[1]; 
    char name[32];
    char *inode_str = uint32_to_str(inode);  
    FILE *curr_inode = fopen(inode_str, "r"); 
    
    while (fread(num, sizeof(uint32_t), 1, curr_inode)) { 
        fread(&name, sizeof(char), 32, curr_inode);
        fprintf(stdout, "%i %s\n", num[0], name);
    }

    free(inode_str); 
    fclose(curr_inode);    
} 

void exit_command() {  
    exit(1); 
}

int cd_command(char *string, uint32_t inode, char inodes_list[]) {
    strsep(&string, " "); 
    inode = search_inodes(inode, string, inodes_list);
    return inode; 
} 

uint32_t new_entry(uint32_t inode, char *string, char inodes_list[]) {
    char *inode_str = uint32_to_str(inode); 
    FILE *curr_inode = fopen(inode_str, "a"); 
    uint32_t avail = 3000; 

    free(inode_str); 
    // now I need to calculate the actual inode number
    for (uint32_t i = 0; i < MAX_SIZE; i++) {
        if (inodes_list[i] == '\0') {
            avail = i; 
            break; 
        }
    } 

    if (avail != 3000) {
        uint32_t num[1] = {avail}; 

        //strsep(&string, " ");    
        fwrite(&num[0], sizeof(uint32_t), 1, curr_inode); 
        fwrite(remove_whitespaces(string), sizeof(char), 32, curr_inode); 
        fclose(curr_inode);  
        return avail;
    } else { 
        fprintf(stderr, "ERROR: no inodes available");
        fclose(curr_inode);
        return avail;  
    }
} 

int already_exists (uint32_t inode, char *string, char inodes_list[]) {
    uint32_t num[1]; 
    char name[32]; 
    int match = 0; 
    char *dup = NULL; 
    char *string2 = NULL; 
    char *inode_str = uint32_to_str(inode); 
    FILE *curr_inode = fopen(inode_str, "r"); 
    
    while (fread(num, sizeof(uint32_t), 1, curr_inode)) { 
        fread(&name, sizeof(char), 32, curr_inode); 
        dup = strdup(name); 
        string2 = strsep(&dup, "\n"); 
        if (!strcmp(string2, remove_whitespaces(string))) { 
            match = 1;
            break;  
        }
    }

    free(dup); 
    free(string2); 
    free(inode_str); 
    fclose(curr_inode); 
    return match;
}

void write_inode_file (uint32_t index, uint32_t inode, char type, char *file_name) {
    char *inode_str = uint32_to_str(index);
    FILE *inode_file = fopen(inode_str, "w");
    FILE *inodes_list = fopen("inodes_list", "a"); 
    uint32_t num[2] = {index, inode};
    char curr_dir[32] = ".";  
    char dir[32] = "..";  
    char *string[2] = {curr_dir, dir};
    char mode[1] = {type};  
    char *name[1] = {file_name}; 

    fflush(stdin);
    fflush(stderr); 
    fflush(stdout);  
    // I need to clear the buffer before I start writing... I just don't know how 
    if (type == 'd') { 
        fwrite(&num[0], sizeof(uint32_t), 1, inode_file); 
        fwrite(string[0], sizeof(char), 32, inode_file); 
        fwrite(&num[1], sizeof(uint32_t), 1, inode_file); 
        fwrite(string[1], sizeof(char), 32, inode_file);
    } else { 
        fwrite(name[0], sizeof(char), 32, inode_file); 
    }

    fwrite(&num[0], sizeof(uint32_t), 1, inodes_list); 
    fwrite(&mode[0], sizeof(char), 1, inodes_list); 

    free(inode_str); 
    fclose(inode_file); 
    fclose(inodes_list); 
} 

uint32_t mkdir_command(uint32_t inode, char *string, char inodes_list[]) { 
    uint32_t index = 3000;
   
    strsep(&string, " "); 
    if (!already_exists(inode, string, inodes_list)) {
        index = new_entry(inode, string, inodes_list);
        if (index != 3000) { 
            write_inode_file(index, inode, 'd', string);  
        }
        return index; 
    } else {
        fprintf(stderr, "ERROR: directory already exists\n");  
        return index; 
    }
}

uint32_t touch_command(uint32_t inode, char *string, char inodes_list[]) {
    uint32_t index = 3000; 

    strsep(&string, " "); 
    if (!already_exists(inode, string, inodes_list)) {
        index = new_entry(inode, string, inodes_list);  
        if (index != 3000) { 
            write_inode_file(index, inode, 'f', string); 
        }
        return index; 
    } else { 
        return index; 
    }
}

void traverse_directory(char inodes_list[]) {
    uint32_t inode = 0; 
    char *command = NULL; 
    char *dup = NULL;
    size_t size = 10;
    char *string = NULL; 
    int index = 0; 
    char *inode_str = uint32_to_str(inode); 
    FILE *inode_0 = fopen(inode_str, "r"); 

    if (inode_0 == NULL) {
        fprintf(stderr, "ERROR: inode 0 does not exist\n"); 
	free(inode_str); 
	exit(0); 
    }

    fclose(inode_0); 
    free(inode_str); 

    while(1) {
       fprintf(stdout, "> "); 
       getline(&command, &size, stdin);  
       dup = strdup(command); // I feel like I need to free dup but I can't figure out how to do it  validly 
       string = remove_whitespaces(strsep(&dup, "\n"));  
            if (!strcmp(string, "ls")) {
                ls_command(inode); 
            } else if (!strcmp(string, "exit")) { 
		free(string); 
                free(command); 
                exit_command(); 
            } else if (strstr(string, "cd") != NULL) {
		if (!isspace(string[2])) {
		    fprintf(stderr, "ERROR: not a valid cd command\n"); 
	        } else { 
                    inode = cd_command(string, inode, inodes_list); 
		}
            } else if (strstr(string, "mkdir") != NULL) { 
		if (!isspace(string[5])) { 
	            fprintf(stderr, "ERROR: not a valid mkdir command\n"); 
		}
		else { 
	            index = mkdir_command(inode, string, inodes_list);
                    if (index != 3000) {
                        inodes_list[index] = 'd';
                    }
		}
            } else if (strstr(string, "touch") != NULL) { 
                if (!isspace(string[5])) {
	            fprintf(stderr, "ERROR: not a valid touch command\n"); 
	        } else { 
		    index = touch_command(inode, string, inodes_list); 
                    if (index != 3000) {
                        inodes_list[index] = 'f'; 
                    }
		}
            } else {
                fprintf(stderr, "ERROR: not a valid command\n"); 
            }   
    }
}

void load_inodes() { 
    FILE *inodes_file = NULL; 
    char type[1]; 
    uint32_t num[1];
    char inodes[MAX_SIZE] = {'\0'};   

    inodes_file = fopen("inodes_list", "r"); 
    
    if (inodes_file == NULL) {
       fprintf(stderr, "ERROR: failed to open inodes_list"); 
       exit(1); 
    }

    while (fread(num, sizeof(uint32_t), 1, inodes_file)) {  
        fread(type, sizeof(char), 1, inodes_file);
        
        if (num[0] < MAX_SIZE) { 
            inodes[num[0]] = type[0]; 
        }
    } 

    fclose(inodes_file); 
    traverse_directory(inodes);  
}

int main (int argc, char *argv[]) {  
    change_wd(argc, argv);
    load_inodes();   
    return 0;  

}
