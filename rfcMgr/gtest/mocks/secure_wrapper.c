/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2019 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/
#define _GNU_SOURCE
#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#ifdef WITH_RDKLOGGER
#  include "rdk_debug.h"
//#  define VERBOSE_DEBUG
#  define LOG_LIB "LOG.RDK.LIBSYSCALLWRAPPER"
#else
#  define RDK_LOG(a1, a2, args...) //fprintf(stderr, args)
#  define RDK_LOG_INFO 0
#  define RDK_LOG_ERROR 0
#  define LOG_LIB 0
#endif

#define MAX_ARG_LEN 1024
#define MAX_NUM_ARGS 512
#define MAX_NUM_CMDS 32

#define DOUBLE(x) ((x<<8)|x)
#define FAIL(msg...) ({ \
		RDK_LOG(RDK_LOG_ERROR,LOG_LIB, msg); \
		fprintf(stderr, msg); \
		fflush(stderr); \
		goto fail; \
	})

int v_secure_pclose(FILE *stream);
static FILE *v_secure_popen_internal(const char *direction, const char *format, va_list *ap);

#define FD_OPS_CLOSE 1
#define FD_OPS_DUP2  2
#define FD_OPS_OPEN  3

struct fd_ops_t {
	struct fd_ops_t *next, *prev;
	int cmd, fd, srcfd, oflag;
	mode_t mode;
	char path[];
};

typedef struct task_s {
	char **argv;
	enum {
		UNDEFINED  = 0,
		SEMICOLON  = ';',
		BACKGROUND = '&',
		PIPE = '|',
		AND  = DOUBLE('&'),
		OR   = DOUBLE('|'),
	} token;

	struct fd_ops_t *fd_ops;

	struct task_s **subshell;
} task;

static task* new_task() {
	task *new_task = (task *)malloc(sizeof(task));
	if (!new_task) {
		FAIL("malloc: %s\n", strerror(errno));
	}

	new_task->argv = (char **)calloc(MAX_NUM_ARGS + 1, sizeof(char *));
	if (!new_task->argv) {
		FAIL("calloc: %s\n", strerror(errno));
	}

	new_task->token = UNDEFINED;

	new_task->fd_ops = NULL;

	new_task->subshell = NULL;

	return new_task;

fail:
	if (new_task) {
		free(new_task);
	}
	return NULL;
}

static void free_fd_ops(task *task) {
	for (struct fd_ops_t *op = task->fd_ops; op; ) {
		struct fd_ops_t *next = op->next;
		free(op);
		op = next;
	}
}

static void free_task(task *task) {
	free_fd_ops(task);

	for (char **argv = task->argv; *argv; argv++) {
		free(*argv);
	}
	free(task->argv);
	free(task);
}

static void free_task_list(task **task_list) {
	for (task **task = task_list; *task; task++) {
		if ((*task)->subshell) {
			free_task_list((*task)->subshell);
		}
		free_task(*task);
	}
	free(task_list);
}

static int fd_ops_addopen(task *task, int fd, const char *restrict path, int flags, mode_t mode) {
	struct fd_ops_t *op = malloc(sizeof *op + strlen(path) + 1);
	if (!op) return ENOMEM;

	op->cmd = FD_OPS_OPEN;
	op->fd = fd;
	op->oflag = flags;
	op->mode = mode;
	strcpy(op->path, path); // CID 178401 : Calling risky function (DC.STRING_BUFFER)-False positive
	if ((op->next = task->fd_ops)) op->next->prev = op;
	op->prev = 0;
	task->fd_ops = op;
	return 0;
}

static int fd_ops_addclose(task *task, int fd) {
	struct fd_ops_t *op = malloc(sizeof *op);
	if (!op) return ENOMEM;

	op->cmd = FD_OPS_CLOSE;
	op->fd = fd;
	if ((op->next = task->fd_ops)) op->next->prev = op;
	op->prev = 0;
	task->fd_ops = op;
	return 0;
}

