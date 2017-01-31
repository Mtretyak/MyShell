#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>



char* GetUserPath() {
	char *buf = (char*)malloc(sizeof(char) * 512);
	getcwd(buf, 512);
	return buf;
}

void pwd() {
	char *extra_buffer = (char*)malloc(sizeof(char) * 512);
	char *path = getcwd(extra_buffer, 512);
	if (path == NULL)
		printf("ERROR in pwd");
	else printf("%s", extra_buffer);
	free(extra_buffer);
}

char* DeleteExtraSpaces(char *command) {
	int size = strlen(command);
	if (size == 0)
		return NULL;
	int i = 0;
	char *tmp = (char*)malloc(sizeof(char) * (size + 1));
	int j = 0;
	for (;i < size; ++i){
		if (command[i] == ' ')
			if (command[i + 1] == ' ' || j == 0)
				continue;
		tmp[j++] = command[i];
	}
	if (tmp[j - 1] == '\n') --j;
	if (tmp[j - 1] == ' ') --j;
	tmp[j] = 0;
	return tmp;
}

void ParseToWords(char *string, int *number_words, char ***words) {

	int size = strlen(string);
	int i = 0;
	int j = 0; // для скобок
	*number_words = 0;
	for (i = 0; i < size; ++i) {
		if (string[i] == ')')
			--j;
		if (string[i] == '(')
			++j;
		if (string[i] == ' '){
			if (j == 0)
				(*number_words)++;
		}
	}
	(*number_words)++;
	*words = (char**)malloc(sizeof(char*) * (*number_words + 1));
	int current_position = 0;
	for (i = 0; i < *number_words; ++i) {
		int size_current_word = 0;
		j = 0;
		while(current_position + size_current_word < size){
			if (string[current_position + size_current_word] == ')')
				--j;
			if (string[current_position + size_current_word] == '(')
				++j;
			if (string[current_position + size_current_word] == ' '){
				if (j == 0)
					break;
			}
			size_current_word++;
		}
		(*words)[i] = (char*)malloc(sizeof(char) * (size_current_word + 1));
		int k = 0;
		for (k = 0; k < size_current_word; ++k) {
			(*words)[i][k] = string[k + current_position];
		}
		current_position += size_current_word + 1;
		(*words)[i][size_current_word] = 0;
	}
	(*words)[*number_words] = NULL;
}

int FindWord(char **words, int number_words, const char *word) {
	int size = number_words;
	int i = 0;
	for (i = 0; i < size; ++i) {
		if (strcmp(words[i], word) == 0)
			return i;
	}
	return 0;
}

void cd(char **words, int number_words, char *user_path) {
	if (number_words < 2) {
		chdir(user_path);
	}
	else {
		int res = chdir(words[1]);
		if (res < 0) {
			printf("Not found file or catalog\n");
		}
	}
}

void DeleteWords(char **words, int *number_words, int index_left, int index_right) {
	int size  = index_right - index_left + 1;
	int tail_size = *number_words - index_right - 1;
	(*number_words) -= size;
	int i = 0;
	for (i = 0; i < tail_size; ++i) {
		int from = i + index_right + 1;
		int to = i + index_left;
		free(words[i + index_left]);
		words[i] = (char *)malloc(sizeof(char) * (strlen(words[from]) + 1));
		strcpy(words[to], words[from]);
	}
	for (i = 1; i < size; ++i) {
		free(words[*number_words + i]);
	}
	words[*number_words] = NULL;
}

void SimpleProcess(char **words, int number_words, int is_waited, int* status) {
	int pid = fork();
	int tmp;
	if (pid == 0) {
		int res = execvp(words[0], words);
		if (res == -1)
			printf("ERROR: can not launch this simple process: %s\n",words[0]);
		exit(1);
	}
	if (pid < 0) {
		printf("ERROR: not created fork process\n");
	}
	if (pid > 0) {
		if (is_waited == 0)
			wait(&tmp);
		if (WEXITSTATUS(tmp)) *status = 0;
	}
}

