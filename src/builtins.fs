: [ immediate 0 state! ;

: ] 1 state! ;

: dup  0 pick ;
: over 1 pick ;
: swap 1 roll ;
: rot  2 roll ;
: drop 1 ndrop ;

: " [ next-char " compile ] scan-until-char ;