static int fd_ops_adddup2(task *task, int srcfd, int fd) {
	struct fd_ops_t *op = malloc(sizeof *op);
	if (!op) return ENOMEM;

	op->cmd = FD_OPS_DUP2;
	op->srcfd = srcfd;
	op->fd = fd;
	if ((op->next = task->fd_ops)) op->next->prev = op;
	op->prev = 0;
	task->fd_ops = op;
	return 0;
}

static inline int apply_fd_ops(task *task) {
	struct fd_ops_t *op;
	int fd;
	int ret = 0;
	if (!task->fd_ops) {
		return 0;
	}

	for (op = task->fd_ops; op->next; op = op->next);
	for (; op; op = op->prev) {
		switch(op->cmd) {
			case FD_OPS_CLOSE:
				close(op->fd);
				break;
			case FD_OPS_DUP2:
				if (dup2(op->srcfd, op->fd) < 0) {
					ret = -1;
					FAIL("dup2: %s\n", strerror(errno));
				}
				break;
			case FD_OPS_OPEN:
			{
				if ((fd = open(op->path, op->oflag, op->mode)) < 0) {
					ret = -1;
					FAIL("open: %s: %s\n", op->path, strerror(errno));
				}
				if (fd != op->fd) {
					if (dup2(fd, op->fd) < 0) {
						ret = -1;
					}
					close(fd);
					if (ret) FAIL("dup2: %s: %s\n", op->path, strerror(errno));
				}
				break;
			}
		}
	}
fail:
	return ret;
}

// custom vsnprintf implementation to make sure that ap gets incremented as args are processed
static int _vsnprintf(char *str, size_t size, const char *format, va_list *ap) {
	va_list ap_copy;
	va_copy(ap_copy, *ap);
	int ret = vsnprintf(str, size, format, ap_copy);
	va_end(ap_copy);

	for (const char *ptr = format; *ptr; ptr++) {
		if (ptr[0] == '%') {
			if (ptr[1] == '%') {
				ptr++;
				continue;
			}
			va_arg(*ap, void *);
		}
	}

	return ret;
}

typedef struct {
	task **task_list;
	int bytes_consumed;
} parser_result;

