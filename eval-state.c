static struct RuntimeState
{
	static struct Flags
	{
		bool force_traceon;
		bool force_prefix;
		bool run_interactive;
		bool line_directive;
	}
	FLAGS;
	
	
	static struct IO
	{
		FILE *instream;
		FILE *outstream;
		FILE *errstream;
		FILE *hold_stream;
		FILE *null_stream;
	}
	IO;

	static struct ShellPipe
	{
		FILE *last_pipe;
		uint8_t last_exec;
		size_t last_exec_len;
	}
	SHELL_PIPE;

	static struct TimeInfo
	{
		struct tm *broken_down;
		time_t 	   epoch;
		uint8_t    formatted[MAX_TIMEFMTBUFF];
	}
	TIME_INFO;

	static struct Tracer
	{
		bool trace_on;
		uint8_t LAST_TRACE[MAX_TRACE_LEN];
	}
	TRACER;

	static struct Container
	{
		gl_stack_t *SYMSTACK;
		gl_hash_map_t *SYMTABLE;
		gnuc_obstack_t *EVALPOOL;
	}
	CONTAINERS;

	static struct Invokation
	{
		size_t argc;
		size_t ARGVLENS[MAX_ARGV_NUM];
		uint8_t ARGV[MAX_ARGV_NUM][MAX_ARGV_LEN];
		uint8_t *argv_space_joined;
		uint8_t *argv_comma_joined;
		FILE *current_include_file;
		FIlE *quoted_stream;
		uint8_t *quoted_buffer;
		uint8_t *current_include_line;
		size_t current_include_linelen;
		size_t quoted_buffer_len;
		int shell_exit_code;
		uint32_t *translit_map;
		regex_t *compiled_re;
		pcre *compiled_pcre;
		regmatch_t pmatch[MAX_PMATCH];
		regoff_t start_offset;
		regoff_t end_offset;
	}
	INVOKATION;

	static struct RegexApi
	{
		(*compiler)(regex_t*, const char*);
		(*executor)(const regex_t*, const char*, size_t, regmatch_t pmatch[], int);
		(*errfunc)(int, const regex_t, char *, size_t);
		(*dealloc)(regex_t);
	}
	REGEX_API;
	
	static struct JmpBufs
	{
		jmp_buf BUILTIN_POSIX[NUM_POSIX_BUILTINS];
		jmp_buf BUILTIN_NONSTD[NUM_NONSTD_BUILTINS];
		jmp_buf ERRORS[NUM_ERRORS];
	} 
	NONLOC_JMPBUFS;

	static struct Diversions
	{
		FILE *STREAMS[NUM_DIVERT];
		size_t LENGTHS[NUM_DIVERTS];
		uint8_t *BUFFERS[NUM_DIVERTS];
		int divert_num;
		int undivert_num;
	}
	DIVERSIONS;
	
	static struct Wrap
	{
		FILE *stream;
		size_t length;
		uint8_t *buffer;
	}
	WRAP;

	static struct Syntax
	{
		uint8_t left_quote[MAX_TOKEN_LEN];
		uint8_t right_quote[MAX_TOKEN_LEN];
		uint8_t begin_comment[MAX_TOKEN_LEN];
		uint8_t end_comment[MAX_TOKEN_LEN];
		size_t left_quote_len;
		size_t right_quote_len;
		size_t begin_comment_len;
		size_t end_comment_len;
	}
	SYNTAX;

	static struct AwkArrays
	{	
	        static struct AwkArray
        	{
	                uintptr_t key;
                	uintptr_t value;
        	        enum AwkArrayType { INT, STR } value_type;
	        }
        	**AWK_ARRAY;
	        gl_hashmap_t *addresses;
        	size_t next_index;
	}
	AWK_ARRAYS;
	
	static struct m4Parser
	{
		uint8_t *current_line;
		uint8_t *formatstr;
		size_t current_line_len;
		int current_id;
	}
	M4_PARSER;
}
STATE;

#define CURR_ARGC	STATE.INVOKATION.argc
#define ARGV_name 	&STATE.INVOKATION.ARGV[0][0]
#define ARGV_nth(n)	&STATE.INVOKATION.ARGV[n][0]
#define ARGV_star	&STATE.INVOKATION.argv_comma_joined[0]
#define ARGV_atsign	&STATE.INVOKATION.argv_space_joined[0]
#define ARGVLEN_name	STATE.INVOKATION.ARGVLEN[0]
#define ARGVLEN_nth(n)  sTATE.INVOKATIOn.ARGVLEN[n]	
#define DEFN_stream	STATE.INVOKATION.quoted_stream
#define DEFN_string	&STATE.INVOKATION.quoted_buffer[0]
#define DEFN_strlen	STATE.INVOKATION.quoted_len


#define INSTREAM	STATE.IO.instream
#define OUTSTREAM	STATE.IO.outstream
#define PRISTREAM	STATE.IO.errstream
#define HOLDSTREAM	STATE.IO.holdstream
#define NULLSTREAM	STATE.IO.nullstream

#define SWAP_HOLD_STREAM(to_hold, to_replace)				\
	do {  HOLDSTREAM = to_hold; TO_HOLD = to_replace;  } while (0)
#define RESTORE_HELD_STREAM(held_stream)				\
	do { held_stream = HOLDSTREAM; } while (0)





