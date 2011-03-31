#!/usr/bin/awk -f

BEGIN {
	FS = ","
	print "/*\n * IPFIX structs, types and definitions\n *\n * This is a generated file. Do not edit! \n *\n */\n\n/*\n * ipfix information element list\n */\nipfix_field_type_t ipfix_ft_fokus[] = {"

}


NF==0 { next }
/#/   { next }


match ($0, /\/\/|\*|\/\*/) {
	split($0, aux, substr($0, RSTART, RLENGTH))
	print substr($0, RSTART, RLENGTH)" "aux[2] }

/\*\// {
	print "*/"
}


NF==6 {
	print "\t{ IPFIX_ENO_FOKUS, "$2", "$3", "$4", \n\t "$5", "$6" },"
}

END {
	print "\t{ 0, 0, -1, 0, NULL, NULL, }\n};"
}