static parser_result command_parser(const char *format, va_list *ap) {
	int n_tasks = 0;
	task **task_list = (task **)calloc(MAX_NUM_CMDS + 1, sizeof(task *));
	task *current_task = new_task();
	int n_args = 0;
	char temp[MAX_ARG_LEN * 2 + 1]; // stores a format string (* 2 since %% is two bytes but one character)

	int i = 0;  // input position
	int o = 0;  // output position   (per arg)
	int oc = 0; // output characters (per arg)

	int redirect_token = 0;
	enum {
		UNDEFINED = -1,
		AMP   = -2, // "&>/dev/null"
		// else "n>/dev/null" where n is redirect_fd
	} redirect_fd = UNDEFINED;
	enum {
		DEFAULT,
		NEW_ARG, // force new argument even if empty
		INPLACE, // inplace % format expansion
		REDIR,   // filename for redirect
	} arg_processing = DEFAULT;
	int match;

	do {
		if (task_list == NULL || current_task == NULL) {
			FAIL("Could not allocate memory for task\n");
		}

		if (oc > MAX_ARG_LEN) {
			temp[MAX_ARG_LEN] = '\0';
			FAIL("Argument too long: %s\n", temp);
		}

		if (n_args >= MAX_NUM_ARGS) {
			FAIL("Too many arguments\n");
		}

		if (n_tasks >= MAX_NUM_CMDS) {
			FAIL("Too many commands\n");
		}

		match = format[i];
		switch(match) {
			// whitespace
			case  ' ':
			case '\t':
				i++;
				break;

			// subshell expression
			case '(':
			{
				if (n_args) {
					FAIL("Syntax error near unexpected '('\n");
				}

				i++;
				parser_result result = command_parser(&format[i], ap);

				if (result.bytes_consumed < 1) {
					FAIL("Unable to parse subexpression\n");
				}

				i += result.bytes_consumed;
				current_task->subshell = result.task_list;
				break;
			}
			case ')':
				i++;
				current_task->token = SEMICOLON;
				match = 0;
				break;


			// eval & quotations
			case '`':  // eval
			case '\'': // single
			case '\"': // double
			{
				int eval_start   = o;
				int eval_start_c = oc;

				if (match == '`' && o > 0 && arg_processing != INPLACE) {
					arg_processing = INPLACE;
					break;
				}

				i++; // consume start quote

				while (format[i] != '\0' && format[i] != match && oc <= MAX_ARG_LEN) {
					temp[o++] = format[i++];
					oc++;
				}

				if (oc > MAX_ARG_LEN) {
					// error condition handled above
					continue;
				}

				if (format[i] == '\0') {
					FAIL("EOF while looking for matching '%c'\n", match);
				}

				i++; // consume end quote

				if (match != '`') {
					arg_processing = NEW_ARG;
					continue;
				}

				// eval
				temp[o] = '\0';
				o  = eval_start;
				oc = eval_start_c;

				if (!n_args) {
					FAIL("Eval can't be first argument\n");
				}

				FILE *fp = v_secure_popen_internal("r", &temp[o], ap);
				if (!fp) {
					FAIL("Unexpected error on eval\n");
				}
				while ((match = fgetc(fp)) != EOF) {
					if (oc > MAX_ARG_LEN) {
						v_secure_pclose(fp);
						temp[MAX_ARG_LEN] = '\0';
						FAIL("Argument too long: %s\n", temp);
					}

					if (n_args >= MAX_NUM_ARGS) {
						v_secure_pclose(fp);
						FAIL("Too many arguments\n");
					}

					switch(match) {
						case ' ':
						case '\t':
						case '\r':
						case '\n':
							// whitespace; check if anything follows and split into new arg
							match = fgetc(fp);
							if (o > 0 && match != EOF) {
								temp[o] = '\0';

								char arg[MAX_ARG_LEN + 1];
								_vsnprintf(arg, sizeof(arg), temp, ap);
								arg[MAX_ARG_LEN] = '\0';
								if((current_task->argv[n_args++] = strdup(arg)) == NULL) {
									v_secure_pclose(fp);
									FAIL("strdup failed \n");
                                                                }
								o = oc = 0;
							}
							ungetc(match, fp);
							break;

						case '%':
							temp[o++] = '%'; /* Fall */
						default:
							temp[o++] = match;
							oc++;
					}
				}
				v_secure_pclose(fp);

				arg_processing = DEFAULT;
				continue;
			}
			// end of command sequences
			case '&': // also &&
			case '|': // also ||
				if (current_task->token != 0) {
					FAIL("unexpected token\n");
				}

				// command &>/dev/null
				if (format[i] == '&' && format[i + 1] == '>') {
					i++;
					redirect_fd = AMP; //&> redirect
					continue;
				}

				if (format[i + 1] == match) {
					current_task->token = DOUBLE(match);
					i += 2;
				} else {
					current_task->token = match;
					i++;
				}
				break;
			case ';':
				current_task->token = match;
				i++;
				break;

			case '\0':
				current_task->token = ';';
				break;

			// file redirects
			case '<':
			case '>': // also >>
				if (redirect_token) {
					FAIL("unexpected redirect\n");
				}

				// parse and consume fd (still sitting in temp due to lack of whitespace before token)
				if (o > 0) {
					temp[o] = '\0';

					char *end = NULL;
					redirect_fd = strtol(temp, &end, 10);
					if (end != &temp[o]) {
						redirect_fd = UNDEFINED;
						break;
					}

					// found fd, consume argument (restart the output buffer)
					o = oc = 0;
				} else if (redirect_fd == UNDEFINED) {
					// assume fd based on direction
					redirect_fd = (match == '<') ? 0 : 1;
				}

				// consume redirect token
				if (format[i + 1] == match) {
					redirect_token = DOUBLE(match);
					i += 2;
				} else {
					redirect_token = match;
					i++;
				}

				// handle "2>&1" redirects
				if (format[i] == '&') {
					i++; // consume &

					char *end = NULL;
					int fd = strtol(&format[i], &end, 10);
					if (end == &format[i]) {
						FAIL("redirect error\n");
					}

					if (redirect_fd == AMP) {
						fd_ops_adddup2(current_task, 1, fd);
						fd_ops_adddup2(current_task, 2, fd);
					} else {
						fd_ops_adddup2(current_task, fd, redirect_fd);
					}

					i = end - format; // consume fd
					redirect_token = 0;
					redirect_fd = UNDEFINED;
					break;
				}

				// filename redirect; we'll pick up the filename after the switch loop
				arg_processing = REDIR;
				break;

			// normal arguments
			default:
				temp[o++] = format[i++];
				oc++;
				continue;
		}
		// switch statement blocks until a full argument is parsed

		if (o > 0 || arg_processing == NEW_ARG) {
			temp[o] = '\0';

			if (current_task->subshell && arg_processing != REDIR) {
				FAIL("syntax error near unexpected token: '%s'\n", temp);
			}

			char arg[MAX_ARG_LEN + 1];
			o = oc = _vsnprintf(arg, sizeof(arg), temp, ap);
			arg[MAX_ARG_LEN] = '\0';

			if (oc > MAX_ARG_LEN) {
				FAIL("Argument too long: %s\n", arg);
			}

			switch (arg_processing) {
				case NEW_ARG:
				default:
					if((current_task->argv[n_args++] = strdup(arg))==NULL)
						FAIL("strdup failed \n");
					break;

				case INPLACE:
					o = oc = 0;
					for (char *c = arg; *c; c++) {
						switch (*c) {
							case '%':
								temp[o++] = '%';
								/* Fall */
							default:
								temp[o++] = *c;
								oc++;
						}
					}
					continue; // stay INPLACE

				case REDIR:
				{
					if (strcmp(temp, arg) != 0) {
						FAIL("redirection to variable is insecure\n");
					}


					bool amp_redirect = false;
					if (redirect_fd == AMP) { // &> redirect
						redirect_fd = 1;
						amp_redirect = true;
					}

					switch (redirect_token) {
						case '<':         fd_ops_addopen(current_task, redirect_fd, temp, O_RDONLY, 0644); break;
						case '>':         fd_ops_addopen(current_task, redirect_fd, temp, O_WRONLY | O_CREAT | O_TRUNC,  0644); break;
						case DOUBLE('>'): fd_ops_addopen(current_task, redirect_fd, temp, O_WRONLY | O_CREAT | O_APPEND, 0644); break;

						default: FAIL("unsupported redirect\n");
					}

					if (amp_redirect) {
						fd_ops_addclose(current_task, 2);
						fd_ops_adddup2(current_task, 1, 2);
					}

					redirect_token = 0;
					redirect_fd = UNDEFINED;

					break;
				}
			}

			o = oc = 0;
			arg_processing = DEFAULT;
		}

		// token means we've reached the end of a command
		if ((n_args || current_task->subshell) && current_task->token) {
			task_list[n_tasks++] = current_task;
			current_task = new_task();
			n_args = 0;
		}
	} while (match);

	free_task(current_task);

	return (parser_result) {
		.task_list = task_list,
		.bytes_consumed = i,
	};

fail:
	if (current_task) free_task(current_task);
	if (task_list) free_task_list(task_list);

	return (parser_result) {
		.task_list = NULL,
		.bytes_consumed = -1,
	};
}