void RedirectedProcess(char **words, int number_words, int is_redirected_stream, int is_waited, int* status) {
	int tmp;
	int pid = fork();
	if (pid == 0) {
		int file;
		if (strcmp(words[is_redirected_stream], ">") == 0) {
			file = open(words[is_redirected_stream + 1], O_WRONLY | O_TRUNC | O_CREAT, 0644);
			if (file < 0){
				printf("Can not open input file: %s\n",words[is_redirected_stream + 1]);
				exit(1);
			}
			int h = dup2(file, 1);
			if (h < 0) {
				printf("ERROR dup2 if file\n");
				exit(1);
			}
		}
		if (strcmp(words[is_redirected_stream], ">>") == 0) {
			file = open(words[is_redirected_stream + 1], O_WRONLY | O_APPEND | O_CREAT, 0644);
			if (file < 0){
				printf("Can not open input file: %s\n", words[is_redirected_stream + 1]);
				exit(1);
			}
			int h = dup2(file, 1);
			if (h < 0)
				printf("ERROR dup2 in file\n");
		}
		if (strcmp(words[is_redirected_stream], "<") == 0) {
			file = open(words[is_redirected_stream + 1], O_RDONLY, 0644);
			if (file < 0){
				printf("Can not open output file: %s\n",words[is_redirected_stream + 1]);
				exit(1);
			}	
			int h = dup2(file, 0);
			if (h < 0)
				printf("ERROR dup2 in file\n");
		}
		DeleteWords(words, &number_words, is_redirected_stream, is_redirected_stream + 1);
		is_redirected_stream = FindWord(words, number_words, ">");
		if (is_redirected_stream == 0)
			is_redirected_stream = FindWord(words, number_words, ">>");
		if (is_redirected_stream == 0)
			is_redirected_stream = FindWord(words, number_words, "<");
		if (is_redirected_stream) {
			return;
			RedirectedProcess(words, number_words, is_redirected_stream, is_waited, status);
		}
		int res = execvp(words[0], words);
		if (res < 0)
			printf("ERROR: can not launch this redirected process: %s\n", words[0]);
		exit(1);
	}
	if (pid < 0)
		printf("ERROR: not create fork process\n");
	if (pid > 0) {
		if (is_waited == 0)
			wait(&tmp);
	if (WEXITSTATUS(tmp)) *status = 0;
	}
}

void Print_wlist(char ** wlist) {
	int i = 0;
	while (*(wlist + i)) {
		fprintf(stderr, "%s\n", wlist[i]);
		++i;
	}
}

void Print_wlist2(char *** wlist, int size, int sizes[]) {
	int i = 0;
	while(i < size) {
		Print_wlist(wlist[i]);
		//fprintf(stderr, "%d\n", sizes[i]);
		++i;
	}
}

