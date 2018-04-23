: fib ( n -- fib[n] )
	dup 2 >= if
		dup 1 - recurse ( n fib[n - 1] )
		swap 2 - recurse ( fib[n - 1] rib[n - 2] )
		+
	then ;

see fib
35 fib .