static inline int execute_task_list(task **task_list);

static int execute_task(task *current_task, int close_list[3]) {
	char **argv = current_task->argv;
	int ret = -1;

#ifdef VERBOSE_DEBUG
	if (current_task->subshell) {
		RDK_LOG(RDK_LOG_INFO, LOG_LIB, "subshell\n");
	} else for (int n=0; current_task->argv[n]; n++) {
		RDK_LOG(RDK_LOG_INFO, LOG_LIB, "arg%d: \"%s\"\n", n, current_task->argv[n]);
	}
#endif

	pid_t child_pid;
	if (current_task->subshell) {
		child_pid = vfork(); // can be a vfork
		if (child_pid == -1) {
			FAIL("fork: %s\n", strerror(errno));

		} else if (child_pid == 0) {
			if (apply_fd_ops(current_task) == 0) {
				for (int i=0; i<3; i++) {
					int fd = close_list[i];
					if (fd != -1) close(fd);
				}
				ret = execute_task_list(current_task->subshell);
			}

			_exit(ret);
			/* noreturn */
		}

		int wstatus;
		while (waitpid(child_pid, &wstatus, 0) == -1) {
			if (errno != EINTR){
				fprintf(stderr, "child exited unexpectedly\n");
				break;
			}
		}

		if (WIFEXITED(wstatus)) {
			ret = WEXITSTATUS(wstatus);
		}

		return ret;
	}

	child_pid = vfork(); // can be a vfork
	if (child_pid == -1) {
		FAIL("fork: %s\n", strerror(errno));
	} else if (child_pid == 0) {
		if (apply_fd_ops(current_task) == 0) {
			for (int i=0; i<3; i++) {
				int fd = close_list[i];
				if (fd != -1) close(fd);
			}
		}

		execvp(argv[0], argv);
		fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));

		_exit(-1);
		/* noreturn */
	}

	if (current_task->token == BACKGROUND || current_task->token == PIPE) {
		// we're not waiting for this command to complete
		ret = 0;
	} else {
		int wstatus;
		while (waitpid(child_pid, &wstatus, 0) == -1) {
			if (errno != EINTR){
				fprintf(stderr, "child exited unexpectedly\n");
				ret = -1;
				break;
			}
		}

		if (WIFEXITED(wstatus)) {
			ret = WEXITSTATUS(wstatus);
		}
	}