void PipeProcess(char **words, int number_words, int is_waited,int* status) {
	int i = 0;
	int number_commands = 1;
	for (i = 0; i < number_words; ++i)
		if (strcmp(words[i], "|") == 0)
			number_commands++;
	int sizes[number_commands];
	//int IsToDo
	char ***commands = (char ***)malloc(sizeof(char **) * number_commands);
	for (i = 0; i < number_commands - 1; ++i) {
		int k = FindWord(words, number_words, "|");
		sizes[i] = k;
		commands[i] = (char **)malloc(sizeof(char *) * (k + 1));
		int j = 0;
		for (j = 0; j < k; ++j) {
			commands[i][j] = (char *) malloc(sizeof(char) * (strlen(words[j]) + 1));
			strcpy(commands[i][j], words[j]);
		}
		commands[i][k] = NULL;

		DeleteWords(words, &number_words, 0, k);
	}
	commands[number_commands - 1] = (char **) malloc(sizeof(char *) * (number_words + 1));
	int j = 0;
	sizes[number_commands - 1] = number_words;
	for (j = 0; j < number_words; ++j){
		commands[i][j] = (char *) malloc(sizeof(char) * (strlen(words[j]) + 1));
		strcpy(commands[i][j], words[j]);
	}
	commands[i][number_words] = NULL;
	//int pid2 = fork();
	//if (pid2  == 0){
		int pipes[number_commands - 1][2];
		int arr[number_commands];
		int is_redirected_stream_first, is_redirected_stream_last;
		//Print_wlist2(commands, number_commands, sizes);
		for (i = 0; i < number_commands - 1; ++i) {
			if(pipe(pipes[i]) < 0) printf("ERROR with pipe");
		}
		for (i = 0; i < number_commands; ++i) {
			arr[i] = fork();
			if (arr[i] < 0){
				printf("ERROR: not create this processes\n");
				exit(1);
			}
			if (arr[i] == 0){
				if (i == 0) {
					is_redirected_stream_first = FindWord(commands[i], sizes[i], "<");
				}
				if (i == number_commands - 1){
					is_redirected_stream_last = FindWord(commands[i], sizes[i], ">");
					if (is_redirected_stream_last == 0)
					is_redirected_stream_last = FindWord(commands[i], sizes[i], ">>");
				}
				if (i == 0 && is_redirected_stream_first) {
					int file = open(commands[i][is_redirected_stream_first + 1], O_RDONLY, 0644);
					if (file == -1) {printf("cant open this file :%s\n",commands[i][is_redirected_stream_first + 1]); exit(1);}
					int h = dup2(file, 0);
					if (h < 0) {
						printf("ERROR dup2 in pipes\n");
						exit(1);
					}
					DeleteWords(commands[i], &sizes[i], is_redirected_stream_first, is_redirected_stream_first + 1);
				}
				if (i == number_commands - 1 && is_redirected_stream_last){
					int file;
					if (strcmp(commands[i][is_redirected_stream_last], ">") == 0)
						{ if (file = open(commands[i][is_redirected_stream_last + 1], O_WRONLY | O_TRUNC | O_CREAT, 0644) == -1)
							{printf("cant open this file :%s\n",commands[i][is_redirected_stream_last + 															1]);	
								exit(1);
							}					
						}
					else
						if (file = open(commands[i][is_redirected_stream_last + 1], O_WRONLY | O_APPEND | O_CREAT, 0644) == -1){
							printf("cant open this file :%s\n",commands[i][is_redirected_stream_last + 1]);
							exit(1);
						}
					int h = dup2(file, 1);
					if (h < 0) {
						printf("ERROR dup2 in pipes\n");
						exit(1);
					}
					DeleteWords(commands[i], &sizes[i], is_redirected_stream_last, is_redirected_stream_last + 1);
				}
				if (i != 0 && i != number_commands - 1){
					for (j = 0;j < (number_commands - 1); ++j){
						if (j == i - 1) continue;
						if (j == i) continue;
					close(pipes[j][0]); close(pipes[j][1]);}
					close(pipes[i - 1][1]);
					int h = dup2(pipes[i - 1][0], 0);
					if (h < 0) {
						printf("ERROR dup2 in pipes%d\n", i - 1);
						exit(1);
					}
					close(pipes[i][0]);
					int h1 = dup2(pipes[i][1], 1);
					if (h1 < 0) {
						printf("ERROR dup2 in pipes %d\n", i);
						exit(1);
					}
				}else if (i == 0){
					for(j = 1; j < (number_commands - 1); ++j){ close(pipes[j][0]); close(pipes[j][1]);}			
					close(pipes[0][0]);				
					int h = dup2(pipes[0][1], 1);
					if (h < 0) {
						printf("ERROR dup2 in pipes in conv %d\n", i);
						exit(1);
					}
				}
				else{
					for (j = 0;j < (number_commands - 2); ++j){ close(pipes[j][0]); close(pipes[j][1]);}	
					close(pipes[number_commands - 2][1]);
					int h = dup2(pipes[number_commands - 2][0], 0);
					if (h < 0) {
						printf("ERROR dup2 in pipes in conv\n");
						exit(1);
					}
				}
				int res = execvp(commands[i][0], commands[i]);
				if (res < 0)
					printf("ERROR: can not launch this redirected process: %s\n", commands[i][0]);
				
				exit(1);
            		}
		}
	for (i = 0;i < number_commands - 1; ++i){ close(pipes[i][0]); close(pipes[i][1]);}
		if (is_waited == 0)
			for (i = 0; i < number_commands; ++i)
				wait(&arr[i]);
		//printf("st = %s\n",status);
		for (i = 0;i < number_commands; ++i)	
			if (WEXITSTATUS(arr[i])) {*status = 0;}
	//}
	/*if (pid2 < 0)
		printf("ERROR: not create this processes\n");
	if (pid2 > 0) {
		if (is_waited == 0)
			wait(0);
		*status = 0;
	}*/
}

