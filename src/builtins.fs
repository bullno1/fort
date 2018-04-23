: [ immediate 0 state! ;

: ] 1 state! ;

: [c] immediate compile-only next-char compile ;

: ( immediate clear-scan-buf [ next-char ) compile ] scan-until-char ;
: \ immediate clear-scan-buf [ 10 compile ] scan-until-char ;
: see ' word.inspect ;

\ Stack manipulation
: dup ( a -- a a )
	0 pick ;

: over ( a b -- a b a )
	1 pick ;

: rot ( a b c -- b c a )
	2 roll ;

: -rot ( a b c -- c a b )
	rot rot ;

: drop ( a a -- a )
	1 ndrop ;

: nip ( a b -- b )
	swap drop ;

: tuck ( a b -- b a b )
	swap over ;

\ Word manipulation

: word.at! ( word index data -- )
	word.>at drop ;

: word.name! ( word name -- )
	word.>name drop ;

: word.flags! ( word flags -- )
	word.>flags drop ;

\ Flow control

: here
	current-word word.length ;

: >mark ( -- a ) \ a = current place in code
	here 0 compile ;

: >resolve ( a -- ) \ a = >mark'd location
	dup here ( mark mark location )
	swap - ( mark delta )
	current-word -rot word.at! ;

: postpone immediate
	' compile ;

: quote> immediate
	' quote compile ;

: compile-jmp
	quote> (jmp) compile ;

: compile-jmp0
	quote> (jmp0) compile ;

: if ( a -- )
	immediate compile-only

	compile-jmp0 >mark ;

: else ( a -- )
	immediate compile-only

	compile-jmp >mark \ jump to then
	swap >resolve ;

: then ( a -- )
	immediate compile-only

	>resolve ;

: recurse immediate compile-only
	current-word compile ;

\ String literal
: "
	immediate
	clear-scan-buf [c] " scan-until-char scan-buf
	state if compile then ;