fail:
#ifdef VERBOSE_DEBUG
	RDK_LOG(RDK_LOG_INFO, LOG_LIB, "ret: %d\n", ret);
#endif

	return ret;
}

static inline int execute_task_list(task **task_list) {
	int ret = -1;

	int skip = 0;    // short circuit evaluation when using AND, OR
	int pipefd = -1; // output of previous pipe (mapped to stdin)

	for (int n_tasks = 0; n_tasks < MAX_NUM_CMDS && task_list && task_list[n_tasks]; n_tasks++) {
		task *current_task = task_list[n_tasks];
		if (!skip) {
			int close_list[3] = { -1, -1, -1 }; // old_std, old_stdout, pipes[0]

			int old_stdin = -1;
			if (pipefd != -1) {
				// remap pipefd to stdin(0)
				old_stdin = dup(0);
				close(0);
				dup2(pipefd, 0);
				close(pipefd);

				close_list[0] = old_stdin;
				pipefd = -1;
			}

			int pipes[2];
			int old_stdout = -1;
			if (current_task->token == PIPE) {

				if (pipe(pipes) == -1) {
					FAIL("pipe: %s\n", strerror(errno));
				}

				// remap pipes[1] to stdout(1)
				old_stdout = dup(1);
				close(1);
				dup2(pipes[1], 1);
				close(pipes[1]);

				close_list[1] = old_stdout;
				close_list[2] = pipes[0];
				pipefd = pipes[0];
			}

			ret = execute_task(current_task, close_list);

			if (old_stdin != -1) {
				close(0);
				dup2(old_stdin, 0);
				close(old_stdin);
			}

			if (old_stdout != -1) {
				close(1);
				dup2(old_stdout, 1);
				close(old_stdout);
			}

		} else if (current_task->token == PIPE) {
			// skipped_command | another_skipped_command
			skip++;
		}

		if (skip > 0) {
			// skipped_command ; command
			skip--;
		}

		if (ret == 0 && current_task->token == OR) {
			// true_command || skipped_command
			skip++;
		}

		if (ret != 0 && current_task->token == AND) {
			// false_command && skipped_command
			skip++;
		}
	}

fail:
	return ret;
}

