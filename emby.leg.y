%{

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define NUM_FN

static struct EmbyState
{
	FILE *input_stream;
	FILE *memory_stream;
	uint8_t *memory_string;
	size_t memory_size;
	uintptr_t (*embyfn_t)(uintptr_t, ...) 
				current_function, FUNCTIONS[NUM_FN];
}
EMBY_STATE;

<:
print "\n\n";

$EMBY_INPUT_FUNCTIONS = <<'__EOF';
add
sub
mul
rem
idiv
fdiv
exec
print
ifeq 
ifne
iflt
ifle
ifgt
ifge
forin
foreach
while
unless
select
__EOF

my $i = 0;
my $perf = "";

for (chomp(my $line = $EMBY_INPUT_FUNCTIONS))
{
	print "#define ", uc($line), "_ID", " ", $i;
	print "static uintptr_t\n", $line, "(uintptr_t inst, ...);\n";
	$perf = "${perf}\n${line}, ${i++}";
}

print "\n\n";

$PERF_HASH_TABLE_SPEC = <<'__EOF';
%struct-type
%swich=${NUM_SWITCH}

struct embyfn {  int8_t *name; int id; };

%%

${perf}

__EOF

print `printf "${PERF_HASH_TABLE_SPEC}" | gperf`;

print "\n\n";

:>



%}