int max(int number1, int number2) {
	if (number1 > number2)
		return number1;
	else
		return number2;
}

int min(int number1, int number2) {
	if (number1 < number2)
		return number1;
	else
		return number2;
}

int StringCmp(const char *str1, const char *str2) {
	int size1 = strlen(str1), size2 = strlen(str2);
	if (size1 != size2)
		return 0;
	int i = 0;
	for (i = 0; i < min(size1, size2); ++i) {
		if (str1[i] != str2[i])
			return 0;
	}
	return 1;
}



void Concat(char **string1, char **string2) {
	int size1 = strlen(*string1), size2 = strlen(*string2);
	char *buffer = (char*)malloc(sizeof(char) * (size1 + size2 + 1));
	int i = 0;
	for (i = 0; i < size1; ++i)
		buffer[i] = (*string1)[i];
	for (i = size1; i < size1 + size2; ++i)
		buffer[i] = (*string2)[i - size1];
	buffer[size1 + size2] = 0;

	free(*string1);
	*string1 = buffer;
}

void ParseToCommands(char *string, int *number_words, char ***words, int** IsToDo) {
	int size = strlen(string);
	int i = 0;
	int j = 0;//количество скобок
	*number_words = 0;
	for (i = 0; i < size; ++i) {
		if (string[i] == ')')
			--j;
		if (string[i] == '(')
			++j;
		if (string[i] == ';'){
			if (j == 0)
				(*number_words)++;
		}
		if (i < size - 1){
			if (string[i] == '|' && string[i + 1] == '|')
				if (j == 0)
					(*number_words)++;
			if (string[i] == '&' && string[i + 1] == '&')
				if (j == 0)
					(*number_words)++;	
		}
	}
	(*number_words)++;
	*words = (char**)malloc(sizeof(char*) * (*number_words));
	*IsToDo = (int*)malloc(sizeof(int) * (*number_words));
	int current_position = 0;
	int x;
	for (i = 0; i < *number_words; ++i) {
		j = 0;
		int size_current_word = 0;
		x = 0;
		while(current_position + size_current_word < size){	
			if (string[current_position + size_current_word] == ')')
				--j;
			if (string[current_position + size_current_word] == '(')
				++j;
			if (string[current_position + size_current_word] == ';'){
				if (j == 0){
					(*IsToDo)[i] = 0;
					break;
				}
			}
			if (current_position + size_current_word < size - 1){
				if (string[current_position + size_current_word] == '|' && string[current_position + size_current_word + 1] == '|')
					if (j == 0){
						x = 1;
						(*IsToDo)[i] = 1;
						break;	
					}
				if (string[current_position + size_current_word] == '&' && string[current_position + size_current_word + 1] == '&')
					if (j == 0){
						x = 1;
						(*IsToDo)[i] = 2;
						break;	
					}
			}
			size_current_word++;
		}
		(*words)[i] = (char*)malloc(sizeof(char) * (size_current_word + 1));
		int k = 0;
		for (k = 0; k < size_current_word; ++k) {
			(*words)[i][k] = string[k + current_position];
		}
		current_position += size_current_word + 1 + x;
		(*words)[i][size_current_word] = 0;
	}
}
void PrepareForExec(int* exit){
	char *str  = NULL;
	char** command = 0;
	size_t s;
	char **words = 0;
	int number_words = 0;
	char *user_path = 0;
	user_path = GetUserPath();
	int is_waited = 0;
	int commands_num;
	int i = 0;
	int status = 1;
	int *IsToDo;
	int size = getline(&str , &s, stdin);
	if (!strcmp(str,"\n")) return; 
	if (!strlen(str)) {printf("\n");*exit = 1; return;}
	str[size] = 0;
	int is_pipes = 0, is_redirected_stream = 0;
	str = DeleteExtraSpaces(str);
	if (!strlen(str)) return;
	ParseToCommands(str, &commands_num, &command,&IsToDo);
	//printf("num com = %d\n",commands_num);
	for ( i = 0; i < commands_num; ++i){
		status = 1;
		command[i] = DeleteExtraSpaces(command[i]);
		ParseToWords(command[i], &number_words, &words);
		//printf("%s\n",words[0]);
		is_waited = FindWord(words, number_words, "&");
		if (FindWord(words, number_words, "|")) {
			is_pipes = 1;
		}else is_pipes = 0;
		is_redirected_stream = FindWord(words, number_words, ">");
		if (is_redirected_stream == 0)
				is_redirected_stream = FindWord(words, number_words, ">>");
		if (is_redirected_stream == 0)
			is_redirected_stream = FindWord(words, number_words, "<");
		int com = 0;
		if (StringCmp(words[0], "cd"))
			com = 1;
		if (StringCmp(words[0], "pwd"))
			com = 2;
		if (StringCmp(words[0], "exit"))
			com = 3;
		switch (com) {
			case 1: {
				cd(words, number_words, user_path);
				break;
			}
			case 2: {
				pwd();
				printf("\n");
				break;
			}
			case 3: {
				*exit = 1;
				break;
			}
			default: {
				if (is_pipes == 0 && is_redirected_stream == 0) {
					if (is_waited != 0) {
						DeleteWords(words, &number_words, is_waited, is_waited);
					}
					SimpleProcess(words, number_words, is_waited, &status);
					break;
				}
				if (is_pipes == 0 && is_redirected_stream) {
					if (is_waited != 0) {
						DeleteWords(words, &number_words, is_waited, is_waited);
					}
					RedirectedProcess(words, number_words, is_redirected_stream, is_waited, &status);
					break;
				}
				if (is_pipes == 1) {
					if (is_waited != 0) {
						DeleteWords(words, &number_words, is_waited, is_waited);
					}
					PipeProcess(words, number_words, is_waited,&status);
					break;
				}
			}
		}
		int j;
		if (words){
			j = 0;
			while(!words[j]) 
				free(words[j++]);
			free(words);
			words = NULL;
	}
	//printf("status = %d\n", status);
	//if ( IsToDo[i] != 0) break;
	if ( IsToDo[i] == 1)
		if (status == 1) break;			
		else if (IsToDo[i] == 2)
			if (status == 0) break;
	}	
	if (IsToDo){
		free(IsToDo);
		IsToDo = NULL;
	}
	if (command){		
		for ( i = 0; i < commands_num; ++i)
			free(command[i]);
		 free(command); 
		command = NULL;
	}
	if (str) {free(str);str = NULL;}
	free(user_path);
}

int main(int argc, char **argv) {
	int IsExit = 0;
	while(1) {
		pwd();
		printf(" $ ");
		PrepareForExec(&IsExit);
		if (IsExit)
			break;
	}
	return 0;
}