static task ** v_secure_system_internal(const char *format, va_list *ap) {
#ifdef VERBOSE_DEBUG
	va_list ap_log;
	char cmd_log[1024];

	va_copy(ap_log, *ap);
	vsnprintf(cmd_log, sizeof(cmd_log), format, ap_log);
	cmd_log[sizeof(cmd_log)-1] = '\0';
	RDK_LOG(RDK_LOG_INFO, LOG_LIB, "wrapper template: %s\n", format);
	RDK_LOG(RDK_LOG_INFO, LOG_LIB, "wrapper command: %s\n", cmd_log);
	va_end(ap_log);
#endif

	return command_parser(format, ap).task_list;
}

int v_secure_system(const char *format, ...) {
	int ret = -1;
	pid_t child_pid = 0; // CID 178415 : Branch past initialization (PW.BRANCH_PAST_INITIALIZATION)
	task **task_list;

	va_list ap;
	va_start(ap, format);
	task_list = v_secure_system_internal(format, &ap);
	va_end(ap);

	if (!task_list) {
		FAIL("shell failure");

	}

	child_pid = vfork(); // can be a vfork
	if (child_pid == -1) {
		free_task_list(task_list);
		FAIL("fork: %s\n", strerror(errno));

	} else if (child_pid == 0) {
		int child_ret;

		child_ret = execute_task_list(task_list);

		_exit(child_ret);
		/* noreturn */
	}

	free_task_list(task_list);

	int wstatus;
	while (waitpid(child_pid, &wstatus, 0) == -1) {
		if (errno != EINTR){
			fprintf(stderr, "child exited unexpectedly\n");
			break;
		}
	}

	if (WIFEXITED(wstatus)) {
		ret = WEXITSTATUS(wstatus);
	}

fail:
	return ret;
}

/*
 * popen() wrapper
 */

typedef struct pstatus_t {
	int fd;
	int pid;
	struct pstatus_t *next;
} pstatus_t;

pstatus_t *popen_list = NULL;

static pthread_mutex_t  pstat_lock = PTHREAD_MUTEX_INITIALIZER;
int v_secure_pclose(FILE *stream) {
	int fd = fileno(stream);
	pstatus_t *pstatus, **pp = &popen_list;

	pthread_mutex_lock(&pstat_lock);
	while (*pp && (*pp)->fd != fd) {
		pp = &(*pp)->next;
	}

	pstatus = *pp;

	if (!pstatus) {
		fprintf(stderr, "pclose failed to find fd\n");
	        pthread_mutex_unlock(&pstat_lock);
		return -1;
	}

	int ret = -1;

	fclose(stream);

	int wstatus;
	while (waitpid(pstatus->pid, &wstatus, 0) == -1) {
		if (errno != EINTR){
			fprintf(stderr, "child exited unexpectedly\n");
			break;
		}
	}

	if (WIFEXITED(wstatus)) {
		ret = WEXITSTATUS(wstatus);
	}

	(*pp) = pstatus->next;
	pthread_mutex_unlock(&pstat_lock);
	free(pstatus);

	return ret;
}

