# usage: sh xxd.sh [The start address of file] [The size of file]
xxd -u -a -g l -s $2 -l $3 $1
# below are explanation of the parameter 
# -u  use upper case hex letters. Defaule is lower case.
# -a | -autoskip
#	toggle autoskip: A single '*' replaces nul-lines. Default off.
