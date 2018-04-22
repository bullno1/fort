: [ immediate 0 state! ;

: ] 1 state! ;

: [c] immediate compile-only next-char compile ;

: ( immediate clear-scan-buf [ next-char ) compile ] scan-until-char ;
: \ immediate clear-scan-buf [ 10 compile ] scan-until-char ;

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

\ String literal
: " clear-scan-buf [ next-char " compile ] scan-until-char scan-buf ;