static FILE *v_secure_popen_internal(const char *direction, const char *format, va_list *ap) {
	pstatus_t *pstatus = (pstatus_t *)malloc(sizeof(*pstatus));
	pid_t child_pid;
	int pipes[2];
	int dir = (*direction == 'r') ? 1 : 0;
	task **task_list = NULL; // CID 178417, CID 178424 : Branch past initialization (PW.BRANCH_PAST_INITIALIZATION)

	if (!pstatus) {
		FAIL("malloc: %s\n", strerror(errno));
	}

	if (pipe(pipes) == -1) {
		FAIL("pipe: %s\n", strerror(errno));
	}

	task_list = v_secure_system_internal(format, ap);
	if (!task_list) {
                close(pipes[0]);
                close(pipes[1]);
		FAIL("shell failure");

	}

	pthread_mutex_lock(&pstat_lock);
	child_pid = fork();
	if (child_pid == -1) {
		close(pipes[0]);
		close(pipes[1]);
                free_task_list(task_list);
	        pthread_mutex_unlock(&pstat_lock);
		FAIL("fork: %s\n", strerror(errno));

	} else if (child_pid == 0) {
		fflush(stdout);
		fflush(stderr);

		close(dir);
		close(pipes[1 - dir]);
		dup2(pipes[dir], dir);
		close(pipes[dir]);

		for (pstatus_t *p = popen_list; p; p = p->next) {
			close(p->fd);
		}

		int child_ret = execute_task_list(task_list);

		_exit(child_ret);
		/* noreturn */
	}

	free_task_list(task_list);

	pstatus->pid = child_pid;

	close(pipes[dir]);
	pstatus->fd = pipes[1 - dir];

	pstatus->next = popen_list;
	popen_list = pstatus;
	pthread_mutex_unlock(&pstat_lock);

	return fdopen(pstatus->fd, direction);

fail:
	if (pstatus) {
		free(pstatus);
	}
	return NULL;
}

FILE *v_secure_popen(const char *direction, const char *format, ...) {
	FILE *ret;
	va_list ap;

	va_start(ap, format);
	ret = v_secure_popen_internal(direction, format, &ap);
	va_end(ap);

	return ret;
}

/*
 * Legacy API compatibility
 */

int contains_secure_separator(char *str) {
	return -1;
}

int secure_system_call_p(const char *cmd, char *argv[]) {
	int ret = -1;

#ifdef VERBOSE_DEBUG
	for (int n=0; argv[n]; n++) {
		RDK_LOG(RDK_LOG_INFO, LOG_LIB, "arg%d: \"%s\"\n", n, argv[n]);
	}
#endif

	pid_t child_pid = fork();
	if (child_pid == -1) {
		FAIL("fork: %s\n", strerror(errno));

	} else if (child_pid == 0) {
		if (execvp(argv[0], argv) == -1) {
			perror(argv[0]);
		}

		_exit(-1);
		/* noreturn */
	}

	int wstatus;
	while (waitpid(child_pid, &wstatus, 0) == -1) {
		if (errno != EINTR){
			fprintf(stderr, "child exited unexpectedly\n");
			break;
		}
	}

	if (WIFEXITED(wstatus)) {
		ret = WEXITSTATUS(wstatus);
	}

#ifdef VERBOSE_DEBUG
	RDK_LOG(RDK_LOG_INFO, LOG_LIB, "ret: %d\n", ret);
#endif

fail:
	return ret;
}

int secure_system_call_vp(const char *cmd, ...) {
	int ret = -1;

	char *arg[MAX_NUM_ARGS + 1];

	char *temp_arg;
	int n_args = 0;
	va_list ap;

	if (cmd == NULL) {
		FAIL("NULL input given\n");
	}

	arg[n_args++] = (char *)cmd;

	va_start(ap, cmd);
	do {
		if (n_args >= MAX_NUM_ARGS) {
			va_end(ap);
			FAIL("Too many arguments\n");
		}

		temp_arg = arg[n_args++] = va_arg(ap, char *);

	} while(temp_arg);

	ret = secure_system_call_p(cmd, arg);

	va_end(ap);
fail:
	return ret;
}
