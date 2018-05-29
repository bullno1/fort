: nop ;

: see ' word.inspect ;

: dup 0 pick ;

: drop 1 ndrop ;

: add-flag-to-current-word
	current-word word.flags |
	current-word swap word.>flags drop ;

: immediate 1 add-flag-to-current-word ;

' immediate 1 2 | word.>flags drop

: [ immediate 0 state! ;

: ] 1 state! ;

: ( immediate clear-scan-buf [ next-char ) compile ] scan-until-char ;
: \ immediate clear-scan-buf [ 10 compile ] scan-until-char ;

\ We can use comments now

: quote> immediate
	\ compile the next word in the input stream into the current word
	' quote compile ;

: constant
	word.create
		word.->colon
		next-token word.>name
		swap word.push
		quote> exit word.push
	word.register ;

1 constant word.IMMEDIATE
2 constant word.COMPILE-ONLY
4 constant word.NO-INLINE

: compile-only word.COMPILE-ONLY add-flag-to-current-word ;

' compile-only word.IMMEDIATE word.COMPILE-ONLY | word.>flags drop

: no-inline immediate compile-only word.NO-INLINE add-flag-to-current-word ;

: [c] immediate compile-only next-char compile ;

\ Arithmethic short hand

: 0= 0 = ;
: 0< 0 < ;
: 0> 0 > ;
: 0<= 0 <= ;
: 0>= 0 >= ;
: 1+ 1 + ;
: 1- 1 - ;

\ Stack manipulation
: over ( a b -- a b a )
	1 pick ;

: rot ( a b c -- b c a )
	2 roll ;

: -rot ( a b c -- c a b )
	rot rot ;

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

: compile-jmp
	quote> (jmp) compile ;

: compile-jmp0
	quote> (jmp0) compile ;

: if ( interpret: a -- )
     ( compile: -- mark )
	immediate compile-only

	compile-jmp0 >mark ;

: else ( interpret:  -- )
	   ( compile: mark1 -- mark2 )
	immediate compile-only

	compile-jmp >mark \ jump to then
	swap >resolve ;

: then ( interpret:  -- )
	   ( compile: mark -- )
	immediate compile-only

	>resolve ;

: recurse immediate compile-only
	current-word compile ;

: <mark ( -- a ) \ a = current place in code
	here ;

: <resolve ( a -- ) \ a = <mark'd location

	here - compile ;

: begin ( interpret: -- )
        ( compile: -- mark )
	immediate compile-only

	<mark ;

: until ( interpret: cond -- )
        ( compile: mark -- )
	immediate compile-only

	compile-jmp0 <resolve ;

: while ( interpret: cond -- )
        ( compile: mark -- mark mark )
	immediate compile-only

	postpone if swap ;

: repeat ( interpret: -- )
        ( compile: mark -- )
	immediate compile-only

	compile-jmp <resolve
	postpone then ;

\ Utils

: . print drop ;

\ String literal
: "
	immediate
	clear-scan-buf [c] " scan-until-char scan-buf
	state if compile then ;
